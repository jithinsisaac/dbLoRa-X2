/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/

/*******************************************************************************
 * dbLoRa-X2
 * ATMega328 with HopeRF RFM95 or Dorji DRF1276
 * Additions: LM35 at A2 for Temperature Measurement
 * Additions: Voltage Divider circuit at A0 for checking Battery Level
 * By: Prof. Jithin Saji Isaac
 * Don Bosco Institute of Technology, Mumbai
 * *******************************************************************************/

//***SENSOR***
float A0_voltage; //LM35
float tempc; //Temp in Celcius

float A2_voltage; //Voltage at Voltage Divider network
float battery_voltage; //Battery Voltage
//************

#include <lmic.h>

#include <hal/hal.h>

#include <SPI.h>

// This EUI must be in little-endian form. 
// Do not change- CAN BE ANYTHING FOR Loraserver.io
static
const u1_t PROGMEM APPEUI[8] = { 0x, 0x, 0x01, 0x, 0x7, 0x, 0x, 0x70 }; 
void os_getArtEui(u1_t * buf) {
  memcpy_P(buf, APPEUI, 8);
}

// This should also be in little endian format 
// Should be same as in Loraserver.io
// Write LSB first from loraserver.io
static
const u1_t PROGMEM DEVEUI[8] = { 0x, 0x, 0x, 0x, 0x9, 0x8, 0x2, 0x00 }; 
void os_getDevEui(u1_t * buf) {
  memcpy_P(buf, DEVEUI, 8);
}

// This key should be in big endian format 
static
const u1_t PROGMEM APPKEY[16] = { 0x5, 0xB, 0x, 0xA, 0xB, 0x6, 0x5, 0xD, 0x, 0x6, 0x, 0x, 0x, 0x, 0x7, 0x6A };
void os_getDevKey(u1_t * buf) {
  memcpy_P(buf, APPKEY, 16);
}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds
const unsigned TX_INTERVAL = 10;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 9,
  .dio = {
    2,
    6,
    7
  },
};

void onEvent(ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
  case EV_SCAN_TIMEOUT:
    Serial.println(F("EV_SCAN_TIMEOUT"));
    break;
  case EV_BEACON_FOUND:
    Serial.println(F("EV_BEACON_FOUND"));
    break;
  case EV_BEACON_MISSED:
    Serial.println(F("EV_BEACON_MISSED"));
    break;
  case EV_BEACON_TRACKED:
    Serial.println(F("EV_BEACON_TRACKED"));
    break;
  case EV_JOINING:
    Serial.println(F("EV_JOINING"));
    break;
  case EV_JOINED:
    Serial.println(F("EV_JOINED")); {
      u4_t netid = 0;
      devaddr_t devaddr = 0;
      u1_t nwkKey[16];
      u1_t artKey[16];
      LMIC_getSessionKeys( & netid, & devaddr, nwkKey, artKey);
      Serial.print("netid: ");
      Serial.println(netid, DEC);
      Serial.print("devaddr: ");
      Serial.println(devaddr, HEX);
      Serial.print("artKey: ");
      for (int i = 0; i < sizeof(artKey); ++i) {
        Serial.print(artKey[i], HEX);
      }
      Serial.println("");
      Serial.print("nwkKey: ");
      for (int i = 0; i < sizeof(nwkKey); ++i) {
        Serial.print(nwkKey[i], HEX);
      }
      Serial.println("");
    }
    // Disable link check validation (automatically enabled
    // during join, but because slow data rates change max TX
    // size, we don't use it in this example.
    LMIC_setLinkCheckMode(0);
    break;

  case EV_JOIN_FAILED:
    Serial.println(F("EV_JOIN_FAILED"));
    break;
  case EV_REJOIN_FAILED:
    Serial.println(F("EV_REJOIN_FAILED"));
    break;
  case EV_TXCOMPLETE:
    Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
    if (LMIC.txrxFlags & TXRX_ACK)
      Serial.println(F("Received ack"));
    if (LMIC.dataLen) {
      Serial.print(F("Received "));
      Serial.print(LMIC.dataLen);
      Serial.println(F(" bytes of payload"));

      //**************DEVICE CONTROL CODE LINES******************

      if (LMIC.dataLen == 1) {
        char result = LMIC.frame[LMIC.dataBeg + 0];
        if (result == '0') {
          Serial.println("Downlink Payload Data: 0");
          digitalWrite(A3, LOW);
          Serial.println("LED Turned Off!");
        }
        if (result == '1') {
          Serial.println("Downlink Payload Data: 1");
          digitalWrite(A3, HIGH);
          Serial.println("LED Turned On!");
        }
      }
      //***********************************************************
    }
    Serial.println();
    // Schedule next transmission
    os_setTimedCallback( & sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
    break;
  case EV_LOST_TSYNC:
    Serial.println(F("EV_LOST_TSYNC"));
    break;
  case EV_RESET:
    Serial.println(F("EV_RESET"));
    break;
  case EV_RXCOMPLETE:
    // data received in ping slot
    Serial.println(F("EV_RXCOMPLETE"));
    break;
  case EV_LINK_DEAD:
    Serial.println(F("EV_LINK_DEAD"));
    break;
  case EV_LINK_ALIVE:
    Serial.println(F("EV_LINK_ALIVE"));
    break;

  case EV_TXSTART:
    Serial.println(F("EV_TXSTART"));
    break;
  default:
    Serial.print(F("Unknown event: "));
    Serial.println((unsigned) ev);
    break;
  }
}

void do_send(osjob_t * j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    //************SENSOR*******************
    A2_voltage = analogRead(A2); //Reading the value from LM35 sensor
    A2_voltage = (A2_voltage * 330) / 1023;
    tempc = A2_voltage; // Storing value in Degree Celsius
    Serial.print("Temp in DegreeC= ");
    Serial.println(tempc);

    A0_voltage = (analogRead(A0) * 3.3 / 1024);
    Serial.print("A0_voltage: ");
    Serial.println(A0_voltage);

    battery_voltage = ((A0_voltage * 3.2) / (1));
    Serial.print("Battery_Voltage: ");
    Serial.println(battery_voltage);

    uint32_t temp = tempc * 100;
    uint32_t batteryV = battery_voltage * 100;

    byte payload[4];
    payload[0] = highByte(temp);
    payload[1] = lowByte(temp);
    payload[2] = highByte(batteryV);
    payload[3] = lowByte(batteryV);
    //***************************************

    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, (uint8_t * ) payload, sizeof(payload), 0);

    Serial.print("Channel Frequency: ");
    Serial.print(LMIC.freq);
    Serial.println(" MHz");

    Serial.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  Serial.begin(9600);
  Serial.println("LoRaWAN: End node sending data to Network Server via Gateway.");
  Serial.println("Starting Transmission by OTAA Mode.");
  Serial.println("Device EUI, Application EUI & Application Key obtained from Server side (Own/Tata/Senraco).");
  Serial.println("");

  //***SENSOR***
  pinMode(A0, INPUT); //Battery Check Voltage divider
  pinMode(A2, INPUT); //LM35_Working at 3.3V_Should have been 4V+

  //************

  //***DEVICE CONTROL***
  pinMode(A3, OUTPUT); //FOR dbLoRa-X2
  //********************

  // LMIC init
  os_init();

  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Let LMIC compensate for +/- 1% clock error
  LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100); //1%-2% for RFM95, 10% for Dorji DRF1276

  // Start job (sending automatically starts OTAA too)
  do_send( & sendjob);
}

void loop() {
  os_runloop_once();
}
