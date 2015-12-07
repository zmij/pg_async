/**
 * @file /tip-server/include/tip/http/server/mime_types.hpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#ifndef TIP_HTTP_SERVER_MIME_TYPES_HPP
#define TIP_HTTP_SERVER_MIME_TYPES_HPP

#include <string>

namespace tip {
namespace http {
namespace mime_types {

/** Convert a file extension into a MIME type. */
std::string extension_to_type(const std::string& extension);

} // namespace mime_types
} // namespace http
}  // namespace tip

#endif // HTTP_SERVER3_MIME_TYPES_HPP
