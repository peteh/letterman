/*
   RadioLib SX127x Receive Example
   This example listens for LoRa transmissions using SX127x Lora modules.
   To successfully receive data, the following settings have to be the same
   on both transmitter and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word
    - preamble length
   Other modules from SX127x/RFM9x family can also be used.
   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---lora-modem
   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/
#include <RadioLib.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <MqttDevice.h>
#include "utils.h"
#include "config.h"

#define LORA_FREQ 868.0

U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2 = nullptr;

SX1276 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST);

WiFiClient net;
PubSubClient client(net);
const char *HOMEASSISTANT_STATUS_TOPIC = "homeassistant/status";
const char *HOMEASSISTANT_STATUS_TOPIC_ALT = "ha/status";

MqttDevice mqttDevice(composeClientID().c_str(), "Letterman", "Letterman-Lora", "maker_pt");

MqttBinarySensor mqttNewMailSensor(&mqttDevice, "letterman_new_mail", "Mailbox New Mail");
MqttBinarySensor mqttDoorSensor(&mqttDevice, "letterman_door", "Mailbox Door");
MqttBinarySensor mqttMotionSensor(&mqttDevice, "letterman_motion", "Mailbox Motion");
MqttBinarySensor mqttVibrationSensor(&mqttDevice, "letterman_vibration", "Mailbox Vibration");

bool g_newMail = false;
bool g_sensorMotionDetected = false;
bool g_sensorDoorOpen = false;
bool g_sensorVibrationDetected = false;

// flag to indicate that a packet was received
volatile bool g_receivedFlag = false;

// disable interrupt when it's not needed
volatile bool g_enableInterrupt = true;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void)
{
  // check if the interrupt is enabled
  if (!g_enableInterrupt)
  {
    return;
  }

  // we got a packet, set the flag
  g_receivedFlag = true;
}

void publishConfig(MqttEntity *entity)
{
  String payload = entity->getHomeAssistantConfigPayload();
  char topic[255];
  entity->getHomeAssistantConfigTopic(topic, sizeof(topic));
  client.publish(topic, payload.c_str());

  entity->getHomeAssistantConfigTopicAlt(topic, sizeof(topic));
  client.publish(topic,
                 payload.c_str());
}

void publishConfig()
{
  publishConfig(&mqttNewMailSensor);
  publishConfig(&mqttDoorSensor);
  publishConfig(&mqttMotionSensor);
  publishConfig(&mqttVibrationSensor);
}



void publishNewMailSensor()
{
  client.publish(mqttNewMailSensor.getStateTopic(), (g_newMail ? mqttNewMailSensor.getOnState() : mqttNewMailSensor.getOffState()));
}

void publishDoorSensor()
{
  client.publish(mqttDoorSensor.getStateTopic(), (g_sensorDoorOpen ? mqttDoorSensor.getOnState() : mqttDoorSensor.getOffState()));
}

void publishMotionSensor()
{
  client.publish(mqttMotionSensor.getStateTopic(), (g_sensorMotionDetected ? mqttMotionSensor.getOnState() : mqttMotionSensor.getOffState()));
}

void publishVibrationSensor()
{
  client.publish(mqttVibrationSensor.getStateTopic(), (g_sensorVibrationDetected ? mqttVibrationSensor.getOnState() : mqttVibrationSensor.getOffState()));
}

void publishSensors()
{
  publishNewMailSensor();
  publishDoorSensor();
  publishMotionSensor();
  publishVibrationSensor();
}

void connectToMqtt()
{
  log_i("connecting to MQTT...");
  // TODO: add security settings back to mqtt
  // while (!client.connect(mqtt_client, mqtt_user, mqtt_pass))
  for (int i = 0; i < 3 && !client.connect(composeClientID().c_str()); i++)
  {
    Serial.print(".");
    delay(3000);
  }
  client.subscribe(HOMEASSISTANT_STATUS_TOPIC);
  client.subscribe(HOMEASSISTANT_STATUS_TOPIC_ALT);

  publishConfig();
  delay(200);
  publishSensors();
}

void connectToWifi()
{
  log_i("Connecting to wifi...");
  WiFi.begin(wifi_ssid, wifi_pass);
  // TODO: really forever? What if we want to go back to autoconnect?
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  log_i("\n Wifi connected!");
}

void initBoard()
{
  Serial.begin(115200);
  Serial.println("initBoard");

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  Serial.println("SPI init done");
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println("Oled init done");

  /**
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, HIGH); delay(20);
  digitalWrite(OLED_RST, LOW);  delay(20);
  digitalWrite(OLED_RST, HIGH); delay(20);

  Serial.println("Oled RST done");
  */
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0)
  {
    Serial.println("Started OLED");
    u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
    u8g2->begin();
    u8g2->clearBuffer();
    u8g2->setFlipMode(0);
    u8g2->setFontMode(1); // Transparent
    u8g2->setDrawColor(1);
    u8g2->setFontDirection(0);
    u8g2->firstPage();
    do
    {
      u8g2->setFont(u8g2_font_inb19_mr);
      u8g2->drawStr(0, 30, "LilyGo");
      u8g2->drawHLine(2, 35, 47);
      u8g2->drawHLine(3, 36, 47);
      u8g2->drawVLine(45, 32, 12);
      u8g2->drawVLine(46, 33, 12);
      u8g2->setFont(u8g2_font_inb19_mf);
      u8g2->drawStr(58, 60, "LoRa");
    } while (u8g2->nextPage());
    u8g2->sendBuffer();
    u8g2->setFont(u8g2_font_fur11_tf);
    delay(500);
  }
  Serial.println("Display logo done");

  if (u8g2)
  {
    u8g2->clearBuffer();
    do
    {
      u8g2->setCursor(0, 16);
      u8g2->println("Waiting to receive data");
      ;
    } while (u8g2->nextPage());
  }
  Serial.println("Initboard done");
}

void callback(char *topic, byte *payload, unsigned int length)
{
  log_d("Mqtt msg arrived [%s]", topic);
  

  // publish config when homeassistant comes online and needs the configuration again
  if (strcmp(topic, HOMEASSISTANT_STATUS_TOPIC) == 0 ||
           strcmp(topic, HOMEASSISTANT_STATUS_TOPIC_ALT) == 0)
  {
    if (strncmp((char *)payload, "online", length) == 0)
    {
      publishConfig();
      delay(200);
      publishSensors();
    }
  }
}

void setup()
{
  mqttNewMailSensor.setIcon("mdi:mail");
  mqttDoorSensor.setDeviceClass("door");
  mqttMotionSensor.setDeviceClass("motion");
  mqttVibrationSensor.setDeviceClass("vibration");
  initBoard();
  // When the power is turned on, a delay is required.
  delay(1500);
  int state = radio.begin(LORA_FREQ);
  if (u8g2)
  {
    if (state != RADIOLIB_ERR_NONE)
    {
      u8g2->clearBuffer();
      u8g2->drawStr(0, 12, "Initializing: FAIL!");
      u8g2->sendBuffer();
    }
  }

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
  // set the function that will be called
  // when new packet is received
  radio.setDio0Action(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[SX1278] Starting to listen ... "));
  state = radio.startReceive();
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

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // radio.standby()
  // radio.sleep()
  // radio.transmit();
  // radio.receive();
  // radio.readData();
  // radio.scanChannel();

  WiFi.mode(WIFI_STA);
  WiFi.hostname(composeClientID().c_str());
  WiFi.setAutoConnect(true);
  WiFi.begin(wifi_ssid, wifi_pass);

  connectToWifi();
  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    log_i("Start updating %s", type); });

  ArduinoOTA.onEnd([]()
                   { log_i("End Update"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
    log_e("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      log_e("Arduino OTA: Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      log_e("Arduino OTA: Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      log_e("Arduino OTA: Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      log_e("Arduino OTA: Receive Failed");
    } else if (error == OTA_END_ERROR) {
      log_e("Arduino OTA: End Failed");
    } });

  ArduinoOTA.begin();

  log_i("Connected to SSID: %s", wifi_ssid);
  //log_i("IP address: %s", WiFi.localIP());

  client.setBufferSize(512);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

bool processIncomingLora()
{
  if (!g_receivedFlag)
  {
    return false;
  }
  uint8_t buffer[1000];
  // disable the interrupt service routine while
  // processing the data
  g_enableInterrupt = false;

  // reset flag
  g_receivedFlag = false;

  // you can read received data as an Arduino String
  uint16_t length = radio.getPacketLength();
  int16_t state = radio.readData(buffer, sizeof(buffer));

  bool success = false;

  if (state == RADIOLIB_ERR_NONE)
  {
    // packet was successfully received
    Serial.println(F("[SX1278] Received packet!"));

    // TODO error handling and sanity checking: 
    // header + defined length
    // print data of the packet
    //Serial.print(F("[SX1278] Data:\t\t"));
    //Serial.println(str);

    //g_newMail = strcmp(doc["newmail"], "on") == 0;
    if (length != 4)
    {
      Serial.println(F("[SX1278] Length error!"));
    }
    else if (buffer[0] != 'l' || buffer[1] != 'm')
    {
      Serial.println(F("[SX1278] Header error!"));
    }
    else
    {
      uint8_t status = buffer[2];

      g_sensorDoorOpen = (status >> 0) & 1;
      g_sensorMotionDetected = (status >> 1) & 1;
      g_sensorVibrationDetected = (status >> 2) & 1;
      success = true;

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("[SX1278] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("[SX1278] SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));

      // print frequency error
      Serial.print(F("[SX1278] Frequency error:\t"));
      Serial.print(radio.getFrequencyError());
      Serial.println(F(" Hz"));

      if (u8g2)
      {
        u8g2->clearBuffer();
        char buf[256];
        u8g2->drawStr(0, 12, "Received OK!");
        snprintf(buf, sizeof(buf), "d:%d m:%d v:%d", g_sensorDoorOpen, g_sensorMotionDetected, g_sensorVibrationDetected);
        u8g2->drawStr(5, 26, buf);
        snprintf(buf, sizeof(buf), "RSSI:%.2f", radio.getRSSI());
        u8g2->drawStr(0, 40, buf);
        snprintf(buf, sizeof(buf), "SNR:%.2f", radio.getSNR());
        u8g2->drawStr(0, 54, buf);
        u8g2->sendBuffer();
      }
    }
  }
  else if (state == RADIOLIB_ERR_CRC_MISMATCH)
  {
    // packet was received, but is malformed
    Serial.println(F("[SX1278] CRC error!"));
  }
  else
  {
    // some other error occurred
    Serial.print(F("[SX1278] Failed, code "));
    Serial.println(state);
  }

  // put module back to listen mode
  radio.startReceive();

  // we're ready to receive more packets,
  // enable interrupt service routine
  g_enableInterrupt = true;
  return success;
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    log_w("WiFi not connected, trying to reconnect, state: %d", WiFi.status());
    WiFi.reconnect();
  }

  if (!client.connected())
  {
    log_w("Mqtt not connected, trying to reconnect");
    connectToMqtt();
  }
  client.loop();
  ArduinoOTA.handle();
  if(processIncomingLora())
  {
    publishSensors();
  }
}