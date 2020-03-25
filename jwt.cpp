#include "jwt.hpp"
#include "base64.hpp"

#include <cstring>
#include <sstream>
#include <mbedtls/pk.h>
#include <mbedtls/error.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

// TODO: Give proper credit..

namespace jwt
{
    // TODO: Better error mechanism.
    std::string encode(const std::string& alg, const std::string& type, const std::string& payload, const uint8_t* key, size_t key_size)
    {
        std::string ret;
        std::stringstream header_ss;
        header_ss << "{\"alg\":\"" << alg << "\",\"typ\":\"" << type << "\"}";
        std::string header = header_ss.str();

        ret += base64::encode_urlsafe(header);
        ret += ".";
        ret += base64::encode_urlsafe(payload);

        mbedtls_pk_context pk_context;
        mbedtls_pk_init(&pk_context);
        int rc = mbedtls_pk_parse_key(&pk_context, key, key_size, NULL, 0);
        if (rc != 0) {
            return "";
        }

        uint8_t digest[32];
        const unsigned char* concated_ptr = (unsigned char*) ret.c_str();
        rc = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), concated_ptr, ret.length(), digest);
        if (rc != 0) {
            return "";
        }

        uint8_t* o_buf = new uint8_t[5000]; // TODO: What is the actual length.
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_entropy_init(&entropy);

        const char* pers = "__ms_jwt_entropy";
        mbedtls_ctr_drbg_seed(
            &ctr_drbg,
            mbedtls_entropy_func,
            &entropy,
            (const unsigned char*)pers,
            strlen(pers));

        size_t ret_size;
        rc = mbedtls_pk_sign(&pk_context, MBEDTLS_MD_SHA256, digest, sizeof(digest), o_buf, &ret_size, mbedtls_ctr_drbg_random, &ctr_drbg);
        if (rc != 0) {
            return "";
        }

        std::string encoded_signature = base64::encode_urlsafe(o_buf, ret_size);
        ret += ".";
        ret += encoded_signature;

        mbedtls_pk_free(&pk_context);
        delete[] o_buf;

        return ret;
    }
}
