/*
 * configuration.cpp
 *
 *  Created on: Aug 30, 2015
 *      Author: zmij
 */

#include <awm/game/server/configuration.hpp>
#include <awm/game/server/config.hpp>
#include <iostream>
#include <fstream>
#include <tip/version.hpp>

namespace awm {
namespace game {
namespace server {

LOCAL_LOGGING_FACILITY(CONFIG, OFF);

namespace po = boost::program_options;

configuration::configuration() :
		db_connection_pool(8),
		threads(4),
		log_use_colors(false),
		session_timeout(15),
		session_cleanup_interval(60)
{
	options_description generic("Generic options");
	generic.add_options()
		("config-file,c",
				po::value<std::string>(&config_file_name_), "Configuration file")
		("version", "Print version and exit")
		("help", "Print help and exit")
	;
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
		("main-database,d", po::value<std::string>(&main_database),
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
	options_description log_options("Log options");
	log_options.add_options()
		("log-file", po::value<std::string>(&log_target), "Log file path")
		("log-date-format", po::value<std::string>(&log_date_format)->default_value("%d.%m", "DD.MM"),
				"Log date format. See boost::date_time IO documentation for available flags")
		("log-level,v",
			po::value<logger::event_severity>(&log_level)->default_value(logger::INFO),
			"Log level (TRACE, DEBUG, INFO, WARNING, ERROR)")
		("log-colors", "Output colored log")
	;
	options_description l10n_options("Localization options");
	l10n_options.add_options()
		("l10n-root", po::value<std::string>(&l10n_root)->default_value(L10N_ROOT),
				"Root of localized messages catalog")
		("languages", po::value<std::string>(&languages)->default_value(L10N_LANGUAGES),
				"Supported languages for the server instance")
		("default-language", po::value<std::string>(&default_language),
				"Default language for the server instance. "
				"Will use the first in the language list if not set")
	;

	command_line_.
		add(generic).
		add(game).
		add(server).
		add(database).
		add(cache).
		add(log_options).
		add(l10n_options)
	;
	config_file_options_.
		add(server).
		add(game).
		add(database).
		add(cache).
		add(log_options).
		add(l10n_options)
	;
}

void
configuration::add_command_line_options(options_description const& desc)
{
	command_line_.add(desc);
}
void
configuration::add_config_file_options(options_description const& desc)
{
	config_file_options_.add(desc);
}

void
configuration::parse_options(int argc, char* argv[], variables_map& vm)
{
	po::store(po::parse_command_line(argc, argv, command_line_), vm);
	po::notify(vm);
	if (!config_file_name_.empty()) {
		std::ifstream cfg(config_file_name_.c_str());
		if (cfg) {
			po::store(po::parse_config_file(cfg, config_file_options_), vm);
			po::notify(vm);
		}
	}
	log_use_colors = vm.count("log-colors");
}

void
configuration::usage(std::ostream& os)
{
	os << "TIP server " << tip::VERSION << " branch " << tip::BRANCH
		<< " usage:\n"
		<< command_line_ << "\n";
}

configuration&
configuration::instance()
{
	static configuration config_;
	return config_;
}

} /* namespace server */
} /* namespace game */
} /* namespace awm */
