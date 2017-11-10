#include "RCSwitch2.h"

const int BUF_SIZE = 8;

RCSwitch mySwitch = RCSwitch();
RCSwitch sendSwitch = RCSwitch();

char payload[BUF_SIZE];
static const uint8_t PIEZO_PINS[] = {A0,A1,A2,A3,A4};
static const uint8_t SOLENOID_PINS[] = {8, 9, 10, 11, 12};
int NUM_PIEZOS = 5;
int NUM_SOLENOIDS = 5;

int lastSolenoidTestTime = 0;
unsigned long lastPiezoProcessingTime = 0;
int filtered_piezo_recordings[] = {0, 0, 0, 0, 0};
int raw_piezo_recordings[] = {0, 0, 0, 0, 0};

int uno = 0;
int due = 1;
int current = uno;
int mask = 0b00000001;
unsigned long count = 0;
unsigned long micIgnoreTimes[] = {0, 0, 0, 0, 0};

void setup() {
  setUpAllSolenoidPins();
  setUpAllPiezoPins();
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  

  // Transmitter is connected to Arduino Pin #3
  sendSwitch.enableTransmit(3);
//  sendSwitch.setPulseLength(300);

  // 0 means digital pin 2 on duemilanove.
  // the library does some under the hood conversion to pin 2
  // digitalPinToInterrupt() should do it.
  mySwitch.enableReceive(2);

  // Optional set pulse length.
  // mySwitch.setPulseLength(320);

}

void strikeChime(int index) {
  chimeHigh(index);
  delay(15); // delay, because pin takes time to accelerate
  chimeLow(index);
}

void chimeHigh(int index) {
  micIgnoreTimes[index] = millis();
  digitalWrite(SOLENOID_PINS[index], HIGH);
}

void chimeLow(int index) {
  digitalWrite(SOLENOID_PINS[index], LOW);
}

void sendMessage(char * msg) {
  digitalWrite(LED_BUILTIN, HIGH);
  sendSwitch.send(payload);
  digitalWrite(LED_BUILTIN, LOW);
}

void flashLed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
//  testAllSolenoids();
  long theTime = millis();
  updatePiezoValues();
  
  if (theTime - lastPiezoProcessingTime > 100) {
    int mic0 = filtered_piezo_recordings[0];
    int mic1 = filtered_piezo_recordings[1];
    int mic2 = filtered_piezo_recordings[2];
    int mic3 = filtered_piezo_recordings[3];
    int mic4 = filtered_piezo_recordings[4];
    
  
    if (mic0 + mic1 + mic2 + mic3 + mic4 > 0) {
      char buf[30];
      sprintf(buf, "%d-%d-%d-%d-%d \n", 
          mic0, mic1, mic2, mic3, mic4);
      Serial.print(buf);
  
      payload[0] = mic0 > 0 ? '1' : '0';
      payload[1] = mic1 > 0 ? '1' : '0';
      payload[2] = mic2 > 0 ? '1' : '0';
      payload[3] = mic3 > 0 ? '1' : '0';
      payload[4] = mic4 > 0 ? '1' : '0';
      payload[5] = '0';
      payload[6] = current + '0'; 
      payload[7] = '\0';
      Serial.println(payload);
      sendMessage(payload);
    }
    resetPiezoValues();  
  }
  
  
//  Serial.print("- ");
  if (mySwitch.available()) {
    int value = mySwitch.getReceivedValue();

    if (value == 0) {
      Serial.print("Unknown encoding");
    } else {
      int sender = (value) & mask;
      if (sender != current) {
        int receivedChimes[NUM_SOLENOIDS];

        for (int i = 0; i < NUM_SOLENOIDS; i++) {
          receivedChimes[i] = (value >> (BUF_SIZE - 2 - i)) & mask;
        }
        
//        int chime0 = (value >> 6) & mask;
//        int chime1 = (value >> 5) & mask;
//        int chime2 = (value >> 4) & mask;
//        int chime3 = (value >> 3) & mask;
//        int chime4 = (value >> 2) & mask;

          for (int i = 0; i < NUM_SOLENOIDS; i++) {
            if (receivedChimes[i] == 1) {
              chimeHigh(i);  
            }
            
          }
          delay(15);
          for (int i = 0; i < NUM_SOLENOIDS; i++) {
            chimeLow(i);
          }


//  
        char buf[30];
        sprintf(buf, "sender(%d) %d-%d-%d-%d-%d\n", sender, 
        receivedChimes[0], receivedChimes[1], receivedChimes[2], receivedChimes[3], receivedChimes[4]);
////  
//        Serial.print("Received :");
        Serial.print(buf);
//        Serial.print( " : " );
//        Serial.print( mySwitch.getReceivedBitlength() );
//        Serial.print("bit ");
//        Serial.print("Protocol: ");
//        Serial.println( mySwitch.getReceivedProtocol() );

      
          
      }
      
    }

    mySwitch.resetAvailable();   
  }  
 

  delay(10);
//  delay(200);
  count++;

  //  // Switch off
  //  mySwitch.switchOff("11001", "01010");
  //
  //  // Wait another second
  //  delay(1000);

}

void resetPiezoValues() {
  for (int i = 0; i < NUM_PIEZOS; i++) {
    filtered_piezo_recordings[i] = 0;
  }
  lastPiezoProcessingTime = millis();
}

void updatePiezoValues() {
  for (int i = 0; i < NUM_PIEZOS; i++) {
    bool recentlyHitChime = millis() - micIgnoreTimes[i] < 250;
    if (!recentlyHitChime) {
      float raw = analogRead(PIEZO_PINS[i]);
//      float clean = 0;
//      if (raw < 10) {
//        clean = 0;
//      } else if (raw < 200) {
//        clean = 1;
//      } else {
//        clean = 2;
//      }
      if (raw - raw_piezo_recordings[i] > 40) {
        filtered_piezo_recordings[i] = raw;    
      }

      raw_piezo_recordings[i] = raw;
      
    }
  }
}

void setUpAllPiezoPins() {
  for (int i = 0; i < NUM_PIEZOS; i++) {
    pinMode(PIEZO_PINS[i], INPUT);
  }
}

void setUpAllSolenoidPins() {
  for (int i = 0 ; i < NUM_SOLENOIDS; i++) {
    pinMode(SOLENOID_PINS[i], OUTPUT);
    digitalWrite(SOLENOID_PINS[i], LOW);
    delay(100); // computer complains if we try to reset too fast
  }
}

void testAllSolenoids() {
  int spacing = 350;
  int counter = count % spacing * NUM_SOLENOIDS; // when running this routine, only execute it every 10000 frames
  
  if (counter == 0) {
     strikeChime(0);
  } else if (counter == spacing*1) {
    strikeChime(1);
  } else if (counter == spacing*2) {
    strikeChime(2);
  } else if (counter == spacing*3) {
    strikeChime(3);
  } else if (counter == spacing*4) {
    strikeChime(4);
  }
}

char * int2bin(int a, char *buf) {
    buf += (BUF_SIZE - 1);

    for (int i = 0; i < BUF_SIZE; i++) {
        *buf-- = (a & 1) + '0';

        a >>= 1;
    }

    return buf;
}

