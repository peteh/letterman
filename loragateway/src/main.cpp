/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/ttgo-lora32-sx1276-arduino-ide/
*********/

// example class include

// Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

// Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define RST 14
#define DIO0 26

// 433E6 for Asia
// 866E6 for Europe
// 915E6 for North America
#define BAND 866E6

// OLED pins
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

String LoRaData;

void setup()
{
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

  // initialize Serial Monitor
  Serial.begin(115200);

  // SPI LoRa pins
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  // setup LoRa transceiver module
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  // Serial.println("LoRa Initializing OK!");
  display.setCursor(0, 10);
  display.println("LoRa Initializing OK!");
  display.display();
}

void loop()
{

  // try to parse packet
  int packetSize = LoRa.parsePacket();

  if (packetSize > 5)
  {
    // received a packet
    Serial.println("Received packet: ");
    // read packet header bytes:
    uint32_t sender = LoRa.read();       // recipient address
    uint16_t incomingLength = LoRa.read(); // incoming msg length

    LoRaData = "";

    while (LoRa.available())
    {
      LoRaData += (char)LoRa.read();
    }

    //if (incomingLength != LoRaData.length())
    //{ // check length for error
    //  Serial.println("error: message length does not match length");
    //  return; // skip rest of function
    //}

    Serial.println("Received from: 0x" + String(sender, HEX));
    Serial.println("Sent to: 0x" + String(sender, HEX));
    Serial.println("Message length: " + String(incomingLength));
    Serial.println("Message: " + LoRaData);
    Serial.println("RSSI: " + String(LoRa.packetRssi()));
    Serial.println("Snr: " + String(LoRa.packetSnr()));
    Serial.println();

    // Display information
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("LORA RECEIVER");
    display.setCursor(0, 20);
    display.print("Received packet:");
    display.setCursor(0, 30);
    display.print(LoRaData);
    display.setCursor(0, 40);
    display.print("RSSI:");
    display.setCursor(30, 40);
    display.print(LoRa.packetRssi());
    display.display();
  }
}
