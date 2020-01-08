#include <sstream>

#include "firebase_client.hpp"

FirebaseClient::FirebaseClient(const std::string& endpoint, const std::string& path, const std::string& deviceid) : _deviceid(deviceid)
{
    std::stringstream ss;
    ss << "https://" << endpoint << ".firebaseio.com/" << path << ".json";
    _url = ss.str();
}

bool FirebaseClient::add(const Event& event)
{
    _client.begin(_url.c_str());
    _client.addHeader("Content-Type", "application/json");

    std::string payload = _payload_creation(event);
    int code = _client.POST(payload.c_str());

    _client.end();

    return code == 200;
}

std::string FirebaseClient::_payload_creation(const Event& event) const
{
  std::stringstream ss;

  ss << "{\"timestamp\":\"" << event.timestamp << "\",";
  ss << "\"type\":\"" << event.type << "\",";
  ss << "\"unit\":\"" << _deviceid << "\"}";

  return ss.str();
}