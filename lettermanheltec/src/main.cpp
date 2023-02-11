/*
  Code Base from RadioLib: https://github.com/jgromes/RadioLib/tree/master/examples/SX126x

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <ArduinoJson.h>

#define LORA_MOSI 10
#define LORA_MISO 11
#define LORA_SCK 9

#define LORA_CS 8
#define LORA_IRQ 14
#define LORA_RST 12
#define LORA_BUSY 13
#define LORA_FREQ 868.

#define LED 35
// letterbox door, we want to wire it with pull up resistor to have HIGH when door is open (switch open)
#define INPUT_DOOR 7

// GPIO 2
#define INPUT_MOTION 6

#define WAKEUP_BITMASK (1<<INPUT_MOTION | 1 << INPUT_DOOR)

RTC_DATA_ATTR int bootCount = 0;

SX1262 g_radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, LORA_BUSY);
uint16_t g_msgCounter = 0;
bool g_doorOpen = false;
bool g_motionDetected = false;
bool g_newMail = true;

void initRadio()
{
  // initialize SX1262 with default settings
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  Serial.print(F("[SX1262] Initializing ... "));
  int state = g_radio.begin(LORA_FREQ);
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }
}

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

/*
Method to print the GPIO that triggered the wakeup
*/
void print_GPIO_wake_up()
{
  uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
  Serial.print("GPIO that triggered the wake up: GPIO ");
  Serial.println((log(GPIO_reason)) / log(2), 0);

  if (GPIO_reason & (1 << INPUT_DOOR))
  {
    Serial.println("Door was opened");
  }

  if (GPIO_reason & (1 << INPUT_MOTION))
  {
    Serial.println("Motion was triggered");
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(INPUT_DOOR, INPUT);
  pinMode(INPUT_MOTION, INPUT);
  initRadio();
  // Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  // Print the wakeup reason for ESP32
  print_wakeup_reason();

  // Print the GPIO used to wake up
  print_GPIO_wake_up();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up for an external trigger.
  There are two types for ESP32, ext0 and ext1 .
  ext0 uses RTC_IO to wakeup thus requires RTC peripherals
  to be on while ext1 uses RTC Controller so doesnt need
  peripherals to be powered on.
  Note that using internal pullups/pulldowns also requires
  RTC peripherals to be turned on.
  */
  //if (esp_sleep_enable_ext0_wakeup(GPIO_NUM_6, 1) != ESP_OK) // 1 = High, 0 = Low
  //{
  //  Serial.println("Failed to configure ext0 with the given parameters");
  //}
  //if (esp_sleep_enable_ext0_wakeup(GPIO_NUM_7, 0) != ESP_OK) // 1 = High, 0 = Low
  //{
  //  Serial.println("Failed to configure ext0 with the given parameters");
  //}
  // If you were to use ext1, you would use it like
  if (esp_sleep_enable_ext1_wakeup(WAKEUP_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH) != ESP_OK)
  {
    Serial.println("Failed to configure ext1 with the given parameters");
  }
}

void sendLoRaMsg()
{
  Serial.print(F("[SX1262] Transmitting packet ... "));
  // Identifier:uint16, payloadsize:uint16t, payload
  uint8_t buffer[1000];
  // LoRa.beginPacket();
  // LoRa.write((uint8_t*)(&loraIdentifier),sizeof(loraIdentifier));

  StaticJsonDocument<1000> doc;
  // Add values in the document
  //
  doc["door"] = (g_doorOpen ? "open" : "closed"); // off = closed
  doc["motion"] = (g_motionDetected) ? "on" : "off";  // off = clear
  doc["newmail"] = (g_newMail) ? "on" : "off";  // off = clear
  doc["c"] = g_msgCounter;
  uint16_t length = (uint16_t)serializeJson(doc, buffer, sizeof(buffer));
  // write number of bytes for payload
  // LoRa.write((uint8_t*)(&length), sizeof(length));
  int state = g_radio.transmit(buffer, length);
  g_msgCounter++;
  // write buffer
  // LoRa.write(buffer, length);
  // send it out
  // LoRa.endPacket();

  if (state == RADIOLIB_ERR_NONE)
  {
    // the packet was successfully transmitted
    Serial.println(F("success!"));

    // print measured data rate
    Serial.print(F("[SX1262] Datarate:\t"));
    Serial.print(g_radio.getDataRate());
    Serial.println(F(" bps"));
  }
  else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
  {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));
  }
  else if (state == RADIOLIB_ERR_TX_TIMEOUT)
  {
    // timeout occured while transmitting packet
    Serial.println(F("timeout!"));
  }
  else
  {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  Serial.printf("Sending %d bytes payload", length);
  Serial.println();
  Serial.printf("Buffer: %s", buffer);
  Serial.println();
}

bool ledState = false;


void loop()
{
  digitalWrite(LED, ledState);
  ledState = !ledState;
  g_doorOpen = digitalRead(INPUT_DOOR);
  g_motionDetected = digitalRead(INPUT_MOTION);
  if(g_doorOpen)
  {
    g_newMail = false;
  }
  
  Serial.printf("Door open %d\n", g_doorOpen);
  sendLoRaMsg();
  // wait for a second before transmitting again
  // Go to sleep now
  if (!g_doorOpen && !g_motionDetected)
  {
    Serial.println("Going to sleep now");
    delay(100);
    esp_deep_sleep_start();
  }
  // Serial.println("This will never be printed");
  delay(1000);
}