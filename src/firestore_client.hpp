#ifndef __MS_FIRESTORE_CLIENT_HPP
#define __MS_FIRESTORE_CLIENT_HPP

#include "event.hpp"

#include <string>
#include <map>

#include <HTTPClient.h>

// TODO: Naming convention
class firestore_client
{
    public:
        firestore_client(const std::string& iss, const char* key);
        std::map<std::string, std::pair<std::string, std::string>> get_stream_ids_and_names();
        std::string get_location_name(const std::string& location_id);
        bool add_new_event(const std::string& stream_id, const Event& event);

    private:
        bool update_auth_token();
        void update_auth_token_if_necessary();

        std::string iss_;
        const char* key_;

        HTTPClient http_client_;
        bool auth_token_initialized_;
        std::string cur_auth_token_;
        unsigned long last_auth_token_refresh_time_;
};

#endif
