/**
 * json_body_context.cpp
 *
 *  Created on: 01 сент. 2015 г.
 *      Author: zmij
 */

#include <tip/http/server/json_body_context.hpp>

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/array.hpp>

namespace tip {
namespace http {
namespace server {

reply::id json_body_context::id;

struct json_body_context::impl {
	typedef std::unique_ptr< cereal::JSONInputArchive > input_archive_ptr;
	typedef std::unique_ptr< output_archive_type >		output_archive_ptr;

	status_type			status_;
	input_archive_ptr	input_;
	body_type&			output_body_;
	output_archive_ptr	output_;

	impl(reply r) : status_(empty_body), output_body_(r.response_body())
	{
		if (!r.request_body().empty()) {
			try {
				typedef boost::iostreams::stream_buffer<
						boost::iostreams::array_source > array_source_type;

				body_type const& body = r.request_body();
				array_source_type buffer(body.data(), body.size());
				std::istream is(&buffer);
				input_.reset( new cereal::JSONInputArchive(is) );
				status_ = json_ok;
			} catch (...) {
				status_ = invalid_json;
			}
		}
	}

	void
	flush_out()
	{
		if (output_) {
			output_.reset();
		}
	}
};

json_body_context::json_body_context(reply const& r) :
		reply::context(r), pimpl_(new impl(r))
{
}

json_body_context::~json_body_context()
{
}

json_body_context::status_type
json_body_context::status() const
{
	return pimpl_->status_;
}

json_body_context::input_arhive_type&
json_body_context::incoming()
{
	if (pimpl_->status_ != json_ok) {
		throw json_error("Invalid JSON");
	}
	return *pimpl_->input_;
}

json_body_context::output_archive_type&
json_body_context::outgoing()
{
	if (!pimpl_->output_) {
		pimpl_->output_.reset( new output_archive_type( get_reply().response_stream() ));
	}
	return *pimpl_->output_;
}

//----------------------------------------------------------------------------
std::string const&
json_error::name() const
{
	static std::string NAME_ = "JSON error";
	return NAME_;
}

} /* namespace server */
} /* namespace http */
} /* namespace tip */
