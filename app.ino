// TODO: How to properly keep track of time, what is actually going on with that
//       How to properly do HTTPS requests to firebase
//       How to do deep sleep


#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h" // TODO: what is the diff between quotes and brackets here.
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

// TODO: More efficient and natural way to to do this. Am I using C++ or C?
std::string getMacAddress()
{
  std::stringstream ss;
  ss << std::hex << ESP.getEfuseMac();

  return ss.str();
}

std::string responsePayloadCreation(const std::string& type)
{
  // TODO: Will this time always be right or will it drift with sleep and what not?
  // TODO: Do I have to recalibrate it?
  time_t now = time(nullptr);
  Serial.println(now);

  // TODO: Could possibly create a nice class for this.
  std::stringstream ss;
  ss << "{\"timestamp\":\"" << now << "\",";
  ss << "\"type\":\"" << type << "\",";
  ss << "\"unit\":\"" << getMacAddress() << "\"}";

  return ss.str();
}

// TODO: Consider IRAM_ATTR
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
  pinMode(TRANSIT_LED_PIN, OUTPUT); // TODO: what resistor to use for this LED?

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

// TODO: This should be replaced with an actual sleep
void loop()
{
  if (transitSelectedCount > BUFFER_SIZE)
  {
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
      transitSelectedCount--; // TODO: Figure out a better way to make this atomic. Also "debounce" input presses to help with this.

      // TODO: For sending back data, we want the time to be very accurate to do better analysis. So perhaps we should batch the responses
      // in some way, however, this can be tabled for now after the initial prototype. Perhaps that is too much robustness at this moment.

      digitalWrite(TRANSIT_LED_PIN, HIGH);

      client.begin("https://surveybox-fe69c.firebaseio.com/responses.json");
      client.addHeader("Content-Type", "application/json");
      int code = client.POST(responsePayloadCreation("transit").c_str());

      // TODO: Response logging.

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