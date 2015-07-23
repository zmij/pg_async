/**
 * extended-query-mode.cpp
 *
 *  Created on: Jul 23, 2015
 *      Author: zmij
 */

#include <tip/db/pg/connection.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/resultset.inl>
#include <tip/db/pg/database.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/query.inl>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/detail/protocol.hpp>

#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/connection_pool.hpp>
#include <tip/db/pg/detail/connection_lock.hpp>

#include <tip/db/pg/log.hpp>

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>

#include "db/config.hpp"

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGTEST";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;

namespace {

std::string test_database;

}  // namespace

void
check_params_buffer(std::vector<char>& buffer, int expected_params)
{
	typedef std::vector<char> buffer_type;
	typedef buffer_type::iterator buffer_iterator;
	using namespace tip::db::pg;
	// check the buffer
	buffer_iterator p = buffer.begin();
	buffer_iterator e = buffer.end();
	smallint data_format_count(0);
	p = protocol_read< BINARY_DATA_FORMAT >(p, e, data_format_count);
	if (data_format_count != expected_params) {
		local_log(logger::ERROR) << "Data format count is invalid. "
				<< "Expected " << expected_params
				<< ", actual " << data_format_count;
	}
	smallint df(0);
	for (smallint dfc = 0; dfc < data_format_count && p != e; ++dfc) {
		p = protocol_read< BINARY_DATA_FORMAT >(p, e, df);
	}
	smallint param_count(0);
	p = protocol_read< BINARY_DATA_FORMAT >(p, e, param_count);
	if (param_count != expected_params) {
		local_log(logger::ERROR) << "Param count is invalid. "
				<< "Expected " << expected_params
				<< ", actual " << param_count;
	}
	for (smallint pno = 0; pno < param_count && p != e; ++pno) {
		integer param_size(0);
		p = protocol_read< BINARY_DATA_FORMAT >(p, e, param_size);
		if (param_size == 0)
			local_log(logger::WARNING) << "Param value size is 0";
		p += param_size;
	}

	if (p != e) {
		local_log(logger::ERROR) << "Buffer is not exhausted";
	} else {
		local_log() << "Buffer test is OK";
	}

}

void
print_buf(std::vector<char>& buffer)
{
	std::ostringstream os;
	for (char c : buffer) {
		os << std::setw(2) << std::setbase(16) << std::setfill('0') << (int)c << " ";
	}
	local_log() << "Buffer content " << os.str();
}

int
test_write_params()
{
	using namespace tip::db::pg;
	local_log() << "test_write_params: start";
	typedef std::vector<char> buffer_type;
	typedef buffer_type::iterator buffer_iterator;
	buffer_type buffer;

	{
		buffer.clear();
		tip::db::pg::detail::write_params(buffer, (integer)42);
		print_buf(buffer);
		if (buffer.empty()) {
			std::cerr << "Buffer is empty\n";
			return 1;
		}
		size_t expected = sizeof(smallint) * 3 + sizeof(integer) + sizeof(integer);
		if (buffer.size() != expected) {
			std::cerr << "Test 2: Invalid buffer size. Expected: "
					<< expected
					<< " actual " << buffer.size();
			return 1;
		}

		smallint expected_params = 1;

		buffer_iterator p = buffer.begin();
		buffer_iterator e = buffer.end();
		smallint data_format_count(0);
		p = protocol_read< BINARY_DATA_FORMAT >(p, e, data_format_count);
		if (data_format_count != expected_params) {
			local_log(logger::ERROR) << "Data format count is invalid. "
					<< "Expected " << expected_params
					<< ", actual " << data_format_count;
		}
		smallint df(0);
		for (smallint dfc = 0; dfc < data_format_count && p != e; ++dfc) {
			p = protocol_read< BINARY_DATA_FORMAT >(p, e, df);
		}
		smallint param_count(0);
		p = protocol_read< BINARY_DATA_FORMAT >(p, e, param_count);
		if (param_count != expected_params) {
			local_log(logger::ERROR) << "Param count is invalid. "
					<< "Expected " << expected_params
					<< ", actual " << param_count;
		}
		for (smallint pno = 0; pno < param_count && p != e; ++pno) {
			integer param_size(0);
			p = protocol_read< BINARY_DATA_FORMAT >(p, e, param_size);
			if (param_size == 0)
				local_log(logger::WARNING) << "Param value size is 0";
			integer value(0);
			p = protocol_read< BINARY_DATA_FORMAT >(p, e, value);
			if (value != 42)
				local_log(logger::WARNING) << "Invalid value in buffer";
			//p += param_size;
		}

		if (p != e) {
			local_log(logger::ERROR) << "Buffer is not exhausted";
		} else {
			local_log() << "Buffer test is OK";
		}
	}
	{
		buffer.clear();
		tip::db::pg::detail::write_params(buffer, (smallint)2);
		print_buf(buffer);
		if (buffer.empty()) {
			local_log(logger::ERROR) << "Buffer is empty\n";
			return 1;
		}
		size_t expected = sizeof(smallint) * 3 + sizeof(integer) + sizeof(smallint);
		if (buffer.size() != expected) {
			std::cerr << "Test 1: Invalid buffer size. Expected: "
					<< expected
					<< " actual " << buffer.size();
			return 1;
		}

		check_params_buffer(buffer, 1);
	}
	{
		buffer.clear();
		tip::db::pg::detail::write_params(buffer, (bigint)42);
		print_buf(buffer);
		if (buffer.empty()) {
			std::cerr << "Buffer is empty\n";
			return 1;
		}
		size_t expected = sizeof(smallint) * 3 + sizeof(integer) + sizeof(bigint);
		if (buffer.size() != expected) {
			std::cerr << "Test 3: Invalid buffer size. Expected: "
					<< expected
					<< " actual " << buffer.size();
			return 1;
		}
		check_params_buffer(buffer, 1);
	}

	{
		buffer.clear();
		tip::db::pg::detail::write_params(buffer, (integer)42, (smallint)324);
		if (buffer.empty()) {
			std::cerr << "Buffer is empty\n";
			return 1;
		}
		size_t expected = sizeof(smallint) * 4 + sizeof(integer) + sizeof(integer) +
				sizeof(integer) + sizeof(smallint);
		if (buffer.size() != expected) {
			std::cerr << "Test 4: Invalid buffer size. Expected: "
					<< expected
					<< " actual " << buffer.size();
			return 1;
		}
		check_params_buffer(buffer, 2);
	}

	{
		buffer.clear();
		tip::db::pg::detail::write_params(buffer, (integer)42, (integer)324);
		print_buf(buffer);
		if (buffer.empty()) {
			std::cerr << "Buffer is empty\n";
			return 1;
		}
		size_t expected = sizeof(smallint) * 4 + sizeof(integer) + sizeof(integer) +
				sizeof(integer) + sizeof(integer);
		if (buffer.size() != expected) {
			std::cerr << "Test 5: Invalid buffer size. Expected: "
					<< expected
					<< " actual " << buffer.size();
			return 1;
		}
		check_params_buffer(buffer, 2);
	}

	{
		buffer.clear();
		tip::db::pg::detail::write_params(buffer, (integer)42, (smallint)324, 3.1415926);
		if (buffer.empty()) {
			std::cerr << "Buffer is empty\n";
			return 1;
		}
		check_params_buffer(buffer, 3);
	}
	local_log() << "test_write_params: finish";
	return 0;
}

int
test_execute_prepared()
{
	if (!test_database.empty()) {
		using namespace tip::db::pg;
		connection_options opts = connection_options::parse(test_database);
		local_log(logger::INFO) << "Extended query test";
		boost::asio::io_service io_service;
		bool transaction_error = false;
		int tran_count = 0;
		std::vector<char> params;
		tip::db::pg::detail::write_params(params, (integer)10);

		connection_ptr conn(connection::create(io_service,
		[&](connection_ptr c) {
			if (!tran_count) {
				c->begin_transaction(
				[&](connection_lock_ptr c_lock){
					tran_count++;
					(*c_lock)->execute_prepared("select * from pg_catalog.pg_type where typelem > $1",
					params,
					[&](connection_lock_ptr c, resultset r, bool complete) {
						local_log() << "Received a resultset columns: " << r.columns_size()
								<< " rows: " << r.size()
								<< " completed: " << std::boolalpha << complete;
						if (complete)
							(*c)->terminate();
					}, [](db_error const&) {}, c_lock);
				},
				[&](db_error const&){
					transaction_error = true;
				}, true);
			}
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, connection_error const& ec) {
			ec.what();
		},  opts, {
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"}
		}));
		io_service.run();
	}
	return 0;
}

int
main(int argc, char* argv[])
{
	try {
		logger::min_severity(logger::TRACE);
		logger::use_colors(true);
		test_write_params();
		if (argc > 1) {
			test_database = argv[1];
			test_execute_prepared();
		}
	} catch (std::exception const& e) {
		std::cerr << "Exception: " << e.what() << "\n";
	} catch (...) {
		std::cerr << "Unexpected exception\n";
	}
	return 0;
}
