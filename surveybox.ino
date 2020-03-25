#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <list>
#include <sstream>
#include <string>
#include <time.h>
#include <WebServer.h>
#include <WiFi.h>
#include <map>
#include <utility>

#include <AutoConnect.h>

#include "event.hpp"
#include "jwt.hpp"

const char* key = "My key goes here.";

// TODO: Going to add all the board logic in this file. Apart from some crazy DSL, I don't see a very general way to do this and keep sane.
// TODO: This has to actually be specified properly.
const int WALK_BUTTON_PIN = 23;
const int WHEELS_BUTTON_PIN = 22;
const int TRANSIT_BUTTON_PIN = 21;
const int CAR_BUTTON_PIN = 19;

const int WALK_LED_PIN = 25;
const int WHEELS_LED_PIN = 26;
const int TRANSIT_LED_PIN = 27;
const int CAR_LED_PIN = 14;

const int LED_SELECTION_DURATION_MS = 2000;
const int THROTTLE_INTERVAL_LENGTH = 0;
const int DEBOUNCE_INTERVAL = 300;

const char* NTP_SERVER = "pool.ntp.org";
HTTPClient http_client;

WebServer server;
AutoConnect portal(server);
AutoConnectConfig autoConnectConfig;

// Firestore connection information.
std::string iss = "firestore-admin@surveybox-fe69c.iam.gserviceaccount.com";
std::string scope = "https://www.googleapis.com/auth/datastore";
std::string aud = "https://oauth2.googleapis.com/token";

std::string HTML_PART_1 = " \
<!DOCTYPE html> \
<html> \
    <head> \
        <title>Stream selection</title> \
    </head> \
 \
    <body> \
        <form action=\"/stream_selection\" method=\"post\"> \
";

std::string HTML_PART_2 = " \
            <input type=\"submit\" value=\"Submit\" /> \
        </form> \
    </body> \
</html> \
";

std::string auth_token;
long last_auth_token_refresh = -1;
std::string stream_id;

std::list<Event> events;

std::string create_firestore_jwt_1hr(const std::string& iss, const std::string& scope, const std::string& aud)
{
  std::string alg = "RS256";
  std::string typ = "JWT";

  time_t now = time(nullptr);
  time_t later = now + 3599;

  std::stringstream payload_ss;
  payload_ss << "{ \"iss\": \"" << iss << "\", \"scope\": \"" << scope << "\", \"aud\": \"" << aud << "\",";
  payload_ss << "\"iat\":" << now << ",";
  payload_ss << "\"exp\":" << later << "}";
  std::string payload = payload_ss.str();
  return jwt::encode(alg, typ, payload, (uint8_t *) key, strlen(key) + 1);
}

bool refresh_auth_token_1hr()
{
  std::string s = "https://oauth2.googleapis.com/token?grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer&assertion=";
  std::string jwt = create_firestore_jwt_1hr(iss, scope, aud);
  s += jwt;
  String endpoint = String(s.c_str());

  http_client.begin(endpoint);
  http_client.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http_client.addHeader("Content-Length", "0");
  int http_code = http_client.POST("");
  String resp = http_client.getString();

  http_client.end();

  std::string res = "";
  if (http_code == 200)
  {
    int capacity = JSON_OBJECT_SIZE(4) + resp.length();
    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, resp.c_str());
    if (err)
    {
      Serial.print(F("Deserialize json failed with code: "));
      Serial.println(err.c_str());
      return false;
    }

    if (doc.containsKey("access_token"))
    {
      std::string t(doc["access_token"].as<char*>());
      auth_token = std::move(t);
      last_auth_token_refresh = millis();
    }
  }

  return true;
}

void begin_auth_http_client(const std::string& auth_token, const std::string& endpoint)
{
  std::string authorization_header = "Bearer " + auth_token;
  http_client.begin(endpoint.c_str());
  http_client.addHeader("Authorization", authorization_header.c_str());
}

std::map<std::string, std::pair<std::string, std::string>> get_stream_ids_and_names(const std::string& auth_token)
{
  Serial.println(auth_token.c_str());
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
    begin_auth_http_client(auth_token, endpoint);
    int http_code = http_client.GET();
    String resp = http_client.getString();

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
    std::string location_name = get_location_name(auth_token, location_id);
    streams[stream_id] = std::make_pair(stream_name, location_name);

    another = doc.containsKey("nextPageToken");
    if (another)
    {
      nextPageToken = std::string(doc["nextPageToken"].as<const char*>());
    }

    http_client.end();
  }

  return streams;
}

std::string get_location_name(const std::string& auth_token, const std::string& location_id)
{
  std::string endpoint = "https://firestore.googleapis.com/v1/" + location_id;
  begin_auth_http_client(auth_token, endpoint);

  int http_code = http_client.GET();
  String resp = http_client.getString();
  http_client.end();

  if (http_code != 200)
  {
    return "";
  }

  int capacity = 2*JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + resp.length();
  DynamicJsonDocument doc(capacity);
  DeserializationError err = deserializeJson(doc, resp.c_str());

  if (err)
  {
    return "";
  }

  std::string t(doc["fields"]["name"]["stringValue"].as<const char*>());
  return t;
}

bool add_new_event(const std::string& auth_token, const std::string& stream_id, const Event& event)
{
  std::string endpoint = "https://firestore.googleapis.com/v1/" + stream_id + "/events";
  begin_auth_http_client(auth_token, endpoint);
  http_client.addHeader("Content-Type", "application/json");

  std::stringstream payload_ss;
  payload_ss << "{\"fields\": {\"timestamp\":{\"stringValue\":\"" << event.timestamp << "\"}, ";
  payload_ss << "\"type\":{\"stringValue\":\"" << event.type << "\"}}}";
  std::string payload = payload_ss.str();
  std::stringstream ss;
  ss << payload.length();
  std::string i = ss.str();
  http_client.addHeader("Content-Length", i.c_str());
  int http_code = http_client.POST((uint8_t*) payload.c_str(), payload.length());
  http_client.end();

  return http_code == 200;
}

// Method to display certain led patterns
void display_led_pattern(const std::vector<int>& pattern, unsigned int display_time_ms)
{
  for (auto const& item : pattern)
  {
    digitalWrite(item, HIGH);
    delay(display_time_ms);
    digitalWrite(item, LOW);
  }
}

volatile bool walk_button_pressed = false;
volatile unsigned long walk_interrupt_timestamp = 0;
void IRAM_ATTR walk_button_change_interrupt()
{
  if (millis() - walk_interrupt_timestamp < DEBOUNCE_INTERVAL)
  {
    return;
  }
  walk_interrupt_timestamp = millis();

  walk_button_pressed = true;

  // TODO: This may not be fast enough
  time_t now = time(nullptr);
  events.push_front(Event("walk", now));
}

volatile bool wheels_button_pressed = false;
volatile unsigned long wheels_interrupt_timestamp = 0;
void IRAM_ATTR wheels_button_pressed_interrupt()
{
  if (millis() - wheels_interrupt_timestamp < DEBOUNCE_INTERVAL)
  {
    return;
  }
  wheels_interrupt_timestamp = millis();

  wheels_button_pressed = true;

  // TODO: This may not be fast enough
  time_t now = time(nullptr);
  events.push_front(Event("wheels", now));
}

volatile bool transit_button_pressed = false;
volatile unsigned long transit_interrupt_timestamp = 0;
void IRAM_ATTR transit_button_pressed_interrupt()
{
  if (millis() - transit_interrupt_timestamp < DEBOUNCE_INTERVAL)
  {
    return;
  }
  transit_interrupt_timestamp = millis();

  transit_button_pressed = true;

  // TODO: This may not be fast enough
  time_t now = time(nullptr);
  events.push_front(Event("transit", now));
}

volatile bool car_button_pressed = false;
volatile unsigned long car_interrupt_timestamp = 0;
void IRAM_ATTR car_button_pressed_interrupt()
{
  if (millis() - car_interrupt_timestamp < DEBOUNCE_INTERVAL)
  {
    return;
  }
  car_interrupt_timestamp = millis();

  car_button_pressed = true;

  // TODO: This may not be fast enough
  time_t now = time(nullptr);
  events.push_front(Event("car", now));
}

std::vector<int> wifi_connected_pattern = { WALK_LED_PIN, WHEELS_LED_PIN, TRANSIT_LED_PIN, CAR_LED_PIN };

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("Here at the beginning.");
  pinMode(WALK_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(WALK_LED_PIN, OUTPUT);

  pinMode(WHEELS_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(WHEELS_LED_PIN, OUTPUT);

  pinMode(TRANSIT_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(TRANSIT_LED_PIN, OUTPUT);

  pinMode(CAR_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(CAR_LED_PIN, OUTPUT);

  Serial.println("Starting portal...");

  server.on("/stream_selection", HTTP_GET, [&auth_token]() {
    Serial.println("Here is the auth_token");
    Serial.println(auth_token.c_str());
    std::map<std::string, std::pair<std::string, std::string>> streams = get_stream_ids_and_names(auth_token);
    std::string page = HTML_PART_1;

    for (auto kvp : streams)
    {
      page += "<input type=\"radio\" id=\"" + kvp.first + "\" name=\"stream\" value=\"" + kvp.first + "\">";
      page += "<label for=\"" + kvp.first + "\">(" + kvp.second.first + ", " + kvp.second.second + ")</label><br/>";
    }
    page += HTML_PART_2;

    server.send(200, "text/html", String(page.c_str()));
  });

  server.on("/stream_selection", HTTP_POST, [&]() {
    int args_n = server.args();

    for (int i = 0; i < args_n; i++)
    {
      String name = server.argName(i);
      if (name == "stream")
      {
        String value = server.arg(i);
        stream_id = std::string(value.c_str());
        Serial.println(stream_id.c_str());
      }
    }

    server.send(200, "text/plain", "Successfully deployed!");
  });

  display_led_pattern(wifi_connected_pattern, 2000);

  autoConnectConfig.title = "ModeSurvey";
  autoConnectConfig.apid = "modesurvey_ap";
  autoConnectConfig.psk = "countthemode";
  autoConnectConfig.autoReconnect = false;
  portal.config(autoConnectConfig);
  bool success = portal.begin();
  if (!success)
  {
    Serial.println("Not successfully connected");
  }

  display_led_pattern(wifi_connected_pattern, 2000);

  Serial.println();
  Serial.println("Now connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  configTime(0, 0, NTP_SERVER);

  delay(10000);

  Serial.println("Ready to start.");

  Serial.println("Refreshing authentication token...");
  while (!refresh_auth_token_1hr());
  Serial.println("Refreshed");
  Serial.println(auth_token.c_str());

  attachInterrupt(WALK_BUTTON_PIN, walk_button_change_interrupt, RISING);
  attachInterrupt(WHEELS_BUTTON_PIN, wheels_button_pressed_interrupt, RISING);
  attachInterrupt(TRANSIT_BUTTON_PIN, transit_button_pressed_interrupt, RISING);
  attachInterrupt(CAR_BUTTON_PIN, car_button_pressed_interrupt, RISING);
}

unsigned long walk_led_start_timestamp = 0;
unsigned long wheels_led_start_timestamp = 0;
unsigned long transit_led_start_timestamp = 0;
unsigned long car_led_start_timestamp = 0;

bool to_be_emptied = false;
bool prod_switch_complete = false;
bool walk_button_pressed_cache = false;

void loop()
{
  if (millis() - last_auth_token_refresh > 3500000)
  {
    Serial.println("Refreshing authentication token in loop...");
    while (!refresh_auth_token_1hr());
    Serial.println("Refreshed");
  }

  portal.handleClient();
  // TODO: this will technically work in 95% of cases at the moment, but it can fail in very edge cases, and ideally this will be cleaner and
  // easier to understand.
  if (!prod_switch_complete && false)
  {
    display_led_pattern(wifi_connected_pattern, 2000);
    prod_switch_complete = true;
  }

  // Pedestrian button logic
  if (walk_button_pressed)
  {
    digitalWrite(WALK_LED_PIN, HIGH);
    walk_led_start_timestamp = millis();
    walk_button_pressed = false;
  }

  if (walk_led_start_timestamp > 0 && millis() - walk_led_start_timestamp >= LED_SELECTION_DURATION_MS)
  {
    digitalWrite(WALK_LED_PIN, LOW);
    walk_led_start_timestamp = 0;
  }

  // Wheels button logic
  if (wheels_button_pressed)
  {
    digitalWrite(WHEELS_LED_PIN, HIGH);
    wheels_led_start_timestamp = millis();
    wheels_button_pressed = false;
  }

  if (wheels_led_start_timestamp > 0 && millis() - wheels_led_start_timestamp >= LED_SELECTION_DURATION_MS)
  {
    digitalWrite(WHEELS_LED_PIN, LOW);
    wheels_led_start_timestamp = 0;
  }

  // TRANSIT button logic.
  if (transit_button_pressed)
  {
    digitalWrite(TRANSIT_LED_PIN, HIGH);
    transit_led_start_timestamp = millis();
    transit_button_pressed = false;
  }

  if (transit_led_start_timestamp > 0 && millis() - transit_led_start_timestamp >= LED_SELECTION_DURATION_MS)
  {
    digitalWrite(TRANSIT_LED_PIN, LOW);
    transit_led_start_timestamp = 0;
  }

  // Car button logic
  if (car_button_pressed)
  {
    digitalWrite(CAR_LED_PIN, HIGH);
    car_led_start_timestamp = millis();
    car_button_pressed = false;
  }

  if (car_led_start_timestamp > 0 && millis() - car_led_start_timestamp >= LED_SELECTION_DURATION_MS)
  {
    digitalWrite(CAR_LED_PIN, LOW);
    car_led_start_timestamp = 0;
  }

  if (!to_be_emptied && events.size() > THROTTLE_INTERVAL_LENGTH && stream_id.length() != 0)
  {
    to_be_emptied = true;
  }

  if (to_be_emptied)
  {
    bool added_event = add_new_event(auth_token, stream_id, events.back());
    if (added_event)
    {
      events.pop_back(); // TODO: using the same buffer here and in interrupts means that there is a risk of thread overlap

      // TODO: branching efficiency
      if (events.empty())
      {
        to_be_emptied = false;
      }
    }
  }
}
