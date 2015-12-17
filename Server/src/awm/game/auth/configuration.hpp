/*
 * configuration.hpp
 *
 */

#ifndef TIP_GAME_AUTH_CONFIGURATION_HPP_
#define TIP_GAME_AUTH_CONFIGURATION_HPP_

#include <awm/util/configuration.hpp>

namespace awm {
namespace game {
namespace authn {

class configuration : public awm::util::configuration {
public:
	static configuration&
	instance();
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
};

} /* namespace authn */
} /* namespace game */
} /* namespace awm */

#endif /* TIP_GAME_AUTH_CONFIGURATION_HPP_ */
