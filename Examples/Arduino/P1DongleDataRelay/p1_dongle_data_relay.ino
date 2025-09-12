#include <HardwareSerial.h>

// P1 IN
#define PIN_DATA_IN    1  
#define PIN_REQ_IN     2   

// P1 OUT
#define PIN_REQ_OUT    41 
#define PIN_DATA_OUT   42  

// Status LED 
#define REQ_STATUS_LED  9   

HardwareSerial mySerial(1);

unsigned long lastSendTime = 0;
const int SEND_INTERVAL_MS = 5; // UART buffer polling interval (ms)

uint8_t buffer[512]; // Buffer size

void setup() {
  Serial.begin(115200);

  pinMode(PIN_REQ_OUT, INPUT);
  pinMode(PIN_REQ_IN, OUTPUT);
  pinMode(REQ_STATUS_LED, OUTPUT);
  digitalWrite(PIN_REQ_IN, LOW);
  digitalWrite(REQ_STATUS_LED, LOW);

  mySerial.begin(115200, SERIAL_8N1, PIN_DATA_IN, PIN_DATA_OUT);

  Serial.println("Setup completed");
}

void loop() {
  static int lastReqState = -1;
  int currentReqState = digitalRead(PIN_REQ_OUT);

  // Relay REQ state if changed
  if (currentReqState != lastReqState) {
    digitalWrite(PIN_REQ_IN, !currentReqState); // Invert for receiver
    digitalWrite(REQ_STATUS_LED, !currentReqState);
    lastReqState = currentReqState;
  }

  // Collect and flush UART data periodically
  if (millis() - lastSendTime > SEND_INTERVAL_MS && mySerial.available()) {
    uint16_t length = 0;

    while (mySerial.available() && length < sizeof(buffer)) {
      int byteRead = mySerial.read();
      if (byteRead == -1) {
        Serial.println("[UART ERROR] Could not read a byte from mySerial!");
        break; 
      }
      if (length < sizeof(buffer)) {
        buffer[length++] = (uint8_t)byteRead;
      } else {
        Serial.println("[BUFFER WARNING] Buffer limit reached, data is being ignored!");
        break;
      }
    }

    // Print full packet to Serial Monitor
    for (uint16_t i = 0; i < length; i++) {
      Serial.print((char)buffer[i]); // Print als text
    }
    Serial.println();

    // Relay packet to Module B
    for (uint16_t i = 0; i < length; i++) {
      mySerial.write(buffer[i]);
    }

    lastSendTime = millis();
  }
}