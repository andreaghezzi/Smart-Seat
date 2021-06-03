//LIBRERIE RADIO
#include <printf.h>
#include <nRF24L01.h>
#include <RF24_config.h>
#include <RF24.h>

// C++ code
//CODICE TIMER: https://github.com/inflop/Countimer

#include <Ultrasonic.h> //libreria sensore di prossimità
Ultrasonic ultrasonic(5, 4); //definisco i pin per il sensore di prossimità
#include "Countimer.h"

Countimer timer;

RF24 radio(11,3);
const byte address[6] = "00001"; 

int cm = 0;

int buttonOffAlarm; // variabile che contiene il valore del bottone di spegnimento dell'allarme

int cnt = 3; //numero di volte che si può posticipare il segnale

int buttonPostoneAlarm; //variabile che contiene il valore del bottone per posticipare l'allarme

bool premuto = true; //uso questa variabile per effettuare un effetto toggle

int first = 0; // è la prima volta che si accende o si sta riniziando ad utilizzare dopo lo spegnimento dell'allarme

bool app = true; // uso questa variabile per stoppare il timer quando arriva a 0, altrimenti ricomincia.

int timerValue = 0; //lo uso per fare la differenza tra il timer attuale - quello nuovo

int statoBuzzer = 1;

void setup()
{
  //RADIO
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  
  Serial.begin(9600);
  Serial.println("-----------------------------------------");
  pinMode(2, INPUT);//pulsante per posticipare segnale
  //pinMode(6, OUTPUT); //segnale per fine tempo 
  pinMode(6, OUTPUT); //led che si accende quando si è vicini alla sedia
  //pinMode(8, OUTPUT);//buzzer
  pinMode(9, OUTPUT); //led che si accende quando si assume una postura "scorretta"
  pinMode(10, INPUT);//pulsante per scegliere il valore del timer
  //pinMode(11, INPUT); //bottone per spegnere led quando ci si alza
}

void postoneAlarm(){ //quando l'utente è vicino alla sedia
    if(app == false) {
      digitalWrite(6, HIGH);  
      //tone(8,100,500); //suona il buzzer (pin, frequenza, tempo)
      //digitalWrite(8,HIGH);
      radio.write(&statoBuzzer, sizeof(statoBuzzer));
      if(buttonPostoneAlarm == HIGH && cnt > 0){ // finche ci sono tentativi di posorre il segnale allora si pospone
        Serial.println("ENTRO");
        digitalWrite(6,LOW);
        digitalWrite(8,LOW);
        //noTone(8);
        timer.setCounter(0, 0, 3, timer.COUNT_DOWN, onComplete);
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
  buttonPostoneAlarm = digitalRead(2); //valore del sensore touch per posporre il segnale
  buttonOffAlarm = digitalRead(11); //valore del pulsante per spegnere l'allarme
  if (first == 0){ //faccio il gioco con first perchè almeno alla prima iterazione del loop prende il valore dallo switch e setta il boolean premuto per il toggle
     if (val == 1){ //L'utente ha selezionato 30 minuti
      timer.setCounter(0, 0, 5, timer.COUNT_DOWN, onComplete);
      timer.setInterval(refreshClock, 1000);
      premuto = true;
    }else{ //L'utente ha selezionato 1 ora
      timer.setCounter(0, 0, 10, timer.COUNT_DOWN, onComplete);
      timer.setInterval(refreshClock, 1000);
      premuto = false;
    }
    first = 1;
  }else{
    if (val == 1 && premuto == false){ //L'utente ha selezionato 30 minuti
      if(timer.getCurrentSeconds() == 0){ 
        timer.setCounter(0, 0, 5, timer.COUNT_DOWN, onComplete);
        timer.setInterval(refreshClock, 1000);
        premuto = true;
      }else{ //sto cambiando il timer in corso, passo da 1 ora a 30 minuti
        Serial.print("STO NELL?ELSE ");Serial.println(timer.getCurrentSeconds());
        if(timer.getCurrentSeconds() > 5){
          Serial.println("ENTRO IF ");
          timerValue = timer.getCurrentSeconds() - 5; //tolgo 5 perchè se ad esempio sono passati 2 sec, sono ad 8 secondi - 5 = 3 quindi anche in 5 secondi sono passati 2 secondi (SPIEGO DI MERDA)
          timer.setCounter(0, 0, 5 - timerValue, timer.COUNT_DOWN, onComplete);
        }else{
          Serial.println("ENTRO ELSE ");
          timer.setCounter(0, 0, 0, timer.COUNT_DOWN, onComplete);// vuol dire che ha cambiato quando il tempo che manca è uguale a 5 o meno seocndi quindi il tempo è finitio
        }
        timer.setInterval(refreshClock, 1000);
        premuto = true;
        Serial.print("ENTRO");
      }
    }else if (val == 0 && premuto == true){ //L'utente ha selezionato 1 ora
      if(timer.getCurrentSeconds() == 0){
        timer.setCounter(0, 0, 10, timer.COUNT_DOWN, onComplete);
        timer.setInterval(refreshClock, 1000);
        premuto = false;
      }else{ //sto cambiando il timer in corso, passo da 30 minuti e 1 ora
        timerValue = 10 - timer.getCurrentSeconds();
        timer.setCounter(0, 0, 10 - timerValue, timer.COUNT_DOWN, onComplete);
        timer.setInterval(refreshClock, 1000);
        premuto = false;
        Serial.print("ENTRO");
      }
    }    
  }

  cm = ultrasonic.read();
  if (cm <= 30) { //l'utente è vicino alla sedia (ho messo dieci per comodità...in teoria dovrebbe essere almeno 40 cm)
      if(cm >10){ // l'utente sta assumendo una posizione scorretta
        digitalWrite(9,HIGH);
      }else if (cm <= 10){ //l'utente sta dritto con la schiena
        digitalWrite(9,LOW);
      }
      digitalWrite(7, HIGH);
      if(timer.getCurrentSeconds() != 0 && app == true) { // finche il timer è diverso da 0
        timer.start();
        Serial.print("getCurrentSeconds: ");Serial.print(timer.getCurrentSeconds()); Serial.print(" - Variabile app: "); Serial.println(app);
        delay(1000);
      }else{
        app = false;
        postoneAlarm();
      }
  }else{
    timer.stop();
    digitalWrite(9,LOW);
    if (buttonOffAlarm == HIGH) { // se l'allarme suona e l'utente si è alzato dalla sedia allora premo il pulsante per spegnere l'allarme
      timer.stop();
      first = 0;
      digitalWrite(6, LOW);
      digitalWrite(8,LOW);
      app = true; 
      cnt = 3;
    }
    digitalWrite(7, LOW);
  }
}
