/*
 * session.cpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#include <tip/http/client/session.hpp>

#include <tip/http/common/request.hpp>
#include <tip/http/common/response.hpp>

#include <tip/ssl_context_service.hpp>

#include <tip/util/misc_algorithm.hpp>
#include <tip/log.hpp>

#include <boost/asio.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>

namespace tip {
namespace http {
namespace client {

LOCAL_LOGGING_FACILITY(HTTPSSN, OFF);

struct tcp_transport {
	typedef boost::asio::io_service io_service;
	typedef boost::asio::ip::tcp tcp;
	typedef boost::system::error_code error_code;
	typedef std::function<void(error_code const&)> connect_callback;
	typedef tcp::socket socket_type;

	tcp::resolver resolver_;
	socket_type socket_;

	tcp_transport(io_service& io_service, request::iri_type const& iri, connect_callback cb) :
			resolver_(io_service), socket_(io_service)
	{
		std::string host = static_cast<std::string const&>(iri.authority.host);
		std::string svc = iri.authority.port.empty() ?
				static_cast<std::string const&>(iri.scheme) :
				static_cast<std::string const&>(iri.authority.port);
		tcp::resolver::query qry(host, svc);
		resolver_.async_resolve(qry, std::bind(
			&tcp_transport::handle_resolve, this,
				std::placeholders::_1, std::placeholders::_2, cb
		));
	}

	void
	handle_resolve(error_code const& ec,
			tcp::resolver::iterator endpoint_iterator,
			connect_callback cb)
	{
		if (!ec) {
			boost::asio::async_connect(socket_, endpoint_iterator, std::bind(
				&tcp_transport::handle_connect, this,
					std::placeholders::_1, cb
			));
		} else if (cb) {
			cb(ec);
		}
	}

	void
	handle_connect(error_code const& ec, connect_callback cb)
	{
		if (cb) {
			cb(ec);
		}
	}
	void
	disconnect()
	{
		if (socket_.is_open()) {
			socket_.close();
		}
	}
};

struct ssl_transport {
	typedef boost::asio::io_service io_service;
	typedef boost::asio::ip::tcp tcp;
	typedef boost::system::error_code error_code;
	typedef std::function<void(error_code const&)> connect_callback;
	typedef boost::asio::ssl::stream< tcp::socket > socket_type;

	tcp::resolver resolver_;
	socket_type socket_;

	ssl_transport(io_service& io_service, request::iri_type const& iri, connect_callback cb) :
		resolver_(io_service),
		socket_( io_service,
				boost::asio::use_service<tip::ssl::ssl_context_service>(io_service).context() )
	{
		std::string host = static_cast<std::string const&>(iri.authority.host);
		std::string svc = iri.authority.port.empty() ?
				static_cast<std::string const&>(iri.scheme) :
				static_cast<std::string const&>(iri.authority.port);
		tcp::resolver::query qry(host, svc);
		resolver_.async_resolve(qry, std::bind(
			&ssl_transport::handle_resolve, this,
				std::placeholders::_1, std::placeholders::_2, cb
		));
	}

	void
	handle_resolve(error_code const& ec,
			tcp::resolver::iterator endpoint_iterator,
			connect_callback cb)
	{
		if (!ec) {
			socket_.set_verify_mode(boost::asio::ssl::verify_peer);
			socket_.set_verify_callback(std::bind(
				&ssl_transport::verify_certificate,
				this, std::placeholders::_1, std::placeholders::_2
			));
			boost::asio::async_connect(socket_.lowest_layer(),
				endpoint_iterator, std::bind(
				&ssl_transport::handle_connect, this,
					std::placeholders::_1, cb
			));
		} else if (cb) {
			cb(ec);
		}
	}

	bool
	verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
	{
	    // The verify callback can be used to check whether the certificate that is
	    // being presented is valid for the peer. For example, RFC 2818 describes
	    // the steps involved in doing this for HTTPS. Consult the OpenSSL
	    // documentation for more details. Note that the callback is called once
	    // for each certificate in the certificate chain, starting from the root
	    // certificate authority.

	    // In this example we will simply print the certificate's subject name.
	    char subject_name[256];
	    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	    local_log() << "Verifying " << subject_name;
	    return preverified;
	}
	void
	handle_connect(error_code const& ec, connect_callback cb)
	{
		if (cb) {
			cb(ec);
		}
	}
	void
	disconnect()
	{
		socket_.shutdown();
	}
};

//-----------------------------------------------------------------------------
namespace events {
struct connected {};
struct disconnect {};
struct transport_error {};
struct request {
	http::request_ptr request_;
	session::response_callback callback_;
};
struct response {
	http::response_ptr response_;
};
}  // namespace events

template < typename TransportType, typename SharedType >
struct session_fsm_ :
		public boost::msm::front::state_machine_def< session_fsm_<TransportType, SharedType >>,
		public std::enable_shared_from_this< SharedType > {
	typedef TransportType transport_type;
	typedef SharedType shared_type;
	typedef std::enable_shared_from_this< shared_type > shared_base;
	typedef boost::msm::back::state_machine< session_fsm_< transport_type, shared_type > > session_fsm;
	typedef boost::asio::io_service io_service;
	typedef boost::asio::streambuf buffer_type;
	typedef std::vector< boost::asio::const_buffer > output_buffers_type;
	typedef boost::system::error_code error_code;
	//@{
	/** @name Typedefs for MSM types */
	template < typename ... T >
	using Row = boost::msm::front::Row< T ... >;
	template < typename ... T >
	using Internal = boost::msm::front::Internal< T ... >;
	typedef boost::msm::front::none none;
	template < typename T >
	using Not = boost::msm::front::euml::Not_< T >;
	//@}

	//@{
	/** @name States */
	struct unplugged : public boost::msm::front::state<> {
		typedef boost::mpl::vector<
			events::request
		> deferred_events;
	};
	struct online_ : public boost::msm::front::state_machine_def<online_> {
		typedef boost::msm::back::state_machine< online_ > online;
		template < typename Event >
		void
		on_entry(Event const&, session_fsm& fsm)
		{
			local_log() << "entering idle";
			session_ = &fsm;
		}
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{
			local_log() << "exiting idle";
		}
		//@{
		/** @name States */
		struct wait_request : public boost::msm::front::state<> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{
				local_log() << "entering wait_request";
			}
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{
				local_log() << "exiting wait_request";
			}
		};
		struct wait_response : public boost::msm::front::state<> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ local_log() << "entering wait_response"; }
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{ local_log() << "exiting wait_response"; }
			typedef boost::mpl::vector<
				events::request
			> deferred_events;

			events::request req_;
		};
		typedef wait_request initial_state;
		//@}
		//@{
		/** @name Actions */
		struct send_request {
			template < typename SourceState >
			void
			operator()(events::request const& req, online& fsm, SourceState&,
					wait_response& resp_state)
			{
				fsm.session_->send_request(*req.request_.get());
				resp_state.req_ = req;
			}
		};
		struct process_reply {
			template < typename FSM, typename TargetState >
			void
			operator()(events::response const& resp, FSM&, wait_response& resp_state,
					TargetState&)
			{
				if (resp_state.req_.callback_) {
					resp_state.req_.callback_(resp_state.req_.request_, resp.response_);
				}
				resp_state.req_ = events::request{};
			}
		};
		//@}
		struct transition_table : boost::mpl::vector <
			/*		Start			Event				Next			Action			Guard	      */
			/*  +-----------------+-------------------+---------------+---------------+-------------+ */
			Row <	wait_request,	events::request,	wait_response,	send_request,	none		>,
			Row <	wait_response,	events::response,	wait_request,	process_reply,	none		>
		> {};

		online_() : session_(nullptr) {}
		session_fsm* session_;
	};
	typedef boost::msm::back::state_machine< online_ > online;
	struct terminated : boost::msm::front::terminate_state<> {
		template < typename Event >
		void
		on_entry(Event const&, session_fsm& fsm)
		{
			local_log() << "entering terminated";
			fsm.notify_closed();
		}
	};
	typedef unplugged initial_state;
	//@}
	//@{
	/** @name Actions */
	struct disconnect_transport {
		template < typename Event, typename SourceState, typename TargetState >
		void
		operator()(Event const&, session_fsm& fsm, SourceState&, TargetState&)
		{
			fsm.disconnect();
		}
	};
	//@}
	//@{
	struct transition_table : boost::mpl::vector<
		/*		Start			Event					Next			Action						Guard				  */
		/*  +-----------------+-----------------------+---------------+---------------------------+---------------------+ */
		Row <	unplugged,		events::connected,		online,			none,						none				>,
		Row <	unplugged,		events::
									transport_error,	terminated,		none,						none				>,
		Row <	unplugged,		events::disconnect,		terminated,		none,						none				>,
		Row <	online,			events::disconnect,		terminated,		disconnect_transport,		none				>,
		Row <	online,			events::
									transport_error,	terminated,		none,						none				>
	>{};

	template < typename Event, typename FSM >
	void
	no_transition(Event const& e, FSM&, int state)
	{
		local_log(logger::DEBUG) << "No transition from state " << state
				<< " on event " << typeid(e).name() << " (in transaction)";
	}
	//@}
	session_fsm_(io_service& io_service, request::iri_type const& iri,
			session::session_callback on_close, headers const& default_headers) :
		strand_(io_service),
		transport_(io_service, iri,
			strand_.wrap(
				std::bind( &session_fsm_::handle_connect,
						this, std::placeholders::_1 ))),
		host_(iri.authority.host), scheme_(iri.scheme),
		on_close_(on_close), default_headers_(default_headers)
	{
	}

	virtual ~session_fsm_() {}

	void
	handle_connect(boost::system::error_code const& ec)
	{
		if (!ec) {
			local_log(logger::DEBUG) << "Connected to "
					<< scheme_ << "://" << host_;
			fsm().process_event(events::connected());
		} else {
			local_log(logger::ERROR) << "Error connecting to "
					<< scheme_ << "://" << host_;
			fsm().process_event(events::transport_error());
		}
	}

	void
	send_request(request const& req)
	{
		using std::placeholders::_1;
		using std::placeholders::_2;
		std::ostream os(&outgoing_);
		os << req;
		if (!default_headers_.empty()) {
			os << default_headers_;
		}
		os << "\r\n";
		output_buffers_type buffers;
		buffers.push_back(boost::asio::buffer(outgoing_.data()));
		buffers.push_back(boost::asio::buffer(req.body_));
		boost::asio::async_write(transport_.socket_, buffers,
			strand_.wrap(std::bind(&shared_type::handle_write,
						shared_base::shared_from_this(), _1, _2)));
	}

	void
	handle_write(error_code const& ec, size_t bytes_transferred)
	{
		using std::placeholders::_1;
		using std::placeholders::_2;
		if (!ec) {
			// Start read headers
			boost::asio::async_read_until(transport_.socket_, incoming_, "\r\n\r\n",
				strand_.wrap(std::bind(&shared_type::handle_read_headers,
						shared_base::shared_from_this(), _1, _2)));
		} else {
			fsm().process_event(events::transport_error());
		}
	}
	void
	handle_read_headers(error_code const& ec, size_t bytes_transferred)
	{
		if (!ec) {
			// Parse response head
			std::istream is(&incoming_);
			response_ptr resp(std::make_shared< response >());
			if (resp->read_headers(is)) {
				read_body(resp, resp->read_body(is));
			} else {
				local_log(logger::ERROR) << "Failed to parse response headers";
			}
		} else {
			fsm().process_event(events::transport_error());
		}
	}

	void
	read_body(response_ptr resp, response::read_result_type res)
	{
		using std::placeholders::_1;
		using std::placeholders::_2;
		if (res.result) {
			// success read
			fsm().process_event( events::response{ resp } );
		} else if (!res.result) {
			// failed to read body
			fsm().process_event( events::response{ resp } );
		} else if (res.callback) {
				// need more data
				boost::asio::async_read(transport_.socket_,
						incoming_,
						boost::asio::transfer_at_least(1),
						strand_.wrap(std::bind(&shared_type::handle_read_body,
							shared_base::shared_from_this(), _1, _2,
							resp, res.callback)));
		} else {
			// need more data but no callback
			local_log(logger::WARNING) << "Response read body returned "
					"indeterminate, but provided no callback";
			fsm().process_event( events::response{ resp } );
		}
	}

	void
	handle_read_body(error_code const& ec, size_t bytes_transferred,
			response_ptr resp, response::read_callback cb)
	{
		if (!ec) {
			std::istream is(&incoming_);
			read_body(resp, cb(is));
		} else {
			fsm().process_event(events::transport_error());
		}
	}

	void
	disconnect()
	{
		transport_.disconnect();
	}

	void
	notify_closed()
	{
		if(on_close_) {
			on_close_(shared_base::shared_from_this());
		}
	}
private:
	session_fsm&
	fsm()
	{
		return static_cast< session_fsm& >(*this);
	}
	session_fsm const&
	fsm() const
	{
		return static_cast< session_fsm const& >(*this);
	}
private:
	boost::asio::io_service::strand	strand_;
	transport_type					transport_;
	iri::host						host_;
	iri::scheme						scheme_;
	buffer_type						incoming_;
	buffer_type						outgoing_;
	headers							default_headers_;
	session::session_callback		on_close_;
};
//-----------------------------------------------------------------------------
template < typename TransportType >
class session_impl : public session,
		public boost::msm::back::state_machine< session_fsm_< TransportType,
			session_impl< TransportType > > > {
public:
	typedef boost::msm::back::state_machine< session_fsm_< TransportType,
			session_impl< TransportType > > > base_type;
	typedef session_impl< TransportType > this_type;

	session_impl(io_service& io_service, request::iri_type const& iri,
			session_callback on_close, headers const& default_headers) :
		base_type(std::ref(io_service), iri, on_close, default_headers)
	{
	}

	virtual ~session_impl()
	{
		local_log() << "session_impl::~session_impl";
	}

	virtual void
	do_send_request(request_method method, request::iri_type const& iri,
			body_type const& body, response_callback cb)
	{
		do_send_request( request::create(method, iri, body), cb );
	}

	virtual void
	do_send_request(request_method method, request::iri_type const& iri,
			body_type&& body, response_callback cb)
	{
		do_send_request( request::create(method, iri, std::move(body)), cb );
	}
	virtual void
	do_send_request(request_ptr req, response_callback cb)
	{
		base_type::process_event( events::request{ req, cb });
	}
	virtual void
	do_close()
	{
		base_type::process_event( events::disconnect() );
	}
};


session::session()
{
}

session::~session()
{
}

void
session::send_request(request_method method, request::iri_type const& iri,
		body_type const& body, response_callback cb)
{
	do_send_request(method, iri, body, cb);
}

void
session::send_request(request_method method, request::iri_type const& iri,
		body_type&& body, response_callback cb)
{
	do_send_request(method, iri, std::move(body), cb);
}

void
session::send_request(request_ptr req, response_callback cb)
{
	do_send_request(req, cb);
}

void
session::close()
{
	do_close();
}

session_ptr
session::create(io_service& svc, request::iri_type const& iri,
		session_callback on_close, headers const& default_headers)
{
	typedef session_impl< tcp_transport > http_session;
	typedef session_impl< ssl_transport > https_session;

	if (iri.scheme == tip::iri::scheme{ "http" }) {
		return std::make_shared< http_session >( svc, iri, on_close, default_headers );
	} else if (iri.scheme == tip::iri::scheme{ "https" }) {
		return std::make_shared< https_session >( svc, iri, on_close, default_headers );
	}
	// TODO Throw an exception
	return session_ptr();
}


} /* namespace client */
} /* namespace http */
} /* namespace tip */
