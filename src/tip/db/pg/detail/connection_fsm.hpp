/*
 * connection_fsm.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_BASIC_CONNECTION_NEW_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_BASIC_CONNECTION_NEW_HPP_

#include <boost/noncopyable.hpp>
#include <map>
#include <stack>
#include <set>
#include <memory>

#include <afsm/fsm.hpp>

#include <tip/db/pg/asio_config.hpp>
#include <tip/db/pg/common.hpp>
#include <tip/db/pg/error.hpp>
#include <tip/db/pg/transaction.hpp>
#include <tip/db/pg/resultset.hpp>

#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/protocol.hpp>
#include <tip/db/pg/detail/md5.hpp>
#include <tip/db/pg/detail/result_impl.hpp>
#include <tip/db/pg/detail/connection_observer.hpp>

#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {

class resultset;

namespace events {

struct transport_connected {};
struct authn_event {
    detail::auth_states state;
    detail::message_ptr message;
};
struct complete {};

struct ready_for_query {
    char status;
};

struct row_description {
    mutable std::vector<field_description> fields;
};

struct row_event {
    std::shared_ptr< detail::row_data > data;

    row_event() : data(std::make_shared< detail::row_data >()) {}

    detail::row_data&
    row() { return *data; }

    detail::row_data
    move_row() const
    {
        detail::row_data rd;
        rd.swap(*data);
        return rd;
    }
    // TODO Move accessor
};

struct parse_complete {};
struct bind_complete {};

struct no_data {}; // Prepared query doesn't return data

struct terminate {};
}  /* namespace events */

namespace detail {

LOCAL_LOGGING_FACILITY_CFG_FUNC(PGFSM, config::INTERNALS_LOG, fsm_log);

template < typename Mutex, typename TransportType, typename SharedType >
struct connection_fsm_def : ::afsm::def::state_machine<
            connection_fsm_def< Mutex, TransportType, SharedType > >,
        public std::enable_shared_from_this< SharedType > {

    //@{
    /** @name Misc typedefs */
    using transport_type    = TransportType;
    using shared_type       = SharedType;
    using this_type         = connection_fsm_def<Mutex, transport_type, shared_type>;

    using message_ptr       = std::shared_ptr< message >;
    using prepared_statements_map = std::map< std::string, events::row_description >;
    using result_ptr        = std::shared_ptr< result_impl >;

    using connection_fsm_type = ::afsm::state_machine<this_type, Mutex, connection_observer>;
    using none = ::afsm::none;

    template < typename StateDef, typename ... Tags >
    using state = ::afsm::def::state<StateDef, Tags...>;
    template < typename MachineDef, typename ... Tags >
    using state_machine = ::afsm::def::state_machine<MachineDef, Tags...>;
    template < typename ... T >
    using transition_table = ::afsm::def::transition_table<T...>;
    template < typename Event, typename Action = none, typename Guard = none >
    using in = ::afsm::def::internal_transition< Event, Action, Guard>;
    template <typename SourceState, typename Event, typename TargetState,
            typename Action = none, typename Guard = none>
    using tr = ::afsm::def::transition<SourceState, Event, TargetState, Action, Guard>;

    template < typename Predicate >
    using not_ = ::psst::meta::not_<Predicate>;
    //@}
    //@{
    /** @name Actions */
    struct on_connection_error {
        template < typename SourceState, typename TargetState >
        void
        operator() (error::connection_error const& err, connection_fsm_type& fsm,
                SourceState&, TargetState&)
        {
            fsm.log(logger::ERROR) << "connection::on_connection_error Error: "
                    << err.what();
            fsm.notify_error(err);
        }
    };
    struct disconnect {
        template < typename SourceState, typename TargetState >
        void
        operator() (events::terminate const&, connection_fsm_type& fsm, SourceState&, TargetState&)
        {
            fsm.log() << "connection: disconnect";
            fsm.send(message(terminate_tag));
            fsm.close_transport();
        }
    };
    //@}
    //@{
    /** @name States */
    struct unplugged : state< unplugged > {
        using deferred_events = ::psst::meta::type_tuple<
                events::terminate,
                events::begin,
                events::commit,
                events::rollback,
                events::execute,
                events::execute_prepared
            >;
    };

    struct terminated : ::afsm::def::terminal_state< terminated > {
        template < typename Event >
        void
        on_enter(Event const&, connection_fsm_type& fsm)
        {
            fsm.log() << "entering: terminated";
            fsm.send(message(terminate_tag));
            fsm.notify_terminated();
        }
    };

    struct t_conn : state< t_conn > {
        using deferred_events = ::psst::meta::type_tuple<
                events::terminate,
                events::begin,
                events::commit,
                events::rollback,
                events::execute,
                events::execute_prepared
            >;
        void
        on_enter(connection_options const& opts, connection_fsm_type& fsm)
        {
            fsm.connect_transport(opts);
        }

        template < typename Event >
        void
        on_exit(Event const&, connection_fsm_type& fsm)
        {
            fsm.start_read();
        }
    };

    struct authn : state< authn > {
        using deferred_events = ::psst::meta::type_tuple<
                events::terminate,
                events::begin,
                events::commit,
                events::rollback,
                events::execute,
                events::execute_prepared
            >;
        template < typename Event >
        void
        on_enter(Event const&, connection_fsm_type& fsm)
        {
            fsm.send_startup_message();
        }

        struct handle_authn_event {
            template < typename SourceState, typename TargetState >
            void
            operator() (events::authn_event const& evt, connection_fsm_type& fsm,
                    SourceState&, TargetState&)
            {
                fsm.log() << "authn: handle auth_event";
                switch (evt.state) {
                    case OK: {
                        fsm.log() << "Authenticated with postgre server";
                        break;
                    }
                    case Cleartext : {
                        fsm.log() << "Cleartext password requested";
                        message pm(password_message_tag);
                        pm.write(fsm.options().password);
                        fsm.send(::std::move(pm));
                        break;
                    }
                    case MD5Password: {
                        fsm.log() << "MD5 password requested";
                        // Read salt
                        std::string salt;
                        evt.message->read(salt, 4);
                        connection_options const& co = fsm.options();
                        // Calculate hash
                        std::string pwdhash = boost::md5((co.password + co.user).c_str()).digest().hex_str_value();
                        std::string md5digest = std::string("md5") + boost::md5( (pwdhash + salt).c_str() ).digest().hex_str_value();
                        // Construct and send message
                        message pm(password_message_tag);
                        pm.write(md5digest);
                        fsm.send(::std::move(pm));
                        break;
                    }
                    default : {
                        std::stringstream err;
                        err << "Unsupported authentication scheme "
                                << evt.state << " requested by server";
                        fsm.process_event(error::connection_error(err.str()));
                    }
                }
            }
        };

        using internal_transitions = transition_table<
            in< events::authn_event, handle_authn_event,    none >
        >;
    };

    struct idle : state< idle > {
        template < typename Event >
        void
        on_enter(Event const&, connection_fsm_type& fsm)
        {
            fsm.notify_idle();
        }

        using internal_transitions = transition_table<
        /*                Event            Action        Guard     */
        /*            +-----------------+-----------+---------+*/
            in< events::ready_for_query,    none,        none     >
        >;
    };

    struct transaction : state_machine<transaction> {
        using deferred_events = ::psst::meta::type_tuple< events::terminate >;

        using transaction_fsm_type = ::afsm::inner_state_machine< transaction,
                connection_fsm_type >;

        using transaction_ptr = std::shared_ptr< pg::transaction >;
        using transaction_weak_ptr = std::weak_ptr< pg::transaction >;

        //@{
        /** State forwards */
        struct tran_error;
        //@}
        //@{
        /** @name Actions */
        struct transaction_started {
            template < typename Event, typename SourceState, typename TargetState >
            void
            operator() (Event const&, transaction_fsm_type& fsm, SourceState&, TargetState&)
            {
                fsm.log() << "transaction::transaction_started";
                fsm.notify_started();
            }
        };
        struct commit_transaction {
            template < typename SourceState, typename TargetState >
            void
            operator() (events::commit const&, transaction_fsm_type& fsm, SourceState&, TargetState&)
            {
                fsm.log() << "transaction::commit_transaction";
                fsm.connection().send_commit();
            }
        };
        struct rollback_transaction {
            template < typename SourceState, typename TargetState >
            void
            operator() (events::rollback const&, transaction_fsm_type& fsm, SourceState&, TargetState&)
            {
                fsm.log() << "transaction::rollback_transaction";
                fsm.connection().send_rollback();
                fsm.notify_error(error::query_error("Transaction rolled back"));
            }

            template < typename Event, typename SourceState, typename TargetState >
            void
            operator() (Event const&, transaction_fsm_type& fsm, SourceState&, TargetState&)
            {
                fsm.log() << "transaction::rollback_transaction (on error)";
                fsm.connection().send_rollback();
                fsm.notify_error(error::query_error("Transaction rolled back"));
            }
        };

        struct tran_finished {
            template < typename SourceState, typename TargetState >
            void
            operator() (events::execute const& evt, transaction_fsm_type& fsm,
                    SourceState&, TargetState&)
            {
                fsm.log(logger::WARNING)
                        << "Execute event queued after transaction close";
                if (evt.error) {
                    try {
                        evt.error( error::transaction_closed{} );
                    } catch (::std::exception const& e) {
                        fsm.log(logger::WARNING) << "Exception in execute error handler " << e.what();
                    } catch (...) {
                        // Ignore handler error
                        fsm.log(logger::WARNING) << "Exception in execute error handler";
                    }
                }
            }
            template < typename SourceState, typename TargetState >
            void
            operator() (events::execute_prepared const& evt, transaction_fsm_type& fsm,
                    SourceState&, TargetState&)
            {
                fsm.log(logger::WARNING)
                        << "Execute prepared event queued after transaction close";
                if (evt.error) {
                    try {
                        evt.error( error::transaction_closed{} );
                    } catch (::std::exception const& e) {
                        fsm.log(logger::WARNING) << "Exception in execute prepared error handler " << e.what();
                    } catch (...) {
                        // Ignore handler error
                        fsm.log(logger::WARNING) << "Exception in execute prepared error handler";
                    }
                }
            }
        };
        //@}
        //@{
        /** @name Transaction sub-states */
        struct starting : state< starting > {
            using deferred_events = ::psst::meta::type_tuple<
                    events::execute,
                    events::execute_prepared,
                    events::commit,
                    events::rollback
                >;

            void
            on_enter(events::begin const& evt, transaction_fsm_type& fsm)
            {
                fsm.connection().send_begin(evt);
            }
            using internal_transitions = transition_table<
            /*      Event                       Action        Guard     */
            /*    +----------------------------+-----------+---------+*/
                in< command_complete,   none,        none     >
            >;
        };

        struct idle : state< idle > {
            using idle_fsm = ::afsm::state<idle, transaction_fsm_type>;

            idle_fsm&
            fsm()
            { return static_cast<idle_fsm&>(*this); }

            idle_fsm const&
            fsm() const
            { return static_cast<idle_fsm const&>(*this); }

            transaction_fsm_type&
            tran()
            { return fsm().enclosing_fsm(); }

            transaction_fsm_type const&
            tran() const
            { return fsm().enclosing_fsm(); }

            ::tip::log::local
            log(logger::event_severity s = PGFSM_DEFAULT_SEVERITY) const
            {
                return tran().log();
            }

            void
            on_exit(error::query_error const& err, transaction_fsm_type& fsm)
            {
                fsm.notify_error(err);
            }
            void
            on_exit(error::client_error const& err, transaction_fsm_type& fsm)
            {
                fsm.notify_error(err);
            }

            using internal_transitions = transition_table<
            /*                Event             Action                    Guard     */
            /*            +--------------------+-----------------------+---------+*/
                in< command_complete,           none,                    none     >,
                in< events::ready_for_query,    none,                    none     >
            >;
        };

        struct tran_error : state< tran_error > {
            using internal_transitions = transition_table<
            /*                Event              Action                    Guard     */
            /*            +---------------------+-----------------------+---------+*/
                in< events::commit,        none,                    none    >,
                in< events::rollback,      none,                    none    >
            >;
        };
        struct exiting : state< exiting > {
            template < typename Event >
            void
            on_enter(Event const&, transaction_fsm_type& fsm)
            {
                callback_ = notification_callback{};
            }
            void
            on_enter(events::commit const& evt, transaction_fsm_type& fsm)
            {
                callback_ = evt.callback;
            }
            void
            on_enter(events::rollback const& evt, transaction_fsm_type& fsm)
            {
                callback_ = evt.callback;
            }
            template < typename Event >
            void
            on_exit(Event const&, transaction_fsm_type& fsm)
            {
                if (callback_) {
                    auto cb = callback_;
                    fsm.connection().async_notify(
                        [cb](){
                            try {
                                cb();
                            } catch (::std::exception const& e) {
                                fsm_log(logger::WARNING) << "Exception in notify handler on exit transaction " << e.what();
                            } catch(...) {
                                // Ignore handler error
                                fsm_log(logger::WARNING) << "Exception in notify handler on exit transaction";
                            }
                        }
                    );
                    callback_ = notification_callback{};
                }
            }
            using internal_transitions = transition_table<
            /*                Event                Action                    Guard     */
            /*            +---------------------+-----------------------+---------+*/
                in< command_complete,        none,                    none    >,
                in< events::commit,          none,                    none    >,
                in< events::rollback,        none,                    none    >,
                in< events::execute,         tran_finished,           none    >,
                in< events::execute_prepared,tran_finished,           none    >
            >;

            notification_callback callback_;
        };
        //--------------------------------------------------------------------

        //--------------------------------------------------------------------
        //  Simple query sub state machine
        //--------------------------------------------------------------------
        struct simple_query : state_machine< simple_query > {
            using simple_query_fsm_type =
                    ::afsm::inner_state_machine<simple_query, transaction_fsm_type>;

            simple_query_fsm_type&
            fsm()
            { return static_cast<simple_query_fsm_type&>(*this); }

            simple_query_fsm_type const&
            fsm() const
            { return static_cast<simple_query_fsm_type const&>(*this); }

            transaction_fsm_type&
            tran()
            { return fsm().enclosing_fsm(); }

            transaction_fsm_type const&
            tran() const
            { return fsm().enclosing_fsm(); }

            connection_fsm_type&
            connection()
            { return tran().connection(); }

            connection_fsm_type const&
            connection() const
            { return tran().connection(); }

            ::tip::log::local
            log(logger::event_severity s = PGFSM_DEFAULT_SEVERITY) const
            {
                return tran().log(s);
            }

            void
            on_enter(events::execute const& q, transaction_fsm_type&)
            {
                log() << "Execute query: " << q.expression;
                query_ = q;
                message m(query_tag);
                m.write(q.expression);
                connection().send(::std::move(m));
            }
            template < typename Event, typename FSM >
            void
            on_exit(Event const&, FSM&)
            {
                query_ = events::execute{};
            }
            template < typename FSM >
            void
            on_exit(error::query_error const& err, FSM&)
            {
                tran().notify_error(*this, err);
                query_ = events::execute{};
            }
            template < typename FSM >
            void
            on_exit(error::client_error const& err, FSM&)
            {
                tran().notify_error(err);
                query_ = events::execute();
            }

            using deferred_events = ::psst::meta::type_tuple<
                    events::execute,
                    events::execute_prepared,
                    events::commit,
                    events::rollback
                >;

            //@{
            /** @name Simple query sub-states */
            struct waiting : state< waiting > {
                struct non_select_result {
                    void
                    operator()(command_complete const& evt, simple_query_fsm_type& fsm,
                            waiting&, waiting&)
                    {
                        fsm.tran().log() << "Non-select query complete "
                                << evt.command_tag;
                        // FIXME Non-select query result
                        fsm.tran().notify_result(fsm, result_ptr(new result_impl), true);
                    }
                };

                using internal_transitions = transition_table<
                    in< command_complete,    non_select_result,    none >
                >;
            };

            struct fetch_data : state< fetch_data> {
                using deferred_events = ::psst::meta::type_tuple< events::ready_for_query >;

                fetch_data() : result_( new result_impl ) {}

                void
                on_enter(events::row_description const& rd,
                        simple_query_fsm_type& fsm)
                {
                    // TODO Don't reset the resultset
                    result_.reset(new result_impl); // TODO Number of resultset
                    result_->row_description().swap(rd.fields);
                }

                template < typename Event >
                void
                on_exit(Event const&, simple_query_fsm_type& fsm)
                {
                    fsm.tran().notify_result(fsm, resultset(result_), true);
                }

                struct parse_data_row {
                    template < typename FSM, typename TargetState >
                    void
                    operator() (events::row_event const& row, FSM&,
                            fetch_data& fetch, TargetState&)
                    {
                        // TODO Move the data from event
                        fetch.result_->rows().push_back(row.move_row());
                    }
                };

                using internal_transitions = transition_table<
                    in< events::row_event,    parse_data_row,    none >
                >;

                result_ptr result_;
            };
            using initial_state = waiting;
            //@}
            //@{
            /** @name Transitions for simple query */
            using transitions = transition_table<
                /*      Start           Event                       Next            Action          Guard              */
                /*  +-----------------+----------------------------+---------------+---------------+-----------------+ */
                 tr<    waiting,        events::row_description,    fetch_data,     none,           none            >,
                 tr<    fetch_data,     command_complete,           waiting,        none,           none            >
            >;

            events::execute         query_;
        };
        //--------------------------------------------------------------------

        //--------------------------------------------------------------------
        //  Extended query sub state machine
        //--------------------------------------------------------------------
        struct extended_query : state_machine<extended_query> {
            using extended_query_fsm_type =
                    ::afsm::inner_state_machine<extended_query, transaction_fsm_type>;

            extended_query() : row_limit_(0),
                    result_(new result_impl) {}

            extended_query_fsm_type&
            fsm()
            { return static_cast<extended_query_fsm_type&>(*this); }

            extended_query_fsm_type const&
            fsm() const
            { return static_cast<extended_query_fsm_type const&>(*this); }

            transaction_fsm_type&
            tran()
            { return fsm().enclosing_fsm(); }

            transaction_fsm_type const&
            tran() const
            { return fsm().enclosing_fsm(); }

            connection_fsm_type&
            connection()
            { return tran().connection(); }

            connection_fsm_type const&
            connection() const
            { return tran().connection(); }

            ::tip::log::local
            log(logger::event_severity s = PGFSM_DEFAULT_SEVERITY) const
            {
                return tran().log(s);
            }

            void
            on_enter(events::execute_prepared const& q, transaction_fsm_type&)
            {
                query_ = q;
                std::ostringstream os;
                os << query_.expression;
                if (!query_.param_types.empty()) {
                    os << "{";
                    std::ostream_iterator< oids::type::oid_type > out(os, ",");
                    std::copy( query_.param_types.begin(), query_.param_types.end() - 1, out );
                    os << query_.param_types.back() << "}";
                }
                query_name_ = "q_" +
                    std::string( boost::md5( os.str().c_str() ).digest().hex_str_value() );
            }
            template < typename Event, typename FSM >
            void
            on_exit(Event const&, FSM&)
            {
                query_ = events::execute_prepared();
            }
            template < typename FSM >
            void
            on_exit(error::query_error const& err, FSM&)
            {
                tran().notify_error(*this, err);
                query_ = events::execute_prepared();
            }
            template < typename FSM >
            void
            on_exit(error::client_error const& err, FSM&)
            {
                tran().notify_error(err);
                query_ = events::execute_prepared();
            }

            bool
            is_query_prepared() const
            {
                return connection().is_prepared(query_name_);
            }

            void
            send_parse()
            {
                tran().log() << "Parse query " << query_.expression;
                message cmd(parse_tag);
                cmd.write(query_name_);
                cmd.write(query_.expression);
                cmd.write( (smallint)query_.param_types.size() );
                for (oids::type::oid_type oid : query_.param_types) {
                    cmd.write( (integer)oid );
                }

                message describe(describe_tag);
                describe.write('S');
                describe.write(query_name_);
                cmd.pack(describe);
                cmd.pack(message(sync_tag));

                connection().send(::std::move(cmd));
            }

            void
            send_bind_exec()
            {
                message cmd(bind_tag);
                cmd.write(portal_name_);
                cmd.write(query_name_);
                if (!query_.params.empty()) {
                    auto out = cmd.output();
                    std::copy(query_.params.begin(), query_.params.end(), out);
                } else {
                    cmd.write((smallint)0); // parameter format codes
                    cmd.write((smallint)0); // number of parameters
                }
                if (is_query_prepared()) {
                    events::row_description const& row =
                            connection().get_prepared(query_name_);
                    cmd.write((smallint)row.fields.size());
                    tran().log() << "Write " << row.fields.size() << " field formats";
                    for (auto const& fd : row.fields) {
                        cmd.write((smallint)fd.format_code);
                    }
                } else {
                    cmd.write((smallint)0); // no row description
                }

                tran().log() << "Execute prepared query: " << query_.expression;

                message execute(execute_tag);
                execute.write(portal_name_);
                execute.write(row_limit_);
                cmd.pack(execute);
                cmd.pack(message(sync_tag));

                connection().send(::std::move(cmd));
            }

            using deferred_events = ::psst::meta::type_tuple<
                    events::execute,
                    events::execute_prepared,
                    events::commit,
                    events::rollback
                >;

            //@{
            struct store_prepared_desc {
                template < typename SourceState, typename TargetState >
                void
                operator() (events::row_description const& row, extended_query_fsm_type& fsm,
                        SourceState&, TargetState&)
                {
                    for (auto& fd : row.fields) {
                        if (io::traits::has_binary_parser(fd.type_oid))
                            fd.format_code = BINARY_DATA_FORMAT;
                    }
                    fsm.result_.reset(new result_impl);
                    fsm.result_->row_description() = row.fields; // copy!
                    fsm.connection().set_prepared(fsm.query_name_, row);
                }
                template < typename SourceState, typename TargetState >
                void
                operator() (events::no_data const&, extended_query& fsm,
                        SourceState&, TargetState&)
                {
                    fsm.result_.reset(new result_impl);
                    events::row_description row;
                    fsm.connection().set_prepared(fsm.query_name_, row);
                }
            };
            struct skip_parsing {
                template < typename Event, typename SourceState,
                        typename TargetState>
                void
                operator()(Event const&, extended_query& fsm,
                        SourceState&, TargetState&)
                {
                    fsm.result_.reset(new result_impl);
                    fsm.result_->row_description() =
                            fsm.connection().get_prepared(fsm.query_name_).fields;
                }
            };
            struct parse_data_row {
                template < typename SourceState, typename TargetState >
                void
                operator() (events::row_event const& row, extended_query& fsm,
                        SourceState&, TargetState&)
                {
                    fsm.result_->rows().push_back(row.move_row());
                }
            };
            struct complete_execution {
                template < typename SourceState, typename TargetState >
                void
                operator() (command_complete const& complete, extended_query& fsm,
                        SourceState&, TargetState&)
                {
                    // TODO Process non-select query result
                }
            };
            //@}
            //@{
            /** @name Extended query sub-states */
            struct prepare : state< prepare > {};

            struct parse : state< parse > {
                template < typename Event >
                void
                on_enter(Event const&, extended_query& fsm)
                {
                    fsm.send_parse();
                }

                using internal_transitions = transition_table<
                    in< events::parse_complete,     none,                   none >,
                    in< events::row_description,    store_prepared_desc,    none >,
                    in< events::no_data,            store_prepared_desc,    none >
                >;
            };

            struct bind : state< bind > {
                using deferred_events = ::psst::meta::type_tuple<
                        events::row_event,
                        command_complete
                    >;
                template < typename Event >
                void
                on_enter( Event const&, extended_query_fsm_type& fsm )
                {
                    fsm.send_bind_exec();
                }
            };

            struct exec : state< exec > {
                template < typename Event >
                void
                on_exit(Event const&, extended_query_fsm_type& fsm)
                {
                    fsm.tran().log() << "Exit exec state resultset columns "
                            << fsm.result_->row_description().size()
                            << " rows " << fsm.result_->size();
                    fsm.tran().notify_result(fsm, resultset(fsm.result_), true);
                }

                void
                on_exit(error::query_error const& err, extended_query_fsm_type& fsm)
                {
                    fsm.tran().notify_error(fsm, err);
                }
                void
                on_exit(error::db_error const& err, extended_query_fsm_type& fsm)
                {
                    fsm.tran().notify_error(err);
                }

                using internal_transitions = transition_table<
                    in< events::row_event,  parse_data_row,           none >,
                    in< command_complete,   none,                     none >
                >;
            };

            using initial_state = prepare;
            //@}

            struct is_prepared
            {
                template < typename FSM, typename State >
                bool
                operator()(FSM& fsm, State&) const
                {
                    return fsm.connection().is_prepared(fsm.query_name_);
                }
                template < class EVT, class SourceState, class TargetState>
                bool
                operator()(EVT const&, extended_query_fsm_type& fsm, SourceState&,TargetState&)
                {
                    return fsm.connection().is_prepared(fsm.query_name_);
                }
            };
            //@{
            /** Transitions for extended query
             * https://www.postgresql.org/docs/9.4/static/protocol-flow.html#PROTOCOL-FLOW-EXT-QUERY
             */
            /** @todo Row limits and portal suspended state */
            /** @todo Exit on error handling */
            using transitions = transition_table<
                /*        Start         Event                   Next            Action            Guard                  */
                /*  +-----------------+------------------------+---------------+---------------+---------------------+ */
                 tr<    prepare,       none,                    parse,          none,            not_<is_prepared>    >,
                 tr<    prepare,       none,                    bind,           skip_parsing,    is_prepared         >,
                 tr<    parse,         events::ready_for_query, bind,           none,            none                >,
                 tr<    bind,          events::bind_complete,   exec,           none,            none                >
            >;
            //@}

            events::execute_prepared query_;
            std::string query_name_;
            std::string portal_name_;
            integer row_limit_;

            result_ptr result_;
        };  // extended_query

        using initial_state = starting;
        //@}


        //@{
        /** @name Transition table for transaction */
        using transitions = transition_table<
            /*        Start            Event                    Next            Action                        Guard                  */
            /* +--------------------+--------------------------+---------------+---------------------------+-----------+ */
             tr<    starting,           events::ready_for_query,idle,           transaction_started,        none            >,
             /* +--------------------+--------------------------+---------------+---------------------------+-----------+ */
             tr<    idle,               events::commit,         exiting,        commit_transaction,         none            >,
             tr<    idle,               events::rollback,       exiting,        rollback_transaction,       none            >,
             tr<    idle,               error::query_error,     exiting,        rollback_transaction,       none            >,
             tr<    idle,               error::client_error,    exiting,        rollback_transaction,       none            >,
             /* +--------------------+--------------------------+---------------+---------------------------+-----------+ */
             tr<    idle,               events::execute,        simple_query,   none,                       none            >,
             tr<    simple_query,       events::ready_for_query,idle,           none,                       none            >,
             tr<    simple_query,       error::query_error,     tran_error,     none,                       none            >,
             tr<    simple_query,       error::client_error,    tran_error,     none,                       none            >,
             tr<    simple_query,       error::db_error,        tran_error,     none,                       none            >,
             /* +--------------------+--------------------------+---------------+---------------------------+-----------+ */
             tr<    idle,               events::
                                          execute_prepared,     extended_query, none,                       none            >,
             tr<    extended_query,     events::ready_for_query,idle,           none,                       none            >,
             tr<    extended_query,     error::query_error,     tran_error,     none,                       none            >,
             tr<    extended_query,     error::client_error,    tran_error,     none,                       none            >,
             tr<    extended_query,     error::db_error,        tran_error,     none,                       none            >,
             /* +--------------------+--------------------------+---------------+---------------------------+-----------+ */
             tr<    tran_error,         events::ready_for_query,exiting,        rollback_transaction,        none            >
        >;

        //@}

        void
        notify_started()
        {
            if (callbacks_.started) {
                transaction_ptr t(new pg::transaction( connection().shared_from_this() ));
                tran_object_ = t;
                try {
                    callbacks_.started(t);
                } catch (error::query_error const& e) {
                    log(logger::ERROR)
                            << "Transaction started handler throwed a query_error:"
                            << e.what();
                    connection().process_event(e);
                } catch (error::db_error const& e) {
                    log(logger::ERROR)
                            << "Transaction started handler throwed a db_error: "
                            << e.what();
                    connection().process_event(e);
                } catch (std::exception const& e) {
                    log(logger::ERROR)
                            << "Transaction started handler throwed an exception: "
                            << e.what();
                    connection().process_event(error::client_error(e));
                } catch (...) {
                    log(logger::ERROR)
                            << "Transaction started handler throwed an unknown exception";
                    connection().process_event(error::client_error("Unknown exception"));
                }
            }
        }
        template < typename Source >
        void
        notify_result(Source& state, resultset res, bool complete)
        {
            if (state.query_.result) {
                auto result_cb = state.query_.result;
                auto conn = connection().shared_from_this();
                connection().async_notify(
                [conn, result_cb, res, complete](){
                    conn->log() << "In async notify";
                    try {
                        result_cb(res, complete);
                    } catch (error::query_error const& e) {
                        conn->log(logger::ERROR)
                                << "Query result handler throwed a query_error: "
                                << e.what();
                        conn->process_event(e);
                    } catch (error::db_error const& e) {
                        conn->log(logger::ERROR)
                                << "Query result handler throwed a db_error: "
                                << e.what();
                        conn->process_event(e);
                    } catch (std::exception const& e) {
                        conn->log(logger::ERROR)
                                << "Query result handler throwed an exception: "
                                << e.what();
                        conn->process_event(error::client_error(e));
                    } catch (...) {
                        conn->log(logger::ERROR)
                                << "Query result handler throwed an unknown exception";
                        conn->process_event(error::client_error("Unknown exception"));
                    }
                });
            }
        }

        void
        notify_error(error::db_error const& qe)
        {
            if (callbacks_.error) {
                // If the async error handler throws an exception
                // all we can do - log the error.
                auto error_cb = callbacks_.error;
                auto conn = connection().shared_from_this();
                connection().async_notify(
                [error_cb, conn, qe](){
                    try {
                        error_cb(qe);
                    } catch (std::exception const& e) {
                        conn->log(logger::ERROR)
                                << "Transaction error handler throwed an exception: "
                                << e.what();
                    } catch (...) {
                        conn->log(logger::ERROR)
                                << "Transaction error handler throwed an unknown exception.";
                    }
                });
            }
        }
        template < typename State >
        void
        notify_error(State& state, error::query_error const& qe)
        {
            if (state.query_.error) {
                try {
                    state.query_.error(qe);
                } catch (std::exception const& e) {
                    log(logger::ERROR)   << "Query error handler throwed an exception: "
                            << e.what();
                    notify_error(qe);
                    notify_error(error::client_error(e));
                } catch (...) {
                    log(logger::ERROR) << "Query error handler throwed an unexpected exception";
                    notify_error(qe);
                    notify_error(error::client_error("Unknown exception"));
                }
            } else {
                log(logger::WARNING) << "No query error handler";
                notify_error(qe);
            }
        }

        transaction_fsm_type&
        fsm()
        { return static_cast<transaction_fsm_type&>(*this); }
        transaction_fsm_type const&
        fsm() const
        { return static_cast<transaction_fsm_type const&>(*this); }

        connection_fsm_type&
        connection()
        { return fsm().enclosing_fsm(); }
        connection_fsm_type const&
        connection() const
        { return fsm().enclosing_fsm(); }

        //@{
        /** @name Transaction entry-exit */
        void
        on_enter(events::begin const& evt, connection_fsm_type& fsm)
        {
            connection().in_transaction_ = true;
            callbacks_ = evt;
        }
        template < typename Event, typename FSM >
        void
        on_exit(Event const&, FSM&)
        {
            connection().in_transaction_ = false;
            auto tran = tran_object_.lock();
            if (tran) {
                tran->mark_done();
            }
            tran_object_.reset();
            callbacks_ = events::begin{};
        }
        //@}

        ::tip::log::local
        log(logger::event_severity s = PGFSM_DEFAULT_SEVERITY) const
        {
            return connection().log(s);
        }
        events::begin           callbacks_;
        transaction_weak_ptr    tran_object_;
    }; // transaction state machine
    //------------------------------------------------------------------------

    using initial_state = unplugged;
    //@}
    //@{
    /** @name Connection state transition table */
    using transitions = transition_table<
        /*        Start     Event                       Next            Action                  Guard                      */
        /*  +--------------+---------------------------+---------------+-----------------------+---------------------+ */
        tr<    unplugged,   connection_options,         t_conn,         none,                   none                >,
        tr<    unplugged,   events::terminate,          terminated,     none,                   none                >,

        tr<    t_conn,      events::complete,           authn,          none,                   none                >,
        tr<    t_conn,      error::
                              connection_error,         terminated,     on_connection_error,    none                >,

        tr<    authn,       events::ready_for_query,    idle,           none,                   none                >,
        tr<    authn,       error::
                                connection_error,       terminated,     on_connection_error,    none                >,
        /*                                    Transitions from idle                                                      */
        /*  +-----------------+-------------------+---------------+---------------------------+---------------------+ */
        tr<    idle,        events::begin,              transaction,    none,                   none                >,
        tr<    idle,        events::terminate,          terminated,     disconnect,             none                >,
        tr<    idle,        error::
                              connection_error,         terminated,     on_connection_error,    none                >,
        /*  +-----------------+-------------------+---------------+---------------------------+---------------------+ */
        tr<    transaction, events::ready_for_query,    idle,           none,                   none                >,
        tr<    transaction, error::
                                connection_error,       terminated,     on_connection_error,    none                >
    >;
    //@}
    template< typename Event, typename FSM >
    void
    exception_caught(Event const&, FSM&, ::std::exception& e)
    {
        log(logger::WARNING) << "Exception escaped all catch handlers "
                << e.what();
    }

    //@{
    using io_service_ptr = asio_config::io_service_ptr;
    using client_options_type = std::map< std::string, std::string >;
    using shared_base = std::enable_shared_from_this< shared_type >;

    using asio_io_handler = std::function< void (asio_config::error_code const& error,
            size_t bytes_transferred) >;
    //@}

    //@{
    connection_fsm_def(io_service_ptr svc, client_options_type const& co)
        : shared_base(), io_service_{svc}, transport_{svc},
          client_opts_{co},
          serverPid_{0}, serverSecret_{0}, in_transaction_{false},
          connection_number_{ next_connection_number() }
    {
        incoming_.prepare(8192); // FIXME Magic number, move to configuration
    }
    virtual ~connection_fsm_def() {}
    //@}

    size_t
    number() const
    {
        return connection_number_;
    }

    void
    connect_transport(connection_options const& opts)
    {
        if (opts.uri.empty()) {
            throw error::connection_error("No connection uri!");
        }
        if (opts.database.empty()) {
            throw error::connection_error("No database!");
        }
        if (opts.user.empty()) {
            throw error::connection_error("User not specified!");
        }
        conn_opts_ = opts;
        auto _this = shared_base::shared_from_this();
        transport_.connect_async(conn_opts_,
            [_this](asio_config::error_code const& ec)
            {
                _this->handle_connect(ec);
            });
    }
    void
    close_transport()
    {
        transport_.close();
    }

    void
    start_read()
    {
        auto _this = shared_base::shared_from_this();
        transport_.async_read(incoming_,
            [_this](asio_config::error_code const& ec, size_t bytes_transferred)
            {
                _this->handle_read(ec, bytes_transferred);
            });
    }

    void
    send_startup_message()
    {
        message m(empty_tag);
        create_startup_message(m);
        send(::std::move(m));
    }
    void
    send_begin(events::begin const& evt)
    {
        log() << "Send begin";
        message m(query_tag);
        ::std::ostringstream cmd{"begin"};
        cmd << evt.mode;
        m.write(cmd.str());
        send(::std::move(m));
    }
    void
    send_commit()
    {
        log() << "Send commit";
        message m(query_tag);
        m.write("commit");
        send(::std::move(m));
    }
    void
    send_rollback()
    {
        log() << "Send rollback";
        message m(query_tag);
        m.write("rollback");
        send(::std::move(m));
    }
    void
    send(message&& m, asio_io_handler handler = asio_io_handler())
    {
        if (transport_.connected()) {
            auto msg = ::std::make_shared<message>(::std::move(m));
            auto data_range = msg->buffer();
            auto _this = shared_base::shared_from_this();
            auto write_handler =
                [_this, handler, msg](asio_config::error_code const& ec, size_t sz)
                {
                    if (handler)
                        handler(ec, sz);
                    else
                        _this->handle_write(ec, sz);
                };
            transport_.async_write(
                ASIO_NAMESPACE::buffer(&*data_range.first,
                        data_range.second - data_range.first),
                write_handler
            );
        }
    }

    connection_options const&
    options() const
    { return conn_opts_; }

    //@{
    /** @name Prepared queries */
    bool
    is_prepared ( std::string const& query ) const
    {
        return prepared_.count(query);
    }

    void
    set_prepared( std::string const& query, events::row_description const& row_desc )
    {
        prepared_.insert(std::make_pair(query, row_desc));
    }
    events::row_description const&
    get_prepared( std::string const& query_name ) const
    {
        auto f = prepared_.find(query_name);
        if (f != prepared_.end()) {
            return f->second;
        }
        throw error::db_error("Query is not prepared");
    }
    //@}

    //@{
    bool
    in_transaction() const
    {
        return in_transaction_;
    }
    //@}
    //@{
    /** @connection events notifications */
    void
    notify_idle()
    {
        try {
            do_notify_idle();
        } catch (::std::exception const& e) {
            log(logger::WARNING) << "Exception in on idle handler " << e.what();
        } catch (...) {
            // Ignore handler error
            log(logger::WARNING) << "Exception in on idle handler";
        }
    }
    void
    notify_terminated()
    {
        try {
            do_notify_terminated();
        } catch (::std::exception const& e) {
            log(logger::WARNING) << "Exception in terminated handler " << e.what();
        } catch (...) {
            // Ignore handler error
            log(logger::WARNING) << "Exception in terminated handler";
        }
    }
    void
    notify_error(error::connection_error const& e) { do_notify_error(e); }

    template < typename Handler >
    void
    async_notify(Handler&& h)
    {
        io_service_->post( ::std::forward<Handler>(h) );
    }
    //@}

    ::tip::log::local
    log(logger::event_severity s = PGFSM_DEFAULT_SEVERITY) const
    {
        return fsm_log(s) << "Conn# " << connection_number_ << ": ";
    }
private:
    virtual void do_notify_idle() {};
    virtual void do_notify_terminated() {};
    virtual void do_notify_error(error::connection_error const&) {};
private:
    connection_fsm_type&
    fsm()
    {
        return static_cast< connection_fsm_type& >(*this);
    }
    connection_fsm_type const&
    fsm() const
    {
        return static_cast< connection_fsm_type const& >(*this);
    }

    void
    handle_connect(asio_config::error_code const& ec)
    {
        if (!ec) {
            fsm().process_event(events::complete{});
        } else {
            fsm().process_event( error::connection_error{ec.message()} );
        }
    }
    void
    handle_read(asio_config::error_code const& ec, size_t bytes_transferred)
    {
        if (!ec) {
            // read message
            std::istreambuf_iterator<char> in(&incoming_);
            read_message(in, bytes_transferred);
            // start async operation again
            start_read();
        } else {
            // Socket error - force termination
            fsm().process_event(error::connection_error(ec.message()));
        }
    }
    void
    handle_write(asio_config::error_code const& ec, size_t)
    {
        if (ec) {
            // Socket error - force termination
            fsm().process_event(error::connection_error(ec.message()));
        }
    }

    template < typename InputIter, typename OutputIter >
    InputIter
    copy(InputIter in, InputIter end, size_t max, OutputIter out)
    {
        for (size_t i = 0; i < max && in != end; ++i) {
            *out++ = *in++;
        }
        return in;
    }

    void
    read_message( std::istreambuf_iterator< char > in, size_t max_bytes )
    {
        const size_t header_size = sizeof(integer) + sizeof(byte);
        while (max_bytes > 0) {
            if (!message_) {
                message_.reset(new detail::message);
            }
            auto out = message_->output();

            std::istreambuf_iterator<char> eos;
            if (message_->buffer_size() < header_size) {
                // Read the header
                size_t to_read = std::min((header_size - message_->buffer_size()), max_bytes);
                in = copy(in, eos, to_read, out);
                max_bytes -= to_read;
            }
            if (message_->length() > message_->size()) {
                // Read the message body
                size_t to_read = std::min(message_->length() - message_->size(), max_bytes);
                in = copy(in, eos, to_read, out);
                max_bytes -= to_read;
                assert(message_->size() <= message_->length()
                        && "Read too much from the buffer" );
            }
            if (message_->size() >= 4 && message_->length() == message_->size()) {
                message_ptr m = message_;
                m->reset_read();
                handle_message(m);
                message_.reset();
            }
        }
    }

    void
    create_startup_message(message& m)
    {
        m.write(PROTOCOL_VERSION);
        // Create startup packet
        m.write(options::USER);
        m.write(conn_opts_.user);
        m.write(options::DATABASE);
        m.write(conn_opts_.database);

        for (auto opt : client_opts_) {
            m.write(opt.first);
            m.write(opt.second);
        }
        // trailing terminator
        m.write('\0');
    }

    void
    handle_message(message_ptr m)
    {
        message_tag tag = m->tag();
        if (message::backend_tags().count(tag)) {
            switch (tag) {
                case authentication_tag: {
                    integer auth_state(-1);
                    m->read(auth_state);
                    fsm().process_event(
                            events::authn_event{ (auth_states)auth_state, m });
                    break;
                }
                case command_complete_tag: {
                    command_complete cmpl;
                    m->read(cmpl.command_tag);
                    log() << "Command complete ("
                            << cmpl.command_tag << ")";
                    fsm().process_event(cmpl);
                    break;
                }
                case backend_key_data_tag: {
                    m->read(serverPid_);
                    m->read(serverSecret_);
                    break;
                }
                case error_response_tag: {
                    notice_message msg;
                    m->read(msg);

                    log(logger::ERROR) << "Error " << msg ;
                    error::query_error err(msg.message, msg.severity,
                            msg.sqlstate, msg.detail);
                    fsm().process_event(err);
                    break;
                }
                case parameter_status_tag: {
                    std::string key;
                    std::string value;

                    m->read(key);
                    m->read(value);

                    log() << "Parameter " << key << " = " << value;
                    client_opts_[key] = value;
                    break;
                }
                case notice_response_tag : {
                    notice_message msg;
                    m->read(msg);
                    log(logger::INFO) << "Notice " << msg;
                    break;
                }
                case ready_for_query_tag: {
                    char stat(0);
                    m->read(stat);
                    log() << "Database "
                        << (util::CLEAR) << (util::RED | util::BRIGHT)
                        << conn_opts_.uri
                        << "[" << conn_opts_.database << "]"
                        << logger::severity_color()
                        << " is ready for query (" << stat << ")";
                    fsm().process_event(events::ready_for_query{ stat });
                    break;
                }
                case row_description_tag: {
                    events::row_description rd;
                    smallint col_cnt;
                    m->read(col_cnt);
                    rd.fields.reserve(col_cnt);
                    for (int i =0; i < col_cnt; ++i) {
                        field_description fd;
                        if (m->read(fd)) {
                            rd.fields.push_back(fd);
                        } else {
                            log(logger::ERROR)
                                    << "Failed to read field description " << i;
                            // FIXME Process error
                        }
                    }
                    fsm().process_event(rd);
                    break;
                }
                case data_row_tag: {
                    events::row_event r;
                    if (m->read(r.row())) {
                        fsm().process_event(r);
                    } else {
                        // FIXME Process error
                        log(logger::ERROR) << "Failed to read data row";
                    }
                    break;
                }
                case parse_complete_tag: {
                    log() << "Parse complete";
                    fsm().process_event(events::parse_complete{});
                    break;
                }
                case parameter_desription_tag: {
                    log() << "Parameter descriptions";
                    break;
                }
                case bind_complete_tag: {
                    log() << "Bind complete";
                    fsm().process_event(events::bind_complete{});
                    break;
                }
                case no_data_tag: {
                    fsm().process_event(events::no_data{});
                    break;
                }
                case portal_suspended_tag : {
                    log() << "Portal suspended";
                    break;
                }
                default: {
                    {
                        log(logger::DEBUG)
                                << "Unhandled message "
                                << (util::MAGENTA | util::BRIGHT)
                                << (char)tag
                                << logger::severity_color();
                    }
                    break;
                }
            }

        }
    }

    static size_t
    next_connection_number()
    {
        static std::atomic<size_t> _number{0};
        return _number++;
    }
private:
    friend class transaction;
    asio_config::io_service_ptr     io_service_;
    transport_type                  transport_;

    client_options_type             client_opts_;

    ASIO_NAMESPACE::streambuf       incoming_;

    message_ptr                     message_;

    integer                         serverPid_;
    integer                         serverSecret_;

    prepared_statements_map         prepared_;

    ::std::atomic<bool>             in_transaction_;

    size_t                          connection_number_;
protected:
    connection_options              conn_opts_;
};

//----------------------------------------------------------------------------
// Concrete connection
//----------------------------------------------------------------------------
template < typename TransportType >
class concrete_connection : public basic_connection,
    public ::afsm::state_machine< connection_fsm_def< ::std::mutex, TransportType,
        concrete_connection< TransportType > >, ::std::mutex, connection_observer > {
public:
    using transport_type = TransportType;
    using this_type = concrete_connection< transport_type >;
    using fsm_type =
            ::afsm::state_machine<
                 connection_fsm_def< ::std::mutex, transport_type, this_type >,
                 ::std::mutex, connection_observer >;
public:
    concrete_connection(io_service_ptr svc,
            client_options_type const& co,
            connection_callbacks const& callbacks)
        : basic_connection(), fsm_type(svc, co),
          strand_(*svc),
          callbacks_(callbacks)
    {
        if (PGFSM_DEFAULT_SEVERITY > logger::OFF)
            fsm_type::make_observer();
    }
    virtual ~concrete_connection() {}
    using fsm_type::log;
private:
    //@{
    /** @name State machine abstract interface implementation */
    virtual void
    do_notify_idle() override
    {
        if (callbacks_.idle) {
            callbacks_.idle(fsm_type::shared_from_this());
        } else {
            log(logger::WARNING) << "No connection idle callback";
        }
    }
    virtual void
    do_notify_terminated() override
    {
        if (callbacks_.terminated) {
            callbacks_.terminated(fsm_type::shared_from_this());
        } else {
            log() << "No connection terminated callback";
        }
        callbacks_ = connection_callbacks(); // clean up callbacks, no work further.
    }
    virtual void
    do_notify_error(error::connection_error const& e) override
    {
        log(logger::ERROR) << "Connection error " << e.what();
        if (callbacks_.error) {
            callbacks_.error(connection_ptr(), e);
        } else {
            log(logger::ERROR) << "No connection_error callback";
        }
    }
    //@}

    virtual void
    do_connect(connection_options const& co) override
    {
        fsm_type::process_event(co);
    }

    virtual dbalias const&
    get_alias() const override
    {
        return fsm_type::conn_opts_.alias;
    }

    virtual bool
    is_in_transaction() const override
    {
        return fsm_type::in_transaction();
    }
    virtual void
    do_begin(events::begin&& evt) override
    {
        if (fsm_type::in_transaction()) {
            log(logger::ERROR) << "Cannot begin transaction: already in transaction";
            throw error::db_error("Already in transaction");
        }
        fsm_type::process_event(::std::move(evt));
    }

    virtual void
    do_commit(notification_callback cb) override
    {
        if (!fsm_type::in_transaction()) {
            log(logger::ERROR) << "Cannot commit transaction: not in transaction";
            throw error::db_error("Not in transaction");
        }
        fsm_type::process_event(events::commit{cb});
    }

    virtual void
    do_rollback(notification_callback cb) override
    {
        if (!fsm_type::in_transaction()) {
            log(logger::ERROR) << "Cannot rollback transaction: not in transaction";
            throw error::db_error("Not in transaction");
        }
        fsm_type::process_event(events::rollback{cb});
    }

    virtual void
    do_execute(events::execute&& query) override
    {
        fsm_type::process_event(::std::move(query));
    }

    virtual void
    do_execute(events::execute_prepared&& query) override
    {
        fsm_type::process_event(::std::move(query));
    }

    virtual void
    do_terminate() override
    {
        fsm_type::process_event(events::terminate{});
    }

    asio_config::io_service::strand&
    strand() override
    { return strand_; }
private:
    asio_config::io_service::strand strand_;
    connection_callbacks            callbacks_;
};

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_BASIC_CONNECTION_NEW_HPP_ */
