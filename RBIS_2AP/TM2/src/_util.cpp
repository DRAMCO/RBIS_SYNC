#include "_util.h"

// for wifi connection:
const char* ssid = "ssid-of-network"; // adjust to correct network
const char* pwd = "********"; // adjust to correct pwd


//This is the IP address of TM3
const char* host = "192.168.0.142"; 
const int port = 8088;

void converttobytes(uint64_t* input, uint8_t* res){ // convert uint64_t to array of bytes
  uint64_t temp = ((*input >> 32) << 32); //shift it right then left 32 bits, which zeroes the lower half of the long
  uint32_t LSB = (uint32_t)(*input - temp);
  uint32_t MSB = (uint32_t)(*input >> 32);

  res[0] = (MSB >> 24) & 0xFF;
  res[1] = (MSB >> 16) & 0xFF;
  res[2] = (MSB >> 8) & 0xFF;
  res[3] = MSB & 0xFF;
  res[4] = (LSB >> 24) & 0xFF;
  res[5] = (LSB >> 16) & 0xFF;
  res[6] = (LSB >> 8) & 0xFF;
  res[7] = LSB & 0xFF;

}

int calculateOffsets(struct referencepoint Myreferencepoints[], struct referencepoint Mastersreferencepoints[], int offsets[]){
  int idxo = 0;
  for(int i=0; i<maxreferences; i++){
    for(int j=0; j<maxMasterref; j++){
      if(Myreferencepoints[i].id == Mastersreferencepoints[j].id){
        offsets[idxo] = (int) Mastersreferencepoints[j].timestamp - Myreferencepoints[i].timestamp;
        idxo++;
        break;
      }
    }
  }
  for(int i=0;i<idxo;i++){
    Sprintln(offsets[i]);
  } 
  return idxo;
}

bool receiverefs(referencepoint receivedrefs[], WiFiServer server){
  int length = 0;
  WiFi.disconnect(true);// delete old config
  WiFi.begin(ssid,pwd); //connect to wifi
  Sprintln("Waiting for WIFI connection...");
  int counter=0;
  bool received = false;
  uint8_t data[maxMasterref*8*2];// array for received data
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Sprint(".");
    counter++;
    if(counter>=10){ //after 5 seconds timeout - retry connection
      break;
    }
  }

  if(WiFi.status() == WL_CONNECTED){
    Sprint("connected with IP: ");
    Sprintln(WiFi.localIP());
    server.begin();
  }else{
    WiFi.disconnect();
    return false;
  }

  int ctr=0;
  Sprintln("waiting for client");
  while(!received){ 
    WiFiClient client = server.available();  
    if (client) {                   
      Sprintln("new client");         
      /* check client is connected */            
      while (client.connected() && !received) {     
        if (client.available()) {
          length = client.read(data, maxMasterref*2*8);
          received=true;
        } 
      }
    }
    ctr++;
    delay(500);
    Sprint(".");
    if(ctr>10){
      Sprintln("No client found");
      break;
    }
  }
  if(received){// if data is received, parse it into a struct
    for(int i=0;i<length/16;i++){// go through id-timestamp pairs
      receivedrefs[i].id= ((uint64_t)(data[0+16*i])<<56)+
                          ((uint64_t)(data[1+16*i])<<48)+
                          ((uint64_t)(data[2+16*i])<<40)+
                          ((uint64_t)(data[3+16*i])<<32)+
                          ((uint64_t)(data[4+16*i])<<24)+
                          ((uint64_t)(data[5+16*i])<<16)+
                          ((uint64_t)(data[6+16*i])<<8)+
                          ((uint64_t)(data[7+16*i]));
      receivedrefs[i].timestamp=  ((uint64_t)(data[0+16*i+8])<<56)+
                                  ((uint64_t)(data[1+16*i+8])<<48)+
                                  ((uint64_t)(data[2+16*i+8])<<40)+
                                  ((uint64_t)(data[3+16*i+8])<<32)+
                                  ((uint64_t)(data[4+16*i+8])<<24)+
                                  ((uint64_t)(data[5+16*i+8])<<16)+
                                  ((uint64_t)(data[6+16*i+8])<<8)+
                                  ((uint64_t)(data[7+16*i+8]));
    }
    WiFi.disconnect(); // disconnect from network
    return true;
  }else{
    // if transmission failed, retry
    Sprintln("reception failed");
    WiFi.disconnect(); // disconnect from network
    return false;
  }
}


bool sendreferences(referencepoint refs[], int length){
  WiFi.disconnect(true);
  WiFi.begin(ssid,pwd); // no arguments for enterprise wifi
  int counter = 0;
  Serial.println("connecting to network");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      counter++;
      if(counter>=10){ //after 5 seconds timeout - reset board
          Serial.println("connection failed");
          break;
      }
  }

  if(WiFi.status() == WL_CONNECTED){
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address set: ");
      Serial.println(WiFi.localIP()); //print LAN IP
      Serial.print("connecting to ");
      Serial.println(host);
      WiFiClient client;
      if (!client.connect(host,port)) {
          Serial.println("Connection to esp failed");
          delay(1000);
          client.stop();
          return false;
      }else{
          Serial.println("Connected to esp successful!");
          for(int i=0;i<length;i++){// send references
            uint8_t id_bytes[8];
            uint8_t timestamp_bytes[8];

            converttobytes(&refs[i].id, id_bytes); // convert id to bytearray
            converttobytes(&refs[i].timestamp, timestamp_bytes); // convert timestamp to bytearray
            
            client.write(id_bytes,8); //send id
            client.write(timestamp_bytes,8); // send timestamp
          }
          Serial.println("packet send");
          Serial.println("Disconnecting...");
          client.stop();
          WiFi.disconnect();
          return true;
      }
  }else{
    WiFi.disconnect();
    return false;
  }
}