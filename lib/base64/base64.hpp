#ifndef __MS_BASE64_HPP
#define __MS_BASE64_HPP

#include <string>

namespace base64
{
    std::string encode_urlsafe(const std::string& data);
    std::string encode_urlsafe(const uint8_t* data, size_t len);
}

#endif
