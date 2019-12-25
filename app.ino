#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
#include <string>
#include <sstream>

const char* SSID = "MYNETWORK";
const char* PASSWORD = "MYPASSWORD";
const int TRANSIT_BUTTON_PIN = 23;
const int TRANSIT_LED_PIN = 21;

const char* NTP_SERVER = "pool.ntp.org";

const int BUFFER_SIZE = 10;
const int WAKEUP_INTERVAL_S = 10;

HTTPClient client;

std::string getMacAddress()
{
  std::stringstream ss;
  ss << std::hex << ESP.getEfuseMac();

  return ss.str();
}

std::string responsePayloadCreation(const std::string& type)
{
  time_t now = time(nullptr);
  Serial.println(now);

  std::stringstream ss;
  ss << "{\"timestamp\":\"" << now << "\",";
  ss << "\"type\":\"" << type << "\",";
  ss << "\"unit\":\"" << getMacAddress() << "\"}";

  return ss.str();
}

volatile int transitSelectedCount = 0;
void IRAM_ATTR TransitButtonSelectedInterrupt()
{
  Serial.println("Transit button interrupt");
  transitSelectedCount++;
}

void setup()
{
  Serial.begin(115200);
  pinMode(TRANSIT_BUTTON_PIN, INPUT);
  pinMode(TRANSIT_LED_PIN, OUTPUT);

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

  attachInterrupt(TRANSIT_BUTTON_PIN, TransitButtonSelectedInterrupt, RISING);
}

void loop()
{
  if (transitSelectedCount > BUFFER_SIZE)
  {
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
      transitSelectedCount--;

      digitalWrite(TRANSIT_LED_PIN, HIGH);

      client.begin("https://surveybox-fe69c.firebaseio.com/responses.json");
      client.addHeader("Content-Type", "application/json");
      int code = client.POST(responsePayloadCreation("transit").c_str());

      client.end();

      delay(2500);
      digitalWrite(TRANSIT_LED_PIN, LOW);
    }
  }
  else
  {
    delay(3000);
  }
}