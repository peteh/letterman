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

#define INPUT_DOOR 46

SX1262 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, LORA_BUSY);
uint16_t msgCounter = 0;

void setup()
{
  Serial.begin(115200);
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  
  pinMode(INPUT_DOOR, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, false);
  delay(1000);
  digitalWrite(LED_BUILTIN, true);
  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin(LORA_FREQ);
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

void sendLoRaMsg()
{
  Serial.print(F("[SX1262] Transmitting packet ... "));
  // Identifier:uint16, payloadsize:uint16t, payload
  uint8_t buffer[1000];
  //LoRa.beginPacket();
  //LoRa.write((uint8_t*)(&loraIdentifier),sizeof(loraIdentifier));

  StaticJsonDocument<1000> doc;
  // Add values in the document
  //
  doc["d"] = "on";   // off = closed
  doc["m"] = "on"; // off = clear
  doc["c"] = msgCounter;
  uint16_t length = (uint16_t)serializeJson(doc, buffer, sizeof(buffer));
  // write number of bytes for payload
  //LoRa.write((uint8_t*)(&length), sizeof(length));
  int state = radio.transmit(buffer, length);
  msgCounter++;
  // write buffer
  //LoRa.write(buffer, length);
  // send it out
  //LoRa.endPacket();

  if (state == RADIOLIB_ERR_NONE)
  {
    // the packet was successfully transmitted
    Serial.println(F("success!"));

    // print measured data rate
    Serial.print(F("[SX1262] Datarate:\t"));
    Serial.print(radio.getDataRate());
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


void loop()
{
  sendLoRaMsg();
  // wait for a second before transmitting again
  delay(1000);
}