/*
 * connection_base.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/detail/basic_connection.hpp>

#include <tip/db/pg/detail/startup.hpp>
#include <tip/db/pg/detail/terminated_state.hpp>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/log.hpp>

#include <boost/bind.hpp>
#include <assert.h>

namespace tip {
namespace db {
namespace pg {
namespace detail {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGCONN";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
using tip::log::logger;

template < typename InputIter, typename OutputIter >
InputIter
copy(InputIter in, InputIter end, size_t max, OutputIter out)
{
	for (int i = 0; i < max && in != end; ++i) {
		*out++ = *in++;
	}
	return in;
}

connection_base::connection_base(io_service& service,
		connection_options const& co,
		event_callback const& ready,
		event_callback const& terminated,
		connection_error_callback const& err,
		options_type const& aux)
	:
		conn(co), strand_(service), settings_(aux),
		serverPid_(0), serverSecret_(0),
		state_(*this),
		ready_(ready),
		terminated_(terminated),
		err_(err),
		locked_(false)
{
	//state_.push(connection_state_ptr(new startup_state(*this)));
	incoming_.prepare(8192);
}

connection_base::~connection_base()
{
	local_log(logger::DEBUG) << "**** connection_base::~connection_base()";
}

connection::state_type
connection_base::connection_state() const
{
	if (locked_)
		return connection::LOCKED;
	return state_.state();
}

void
connection_base::transit_state(connection_state_ptr state)
{
	state_.transit_state(state);
}

void
connection_base::push_state(connection_state_ptr state)
{
	state_.push_state(state);
}

void
connection_base::pop_state(basic_state* sender)
{
	state_.pop_state(sender);
}

connection_state_ptr
connection_base::state()
{
	return state_.get();
}

void
connection_base::ready()
{
	if (ready_) {
		ready_(shared_from_this());
	}
}

void
connection_base::error(connection_error const& ec)
{
	if (err_) {
		err_(ec);
	}
}

void
connection_base::terminate()
{
	if (!is_terminated()) {
		state_.terminate([this](){
			transit_state(connection_state_ptr(new terminated_state(*this)));
		});
	}
}

void
connection_base::notify_terminated()
{
	if (terminated_) {
		terminated_(shared_from_this());

		ready_ = event_callback();
		terminated_ = event_callback();
		err_ = connection_error_callback();
	}
	close();
}

bool
connection_base::is_terminated() const
{
	return std::dynamic_pointer_cast< terminated_state const >(state_.get()).get();
}

void
connection_base::lock()
{
	assert(!locked_ && "Attempt to obtain a lock on a locked connection");
	#ifdef WITH_TIP_LOG
	{
		local_log() << (util::CLEAR) << (util::MAGENTA | util::BRIGHT) << "* Lock connection";
	}
	#endif
	locked_ = true;
}

void
connection_base::unlock()
{
	assert(locked_ && "Attempt to release a lock from an unlocked connection");
	#ifdef WITH_TIP_LOG
	{
		local_log() << (util::CLEAR) << (util::MAGENTA | util::BRIGHT) << "* Unlock connection";
	}
	#endif
	locked_ = false;
	state_.handle_unlocked();
}

bool
connection_base::locked() const
{
	return locked_;
}

void
connection_base::create_startup_message(message& m)
{
	m.write(PROTOCOL_VERSION);
	// Create startup packet
	m.write(options::USER);
	m.write(conn.user);
	m.write(options::DATABASE);
	m.write(conn.database);

	for (auto opt : settings_) {
		m.write(opt.first);
		m.write(opt.second);
	}
	// trailing terminator
	m.write('\0');
}

void
connection_base::read_message(std::istreambuf_iterator<char> in, size_t max_bytes)
{
	{
		local_log() << "Received message " << max_bytes << " bytes";
	}
	const size_t header_size = sizeof(integer) + sizeof(byte);
    while (max_bytes > 0) {
        size_t loop_beg = max_bytes;
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
        {
            local_log(logger::OFF) << loop_beg - max_bytes << " bytes consumed, " << max_bytes << " bytes left";
        }
    }
}

void
connection_base::handle_message(message_ptr m)
{
	message_tag tag = m->tag();
    {
        local_log(logger::TRACE) << "Handle message "
        		<< (util::MAGENTA | util::BRIGHT)
        		<< (char)tag
        		<< logger::severity_color()
        		<< " state " << state_.name();
    }
	if (message::backend_tags().count(tag)) {
		switch(tag) {
			case error_response_tag: {
				notice_message msg;
				m->read(msg);

				if (!state_.handle_error(msg)) {
					local_log(logger::ERROR)
							<< "Error " << msg << " is not handled in "
							<< state_.name() << " state";
				}
				break;
			}
			case parameter_status_tag: {
				std::string key;
				std::string value;

				m->read(key);
				m->read(value);

				//local_log() << "Parameter " << key << " = " << value;
				settings_[key] = value;
				break;
			}
			case backend_key_data_tag: {
				m->read(serverPid_);
				m->read(serverSecret_);
				//local_log() << "Server pid: " << serverPid_ << " secret: " << serverSecret_;
				break;
			}
			case notice_response_tag : {
				notice_message msg;
				m->read(msg);
				local_log(logger::INFO) << "Notice " << msg;
				break;
			}
			case command_complete_tag: {
				command_complete complete;
				m->read(complete.command_tag);
				if (!state_.handle_complete(complete)) {
					local_log(logger::WARNING) << "Command complete ("
							<< complete.command_tag << ") is not handled in "
							<< state_.name() << " state";
				}
				break;
			}
			default: {
				if (!state_.handle_message(m)) {
					#ifdef WITH_TIP_LOG
					local_log(logger::WARNING) << "Tag '" << (char)tag
							<< "' is not handled in " << state_.name()
							<< " state";
					#endif
				}
				break;
			}
		}
	} else {
		local_log(logger::ERROR) << "Unknown command from the backend " << (char)tag;
	}
}

void
connection_base::begin_transaction(simple_callback const& cb,
		error_callback const& err, bool autocommit)
{
	if (!in_transaction()) {
		state_.begin_transaction(cb, err, autocommit);
	} else {
		std::ostringstream msg;
		msg << "Cannot start transaction in " << state_.name() << " state";
		local_log(logger::ERROR) << msg.str();
		if (!err)
			throw query_error(msg.str());
		else
			err(query_error(msg.str()));
	}
}

void
connection_base::commit_transaction(simple_callback const& cb,
		error_callback const& err)
{
	if (in_transaction()) {
		local_log() << "Commit transaction";
		state_.commit_transaction(cb, err);
	} else {
		std::ostringstream msg;
		msg << "Cannot commit transaction in " << state_.name() << " state";
		local_log(logger::ERROR) << msg.str();
		if (!err)
			throw query_error(msg.str());
		else
			err(query_error(msg.str()));
	}
}

void
connection_base::rollback_transaction(simple_callback const& cb,
		error_callback const& err)
{
	if (in_transaction()) {
		local_log() << "Rollback transaction";
		state_.rollback_transaction(cb, err);
	} else {
		std::ostringstream msg;
		msg << "Cannot rollback transaction in " << state_.name() << " state";
		local_log(logger::ERROR) << msg.str();
		if (!err)
			throw query_error(msg.str());
		else
			err(query_error(msg.str()));
	}
}

bool
connection_base::in_transaction() const
{
	return state_.in_transaction();
}

void
connection_base::execute_query(std::string const& q, result_callback const& cb,
		query_error_callback const& err)
{
	state_.execute_query(q, cb, err);
}

void
connection_base::execute_prepared(std::string const& q,
		type_oid_sequence const& param_types,
		buffer_type const& params,
		result_callback const& res,
		query_error_callback const& err)
{
	state_.execute_prepared(q, param_types, params, res, err);
}

bool
connection_base::is_prepared(std::string const& query_hash) const
{
	return prepared_statements_.count(query_hash);
}

void
connection_base::set_prepared(std::string const& query_hash,
		row_description const& result_desc)
{
	prepared_statements_.insert(std::make_pair(query_hash, result_desc));
}

connection_base::row_description const&
connection_base::get_prepared_description(std::string const& query_hash) const
{
	auto f = prepared_statements_.find(query_hash);
	if (f == prepared_statements_.end())
		throw std::runtime_error("Statement is not prepared");
	return f->second;
}


void
connection_base::handle_connect(error_code const& ec)
{
	if (!ec) {
		#ifdef WITH_TIP_LOG
		local_log() << "Postgre server @"
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< uri()
				<< "[" << database() << "]"
				<< logger::severity_color()
				<< " connected";
		#endif
		start_read();

		message m(detail::empty_tag);
		create_startup_message(m);
		#ifdef WITH_TIP_LOG
		local_log() << "Startup message size " << m.size();
		#endif
		send(m);
	} else {
		#ifdef WITH_TIP_LOG
		local_log(logger::ERROR) << "Error connecting to db: " << ec.message();
		#endif
		error(connection_error(ec.message()));
	}
}

void
connection_base::handle_write(error_code const& ec, size_t bytes_transfered)
{
	#ifdef WITH_TIP_LOG
	if (!ec) {
		local_log() << "Send message: " << bytes_transfered << " bytes sent";
	} else {
		local_log(logger::ERROR) << "Error sending message: " << ec.message();
	}
	#endif
}

void
connection_base::read_package_complete(size_t bytes)
{
	state_.package_complete(bytes);
}

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip



