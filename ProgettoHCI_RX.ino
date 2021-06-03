#include <printf.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24_config.h>
#include <RF24.h>


RF24 radio(7,8);
const byte address[6] = "00001"; //indirizzo di questo arduino
const byte addressRX[6] = "00002"; //indirizzo dell'arduino che sta sulla sedia


int stopAlarm = 0;
int buttonOffAlarm;
void setup() {
  // put your setup code here, to run once:
  //RADIO
  radio.begin();
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  pinMode(5, OUTPUT);
  pinMode(6, INPUT);
}
int statoBuzzer;
void loop() {
  radio.startListening();
  buttonOffAlarm = digitalRead(6);
  if(radio.available()){
    radio.read(&statoBuzzer,sizeof(statoBuzzer));
    if(statoBuzzer == 1){
      digitalWrite(5,HIGH);
    }else{
      digitalWrite(5,LOW);
    }
  }
  if(buttonOffAlarm == HIGH){
      statoBuzzer = 0;
      radio.stopListening();
      radio.openWritingPipe(addressRX);
      digitalWrite(5,LOW);
      radio.write(&stopAlarm, sizeof(stopAlarm));
    }
}






/*


#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN

const byte address[6] = "00001";

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
}

int stato = 0;
int statoToggle = 0;
void loop() {  
  //trasmette 0 e 1
  Serial.println(stato);
  radio.write(&stato, sizeof(stato));
  radio.write(&statoToggle, sizeof(statoToggle));
  delay(1000);
  stato = !stato;
}
*/
