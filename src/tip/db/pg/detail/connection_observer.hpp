/*
 * connection_observer.hpp
 *
 *  Created on: Jun 3, 2016
 *      Author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_OBSERVER_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_OBSERVER_HPP_

#include <afsm/fsm.hpp>
#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {


struct connection_observer {
    template < typename FSM, typename Event >
    void
    start_process_event(FSM const& fsm, Event const&) const noexcept
    {
        using decayed_event = typename ::std::decay<Event>::type;
        using tip::util::ANSI_COLOR;
        fsm.log() << typeid(Event).name() << ": Start processing";
    }

    template < typename FSM >
    void
    start_process_event(FSM const& fsm, ::afsm::none const&) const noexcept
    {
        fsm.log() << "[default]: Start processing";
    }

    template < typename FSM >
    void
    state_changed(FSM const& fsm) const noexcept
    {
        fsm.log(log::logger::DEBUG) << "State changed to " << fsm.state_name();
    }

    template < typename FSM, typename Event >
    void
    processed_in_state(FSM const& fsm, Event const&) const noexcept
    {
        fsm.log(log::logger::DEBUG) << typeid(Event).name() << ": processed in state "
                << fsm.state_name();
    }

    template < typename FSM, typename Event >
    void
    enqueue_event(FSM const& fsm, Event const&) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::MAGENTA
                << typeid(Event).name() << ": Enqueue";
    }

    template < typename FSM >
    void
    start_process_events_queue(FSM const& fsm) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::MAGENTA
                << "Start processing event queue";
    }
    template < typename FSM >
    void
    end_process_events_queue(FSM const& fsm) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::MAGENTA
                << "End processing event queue";
    }

    template < typename FSM, typename Event >
    void
    defer_event(FSM const& fsm, Event const&) const noexcept
    {
        fsm.log() << (util::ANSI_COLOR::CYAN | util::ANSI_COLOR::BRIGHT)
                << fsm.state_name() << " "
                << typeid(Event).name() << ": Defer";
    }

    template < typename FSM >
    void
    start_process_deferred_queue(FSM const& fsm) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::CYAN
                << "Start processing deferred queue";
    }
    template < typename FSM >
    void
    end_process_deferred_queue(FSM const& fsm) const noexcept
    {
        fsm.log() << util::ANSI_COLOR::CYAN
                << "End processing deferred queue";
    }

    template < typename FSM, typename Event >
    void
    reject_event(FSM const& fsm, Event const&) const noexcept
    {
        fsm.log(log::logger::ERROR) << (util::ANSI_COLOR::RED | util::ANSI_COLOR::BRIGHT)
                << fsm.state_name() << " "
                << typeid(Event).name() << ": Reject.";
    }
};

}  /* namespace detail */
}  /* namespace pg */
}  /* namespace db */
}  /* namespace tip */



#endif /* TIP_DB_PG_DETAIL_CONNECTION_OBSERVER_HPP_ */
