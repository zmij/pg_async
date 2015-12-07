/**
 * json_body_context.hpp
 *
 *  Created on: 01 сент. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_JSON_BODY_CONTEXT_HPP_
#define TIP_HTTP_SERVER_JSON_BODY_CONTEXT_HPP_

#include <tip/http/server/reply.hpp>
#include <tip/http/server/request_handler.hpp>
#include <tip/http/server/reply_context.hpp>
#include <tip/http/server/error.hpp>
#include <cereal/archives/json.hpp>

#include <memory>

namespace tip {
namespace http {
namespace server {

class json_body_context: public reply::context {
public:
	typedef cereal::JSONInputArchive input_arhive_type;
	typedef cereal::JSONOutputArchive output_archive_type;

	enum status_type {
		empty_body,
		invalid_json,
		json_ok
	};
	static reply::id id;
public:
	json_body_context( reply const& r);
	virtual ~json_body_context();

	status_type
	status() const;
	explicit operator bool()
	{ return status() == json_ok; }

	input_arhive_type&
	incoming();
	output_archive_type&
	outgoing();
private:
	struct impl;
	typedef std::unique_ptr<impl> pimpl;
	pimpl pimpl_;
};

template < typename Archive >
void
CEREAL_SAVE_FUNCTION_NAME(Archive& ar, error const& e)
{
	ar(
		::cereal::make_nvp("error", e.name()),
		::cereal::make_nvp("severity", e.severity()),
	    ::cereal::make_nvp("category", e.category()),
		::cereal::make_nvp("message", std::string(e.what()))
	);
}

class json_error : public client_error {
public:
	json_error(std::string const& w) : client_error("JSONPARSE", w) {}
	json_error(std::string const& cat, std::string const& w,
			response_status::status_type s
				= response_status::internal_server_error,
				log::logger::event_severity sv = log::logger::ERROR)
			: client_error(cat, w, s, sv) {}
	virtual ~json_error() {}

	virtual std::string const&
	name() const;
};

template < typename T, bool allow_empty_body = false >
class json_transformer {
public:
	typedef T request_type;
	typedef std::shared_ptr< request_type > pointer;

	pointer
	operator()(reply r) const
	{
		json_body_context& json = use_context< json_body_context >(r);
		if ((bool)json) {
			pointer req(std::make_shared< request_type >());
			try {
				req->serialize(json.incoming());
				return req;
			} catch (std::exception const& e) {
				throw json_error(e.what());
			} catch (...) {
				throw json_error("Invalid JSON request");
			}
		}
		switch (json.status()) {
		case json_body_context::empty_body:
			if (!allow_empty_body)
				throw json_error("Empty body (expected JSON object)");
			break;
		case json_body_context::invalid_json:
			throw json_error("Malformed JSON");
		default:
			throw json_error("Unknown JSON status");
		}
		return pointer{};
	}

	void
	error(reply r, class error const& e) const
	{
		e.log_error();
		json_body_context& json = use_context< json_body_context >(r);
		save(json.outgoing(), e);
		r.done(e.status());
	}
};

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_JSON_BODY_CONTEXT_HPP_ */
