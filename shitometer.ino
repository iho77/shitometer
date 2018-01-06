#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
#include "SSD1306.h" 
#include "images.h"
#include "QuickStats.h"


#define TRIG_PIN 13
#define ECHO_PIN 12
#define ADDR 0x01
#define STATE_PIN 22
#define MAX_DISTANCE 400
#define MEASURE_PERIOD 10000
#define MEASURE_COUNT 11
#define CHANGE_THR 2 
#define PING_PERIOD 1000*60*10

#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI00    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)

#define BAND    433E6  //you can set band here directly,e.g. 868E6,915E6
#define PABOOST true


QuickStats stats; 




typedef struct {
  byte src_addr;
  char msg[250];
} _l_packet;

_l_packet pkt;

float measurements[MEASURE_COUNT];


SSD1306 display(0x3c, 4, 15);

uint8_t src_addr = ADDR;

unsigned long lastmeasuretime, lastping;
volatile int prevdistance, currdistance, measurecount;

void logo()
{
  display.clear();
  display.drawXbm(0,5,logo_width,logo_height,logo_bits);
  display.display();
}


int readDistance() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return lround(pulseIn(ECHO_PIN, HIGH)/58);
}


void initdisplay(){
     pinMode(16,OUTPUT);
     pinMode(25,OUTPUT);
     digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
     delay(50); 
     digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
     display.init();
     display.flipScreenVertically();  
     display.setFont(ArialMT_Plain_10);
     logo();
     delay(1500);
     display.clear();
}


void initlora(){
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI00);
  
  if (!LoRa.begin(BAND,PABOOST))
  {
    messageLog("Starting LoRa failed!");
    while (1);
  }
  messageLog("LoRa Initial success!");
  delay(1000);
}

void setup() {
  // put your setup code here, to run once:
 Serial.begin(115200);

 //init distance sensor
 
 pinMode(TRIG_PIN, OUTPUT);
 pinMode(ECHO_PIN, INPUT);

 
 lastmeasuretime = millis();
 lastping = 0;
 prevdistance = 0;
 currdistance = 0;
 measurecount = 0;


 initdisplay();
 initlora();
 
}

void loop() {
  // put your main code here, to run repeatedly:
  String msg = "";
  
  
  if (millis() - lastping >= PING_PERIOD){

   lastping = millis();  
   msg = "{\"uptime\" :"+String(millis()/1000/60)+"}"; 
   LoRa.beginPacket();
   LoRa.write(&src_addr, 1);
   LoRa.write((uint8_t *)msg.c_str(),msg.length());
   LoRa.endPacket();
   }
  
  if (millis() - lastmeasuretime >= MEASURE_PERIOD){
   lastmeasuretime = millis();  
   measurements[measurecount] = readDistance();
   measurecount ++;
   
  }

  if (measurecount == MEASURE_COUNT ){
    measurecount = 0;
    currdistance = stats.median(measurements, MEASURE_COUNT);
    if ( abs(currdistance-prevdistance) > CHANGE_THR){
    prevdistance = currdistance;
    msg = "{\"dist\" : "+String(currdistance)+"}"; 
    messageLog(msg.c_str());
    LoRa.beginPacket();
    LoRa.write(&src_addr, 1);
    LoRa.write((uint8_t *)msg.c_str(),msg.length());
    LoRa.endPacket();
   }   
  } 
 }




void messageLog(const char *msg){
     display.clear();
     display.setTextAlignment(TEXT_ALIGN_LEFT);
     display.setFont(ArialMT_Plain_10);
     display.drawString(0, 0, msg);
     display.display();
  }



