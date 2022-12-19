#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// JSON Handling
#include <ArduinoJson.h>

// 433E6 for Asia
// 866E6 for Europe
// 915E6 for North America
#define BAND 866E6

// OLED pins
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  // initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false))
  { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("LORA RECEIVER ");
  display.display();


  Serial.println("LoRa Receiver");
  // SPI LoRa pins
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  // setup LoRa transceiver module
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  if (!LoRa.begin(866E6))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
}

void loop()
{
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
    uint8_t buffer[1000];
    // received a packet
    Serial.print("Received packet '");

    // TODO: check read length
    uint32_t sender;
    LoRa.readBytes((uint8_t *)(&sender), sizeof(sender)); // recipient address
    // TODO: check read length
    uint16_t expectedLength;
    LoRa.readBytes((uint8_t *)(&expectedLength), sizeof(expectedLength)); // incoming msg length

    // read packet
    int readLength = LoRa.readBytes(buffer, expectedLength);
    buffer[readLength] = 0;
    Serial.printf("%s", buffer);
    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());

    // Display information
    display.clearDisplay();
    display.setCursor(0, 0);
    for(int i = 0; i < strlen((char*)buffer); i++)
    {
      display.printf("%c", buffer[i]);
    }
    
    display.setCursor(0, 40);

    display.display();

    StaticJsonDocument<1000> doc;
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, buffer);

    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    serializeJsonPretty(doc, Serial);
    Serial.println();

    
  }
}