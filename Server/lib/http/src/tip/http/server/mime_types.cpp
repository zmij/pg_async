/**
 * @file /tip-server/src/tip/http/server/mime_types.cpp
 * @brief
 * @date Jul 8, 2015
 * @author: zmij
 */

#include "tip/http/server/mime_types.hpp"

namespace tip {
namespace http {
namespace mime_types {

struct mapping
{
  const char* extension;
  const char* mime_type;
} mappings[] =
{
  { "css",	"text/css" },
  { "gif",	"image/gif" },
  { "htm",	"text/html" },
  { "html",	"text/html" },
  { "jpg",	"image/jpeg" },
  { "jpeg",	"image/jpeg" },
  { "png",	"image/png" },
  { 0, 0 } // Marks end of list.
};

std::string extension_to_type(const std::string& extension)
{
  for (mapping* m = mappings; m->extension; ++m)
  {
    if (m->extension == extension)
    {
      return m->mime_type;
    }
  }

  return "text/plain";
}

}  // namespace mime_types
}  // namespace http
}  // namespace tip
