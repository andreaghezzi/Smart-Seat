//VERSIONE OK

#include <Ultrasonic.h> //libreria sensore di prossimità
Ultrasonic ultrasonic(5, 4); //definisco i pin per il sensore di prossimità
#include "Countimer.h"//CODICE TIMER: https://github.com/inflop/Countimer

//LIBRERIE RADIO
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8); // CE, CSN CREO L'OGGETTO RADIO

const byte address[6] = "00001"; //INDIRIZZO RICEVENTE
const byte myAddress[6] = "00002"; //INDIRIZZO RICEVENTE

Countimer timer; 

int cm = 0;

int buttonOffAlarm; // variabile che contiene il valore del bottone di spegnimento dell'allarme

int cnt = 3; //numero di volte che si può posticipare il segnale

int buttonPostoneAlarm; //variabile che contiene il valore del bottone per posticipare l'allarme

bool premuto = true; //uso questa variabile per effettuare un effetto toggle

int first = 0; // è la prima volta che si accende o si sta riniziando ad utilizzare dopo lo spegnimento dell'allarme

bool app = true; // uso questa variabile per stoppare il timer quando arriva a 0, altrimenti ricomincia.

int timerValue = 0; //lo uso per fare la differenza tra il timer attuale - quello nuovo

int statoBuzzer = 1;

int stopAlarm = 1;

int alzato = 0; //se 0 vuol dire che sta seduto,altrimenti alzato, la passo all'altro arduino per poter bloccare il buzzer solo se alzato

void setup()
{
  //SETTAGGIO RADIO
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  radio.openReadingPipe(1, myAddress);
  
  Serial.begin(9600);
  Serial.println("--------------------------------------------------------");
  pinMode(2, INPUT);//pulsante per posticipare segnale
  pinMode(6, OUTPUT); //led che si accende quando si è vicini alla sedia
  pinMode(9, OUTPUT); //led che si accende quando si assume una postura "scorretta"
  pinMode(10, INPUT);//pulsante per scegliere il valore del timer
}

void postoneAlarm(){ //quando l'utente è vicino alla sedia
    if(app == false) {
      radio.stopListening();
      delay(300);
      statoBuzzer = 1;
      radio.write(&statoBuzzer, sizeof(statoBuzzer));
      if(buttonPostoneAlarm == HIGH && cnt > 0){ // finche ci sono tentativi di posorre il segnale allora si pospone
        Serial.println("ENTRO E POSTICIPO L'ALLARME");
        statoBuzzer = 0;
        radio.write(&statoBuzzer, sizeof(statoBuzzer));
        timer.setCounter(0, 0, 5, timer.COUNT_DOWN, onComplete);
        // Print current time every 1s on serial port by calling method refreshClock().
        timer.setInterval(refreshClock, 1000);
        timer.start();
        app = true;
        cnt -= 1;
        Serial.print("PUOI POSTICIPARE ANCORA PER: "); Serial.print(cnt);Serial.println(" VOLTE");
      }
      
    }
}
void refreshClock() {
  //Serial.print("Current count time is: ");
}
void onComplete() {
  //Serial.println("Complete!!!");
}
void loop(){
  timer.run();
  int val = digitalRead(10); //valore dello switch per la selezione del tempo
  Serial.print("VALORE DI VAL: ");Serial.println(val);
  buttonPostoneAlarm = digitalRead(2); //valore del sensore touch per posporre il segnale
  buttonOffAlarm = digitalRead(11); //valore del pulsante per spegnere l'allarme
  
  if (first == 0){ // quando viene acceso l'arduino 
     if (val == 1){ //L'utente ha selezionato 30 minuti
      timer.setCounter(0, 0, 30, timer.COUNT_DOWN, onComplete);
      timer.setInterval(refreshClock, 1000);
      premuto = true;
    }else if (val == 0){ //L'utente ha selezionato 1 ora
      timer.setCounter(0, 0, 60, timer.COUNT_DOWN, onComplete);
      timer.setInterval(refreshClock, 1000);
      premuto = false;
    }
    first = 1;
  }
  else{ //è la seconda o successive iterazioni
    if (val == 1 && premuto == false){ //L'utente ha selezionato 30 minuti noi facciamo secondi
      if(timer.getCurrentSeconds() == 0){
        timer.setCounter(0, 0, 30, timer.COUNT_DOWN, onComplete);
        timer.setInterval(refreshClock, 1000);
        premuto = true;
      }else{ //sto cambiando il timer in corso, passo da 1 ora a 30 minuti
        timerValue = timer.getCurrentSeconds() - 30;
        timer.setCounter(0, 0, timerValue, timer.COUNT_DOWN, onComplete);
        timer.setInterval(refreshClock, 1000);
        premuto = true;
      }
    }else if (val == 0 && premuto == true){ //L'utente ha selezionato 1 ora noi facciamo 1 minuto
      if(timer.getCurrentSeconds() == 0){
        timer.setCounter(0, 0, 60, timer.COUNT_DOWN, onComplete);
        timer.setInterval(refreshClock, 1000);
        premuto = false;
      }else{ //sto cambiando il timer in corso, passo da 30 minuit a 1 ora
        Serial.print("PASSO DA 30 a 1 ora");
        timerValue = 60 - timer.getCurrentSeconds();
        timer.setCounter(0, 0,timerValue, timer.COUNT_DOWN, onComplete);
        timer.setInterval(refreshClock, 1000);
        premuto = false;
        Serial.print("ENTRO");
      }
    }    
  }

  cm = ultrasonic.read();
  if (cm <= 60) { //l'utente è vicino alla sedia
      if(cm >10){ // l'utente sta assumendo una posizione scorretta
        digitalWrite(9,HIGH);
      }else if (cm <= 30){ //l'utente sta dritto con la schiena
        digitalWrite(9,LOW);
      }
      digitalWrite(6, HIGH);
      if(timer.getCurrentSeconds() != 0 && app == true) { // finche il timer è diverso da 0
        timer.start();
        Serial.print("getCurrentTime: ");Serial.print(timer.getCurrentTime()); Serial.print(" - Variabile app: "); Serial.println(app);
        delay(1000);
      }else{
        app = false;
        postoneAlarm();
      }
  }else{
    timer.stop();
    digitalWrite(9,LOW);
    digitalWrite(6, LOW);
    radio.startListening();
    if(radio.available()){
      radio.read(&stopAlarm,sizeof(stopAlarm));
      if (stopAlarm == 0) { // se l'allarme suona e l'utente si è alzato dalla sedia allora premo il pulsante per spegnere l'allarme
        timer.stop();
        first = 0;
        app = true; 
        cnt = 3;
      }
    }
  }
}
