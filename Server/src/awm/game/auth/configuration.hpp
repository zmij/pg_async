/*
 * configuration.hpp
 *
 */

#ifndef TIP_GAME_AUTH_CONFIGURATION_HPP_
#define TIP_GAME_AUTH_CONFIGURATION_HPP_

#include <string>
#include <boost/program_options.hpp>
#include <tip/log.hpp>

namespace awm {
namespace game {
namespace authn {

class configuration {
public:
	typedef boost::program_options::options_description options_description;
	typedef boost::program_options::variables_map variables_map;
public:
	static configuration&
	instance();

	void
	add_command_line_options(options_description const&);
	void
	add_config_file_options(options_description const&);

	void
	parse_options(int argc, char* argv[], variables_map& vm);

	void
	usage(std::ostream& os);
public:
	//@{
	/** @name Database connection options */
	std::string						main_database;
	std::string						log_database;
	size_t							db_connection_pool;
	//@}
	//@{
	/** @name Server options */
	std::string						bind_address;
	std::string						bind_port;
	std::string                     external_uri;
	size_t							threads;
	std::string                     pid_file;
	//@}
	//@{
	/** @name Log options */
	std::string						log_target;
	std::string						log_date_format;
	tip::log::logger::event_severity
									log_level;
	bool							log_use_colors;
	//@}
	//@{
	/** @name Cache options */
	size_t							session_timeout;
	size_t							session_cleanup_interval;
	size_t							user_timeout;
	size_t							user_cleanup_interval;
	//@}
	std::string						game_data_root;
	//@{
	/** @name L10N options */
	std::string						l10n_root;
	std::string						languages;
	std::string						default_language;
	//@}
private:
	configuration();
private:
	options_description command_line_;
	options_description config_file_options_;

	std::string			config_file_name_;
};

} /* namespace authn */
} /* namespace game */
} /* namespace awm */

#endif /* TIP_GAME_AUTH_CONFIGURATION_HPP_ */
