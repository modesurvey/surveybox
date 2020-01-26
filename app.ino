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

const char* SSID = "MYNETWORK";
const char* PASSWORD = "MYPASSWORD";

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

const int LED_SELECTION_DURATION_MS = 3000;
const int THROTTLE_INTERVAL_LENGTH = 0;
const int DEBOUNCE_INTERVAL = 500;

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
void IRAM_ATTR walk_button_pressed_interrupt()
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



  // Note that WiFi#begin inside of loop is a workaround for issues with connecting to AP
  // after reset and some boots.
  /*
  Serial.print("Connecting to network");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(WALK_LED_PIN, HIGH);
    delay(1000);
    digitalWrite(WALK_LED_PIN, LOW);
  }
  */
  Serial.println("Starting portal...");

  bool success = portal.begin();
  if (!success)
  {
    Serial.println("Not successfully connected");
  }

  // TODO: Should this exist with the website portal as well?
  std::vector<int> wifi_connected_pattern = { WALK_LED_PIN, WHEELS_LED_PIN, TRANSIT_LED_PIN, CAR_LED_PIN };
  display_led_pattern(wifi_connected_pattern, 2000);

  Serial.println();
  Serial.println("Now connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  //configTime(0, 0, NTP_SERVER);

  attachInterrupt(WALK_BUTTON_PIN, walk_button_pressed_interrupt, RISING);
  attachInterrupt(WHEELS_BUTTON_PIN, wheels_button_pressed_interrupt, RISING);
  attachInterrupt(TRANSIT_BUTTON_PIN, transit_button_pressed_interrupt, RISING);
  attachInterrupt(CAR_BUTTON_PIN, car_button_pressed_interrupt, RISING);
}

unsigned long walk_led_start_timestamp = 0;
unsigned long wheels_led_start_timestamp = 0;
unsigned long transit_led_start_timestamp = 0;
unsigned long car_led_start_timestamp = 0;

bool to_be_emptied = false;

void loop()
{
  portal.handleClient();

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

  if (!to_be_emptied && events.size() > THROTTLE_INTERVAL_LENGTH)
  {
    to_be_emptied = true;
  }

  if (to_be_emptied)
  {
    client.add(events.front());
    events.pop_front();

    // TODO: branching efficiency
    if (events.empty())
    {
      to_be_emptied = false;
    }
  }
}