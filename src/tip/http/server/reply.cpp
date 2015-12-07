/**
 * request_reply.cpp
 *
 *  Created on: 28 авг. 2015 г.
 *      Author: zmij
 */

#include <tip/http/server/reply.hpp>
#include <tip/http/common/request.hpp>
#include <tip/http/common/response.hpp>

#include <tip/http/server/detail/context_registry.hpp>
#include <tip/http/server/locale_manager.hpp>

#include <tip/log.hpp>

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/array.hpp>
#include <set>

namespace tip {
namespace http {
namespace server {

LOCAL_LOGGING_FACILITY(HTTPREPLY, TRACE);

struct reply::impl {
	typedef std::set< cookie, cookie_name_cmp > cookies_type;
	typedef boost::iostreams::stream_buffer<
			boost::iostreams::back_insert_device< body_type >> vector_buff_type;

	io_service_ptr		io_service_;

	request_const_ptr	req_;
	response_ptr		resp_;
	send_response_func 	send_response_;
	send_error_func		send_error_;

	bool				response_sent_;

	vector_buff_type	output_buffer_;
	std::ostream		output_stream_;
	cookies_type		cookies_;
	detail::context_registry registry_;

	impl(io_service_ptr io_service, request_const_ptr req,
			send_response_func sr, send_error_func se) :
		io_service_(io_service), req_(req), resp_(new response{ {1,1} }),
		send_response_(sr), send_error_(se), response_sent_(false),
		output_buffer_(resp_->body_), output_stream_(&output_buffer_)
	{
	}
	~impl()
	{
		if (!response_sent_ && send_error_) {
			send_error_(response_status::internal_server_error);
		}
	}

	void
	add_cookie(cookie&& c)
	{
		if (!response_sent_) {
			cookies_.insert(std::move(c));
		}
	}

	void
	add_cookie(cookie const& c)
	{
		add_cookie(std::move(cookie(c)));
	}

	void
	remove_cookie(std::string const& name)
	{
		using namespace boost::posix_time;
		using namespace boost::date_time;
		using boost::gregorian::date;
		cookie c{
			name, "~deleted~",
			ptime( date(1970, Jan, 1) )
		};
		add_cookie(std::move(c));
	}

	void
	done(response_status::status_type status)
	{
		registry_.clear();
		output_stream_.flush();
		resp_->set_status(status);
		for (auto c: cookies_) {
			resp_->add_cookie(c);
		}
		if (send_response_) {
			send_response_(resp_);
			response_sent_ = true;
		}
	}
	void
	send_error(response_status::status_type status)
	{
		if (send_error_) {
			send_error_(status);
			response_sent_ = true;
		}
	}
};

reply::reply( io_service_ptr io_service, request_const_ptr req,
		send_response_func sr, send_error_func se) :
	pimpl_(new impl{ io_service, req, sr, se })
{
	pimpl_->registry_.set_reply_impl(pimpl_);

	locale_manager& lmgr =
			boost::asio::use_service< locale_manager >(*io_service);
	lmgr.deduce_locale(*this);
}

reply::reply(pimpl pi) : pimpl_(pi)
{
}

reply::io_service_ptr
reply::io_service() const
{
	return pimpl_->io_service_;
}

request_const_ptr
reply::request() const
{
	return pimpl_->req_;
}

request_method
reply::method() const
{
	return pimpl_->req_->method;
}

tip::iri::path const&
reply::path() const
{
	return pimpl_->req_->path;
}

headers const&
reply::request_headers() const
{
	return pimpl_->req_->headers_;
}

reply::body_type const&
reply::request_body() const
{
	return pimpl_->req_->body_;
}

void
reply::add_header(header const& h)
{
	pimpl_->resp_->add_header(h);
}

void
reply::add_header(header && h)
{
	pimpl_->resp_->add_header(std::move(h));
}

void
reply::add_cookie(std::string const& name, std::string const& value)
{
	cookie c { name, value };
	pimpl_->add_cookie(std::move(c));
}

void
reply::add_cookie(cookie const& c)
{
	pimpl_->add_cookie(c);
}

void
reply::add_cookie(cookie && c)
{
	pimpl_->add_cookie(std::move(c));
}

void
reply::remove_cookie(std::string const& name)
{
	pimpl_->remove_cookie(name);
}

reply::body_type&
reply::response_body()
{
	return pimpl_->resp_->body_;
}

reply::body_type const&
reply::response_body() const
{
	return pimpl_->resp_->body_;
}

std::ostream&
reply::response_stream()
{
	return pimpl_->output_stream_;
}

std::locale
reply::get_locale() const
{
	return pimpl_->output_stream_.getloc();
}

std::locale
reply::set_locale(std::locale const& loc)
{
	return pimpl_->output_stream_.imbue(loc);
}

void
reply::done(response_status::status_type status)
{
	pimpl_->done(status);
}

void
reply::client_error(response_status::status_type status)
{
	pimpl_->send_error(status);
}

void
reply::server_error(response_status::status_type status)
{
	pimpl_->send_error(status);
}

detail::context_registry&
reply::context_registry()
{
	return pimpl_->registry_;
}

reply::context::context( reply const& r ) :
		next_(nullptr), wpimpl_(r.pimpl_)
{
}

reply::context::~context()
{
}

request_const_ptr
reply::context::get_request() const
{
	reply::pimpl pi = wpimpl_.lock();
	if (!pi) {
		throw std::runtime_error("Reply already destroyed");
	}
	return pi->req_;
}

reply
reply::context::get_reply() const
{
	reply::pimpl pi = wpimpl_.lock();
	if (!pi) {
		throw std::runtime_error("Reply already destroyed");
	}
	return reply(pi);
}

} /* namespace server */
} /* namespace http */
} /* namespace tip */
