#ifndef _UTIL_H // must be unique name in the project
#define _UTIL_H

#include <Arduino.h>
#include "esp_wifi.h"// lib for Wi-Fi
#include <WiFi.h>

/* define the differente states of the program */
typedef enum app_states{
  SETNEXTTIMER,
  SENDREFERENCES,
  IDLE
} APP_State_t;

//structure for frame control bytes
typedef struct {
    uint16_t version:2;
    uint16_t type:2;
    uint16_t subtype:4;
    uint16_t to_ds:1;
    uint16_t from_ds:1;
    uint16_t mf:1;
    uint16_t retry:1;
    uint16_t pwr:1;
    uint16_t more:1;
    uint16_t w:1;
    uint16_t o:1;
}__attribute__((packed)) wifi_ieee80211_frame_ctrl_t;

//structure for payload of 802.11 frame
typedef struct {
    uint8_t timestamp[8]; // timestamp
    uint8_t beaconinterval[2]; // beacon interval
    uint8_t capabilityinfo[2]; // capability info
    uint8_t lengthssid[2];
    unsigned char rest[]; // payload met eerste bytes=ssid (ssid heeft variabele lengte!!)
}__attribute__((packed)) wifi_ieee80211_payload;

//structure for mac header
typedef struct {
  wifi_ieee80211_frame_ctrl_t frame_ctrl; // frame control
  int16_t duration;
  uint8_t addr1[6]; // receiver address
  uint8_t addr2[6]; // sender address
  uint8_t addr3[6]; // filtering address
  int16_t seqctl; // sequence control
  unsigned char payload[]; // payload
} __attribute__((packed)) wifi_ieee80211_mac_hdr_t;

//structure for ieee 802.11 frame
typedef struct {
  wifi_ieee80211_mac_hdr_t hdr; // mac header
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
}__attribute__((packed)) wifi_ieee80211_packet_t;

//structure to store timestamps+id
struct referencepoint{
    uint64_t timestamp;
    uint64_t id;
};

// beacon frame definition
const uint8_t beaconPacket[109] = {
  /*  0 - 3  */ 0x80, 0x00, 0x00, 0x00, // Type/Subtype: managment beacon frame
  /*  4 - 9  */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
  /* 10 - 15 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x01, // Source
  /* 16 - 21 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x01, // Source

  // Fixed parameters
  /* 22 - 23 */ 0x00, 0x00, // Fragment & sequence number (will be done by the SDK)
  /* 24 - 31 */ 0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // Timestamp
  /* 32 - 33 */ 0xe8, 0x03, // Interval: 0x64, 0x00 => every 100ms - 0xe8, 0x03 => every 1s
  /* 34 - 35 */ 0x31, 0x00, // capabilities Tnformation

  // Tagged parameters

  // SSID parameters
  /* 36 - 37 */ 0x00, 0x20, // Tag: Set SSID length, Tag length: 32
  /* 38 - 69 */ 0x20, 0x20, 0x20, 0x20,
  0x73, 0x79, 0x6e, 0x63,
  0x6e, 0x65, 0x74, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, // SSID

  // Supported Rates
  /* 70 - 71 */ 0x01, 0x08, // Tag: Supported Rates, Tag length: 8
  /* 72 */ 0x82, // 1(B)
  /* 73 */ 0x84, // 2(B)
  /* 74 */ 0x8b, // 5.5(B)
  /* 75 */ 0x96, // 11(B)
  /* 76 */ 0x24, // 18
  /* 77 */ 0x30, // 24
  /* 78 */ 0x48, // 36
  /* 79 */ 0x6c, // 54

  // Current Channel
  /* 80 - 81 */ 0x03, 0x01, // Channel set, length
  /* 82 */      0x01,       // Current Channel 1

  // RSN information
  /*  83 -  84 */ 0x30, 0x18,
  /*  85 -  86 */ 0x01, 0x00,
  /*  87 -  90 */ 0x00, 0x0f, 0xac, 0x02,
  /*  91 -  92 */ 0x02, 0x00,
  /*  93 - 100 */ 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x04, /* changed 0x02(TKIP) to 0x04(CCMP) is default. WPA2 with TKIP not supported by many devices*/
  /* 101 - 102 */ 0x01, 0x00,
  /* 103 - 106 */ 0x00, 0x0f, 0xac, 0x02,
  /* 107 - 108 */ 0x00, 0x00
};



void converttobytes(uint64_t* input, uint8_t* res); // convert uint64_t to array of bytes
bool sendreferences(referencepoint refs[], int length);


#endif 
