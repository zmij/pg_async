/*
 * configuration.cpp
 *
 */

#include <awm/game/auth/configuration.hpp>
#include <awm/game/server/config.hpp>
#include <iostream>
#include <fstream>
#include <tip/version.hpp>

namespace awm {
namespace game {
namespace authn {

LOCAL_LOGGING_FACILITY(CONFIG, OFF);

namespace po = boost::program_options;

configuration::configuration() :
		db_connection_pool(8),
		threads(4),
		session_timeout(15),
		session_cleanup_interval(60)
{
	options_description game("Game data options");
	game.add_options()
		("game-data,w", po::value<std::string>(&game_data_root),
				"Game data root directory")
	;
	options_description server("Server options");
	server.add_options()
		("listen-address,a",
			po::value<std::string>(&bind_address)->default_value("0.0.0.0"),
			"Listen to address")
		("listen-port,p",
			po::value<std::string>(&bind_port)->default_value("65432"),
			"Listen to port")
		("external-uri",
			po::value<std::string>(&external_uri)->default_value("https://localhost/server"),
			"External URI for client")
		("threads,t", po::value<size_t>(&threads)->default_value(4),
			"Server worker thread count")
		("pid-file", po::value<std::string>(&pid_file),
			"Write server pid to file")
	;
	options_description database("Database options");
	database.add_options()
		("main-database,d",
			po::value<std::string>(&main_database)->required(),
			"Main database connection string [mandatory]")
		("log-database,l", po::value<std::string>(&log_database),
			"Log database connection string [mandatory]")
		("connection-pool,n", po::value<size_t>(&db_connection_pool)->default_value(8),
			"Database connection pool size")
	;
	options_description cache("Cache options");
	cache.add_options()
		("session-timeout", po::value<size_t>(&session_timeout)->default_value(15),
			"User session timeout, in minutes")
		("session-cleanup-interval", po::value<size_t>(&session_cleanup_interval)->default_value(60),
			"Session cleanup interval, in seconds")
		("user-timeout", po::value<size_t>(&user_timeout)->default_value(60),
			"User session timeout, in minutes")
		("user-cleanup-interval", po::value<size_t>(&user_cleanup_interval)->default_value(60),
			"Session cleanup interval, in seconds")
	;
	options_description l10n_options("Localization options");
	l10n_options.add_options()
		("l10n-root",
				po::value<std::string>(&l10n_root)
					->default_value(awm::game::server::L10N_ROOT)
					->required(),
				"Root of localized messages catalog")
		("languages", po::value<std::string>(&languages)->default_value(awm::game::server::L10N_LANGUAGES),
				"Supported languages for the server instance")
		("default-language", po::value<std::string>(&default_language),
				"Default language for the server instance. "
				"Will use the first in the language list if not set")
	;

	add_options(game);
	add_options(server);
	add_options(database);
	add_options(cache);
	add_options(l10n_options);
}

configuration&
configuration::instance()
{
	static configuration config_;
	return config_;
}

} /* namespace authn */
} /* namespace game */
} /* namespace awm */
