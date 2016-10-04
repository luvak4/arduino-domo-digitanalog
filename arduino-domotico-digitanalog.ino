#include <VirtualWire.h>
const int led_pin = 2;
const int receive_pin = 11;
const int transmit_pin = 12;
// max lenght of my message
const int MSG_LEN = 7;
// analogic port
const int sensorPin = A0;
unsigned long millPrec=0;
////////////////////////////////
// setup
////////////////////////////////
void setup() {
  pinMode(led_pin, OUTPUT);    // led set pin
  vw_set_tx_pin(transmit_pin); // radio set tx pin
  vw_set_rx_pin(receive_pin);  // radio set rx pin
  vw_setup(2000);              // radio speed
  vw_rx_start();               // radio rx ON
  pinMode(13,OUTPUT);          // out pin 13
  digitalWrite(13,LOW);        // ... set low
}

////////////////////////////////
// loop
////////////////////////////////
void loop(){
  uint8_t buf[MSG_LEN]={0,0,0,0,0,0,0}; // empty buffer
  uint8_t buflen = MSG_LEN;             // lenght of buffer
  if (vw_get_message(buf, &buflen)){    // received a message?
    digitalWrite(13,HIGH);    
    if (buf[0]==0xAA){
      switch (buf[1]){
      case 0x07:digitalWrite(led_pin,HIGH);break;
      case 0x08:digitalWrite(led_pin,LOW);break;
      case 0x09:vw_rx_stop();delay(1000);txStatoRele();vw_rx_start();break;
      case 0x0A:vw_rx_stop();delay(1000);txAnalogicoA0();vw_rx_start();break;
      }
    }
  }
  if ((millis()-millPrec)>5000){   
    millPrec=millis();
    vw_rx_stop();
    txAnalogicoA0();
    delay(200);
    vw_rx_start();
  }
}

void txStatoRele(){
  char msg[MSG_LEN] = {0xAD,0x01,0,0,0,0,0}; // set buffer base-value
  if (!digitalRead(led_pin)){
    msg[1]=0x02;                   // overwrite 2nd byte
  }
  delay(100);                      // delay
  vw_send((uint8_t *)msg,MSG_LEN); // send to tx-radio
  vw_wait_tx();                    // wait until message is gone
}

void txAnalogicoA0(){
  char temp[3];                    
  char msg[MSG_LEN] = {0xAD,0x03,0,0,0,0,0}; // set buffer base-value
  int sensorValue = analogRead(sensorPin);   // read A0
  itoa(sensorValue, temp, 10);  // int to char-array
  msg[2]=temp[0];               // overwrite 3rd byte
  msg[3]=temp[1];               // overwrite 4th byte
  msg[4]=temp[2];               // overwrite 5th byte
  delay(100);                   // delay
  vw_send((uint8_t *)msg,MSG_LEN); // send to tx radio
  vw_wait_tx();                    // wait until message is gone  
}
