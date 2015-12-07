/**
 * @file /tip-server/src/tip/http/server/file_request_handler.cpp
 * @brief
 * @date Jul 9, 2015
 * @author: zmij
 */

#include <tip/http/server/file_request_handler.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>

#include <tip/http/server/mime_types.hpp>
#include <tip/http/common/request.hpp>
#include <tip/http/server/language_context.hpp>
#include <tip/version.hpp>
#include <tip/log/log.hpp>

namespace tip {
namespace http {
namespace server {

LOCAL_LOGGING_FACILITY(FILES, TRACE);

file_request_handler::file_request_handler(std::string const& doc_root)
		: doc_root_(doc_root)
{
}

void
file_request_handler::do_handle_request(reply r)
{
	std::ostringstream os;
	os << r.request()->path;
	std::string request_path = os.str();
	if (request_path.empty() || request_path[0] != '/'
			|| request_path.find("..") != std::string::npos) {
		local_log(logger::WARNING) << "400|Invalid URI";
		r.client_error(response_status::bad_request);
		return;
	}
	// If path ends in slash (i.e. is a directory) then add "index.html".
	if (request_path[request_path.size() - 1] == '/') {
		request_path += "index.html";
	}

	language_context& lctx = use_context<language_context>(r);
	for (auto lng : lctx.languages()) {
		local_log() << "Accept language " << lng.first << " " << lng.second;
	}

	// Determine the file extension.
	std::size_t last_slash_pos = request_path.find_last_of("/");
	std::size_t last_dot_pos = request_path.find_last_of(".");
	std::string extension;
	if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos) {
		extension = request_path.substr(last_dot_pos + 1);
	}

	// Open the file to send back.
	std::string full_path = doc_root_ + request_path;
	std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
	if (!is) {
		local_log(logger::WARNING) << "404|Not found " << request_path;
		r.client_error(response_status::not_found);
		return;
	}
	is.unsetf(std::ios_base::skipws);

	std::streampos fsize = is.tellg();
	is.seekg(0, std::ios::end);
	fsize = is.tellg() - fsize;
	is.seekg(0, std::ios::beg);

	r.response_body().reserve(fsize);
	typedef std::istream_iterator< char > file_iterator;
	file_iterator f(is);
	file_iterator l;

	std::copy(f, l, std::back_inserter(r.response_body()));

	r.add_cookie("foo", "bar");
	r.remove_cookie(extension);
	r.add_header({ ContentType, mime_types::extension_to_type(extension) });
	r.done();
}


}  // namespace server
}  // namespace http
}  // namespace tip


