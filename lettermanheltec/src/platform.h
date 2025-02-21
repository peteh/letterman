#pragma once

#ifdef ARDUINO_heltec_wifi_lora_32_V3

#define LORA_MOSI MOSI
#define LORA_MISO MISO
#define LORA_SCK SCK

#define LORA_CS SS
#define LORA_IRQ DIO0
#define LORA_RST RST_LoRa
#define LORA_BUSY BUSY_LoRa
#define LORA_FREQ 868.

#define LED LED
// letterbox door, we want to wire it with pull up resistor to have HIGH when door is open (switch open)
#define INPUT_DOOR 7

// GPIO 2
#define INPUT_MOTION 6

#define INPUT_VIBRATION 5
#endif

#ifdef ARDUINO_XIAO_ESP32S3

#define LORA_MOSI MOSI
#define LORA_MISO MISO
#define LORA_SCK SCK

#define LORA_CS SS
#define LORA_IRQ D0
#define LORA_RST D2
#define LORA_BUSY D1
#define LORA_FREQ 868.

static const uint8_t LED = LED_BUILTIN;
// letterbox door, we want to wire it with pull up resistor to have HIGH when door is open (switch open)
static const uint8_t INPUT_DOOR = D6;

// GPIO 2
static const uint8_t INPUT_MOTION = D5;

static const uint8_t INPUT_VIBRATION = D7;
#endif