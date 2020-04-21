#ifndef _UTIL_H // must be unique name in the project
#define _UTIL_H

#include <Arduino.h>
#include "esp_wifi.h"// lib for Wi-Fi
#include <WiFi.h>

const wifi_promiscuous_filter_t filt={
    .filter_mask= WIFI_PROMIS_FILTER_MASK_MGMT     //filter for management frames: frame control= b 0000 0001 = WIFI_PROMIS_FILTER_MASK_MGMT
};


/*
--------------------------------------------structure of 802.11 beaconframe-------------------------------------------------------------
frame (struct: wifi_ieee80211_packet_t) = MAC-header (struct: wifi_ieee80211_mac_hdr_t) + payload (struct: wifi_ieee80211_payload)

MAC-header = framectrl (struct: wifi_ieee80211_frame_ctrl_t) + .....
----------------------------------------------------------------------------------------------------------------------------------------
*/
typedef struct { // new structure for frame control bytes
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

typedef struct { // new structure for payload of 802.11 frame
    uint8_t timestamp[8]; // timestamp
    uint8_t beaconinterval[2]; // beacon interval
    uint8_t capabilityinfo[2]; // capability info
    uint8_t lengthssid[2];
    unsigned char rest[]; // payload met eerste bytes=ssid (ssid heeft variabele lengte!!)
}__attribute__((packed)) wifi_ieee80211_payload;

typedef struct { // new structure for mac header
  wifi_ieee80211_frame_ctrl_t frame_ctrl; // frame control
  int16_t duration;
  uint8_t addr1[6]; // receiver address
  uint8_t addr2[6]; // sender address
  uint8_t addr3[6]; // filtering address
  int16_t seqctl; // sequence control
  unsigned char payload[]; // payload
} __attribute__((packed)) wifi_ieee80211_mac_hdr_t;

typedef struct { // new structure for ieee 802.11 frame
  wifi_ieee80211_mac_hdr_t hdr; // mac header
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
}__attribute__((packed)) wifi_ieee80211_packet_t;

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);

const uint8_t MAC_master[6]={0x01, 0x02, 0x03, 0x04, 0x05, 0x01}; // adjust address to match the master 

#endif 
