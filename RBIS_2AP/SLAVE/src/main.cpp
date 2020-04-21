#include <Arduino.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include "_util.h"

#define togglepin 2
//for debugging
#define timerpin 4
#define snifpin 0
#define oneshotpin 5

volatile int toggle = LOW;
volatile int timertoggle = LOW;
volatile int snifftoggle = LOW;
volatile int oneshottoggle = LOW;

volatile bool timer = false;
volatile bool toggled = false;
bool toggledThisSec = false; 

// declare hw timers
hw_timer_t * timer1 = NULL;
hw_timer_t * oneshot = NULL;

void IRAM_ATTR callbacktimer1(){
    toggled = true;

    // debug pin
    timertoggle = !timertoggle;
    digitalWrite(timerpin, timertoggle);
}

void IRAM_ATTR callbackOneshot(){
  toggled = false;
  toggledThisSec = false;

  // debug pin
  oneshottoggle = !oneshottoggle;
  digitalWrite(oneshotpin, oneshottoggle);
}

void IRAM_ATTR sniffer(void* buf, wifi_promiscuous_pkt_type_t type) { //This is where packets end up after they get sniffed

  /*
  data is passed to buf
  buf contains radio meta data (wifi_pkt_rx_ctrl_trx_ctrl) and the payload
  radio meta data can be accessed by pkt->rx_ctrl.member
  payload is parsed to multiple data structures
  */

  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf; // move data from buffer to wifi_promiscuous_pkt_t data type
  wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)pkt->payload; // put packet in wifi_ieee80211_packet_t data type
  wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr; // put mac header in wifi_ieee80211_mac_hdr_t datatype
  wifi_ieee80211_frame_ctrl_t *framecontrol = &hdr->frame_ctrl; // put  2 framecontrol bytes into framecontrol datatype

  if(framecontrol->subtype==8){
    for(int i=0;i<6;i++){// compare MAC address against address of masteresp32
      if(MAC_master[i]!=hdr->addr3[i]){
        return;
      }
    }
    toggled = true;

    //debug pin
    snifftoggle = !snifftoggle;
    digitalWrite(snifpin, snifftoggle);

    // reset periodic timer
    timerRestart(timer1);
    timerAlarmWrite(timer1, 1000000, true);
    timerAlarmEnable(timer1);  
  }
}


void setup() {
  //start Serial 
  Serial.begin(115200);

  //setup wifi 
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // pins for toggling
  pinMode(togglepin,OUTPUT);
  pinMode(timerpin,OUTPUT);
  pinMode(snifpin,OUTPUT);
  pinMode(oneshotpin,OUTPUT);

  //initialize periodic timer
  timer1 = timerBegin(0, 80, true); // timer_id = 0; prescaler=80 => 1 tick = 1 µs; countUp = true;
  timerAttachInterrupt(timer1, &callbacktimer1, true);

  //initialize oneshot timer
  oneshot = timerBegin(1, 80, true); // timer_id = 1; prescaler=80 => 1 tick = 1 µs; countUp = true;
  timerAttachInterrupt(oneshot, &callbackOneshot, true);
}

void loop() {
  if(!toggledThisSec && toggled){
    toggledThisSec = true;
    toggled = false;

    //toggle pin
    toggle = !toggle;
    digitalWrite(togglepin, toggle);

    // set oneshot timer
    timerRestart(oneshot);
    timerAlarmWrite(oneshot, 100000, false);
    timerAlarmEnable(oneshot);  
    //delay(10); // to reset watchdogtimer (for esp32-solo not needed on esp32-wroom)
  }
}