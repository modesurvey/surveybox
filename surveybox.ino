#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <list>
#include <sstream>
#include <string>
#include <time.h>
#include <WebServer.h>
#include <WiFi.h>
#include <map>
#include <memory>
#include <utility>

#include <AutoConnect.h>

#include "event.hpp"
#include "jwt.hpp"
#include "firestore_client.hpp"

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

std::string stream_id;
std::unique_ptr<firestore_client> g_firestore_client;

std::list<Event> events;

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

  server.on("/stream_selection", HTTP_GET, []() {
    Serial.println("Here");
    std::map<std::string, std::pair<std::string, std::string>> streams = g_firestore_client->get_stream_ids_and_names();
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
  g_firestore_client.reset(new firestore_client("firestore-admin@surveybox-fe69c.iam.gserviceaccount.com", key));

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
    bool added_event = g_firestore_client->add_new_event(stream_id, events.back());
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
