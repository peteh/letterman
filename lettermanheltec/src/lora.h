#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include "platform.h"

class LoraSender
{
public:
    LoraSender()
        : m_radio(new Module(LORA_CS, LORA_IRQ, LORA_RST, LORA_BUSY))
    {
        ;
    }
    void begin();
    void sendMessage(bool doorOpen, bool motionDetected, bool newMail);

private:
    SX1262 m_radio;
    uint16_t m_msgCounter = 0;
};

void LoraSender::begin()
{
    // initialize SX1262 with default settings
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    Serial.print(F("[SX1262] Initializing ... "));
    int state = m_radio.begin(LORA_FREQ);
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

void LoraSender::sendMessage(bool doorOpen, bool motionDetected, bool newMail)
{
  Serial.print(F("[SX1262] Transmitting packet ... "));
  // Identifier:uint16, payloadsize:uint16t, payload
  uint8_t buffer[1000];
  // LoRa.beginPacket();
  // LoRa.write((uint8_t*)(&loraIdentifier),sizeof(loraIdentifier));

  StaticJsonDocument<1000> doc;
  // Add values in the document
  //
  doc["door"] = (doorOpen ? "open" : "closed"); // off = closed
  doc["motion"] = (motionDetected) ? "on" : "off";  // off = clear
  doc["newmail"] = (newMail) ? "on" : "off";  // off = clear
  doc["c"] = m_msgCounter;
  uint16_t length = (uint16_t)serializeJson(doc, buffer, sizeof(buffer));
  // write number of bytes for payload
  // LoRa.write((uint8_t*)(&length), sizeof(length));
  int state = m_radio.transmit(buffer, length);
  m_msgCounter++;
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
    Serial.print(m_radio.getDataRate());
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