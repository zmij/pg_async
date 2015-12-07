/*
 * config.in.hpp
 *
 *  Created on: Oct 12, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_SERVER_CONFIG_IN_HPP_
#define TIP_GAME_SERVER_CONFIG_IN_HPP_

#include <string>
#include <vector>

namespace awm {
namespace game {
namespace server {

const std::string L10N_ROOT = "@L10N_MO_DIRECTORY@";
const std::string L10N_LANGUAGES = "@L10N_LANGUAGES@";
const std::vector< std::string > L10N_DOMAINS @L10N_DOMAINS_INIT@;

}  // namespace server
}  // namespace game
}  // namespace awm

#endif /* TIP_GAME_SERVER_CONFIG_IN_HPP_ */
