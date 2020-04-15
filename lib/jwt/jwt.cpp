/*
    Implementation adapted from nkolban in esp32-snippets repo. The url for the file is:
    https://github.com/nkolban/esp32-snippets/blob/master/cloud/GCP/JWT/main.cpp.
*/

#include "jwt.hpp"

#include <base64.hpp>

#include <cstring>
#include <mbedtls/pk.h>
#include <mbedtls/error.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <memory>

namespace
{
    std::string mbedtls_error_to_msg(int error_code)
    {
        // 135 is the just more than the length of the longest error messsage.
        char buffer[135];
        mbedtls_strerror(error_code, buffer, sizeof(buffer));
        return std::string(buffer);
    }
}

namespace jwt
{
    // TODO: Determine if pointer arguments are good or smart pointers should be used.
    std::string encode(const std::string& alg, const std::string& type, const std::string& payload, const uint8_t* key, size_t key_size)
    {
        std::string ret;
        std::string header = "{\"alg\":\"" + alg + "\",\"typ\":\"" + type + "\"}";

        ret += base64::encode_urlsafe(header);
        ret += ".";
        ret += base64::encode_urlsafe(payload);

        mbedtls_pk_context pk_context_raw;
        std::unique_ptr<mbedtls_pk_context, decltype(&mbedtls_pk_free)> pk_context(&pk_context_raw, &mbedtls_pk_free);
        mbedtls_pk_init(&pk_context_raw);
        int rc = mbedtls_pk_parse_key(pk_context.get(), key, key_size, NULL, 0);
        if (rc != 0)
        {
            throw std::runtime_error(mbedtls_error_to_msg(rc));
        }

        uint8_t digest[32];
        rc = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                        reinterpret_cast<const unsigned char*>(ret.c_str()), ret.length(), digest);
        if (rc != 0)
        {
            throw std::runtime_error(mbedtls_error_to_msg(rc));
        }

        uint8_t buf[256];
        mbedtls_entropy_context entropy_raw;
        mbedtls_ctr_drbg_context ctr_drbg_raw;
        std::unique_ptr<mbedtls_entropy_context, decltype(&mbedtls_entropy_free)> entropy(&entropy_raw, &mbedtls_entropy_free);
        std::unique_ptr<mbedtls_ctr_drbg_context, decltype(&mbedtls_ctr_drbg_free)> ctr_drbg(&ctr_drbg_raw, &mbedtls_ctr_drbg_free);
        mbedtls_ctr_drbg_init(ctr_drbg.get());
        mbedtls_entropy_init(entropy.get());

        const char* pers = "__ms_jwt_entropy";
        mbedtls_ctr_drbg_seed(
            ctr_drbg.get(),
            mbedtls_entropy_func,
            entropy.get(),
            reinterpret_cast<const unsigned char*>(pers),
            strlen(pers));

        size_t ret_size;
        rc = mbedtls_pk_sign(pk_context.get(),
                             MBEDTLS_MD_SHA256,
                             digest,
                             sizeof(digest),
                             buf,
                             &ret_size,
                             mbedtls_ctr_drbg_random,
                             ctr_drbg.get());
        if (rc != 0)
        {
            throw std::runtime_error(mbedtls_error_to_msg(rc));
        }

        ret += ".";
        ret += base64::encode_urlsafe(buf, ret_size);

        return ret;
    }
}
