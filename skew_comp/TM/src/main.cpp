#include <Arduino.h>
#include "esp_timer.h" // lib for timermanagement
#include <PriUint64.h>// lib to print 64 bit nr
#include "esp_wifi.h"// lib for Wi-Fi
#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_wpa2.h"
#include "data_manipulation.h"
#include "_util.h"

#define maxreferences 100 //max amount of saved references

//debuging
#define settimer 2
#define saveref 0
#define sendref 4

int togglesettimer = LOW;
int togglesaveref = LOW;
int togglesendref = LOW;


/*----------------------Random-----------------------*/

int curChannel = 1; // channel to sniff on
uint8_t MAC_AP[6]={0x78,0xD2,0x94,0x7D,0xDF,0x48}; // mac adres random router in b225 on channel 1

struct referencepoint referencepoints[maxreferences]; // stores the latest 50 referencepoints
int idx=0;   //set index for referencepoints to zero
bool maxref = false;
bool connectedtowifi = false;

/*----------------------declarations-----------------------*/

// declare appstate
static volatile APP_State_t appState = SETNEXTTIMER;

// declare hw timer
hw_timer_t * timer1 = NULL;

//filter for sniffing
const wifi_promiscuous_filter_t filt={
      //filter for management frames: frame control= b 0000 0001 = WIFI_PROMIS_FILTER_MASK_MGMT
    .filter_mask= WIFI_PROMIS_FILTER_MASK_MGMT
};


/*----------------------functions-----------------------*/


// callback for timer 1
void IRAM_ATTR callbacktimer1(){

  appState = SETNEXTTIMER;
  togglesettimer = !togglesettimer;
  digitalWrite(settimer, togglesettimer);// toggle gpio pin
  if(!connectedtowifi){
    //esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, sizeof(beaconPacket) , false); // send beacon frame to slave; uncomment if slave extension is used
  }
  else{
    uint64_t time = timerRead(timer1);
    uint64_t alarm = time + 1000000 - (time%1000000);

    // set next alarm
    if(alarm>timerRead(timer1)){
      timerAlarmWrite(timer1,alarm , false);
      timerAlarmEnable(timer1); // enable timer interrupt
    }
    else{
      timerAlarmWrite(timer1,alarm + 1000000 , false);
      timerAlarmEnable(timer1); // enable timer interrupt
    }
  }
}


// callback for wifi sniffer
void IRAM_ATTR sniffercallback(void* buf, wifi_promiscuous_pkt_type_t type) { //This is where packets end up after they get sniffed
  
  uint64_t time = timerRead(timer1);
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
    wifi_ieee80211_payload *payload = (wifi_ieee80211_payload*)&ipkt->payload; // put payload of frame in structure ieee80211_payload datatype
  
  // check if it is a beacon frame
  if(framecontrol->subtype==8){
      // check if it is the right access point
    for(int i=0;i<6;i++){// compare MAC adresses
      if(MAC_AP[i]!=hdr->addr3[i]){
        break;
      }
    }

    // do stuf if it is the right access point
    uint64_t id;
    //construct id from payload
    id= ((uint64_t)(payload->timestamp[7])<<56)+
        ((uint64_t)(payload->timestamp[6])<<48)+
        ((uint64_t)(payload->timestamp[5])<<40)+
        ((uint64_t)(payload->timestamp[4])<<32)+
        ((uint64_t)(payload->timestamp[3])<<24)+
        ((uint64_t)(payload->timestamp[2])<<16)+
        ((uint64_t)(payload->timestamp[1])<<8)+
        ((uint64_t)(payload->timestamp[0]));

    if(!maxref){
      //save timestamp and id in struct
      referencepoints[idx].timestamp=time;
      referencepoints[idx].id=id;
      idx++;
      if(idx > maxreferences-1){
        maxref = true;
        appState = SENDREFERENCES;
      }
    }
  }
}

void setup() {
  pinMode(settimer, OUTPUT);
  pinMode(saveref, OUTPUT);
  pinMode(sendref, OUTPUT);

  //initialize timer
  timer1 = timerBegin(0, 80, true); // timer_id = 0; prescaler=80 => 1 tick = 1 µs; countUp = true;
  timerAttachInterrupt(timer1, &callbacktimer1, true); // atatch callback to timer1

   //start Serial
  Serial.begin(115200);
  Serial.println("TM1");

   //initialize Wi-Fi
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_STA);
  esp_wifi_start();

   // set sniffing parameters
  esp_wifi_set_promiscuous(true); // enable promiscuous mode
  esp_wifi_set_promiscuous_filter(&filt); // set filter
  esp_wifi_set_promiscuous_rx_cb(&sniffercallback); // atatch callback function
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);// set Wi-Fi channel
}

void loop() {

switch (appState) {

  case SETNEXTTIMER:{// set the next timer interrupt

    appState = IDLE;
    Serial.println("------------------in set next timer: --------------------");
    // calculate next alarm based on the current value of the timerRead
      uint64_t time = timerRead(timer1);
      uint64_t alarm = time + 1000000 - (time%1000000);

    // set next alarm
    if(alarm>timerRead(timer1)){
      timerAlarmWrite(timer1,alarm , false);
      timerAlarmEnable(timer1); // enable timer interrupt
    }
    else{
      timerAlarmWrite(timer1,alarm + 1000000 , false);
      timerAlarmEnable(timer1); // enable timer interrupt
    }
  }break;

  case SENDREFERENCES:{
  
    esp_wifi_set_promiscuous(false); // disable promiscuous mode
    WiFi.disconnect(true);// delete old config

    Serial.println("--------------------------------in SEND references------------------------------------------");
    connectedtowifi = true;
    int ctr = 0;
    while(true){// send data to slave
      if(sendreferences(referencepoints, maxreferences)){
        break;
      }
      ctr++;
      if(ctr>4){ // stop trying to connect after 4 times
          Serial.println("geen refs gezonden");
          break;
      }
    }
    appState = IDLE;
    //reset index and restart saving references
    idx=0;
    maxref=false;
    // enable sniffing
    esp_wifi_set_promiscuous(true); // enable promiscuous mode
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);// set Wi-Fi channel
    connectedtowifi = false;
  }break;

  case IDLE:{
      
  }break;
};

}
