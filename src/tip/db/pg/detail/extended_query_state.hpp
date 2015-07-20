/*
 * prepare_state.hpp
 *
 *  Created on: Jul 20, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_PREPARE_STATE_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_PREPARE_STATE_HPP_

#include <tip/db/pg/detail/fetch_state.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

class extended_query_state: public basic_state {
public:
	extended_query_state(connection_base&, std::string const& query,
			result_callback cb, query_error_callback err);
	virtual ~extended_query_state() {}
private:
	virtual void
	do_enter();

	virtual std::string const
	get_name() const { return "extended query"; }
	virtual connection::state_type
	get_state() const { return connection::BUSY; }
	virtual bool
	do_handle_message(message_ptr);
private:
	enum stage_type {
		PARSE,
		BIND,
		FETCH
	};
private:
	std::string query_;
	result_callback result_;
	query_error_callback error_;
	stage_type stage_;
};

class parse_state: public basic_state {
public:
	parse_state(connection_base&, std::string const& query_name,
			std::string const& query,
			query_error_callback err);
	virtual ~parse_state() {}
private:
	virtual void
	do_enter();

	virtual std::string const
	get_name() const { return "prepare"; }
	virtual connection::state_type
	get_state() const { return connection::BUSY; }
	virtual bool
	do_handle_message(message_ptr);
private:
	std::string query_name_;
	std::string query_;
	query_error_callback error_;
};

class bind_state : public basic_state {
public:
	bind_state(connection_base&,
			std::string const& query_name,
			std::string const& portal_name);
	virtual ~bind_state() {}
private:
	virtual void
	do_enter();

	virtual std::string const
	get_name() const { return "bind"; }
	virtual connection::state_type
	get_state() const { return connection::BUSY; }
	virtual bool
	do_handle_message(message_ptr);
private:
	std::string query_name_;
	std::string portal_name_;
};

class execute_state : public fetch_state {
public:
	execute_state(connection_base&, std::string const& portal_name,
			result_callback cb, query_error_callback err);
	virtual ~execute_state() {}
private:
	virtual void
	do_enter();

	virtual std::string const
	get_name() const { return "execute"; }
	virtual connection::state_type
	get_state() const { return connection::BUSY; }
	virtual bool
	do_handle_message(message_ptr);
private:
	std::string portal_name_;
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_PREPARE_STATE_HPP_ */
