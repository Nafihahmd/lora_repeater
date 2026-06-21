#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

#include "stm32g0xx_hal.h"
#include "protocol.h"


// ------------------------------------------------------------
// SX1262 wiring
// ------------------------------------------------------------
// SPI1
static const uint8_t LORA_SCK  = PB3;
static const uint8_t LORA_MISO = PB4;
static const uint8_t LORA_MOSI = PB5;

// LoRa Repeater Pin Definitions
static const uint8_t LORA_NSS = PB8;
static const uint8_t LORA_BUSY = PC6;
static const uint8_t LORA_DIO1 = PA15;
static const uint8_t LORA_RST = PB7;

// LoRa Actuator Pin Definitions
// static const uint8_t LORA_NSS = PB8;
// static const uint8_t LORA_BUSY = PA8;
// static const uint8_t LORA_DIO1 = PA0;
// static const uint8_t LORA_RST = PB7;

// LoRa Controller Pin Definitions
// static const uint8_t LORA_NSS  = PB0;
// static const uint8_t LORA_BUSY = PB1;
// static const uint8_t LORA_DIO1 = PA0;   // Shared with Wake Pin
// static const uint8_t LORA_RST  = PB7;

// GPIO Pin Definitions
static const uint8_t ACT_LED = PC14;
static const uint8_t TST_LED = PC15;

// Create LoRa instance using RadioLib (CS, IRQ/DIO1, RST, BUSY)
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);


// ------------------------------------------------------------
// Global Variables
// ------------------------------------------------------------

// ------------------------------------------------------------
// Receive flag
// ------------------------------------------------------------
volatile bool receivedFlag = false;

void setFlag(void)
{
    receivedFlag = true;
}

static void restartRx()
{
    radio.startReceive();
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup()
{
    pinMode(ACT_LED, OUTPUT);
    digitalWrite(ACT_LED, LOW);
    pinMode(TST_LED, OUTPUT);
    digitalWrite(TST_LED, LOW);
    pinMode(LORA_DIO1, INPUT_PULLDOWN);

    Serial.begin(115200);

    Serial.println();
    Serial.println("Repeater starting ");
    SPI.setMOSI(LORA_MOSI);
    SPI.setMISO(LORA_MISO);
    SPI.setSCLK(LORA_SCK);
    SPI.begin();

    ConfigLoRa_t config;
    config.frequency = 865.0;

    int state = radio.begin(865.0, 125.0, 10, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 22, 180, 0, false);

    if(state != RADIOLIB_ERR_NONE)
    {
        Serial.print("Radio init failed: ");
        Serial.println(state);

        while(1)
        {
            delay(1000);
        }
    }

    Serial.println("Radio init OK");

    radio.setPacketReceivedAction(setFlag);

    state = radio.startReceiveDutyCycleAuto();

    if(state != RADIOLIB_ERR_NONE)
    {
        Serial.print("startReceive failed: ");
        Serial.println(state);

        while(1)
        {
            delay(1000);
        }
    }
    
    restartRx();

    Serial.println("Repeater started");
}

// ------------------------------------------------------------
// Loop
// ------------------------------------------------------------
void loop()
{
    if(!receivedFlag)
    {
        return;
    }

    receivedFlag = false;

    uint8_t rxBuf[32];

    int packetLength =
        radio.getPacketLength();

    int state =
        radio.readData(
            rxBuf,
            packetLength);

    if(state != RADIOLIB_ERR_NONE)
    {
        restartRx();
        return;
    }

    if(packetLength < sizeof(ClearHeader))
    {
        restartRx();
        return;
    }

    ClearHeader* hdr =
        (ClearHeader*)rxBuf;

    if(hdr->netId != PROTO_NET_ID)
    {
        restartRx();
        return;
    }

    if(hdr->ttl == 0)
    {
        restartRx();
        return;
    }

    hdr->ttl--;

    Serial.printf(
        "Repeating packet, TTL=%u\n",
        hdr->ttl);

    state =
        radio.transmit(
            rxBuf,
            packetLength);

    if(state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(
            "TX error %d\n",
            state);
    }

    restartRx();
}