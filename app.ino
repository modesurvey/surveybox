#include <string>
#include <sstream>
#include <list>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <time.h>
#include <AutoConnect.h>

#include "firebase_client.hpp"
#include "event.hpp"

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
const int DEBOUNCE_INTERVAL = 150;

const char* NTP_SERVER = "pool.ntp.org";

WebServer server;
AutoConnect portal(server);

std::list<Event> events;

std::string get_mac_address()
{
  std::stringstream ss;
  ss << std::hex << ESP.getEfuseMac();

  return ss.str();
}

// TODO: This is quick and dirty for now. This should be replaced with a web based interface. This is also more extensible
// for more automation in the future. Full story still to be fleshed out.
bool buttons_match_production_key(const std::vector<int>& buttons)
{
  return buttons.size() >= 5 &&
    buttons[buttons.size() - 1] == 1 &&
    buttons[buttons.size() - 2] == 4 &&
    buttons[buttons.size() - 3] == 2 &&
    buttons[buttons.size() - 4] == 3 &&
    buttons[buttons.size() - 5] == 1;
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

FirebaseClient client("surveybox-fe69c", "events-test", get_mac_address());
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

  display_led_pattern(wifi_connected_pattern, 2000);

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
std::vector<int> last_buttons;

void loop()
{
  portal.handleClient();
  // TODO: this will technically work in 95% of cases at the moment, but it can fail in very edge cases, and ideally this will be cleaner and
  // easier to understand.
  if (!prod_switch_complete && buttons_match_production_key(last_buttons))
  {
    display_led_pattern(wifi_connected_pattern, 2000);
    client = FirebaseClient("surveybox-fe69c", "events", get_mac_address());
    prod_switch_complete = true;
  }

  // Pedestrian button logic
  if (walk_button_pressed)
  {
    last_buttons.push_back(1);
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
    last_buttons.push_back(2);
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
    last_buttons.push_back(3);
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
    last_buttons.push_back(4);
    digitalWrite(CAR_LED_PIN, HIGH);
    car_led_start_timestamp = millis();
    car_button_pressed = false;
  }

  if (car_led_start_timestamp > 0 && millis() - car_led_start_timestamp >= LED_SELECTION_DURATION_MS)
  {
    digitalWrite(CAR_LED_PIN, LOW);
    car_led_start_timestamp = 0;
  }

  if (!to_be_emptied && events.size() > THROTTLE_INTERVAL_LENGTH)
  {
    to_be_emptied = true;
  }

  if (to_be_emptied)
  {
    bool added_event = client.add(events.back());
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
