#include <Arduino.h>
#include "esp_timer.h" // lib for timermanagement
#include <PriUint64.h>// lib to print 64 bit nr
#include "esp_wifi.h"// lib for Wi-Fi
#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_wpa2.h"
#include "AsyncUDP.h"
#include "_util.h"
#include "data_manipulation.h"


//debuging
#define settimer 2
#define saveref 0
#define sendref 4

int togglesettimer = LOW;
int togglesaveref = LOW;
int togglesendref = LOW;

/*----------------------declarations-----------------------*/
// set first appstate
static volatile APP_State_t appState = SETNEXTTIMER;

// declare hw timer
hw_timer_t * timer1 = NULL;

//filter for sniffing
const wifi_promiscuous_filter_t filt={
      //filter for management frames: frame control= b 0000 0001 = WIFI_PROMIS_FILTER_MASK_MGMT
    .filter_mask= WIFI_PROMIS_FILTER_MASK_MGMT
};


/*----------------------Random-----------------------*/
int curChannel = 1; // channel to sniff on
uint8_t MAC_TM1[6] = {0x5C,0x35,0x3B,0xDF,0x1,0x1B}; // channel 1
uint8_t MAC_TM3[6] = {0xE6,0x42,0xA6,0x91,0x46,0xAA}; // channel 1 

bool maxref = false;
bool slavefull = false;

struct referencepoint Myreferencepoints[maxreferences]; // stores the latest 100 referencepoints on AP1
struct referencepoint Slavereferencepoints[maxreferences]; // stores last 100 references for the slave on AP2
struct referencepoint Mastersreferencepoints[maxMasterref]; // for reception of master references

int idx=0;   //set index for referencepoints to zero
int idxs=0;  //set index for slave referencepoints to zero 

bool connectedtowifi = false;

int offsets[maxreferences];

/* create a server and listen on port 8088 */
WiFiServer server(8088);


/*----------------------functions-----------------------*/
// callback for timer 1
void IRAM_ATTR callbacktimer1(){
  if(!connectedtowifi){//send beacon
    //esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, sizeof(beaconPacket) , false); // send beaconframe to slave; uncomment if slave extension is used
  }
  else{
    
  }

  togglesettimer = !togglesettimer;
  digitalWrite(settimer, togglesettimer);//toggle gpio pin

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


// callback for wifi sniffer
void IRAM_ATTR sniffercallback(void* buf, wifi_promiscuous_pkt_type_t type) { //This is where packets end up after they get sniffed

  //take timestamp
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
    bool MAC_AP_master=true;
    bool MAC_AP_slave=true;
    for(int i=0;i<6;i++){// compare MAC adresses
      if(MAC_TM1[i]!=hdr->addr3[i]){
        MAC_AP_master=false;
      }
      if(MAC_TM3[i]!=hdr->addr3[i]){
        MAC_AP_slave=false;
      }
    }

    if(MAC_AP_master){// do stuf if it is the right access point
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
        Myreferencepoints[idx].timestamp=time;

        Myreferencepoints[idx].id=id;
        idx++;
        if(idx > maxreferences-1){
          maxref = true;
          appState = RECEIVEANDSEND;
        }
      }
    }

    
    if(MAC_AP_slave){// do stuf if it is the right access point
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

      if(!slavefull){
        //save timestamp and id in struct
        Slavereferencepoints[idxs].timestamp=time;

        Slavereferencepoints[idxs].id=id;
        idxs++;
        if(idxs > maxreferences-1){
          slavefull = true;
        }
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
  Serial.println("TM2");

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
      Sprintln("------------------in set next timer: --------------------");
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

    case RECEIVEANDSEND:{
      esp_wifi_set_promiscuous(false); // disable promiscuous mode
      Sprintln("--------------------------------in RECEIVE------------------------------------------");
      connectedtowifi = true;
      int ctr = 0;
      while(true){
        if(receiverefs(Mastersreferencepoints, server)){
          break;
        }
        ctr++;
        if(ctr>4){
          Serial.println("NO REFS RECEIVED");
          break;
        }
      }
      int ct = 0;
      while(true){// send data to slave
        if(sendreferences(Slavereferencepoints, maxreferences)){
          break;
        }
        ct++;
        if(ct>4){
          Serial.println("NO REFS SEND TO SLAVE");
          break;
        }
      }
      appState = SYNC;
      connectedtowifi = false;
    }break;

    case SYNC:{
    int n = calculateOffsets(Myreferencepoints, Mastersreferencepoints, offsets);
    if( n > 10){// do stuf if we have 10 or more matching timestamps
      int median = findMedian(offsets,n); // calculate median
      uint64_t time = timerRead(timer1)+median;
      timerWrite(timer1, timerRead(timer1)+median); //adjust timer based on median
      appState = SETNEXTTIMER;
      //enable sniffing
      esp_wifi_set_promiscuous(true); // enable promiscuous mode
      esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);// set Wi-Fi channel
      //reset index and restart saving references
      idx=0;
      maxref=false;
      idxs=0;
      slavefull=false;
    }
    else{// if the timestamps didn't overlap, back to sniffing

      //enable sniffing
      esp_wifi_set_promiscuous(true); // enable promiscuous mode
      esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);// set Wi-Fi channel

      //reset index and restart saving references
      idx=0;
      maxref=false;
      idxs=0;
      slavefull=false;
      appState = SETNEXTTIMER;
    }
    }break;
    case IDLE:{
      delay(10); //time to update buisnisslogic otherwise watchdogtimer goes off
    }break;
  };
}
