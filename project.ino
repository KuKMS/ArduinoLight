#include <string.h>
#include <dht11.h>
#include <SoftwareSerial.h>
#include "WiFiEsp.h"
#include "SPI.h"

SoftwareSerial BTSerial(11,10); //Bluetooth module tx:11, rx:10
SoftwareSerial WiFiSerial(13,12); //Wifi module
dht11 DHT11; 
WiFiEspClient client;
IPAddress hostIp;
int status = WL_IDLE_STATUS;
int pin_DHT11 = 2;
int pin_LED_R = 6;
int pin_LED_G = 5;
int pin_LED_B = 3; //PWM available pins
int incomingByte=0;
int onoff[4] = {0,0,0,0};  // 0: total, 1: Red ,2: Blue, 3:Green
int BrightR =0;
int BrightG =0;
int BrightB =0;
char ssid[50];
char pass[50];
String line0="";
String line1="";
const char* host = "www.kma.go.kr";
const String url = "/wid/queryDFSRSS.jsp?zone=";
String location ="1159068000";
int count =0;




void processMessage() {
  int dataCount = 0;
  int type = BTSerial.read();
  Serial.println("-------------- TLV Parsing Start ---------------");
  Serial.print("Type : ");
  Serial.print(type);
  Serial.print(" Length : ");
  while(!BTSerial.available());
  int d_length = BTSerial.read();
  Serial.print(d_length);
  Serial.print(" Data : ");
  if(type == 1 || type == 2) {
    char stringData[d_length];
    for(dataCount = 0; dataCount < d_length; dataCount++) {
      while(!BTSerial.available());
      stringData[dataCount] = BTSerial.read();
    }
    if(type==1){strcpy(ssid,stringData);}
    else if(type==2){strcpy(pass,stringData);}
    Serial.print(stringData);
  }
  if(type == 3) {
    while(!BTSerial.available());
      unsigned long post = (unsigned long)BTSerial.read();
      for(dataCount = 0; dataCount < d_length-1; dataCount++) {
        while(!BTSerial.available());
        post = post << 8;
        post += (unsigned long)BTSerial.read();
      }
      location = String(post);
      Serial.print(post);
  }
  if(type == 4 || type == 5 || type == 6 || type == 7) {
    while(!BTSerial.available());
    int power = (int)BTSerial.read();
    if(power) {
      switch(type){
        case 4: onoff[0]=1; break;
        case 5: onoff[1]=1; break;
        case 6: onoff[2]=1; break;
        case 7: onoff[3]=1; break;
        default:;
      }
    } else {
      switch(type){
        case 4: onoff[0]=0; break;
        case 5: onoff[1]=0; break;
        case 6: onoff[2]=0; break;
        case 7: onoff[3]=0; break;
        default:;
      }
    }
    Serial.print(power);
  }
  if(type == 0) {
    Serial.print("This is End Mark. flush BTSerial..");
    delay(50);
    while(BTSerial.available()) {
      BTSerial.read();
    }
  }
  Serial.println("\n-------------- TLV Parsing End ---------------");
}

void dht11Check(){

  int chk = DHT11.read(pin_DHT11);
  Serial.write("Temperature : ");
  if(DHT11.temperature >=10){
    Serial.write((DHT11.temperature/10)+'0');
  };
  Serial.write((DHT11.temperature%10)+'0');
  Serial.write("[C] Humidity : ");
  if(DHT11.humidity >=10){
    Serial.write((DHT11.humidity/10)+'0');
  };
  Serial.write((DHT11.humidity%10)+'0');
  Serial.println("[%]");

  BrightR = (int)(DHT11.temperature*255/50); //0~50 -> 0~255
  BrightB = (int)((DHT11.humidity-20)*255/70); //20~90 -> 0~255

}


void wificonnect(){
  WiFi.init(&WiFiSerial);
  if(WiFi.status() == WL_NO_SHIELD){
    Serial.println("WiFi shield not present");
    return;
  }
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  
}

void getweather(){
  while(client.available()) { 
    // 날씨 정보가 들어오는 동안 
    char c = client.read(); 
    if (c == '<') count++; 
    if (count > 40 && count <= 78) line0 += c; 
    else if (count > 78 && count <= 118) line1 += c; 
    else if (count > 118) break; 
  }
  
}
//서버와 연결
void connectToServer() {
  Serial.println("connecting to server...");
  if (client.connect(host, 80)) {
    Serial.println("Connected! Making HTTP request to www.kma.go.kr");
    
    client.print("GET " + url + location + " HTTP/1.1\r\n" +  
                 "Host: " + host + "\r\n" +  
                 "Connection: close\r\n\r\n");
  }
}

void ledControl()
{
  if(onoff[0]==1){
    if(onoff[1]==1)analogWrite(pin_LED_R,BrightR);
    else analogWrite(pin_LED_R,0);
    if(onoff[2]==1)analogWrite(pin_LED_B,BrightB);
    else analogWrite(pin_LED_B,0);
    if(onoff[3]==1)analogWrite(pin_LED_G,BrightG);
    else analogWrite(pin_LED_G,0);
  }else{
    analogWrite(pin_LED_R,0);
    analogWrite(pin_LED_B,0);
    analogWrite(pin_LED_G,0);
  }
}



void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
  pinMode(9,INPUT);
  strcat(ssid,"");
  strcat(pass,"");
  Serial.println("Start setup");
  while(1){
    if(BTSerial.available()>0){
      processMessage();
      processMessage();
      processMessage();
      processMessage();
    }
    if(strlen(ssid)!=0){
      WiFiSerial.begin(9600);      
      Serial.print("ssid:");
      Serial.println(ssid);
      Serial.print("pw:");
      Serial.println(pass);
      wificonnect();
    }
    if(status==WL_CONNECTED){
      delay(1000);
      break;
    }else{
      strcpy(ssid,"");
      strcpy(pass,"");
      WiFiSerial.end();
    }
  }
  
  connectToServer();
  unsigned int count_time=0;
  int countval=0;
  while(1){
    if (millis() - count_time >= 100) { 
      count_time = millis();
      Serial.print("."); 
      countval++;
    }
    getweather();
    if (count != 0) { // 수신 값 있으면 
      while(client.available()) {  // 나머지 날씨정보 버리기
        char c = client.read(); 
      }
      if (!client.connected()) {   // 날씨정보 수신 종료됐으면
        Serial.println();
        Serial.println("Disconnecting from server...");
        client.stop();
      }
      String temp; 
      int sky_start;
      for(int j=0; j<line0.length();j++){
        if(line0[j] == 's' && line0[j+1] == 'k' && line0[j+2]=='y'){
          sky_start =j;
          break;
        }
      }
      temp = line0.substring(sky_start + 4, sky_start+5);
      Serial.print(F("sky: ")); 
      Serial.println(temp); 
      BrightG = temp.toInt(); // 자료형 변경 String -> float Serial.print(F("temp0: ")); Serial.println(temp0);


      Serial.println("weather data for parsing");
      Serial.println(line0);
      count = 0;
    } 
    if(countval==30) break;
  }
  dht11Check();
  Serial.println("Start Mood Light...");
  BTSerial.end();
  BTSerial.begin(9600);
}


void loop() {
  // When receiving Bluetooth data
  if(BTSerial.available() > 0){
    processMessage();
    dht11Check();
    delay(200);
  }

      
  ledControl();
  delay(500);
}
