#include "_util.h"
// for wifi connection:
const char* ssid = "ssid-of-network"; // adjust to correct network
const char* pwd = "**********"; // adjust to correct pwd

// ip adres of TM2
const char* host = "192.168.0.135";
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