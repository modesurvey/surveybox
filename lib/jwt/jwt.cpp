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
#include <Arduino.h>

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

        mbedtls_pk_context pk_context;
        mbedtls_pk_init(&pk_context);
        int rc = mbedtls_pk_parse_key(&pk_context, key, key_size, NULL, 0);
        if (rc != 0)
        {
            //mbedtls_pk_free(&pk_context);
            throw std::runtime_error(mbedtls_error_to_msg(rc));
        }

        uint8_t digest[32];
        const unsigned char* concated_ptr = reinterpret_cast<const unsigned char*>(ret.c_str());
        rc = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), concated_ptr, ret.length(), digest);
        if (rc != 0)
        {
            //mbedtls_pk_free(&pk_context);
            throw std::runtime_error(mbedtls_error_to_msg(rc));
        }

        std::unique_ptr<uint8_t> o_buf(new uint8_t[300]); // TODO: What is the actual length.
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_entropy_init(&entropy);

        const char* pers = "__ms_jwt_entropy";
        mbedtls_ctr_drbg_seed(
            &ctr_drbg,
            mbedtls_entropy_func,
            &entropy,
            reinterpret_cast<const unsigned char*>(pers),
            strlen(pers));

        size_t ret_size;
        rc = mbedtls_pk_sign(&pk_context, MBEDTLS_MD_SHA256, digest, sizeof(digest), o_buf.get(), &ret_size, mbedtls_ctr_drbg_random, &ctr_drbg);
        if (rc != 0)
        {
            //mbedtls_pk_free(&pk_context);
            throw std::runtime_error(mbedtls_error_to_msg(rc));
        }

        ret += ".";
        ret += base64::encode_urlsafe(o_buf.get(), ret_size);

        mbedtls_pk_free(&pk_context);

        return ret;
    }
}
