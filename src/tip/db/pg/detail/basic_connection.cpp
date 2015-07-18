/*
 * connection_base.cpp
 *
 *  Created on: 11 июля 2015 г.
 *      Author: brysin
 */

#include <tip/db/pg/detail/basic_connection.hpp>

#include <tip/db/pg/detail/startup.hpp>
#include <tip/db/pg/detail/terminated_state.hpp>
#include <tip/db/pg/error.hpp>

#include <tip/log/log.hpp>
#include <tip/log/ansi_colors.hpp>

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

namespace options {

const std::string HOST				= "host";
const std::string PORT				= "port";
const std::string USER				= "user";
const std::string DATABASE			= "database";
const std::string CLIENT_ENCODING	= "client_encoding";
const std::string APPLICATION_NAME	= "application_name";

}  // namespace options

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
		event_callback ready,
		event_callback terminated,
		connection_error_callback err,
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
	{
		local_log() << (util::CLEAR) << (util::MAGENTA | util::BRIGHT) << "* Lock connection";
	}
	locked_ = true;
}

void
connection_base::unlock()
{
	assert(locked_ && "Attempt to release a lock from an unlocked connection");
	{
		local_log() << (util::CLEAR) << (util::MAGENTA | util::BRIGHT) << "* Unlock connection";
	}
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
	if (!message_) {
		message_.reset(new detail::message);
	}
	auto out = message_->output();

	std::istreambuf_iterator<char> eos;
	if (message_->length() == 0) {
		// Read the header
		size_t to_read = std::min(5ul, max_bytes);
		in = copy(in, eos, to_read, out);
		max_bytes -= to_read;
	}
	if (message_->length() > message_->size()) {
		// Read the message body
		size_t to_read = std::min(message_->length() - message_->size(), max_bytes);
		in = copy(in, eos, to_read, out);
		max_bytes -= to_read;
	}
	if (message_->length() == message_->size()) {
		message_ptr m = message_;
		m->reset_read();
		handle_message(m);
		message_.reset();
	}
	if (max_bytes > 0) {
		read_message(in, max_bytes);
	}
}

void
connection_base::handle_message(message_ptr m)
{
	message_tag tag = m->tag();
	if (message::backend_tags().count(tag)) {
		if (tag == error_response_tag) {
			notice_message msg;
			m->read(msg);

			if (!state_.handle_error(msg)) {
				local_log(logger::ERROR)
						<< "Error " << msg << " is not handled in "
						<< state_.name() << " state";
			}
		} else {
			switch(tag) {
				// TODO Handle server general messages
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
				default: {
					if (!state_.handle_message(m)) {
						local_log(logger::WARNING) << "Tag '" << (char)tag
								<< "' is not handled in " << state_.name()
								<< " state";
					}
					break;
				}
			}
		}
	} else {
		local_log(logger::ERROR) << "Unknown command from the backend " << (char)tag;
	}
}

void
connection_base::begin_transaction(simple_callback cb, error_callback err, bool autocommit)
{
	state_.begin_transaction(cb, err, autocommit);
}

void
connection_base::commit_transaction(simple_callback cb, error_callback err)
{
	state_.commit_transaction(cb, err);
}

void
connection_base::rollback_transaction(simple_callback cb, error_callback err)
{
	state_.rollback_transaction(cb, err);
}

bool
connection_base::in_transaction() const
{
	return state_.in_transaction();
}

void
connection_base::execute_query(std::string const& q, result_callback cb, query_error_callback err)
{
	state_.execute_query(q, cb, err);
}
void
connection_base::handle_connect(error_code const& ec)
{
	if (!ec) {
		local_log() << "Postgre server @"
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< uri()
				<< "[" << database() << "]"
				<< logger::severity_color()
				<< " connected";
		start_read();

		message m(detail::empty_tag);
		create_startup_message(m);
		local_log() << "Startup message size " << m.size();
		send(m);
	} else {
		local_log(logger::ERROR) << "Error connecting to db: " << ec.message();
		error(connection_error(ec.message()));
	}
}

void
connection_base::handle_write(error_code const& ec, size_t bytes_transfered)
{
	if (!ec) {
		local_log() << "Send message: " << bytes_transfered << " bytes sent";
	} else {
		local_log(logger::ERROR) << "Error sending message: " << ec.message();
	}
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



