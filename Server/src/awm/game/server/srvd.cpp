/**
 * server-main.cpp
 *
 *  Created on: 30 авг. 2015 г.
 *      Author: zmij
 */

#include <tip/log.hpp>
#include <tip/db/pg.hpp>

#include <tip/http/server/server.hpp>
#include <tip/http/server/request_dispatcher.hpp>
#include <tip/http/server/locale_manager.hpp>

#include <tip/version.hpp>

#include <awm/game/server/config.hpp>
#include <awm/game/world/world.hpp>

#include <awm/game/auth/configuration.hpp>
#include <awm/game/auth/session.hpp>
#include <awm/game/auth/user.hpp>
#include <awm/game/auth/password_authn_handler.hpp>
#include <awm/game/auth/vendor_user_id.hpp>
#include <awm/game/auth/token.hpp>
#include <awm/game/auth/user_language.hpp>
#include <awm/game/auth/stats.hpp>

#include <awm/game/lobby/tank.hpp>

#include <string>
#include <iostream>
#include <fstream>

LOCAL_LOGGING_FACILITY(TIPSRV, TRACE);

bool
check_options()
{
	namespace pg = tip::db::pg;
	using tip::http::server::locale_manager;
	using awm::game::authn::configuration;

	configuration& cfg = configuration::instance();

	pg::db_service::initialize(cfg.db_connection_pool,
		{
			{"application_name", "TIP Game Server"},
			{"client_encoding", "UTF8"}
		});
	pg::connection_options main =
			pg::connection_options::parse(cfg.main_database);
	main.alias = pg::dbalias{"main"};
	if (main.uri.empty()) {
		std::cerr << "Main database URI is empty\n";
		return false;
	}
	if (main.database.empty()) {
		std::cerr << "Main database dbname is empty\n";
		return false;
	}
	if (main.user.empty()) {
		std::cerr << "Main database username is empty\n";
		return false;
	}
	pg::db_service::add_connection(main);

	locale_manager& loc_mgr = boost::asio::use_service< locale_manager >( *pg::db_service::io_service() );
	if (cfg.l10n_root.empty()) {
		std::cerr << "Localization messages root is empty\n";
		return false;
	}
	loc_mgr.add_messages_path(cfg.l10n_root);
	if (cfg.languages.empty()) {
		std::cerr << "Supported languages list is empty\n";
		return false;
	}
	loc_mgr.add_languages(cfg.languages);

	for (auto&& domain : awm::game::server::L10N_DOMAINS) {
		loc_mgr.add_messages_domain(domain);
	}
	awm::game::authn::user_language user_lang;
	loc_mgr.set_reply_callback( user_lang );
	if (!cfg.default_language.empty()) {
		loc_mgr.set_default_language(cfg.default_language);
	}

	awm::game::authn::set_authn_response_lobby_uri( cfg.external_uri );

	std::locale::global(loc_mgr.default_locale());

	return true;
}

void
configure_request_dispatcher(tip::http::server::request_dispatcher_ptr disp)
{
	namespace http = tip::http;
	// Authn
	disp->add_handler< awm::game::authn::password_authn_handler >
		(http::POST, "/api/login/password");
	disp->add_handler< awm::game::authn::token_authn_handler >
		(http::POST, "/api/login/token");

	disp->add_handler< awm::game::authn::ios::vendor_uid_reg_handler >
		(http::POST, "/api/register/apple-vendor");

	// Stats
	disp->add_handler< awm::game::authn::current_online >
		(http::GET, "/api/stats/online");
}

class stream_redirect {
public:
	stream_redirect(std::ostream& stream,
			std::string const& file_name,
			std::ios::open_mode open_mode = std::ofstream::out | std::ofstream::app) :
		stream_(stream), old_(stream.rdbuf())
	{
		file_.open(file_name.c_str(), std::ofstream::out | std::ofstream::app);
		if (!file_.good()) {
			std::ostringstream msg;
			msg << "Failed to open file " << file_name
					<< ": " << strerror(errno) << "\n";
			throw std::runtime_error(msg.str());
		}
		stream_.rdbuf(file_.rdbuf());
	}
	~stream_redirect()
	{
		stream_.rdbuf(old_);
	}
private:
	std::ostream& stream_;
	std::streambuf* old_;
	std::ofstream file_;
};

namespace {

std::shared_ptr< stream_redirect > redirect_cerr;

}  // namespace

int
main(int argc, char* argv[])
{
	namespace pg = tip::db::pg;
	using tip::http::server::request_dispatcher;
	using tip::http::server::server;
	using awm::game::authn::configuration;
	using awm::game::world::world;
	using request_dispatcher_ptr = tip::http::server::request_dispatcher_ptr;

	try {
		logger::set_proc_name(argv[0]);
		boost::program_options::variables_map vm;
		configuration& cfg = configuration::instance();
		if (cfg.parse_options(argc, argv)) {
			logger::use_colors(cfg.log_use_colors);
			logger::min_severity(cfg.log_level);
			logger::set_stream(std::cerr);

			if (!cfg.log_target.empty()) {
				try {
					redirect_cerr = std::make_shared< stream_redirect >( std::cerr, cfg.log_target );
				} catch (std::runtime_error const& e) {
					std::cerr << e.what();
				}
			}

			if (!cfg.pid_file.empty()) {
				std::ofstream pid_file(cfg.pid_file);
				pid_file << getpid() << "\n";
			}

			local_log(logger::INFO) << argv[0] << " server version "
					<< tip::VERSION << " branch " << tip::BRANCH;
			if (!check_options()) {
				cfg.usage(std::cerr);
				return 1;
			}

			// Reading world prototypes
			local_log(logger::INFO) << "Reading world prototypes";
			world& wrld = world::instance();
			wrld.import(cfg.game_data_root);

			request_dispatcher_ptr dispatcher(std::make_shared<request_dispatcher>());
			// Configure dispatcher
			configure_request_dispatcher(dispatcher);

			local_log(logger::INFO) << "Bind to address " << cfg.bind_address
					<< ":" << cfg.bind_port;
			local_log(logger::INFO) << "Threads " << cfg.threads
					<< " connection pool size " << cfg.db_connection_pool;
			// Initialize session LRU cache
			awm::game::authn::session::register_lru(
					cfg.session_cleanup_interval,
					cfg.session_timeout);
			// Initialize user LRU cache
			awm::game::authn::user::register_lru(
					cfg.user_cleanup_interval,
					cfg.user_timeout);

			// Create server instance
			server s(pg::db_service::io_service(),
					cfg.bind_address, cfg.bind_port, cfg.threads,
					dispatcher,
					[](){
						// FIXME clean up caches before calling the db_service stop
						awm::game::authn::session::clear_lru();
						awm::game::authn::user::clear_lru();
						pg::db_service::stop();
					});
			// Run server
			s.run();
		}
	} catch (std::exception const& e) {
		local_log(logger::ERROR) << "Uncaught exception: " << e.what();
		return 1;
	} catch (...) {
		local_log(logger::ERROR) << "Uncaught unexpected exception";
		return 1;
	}
	return 0;
}
