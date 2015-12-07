/**
 * @file /tip-server/include/tip/http/server/header.hpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#ifndef HTTP_SERVER3_HEADER_HPP
#define HTTP_SERVER3_HEADER_HPP

#include <string>

namespace tip {
namespace http {
namespace server {

struct header
{
  std::string name;
  std::string value;
};

}  // namespace server
}  // namespace http
}  // namespace tip

#endif /* TIP_HTTP_SERVER_HEADER_HPP */
