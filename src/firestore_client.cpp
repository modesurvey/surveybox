#include "firestore_client.hpp"
#include "jwt.hpp"

#include <iomanip>
#include <cstring>
#include <ctime>
#include <sstream>

#include <ArduinoJson.h>

namespace
{
    std::string create_gc_1hr_jwt(const std::string& alg, const std::string& typ, const std::string& iss, const std::string& scope, const std::string& aud,
                                  const char* key)
    {
        time_t now = time(nullptr);
        time_t later = now + 3599;

        std::stringstream payload_ss;
        payload_ss << "{ \"iss\": \"" << iss << "\", \"scope\": \"" << scope << "\", \"aud\": \"" << aud << "\",";
        payload_ss << "\"iat\":" << now << ",";
        payload_ss << "\"exp\":" << later << "}";
        std::string payload = payload_ss.str();

        return jwt::encode(alg, typ, payload, reinterpret_cast<const uint8_t*>(key), strlen(key) + 1);
    }


    void begin_auth_http_client(HTTPClient& http_client, const std::string& auth_token, const std::string& endpoint)
    {
        std::string authorization_header = "Bearer " + auth_token;
        http_client.begin(endpoint.c_str());
        http_client.addHeader("Authorization", authorization_header.c_str());
    }

    std::string epoch_to_rfc3339(std::time_t epoch)
    {
        std::tm tm = *std::gmtime(&epoch);

        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

        return ss.str();
    }
}


firestore_client::firestore_client(const std::string& iss, const char* key) : iss_(iss), key_(key), auth_token_initialized_(false)
{ }


bool firestore_client::update_auth_token()
{
    std::string jwt;
    try
    {
        jwt = create_gc_1hr_jwt("RS256", "JWT", iss_, "https://www.googleapis.com/auth/datastore", "https://oauth2.googleapis.com/token", key_);
    }
    catch (const std::runtime_error& e)
    {
        return false;
    }
    std::string s = "https://oauth2.googleapis.com/token?grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer&assertion=";
    s += std::move(jwt);

    http_client_.begin(s.c_str());
    http_client_.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http_client_.addHeader("Content-Length", "0");
    int http_code = http_client_.POST("");
    String resp = http_client_.getString();

    if (http_code == 200)
    {
        int capacity = JSON_OBJECT_SIZE(4) + resp.length();
        DynamicJsonDocument doc(capacity);
        DeserializationError err = deserializeJson(doc, resp.c_str());
        if (err)
        {
            http_client_.end();
            return false;
        }

        if (doc.containsKey("access_token"))
        {
            cur_auth_token_ = std::string(doc["access_token"].as<char*>());
            last_auth_token_refresh_time_ = millis();
        }
    }

    http_client_.end();

    return true;
}


std::string firestore_client::get_location_name(const std::string& location_id)
{
    update_auth_token_if_necessary();

    std::string endpoint = "https://firestore.googleapis.com/v1/" + location_id;
    begin_auth_http_client(http_client_, cur_auth_token_, endpoint);

    int http_code = http_client_.GET();
    String resp = http_client_.getString();

    if (http_code != 200)
    {
        http_client_.end();
        throw std::runtime_error("HTTP response had non-200 response code.");
    }

    int capacity = 2*JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + resp.length();
    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, resp.c_str());

    if (err)
    {
        http_client_.end();
        throw std::runtime_error("Error deserializing json response.");
    }

    http_client_.end();

    return std::string(doc["fields"]["name"]["stringValue"].as<const char*>());
}


bool firestore_client::add_new_event(const std::string& stream_id, const Event& event)
{
    update_auth_token_if_necessary();

    std::string endpoint = "https://firestore.googleapis.com/v1/" + stream_id + "/events";
    begin_auth_http_client(http_client_, cur_auth_token_, endpoint);
    http_client_.addHeader("Content-Type", "application/json");

    std::string timestamp = epoch_to_rfc3339(event.timestamp);
    std::stringstream payload_ss;
    payload_ss << "{\"fields\": {\"timestamp\":{\"timestampValue\":\"" << timestamp << "\"}, ";
    payload_ss << "\"type\":{\"stringValue\":\"" << event.type << "\"}}}";
    std::string payload = payload_ss.str();

    std::stringstream ss;
    ss << payload.length();
    std::string i = ss.str();
    http_client_.addHeader("Content-Length", i.c_str());

    int http_code = http_client_.POST((uint8_t*) payload.c_str(), payload.length());
    http_client_.end();

    return http_code == 200;
}

std::map<std::string, std::pair<std::string, std::string>> firestore_client::get_stream_ids_and_names()
{
    update_auth_token_if_necessary();

    std::map<std::string, std::pair<std::string, std::string>> streams;
    bool another = true;
    std::string nextPageToken = "";
    while (another)
    {
        std::string endpoint = "https://firestore.googleapis.com/v1/projects/surveybox-fe69c/databases/(default)/documents/streams";
        endpoint += "?pageSize=1";
        if (nextPageToken != "")
        {
            endpoint += "&pageToken=" + nextPageToken;
        }
        begin_auth_http_client(http_client_, cur_auth_token_, endpoint);
        int http_code = http_client_.GET();
        String resp = http_client_.getString();

        if (http_code != 200)
        {
            return streams;
        }

        int capacity = JSON_ARRAY_SIZE(1) + 2*JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + resp.length();
        DynamicJsonDocument doc(capacity);

        DeserializationError err = deserializeJson(doc, resp.c_str());
        if (err)
        {
            another = false;
            continue;
        }

        std::string stream_id(doc["documents"][0]["name"].as<const char*>());
        std::string stream_name(doc["documents"][0]["fields"]["name"]["stringValue"].as<const char*>());
        std::string location_id(doc["documents"][0]["fields"]["location"]["referenceValue"].as<const char*>());

        another = doc.containsKey("nextPageToken");
        if (another)
        {
            nextPageToken = std::string(doc["nextPageToken"].as<const char*>());
        }
        http_client_.end();

        std::string location_name = get_location_name(location_id);
        streams[stream_id] = std::make_pair(stream_name, location_name);
    }

    return streams;
}

void firestore_client::update_auth_token_if_necessary()
{
    if (!auth_token_initialized_ || (millis() - last_auth_token_refresh_time_ > 3500 * 1000))
    {
        if (!update_auth_token())
        {
            throw std::runtime_error("Unable to initialize or refresh the auth token for firestore.");
        }
        auth_token_initialized_ = true;
        last_auth_token_refresh_time_ = millis();
    }
}
