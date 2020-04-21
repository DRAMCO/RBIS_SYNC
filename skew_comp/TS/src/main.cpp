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
uint8_t MAC_AP[6] = {0x5C,0x35,0x3B,0xDF,0x1,0x1B}; // channel 1

bool maxref = false;
struct referencepoint Myreferencepoints[maxreferences]; // stores the latest 50 referencepoints
struct referencepoint Mastersreferencepoints[maxMasterref];
int idx=0;   //set index for referencepoints to zero

bool connectedtowifi = false;
int offsets[maxreferences];
uint64_t time_off_offset[maxreferences];

int skewfactor = 0;
int skewcounter = 0;

/* create a server and listen on port 8088 */
WiFiServer server(8088);


/*----------------------functions-----------------------*/
// callback for timer 1
void IRAM_ATTR callbacktimer1(){
  if(!connectedtowifi){//send beacon
    esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, sizeof(beaconPacket) , false); // send beaconframe to slave; uncomment if slave extension is used
  }
  else{
    
  }

  togglesettimer = !togglesettimer;
  digitalWrite(settimer, togglesettimer);// toggle gpio pin

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
  // adjust timer for skew
  timerWrite(timer1, timerRead(timer1)+skewfactor);
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
    bool juisteMAC=true;
    for(int i=0;i<6;i++){// compare MAC adresses
      if(MAC_AP[i]!=hdr->addr3[i]){
        juisteMAC=false;
      }
    }
    if(juisteMAC){// do stuf if it is the right access point

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
          appState = RECEIVE;
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
  Serial.println("TS");

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

    case RECEIVE:{
      esp_wifi_set_promiscuous(false); // disable promiscuous mode
      Sprintln("--------------------------------in RECEIVE------------------------------------------");
      connectedtowifi = true;
      int ctr = 0;
      while(true){
        if(receiverefs(Mastersreferencepoints, server)){
          break;
        }
        ctr++;
        if(ctr>5){
          Serial.println("NO REFS RECEIVED");
          break;
        }
      }
      //print refs
      for(int i=0;i<maxreferences;i++){
        Sprint("Master timestamp: ");
        Sprint(PriUint64<DEC>(Mastersreferencepoints[i].timestamp));
        Sprint("   Master id: ");
        Sprint(PriUint64<DEC>(Mastersreferencepoints[i].id));
        Sprint("My timestamp: ");
        Sprint(PriUint64<DEC>(Myreferencepoints[i].timestamp));
        Sprint("   Master id: ");
        Sprintln(PriUint64<DEC>(Myreferencepoints[i].id));
      }
      appState = SYNC;
      connectedtowifi = false;
    }break;

    case SYNC:{
    int n = calculateOffsets(Myreferencepoints, Mastersreferencepoints, offsets, time_off_offset); 
    if( n > 10){// do stuf if we have 10 or more matching timestamps
  
      
      int median = removeOutliers(offsets, n, 3); // remove outliers from offsets

      double m,b,r;
     
     // perform linear regression on offsets and adjust skewfactor
      if(linreg(n, time_off_offset, offsets, &m, &b, &r) == 0 && abs(m)<10){
        skewfactor = round(m);
      }

      // calculate skew that occured since the median offset
      if(n%2 == 0){
        skew = ((timerRead(timer1) - time_off_offset[n/2])/1000000)*m;
      }
      else{
        skew = ((timerRead(timer1) - time_off_offset[(n+1)/2])/1000000)*m;
      }
    
      uint64_t time = timerRead(timer1)+median;
      if(abs(skew) < 200){ // if the skew value is realistic adjust based on skew
        timerWrite(timer1, timerRead(timer1)+median+skew);
      }else{ // if the skew value is not realistic adjust on the media offset only
        timerWrite(timer1, timerRead(timer1)+median);
        Sprintln("no skew for offset");
      }
      
      appState = SETNEXTTIMER;
      //enable sniffing
      esp_wifi_set_promiscuous(true); // enable promiscuous mode
      esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);// set Wi-Fi channel
      //reset index and restart saving references
      idx=0;
      maxref=false;
    }
    else{// if the timestamps didn't overlap, back to sniffing

      //enable sniffing
      esp_wifi_set_promiscuous(true); // enable promiscuous mode
      esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);// set Wi-Fi channel

      //reset index and restart saving references
      idx=0;
      maxref=false;
      appState = SETNEXTTIMER;
    }
    }break;
    case IDLE:{
      delay(10); //time to update buisnisslogic otherwise watchdogtimer goes off
    }break;
  };
}
