#include <string>
#include <sstream>
#include <list>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

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

const int THROTTLE_INTERVAL_LENGTH = 0;
const int DEBOUNCE_INTERVAL = 250;

const char* NTP_SERVER = "pool.ntp.org";

std::list<Event> my_events;

std::string get_mac_address()
{
  std::stringstream ss;
  ss << std::hex << ESP.getEfuseMac();

  return ss.str();
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
  my_events.push_front(Event("walk", now));
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
  my_events.push_front(Event("wheels", now));
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
  my_events.push_front(Event("transit", now));
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
  my_events.push_front(Event("car", now));
}

FirebaseClient client("surveybox-fe69c", "events", get_mac_address());

void setup()
{
  Serial.begin(115200);
  pinMode(WALK_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(WALK_LED_PIN, OUTPUT);

  pinMode(WHEELS_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(WHEELS_LED_PIN, OUTPUT);

  pinMode(TRANSIT_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(TRANSIT_LED_PIN, OUTPUT);

  pinMode(CAR_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(CAR_LED_PIN, OUTPUT);

  Serial.print("Connecting to network");
  delay(4000);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Now connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  configTime(0, 0, NTP_SERVER);

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
  // Pedestrian button logic
  if (walk_button_pressed)
  {
    digitalWrite(WALK_LED_PIN, HIGH);
    walk_led_start_timestamp = millis();
    walk_button_pressed = false;
  }

  if (walk_led_start_timestamp > 0 && millis() - walk_led_start_timestamp >= 5000)
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

  if (wheels_led_start_timestamp > 0 && millis() - wheels_led_start_timestamp >= 5000)
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

  if (transit_led_start_timestamp > 0 && millis() - transit_led_start_timestamp >= 5000)
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

  if (car_led_start_timestamp > 0 && millis() - car_led_start_timestamp >= 5000)
  {
    digitalWrite(CAR_LED_PIN, LOW);
    car_led_start_timestamp = 0;
  }

  if (!to_be_emptied && my_events.size() > THROTTLE_INTERVAL_LENGTH)
  {
    to_be_emptied = true;
  }

  if (to_be_emptied)
  {
    client.add(my_events.front());
    my_events.pop_front();

    // TODO: branching efficiency
    if (my_events.empty())
    {
      to_be_emptied = false;
    }
  }
}