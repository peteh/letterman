#include <Arduino.h>
#include <LoRa.h>
// display include
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

/*
Deep Sleep with External Wake Up
=====================================
This code displays how to use deep sleep with
an external trigger as a wake up source and how
to store data in RTC memory to use it over reboots

This code is under Public Domain License.

Hardware Connections
======================
Push Button to GPIO 33 pulled down with a 10K Ohm
resistor

NOTE:
======
Only RTC IO can be used as a source for external wake
source. They are pins: 0,2,4,12-15,25-27,32-39.

Author:
Pranav Cherukupalli <cherukupallip@gmail.com>
*/
// LoRa Band (change it if you are outside Europe according to your country)
#define LORA_BAND 866E6

// TODO: find pins that can be used for wakeup
// TODO: maybe use shift instead
// RTC pins:
//    ESP32: 0, 2, 4, 12-15, 25-27, 32-39;
//    ESP32-S2: 0-21;
//    ESP32-S3: 0-21.

// GPIO 2
#define INPUT_MOTION 2
// letterbox door, we want to wire it with pull up resistor to have HIGH when door is open (switch open)
#define INPUT_DOOR 15

#define WAKEUP_BITMASK_INPUT_MOTION (1 << (INPUT_MOTION - 1))
#define WAKEUP_BITMASK_INPUT_DOOR (1 << (INPUT_DOOR - 1))

#define WAKEUP_BITMASK (WAKEUP_BITMASK_INPUT_MOTION | WAKEUP_BITMASK_INPUT_DOOR)

RTC_DATA_ATTR int bootCount = 0;

#define SCREEN_WIDTH 128                                      // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                      // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire); //, OLED_RST);
uint16_t msgCounter = 0;
uint32_t loraIdentifier = 0xDEADBEEF;

void resetDisplay()
{
  digitalWrite(OLED_RST, LOW);
  delay(25);
  digitalWrite(OLED_RST, HIGH);
}

void initDisplay()
{
  Serial.println("Initializing display...");

  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println("Wire done");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, true))
  {
    Serial.println("Failed to initialize the display");
    for (;;)
      ;
  }
  Serial.println("Display initialized");
  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("LORA SENDER");
  display.display();
}

void initLoRa()
{
  Serial.println("Initializing LoRa....");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  // Start LoRa using the frequency
  int result = LoRa.begin(LORA_BAND);
  if (result != 1)
  {
    display.setCursor(0, 10);
    display.println("Failed to start LoRa network!");
    for (;;)
      ;
  }
  Serial.println("LoRa initialized");
  display.setCursor(0, 15);
  display.println("LoRa network OK!");
  display.display();
  delay(2000);
}

void sendLoRaMsg()
{
  // Identifier:uint16, payloadsize:uint16t, payload
  uint8_t buffer[1000];
  LoRa.beginPacket();
  LoRa.write((uint8_t*)(&loraIdentifier),sizeof(loraIdentifier));

  StaticJsonDocument<1000> doc;
  // Add values in the document
  //
  doc["d"] = "on";   // off = closed
  doc["m"] = "on"; // off = clear
  doc["c"] = msgCounter;
  uint16_t length = (uint16_t)serializeJson(doc, buffer, sizeof(buffer));
  // write number of bytes for payload
  LoRa.write((uint8_t*)(&length), sizeof(length));
  // write buffer
  LoRa.write(buffer, length);
  // send it out
  LoRa.endPacket();

  Serial.printf("Sending %d bytes payload", length);
  Serial.println();
  Serial.printf("Buffer: %s", buffer);
  Serial.println();
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

  if (GPIO_reason & WAKEUP_BITMASK_INPUT_DOOR)
  {
    Serial.println("Door was opened");
  }

  if (GPIO_reason & WAKEUP_BITMASK_INPUT_MOTION)
  {
    Serial.println("Motion was triggered");
  }
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  delay(1000); // Take some time to open up the Serial Monitor
  initDisplay();
  Serial.println("Init done");
  initLoRa();
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
  // esp_deep_sleep_enable_ext0_wakeup(GPIO_NUM_15,1); //1 = High, 0 = Low

  // If you were to use ext1, you would use it like
  if (esp_sleep_enable_ext1_wakeup(WAKEUP_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH) != ESP_OK)
  {
    Serial.println("Failed to configure ext1 with the given parameters");
  }

  // Go to sleep now
  Serial.println("Going to sleep now");
  delay(1000);
  // esp_deep_sleep_start();
  // Serial.println("This will never be printed");
}
bool ledState = false;
void loop()
{
  Serial.println("Sending Message");
  sendLoRaMsg();
  Serial.println("Finished sending Message");
  msgCounter++;
  ledState = !ledState;
  digitalWrite(LED_BUILTIN, ledState);
  delay(5000);
}
