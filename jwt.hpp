#ifndef __MS_JWT_HPP
#define __MS_JWT_HPP

#include <string>

namespace jwt
{
    std::string encode(const std::string& alg, const std::string& type, const std::string& payload, const uint8_t* key, size_t key_size);
}

#endif
