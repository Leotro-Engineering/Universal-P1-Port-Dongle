#include <HardwareSerial.h>

// P1 IN
#define PIN_DATA_IN     1  
#define PIN_REQ_IN      2   

// LED linked to REQ
#define REQ_STATUS_LED  9   

HardwareSerial mySerial(1);

unsigned long lastRequestTime = 0;
const unsigned long REQUEST_INTERVAL_MS = 1000; // 1 second interval

void setup() {
  Serial.begin(115200);

  pinMode(PIN_REQ_IN, OUTPUT);
  pinMode(REQ_STATUS_LED, OUTPUT);
  digitalWrite(PIN_REQ_IN, LOW);
  digitalWrite(REQ_STATUS_LED, LOW);

  mySerial.begin(115200, SERIAL_8N1, PIN_DATA_IN, -1); 

  Serial.println("Setup completed");
}

void loop() {
  unsigned long currentMillis = millis();

  // Send request pulse 
  if (currentMillis - lastRequestTime >= REQUEST_INTERVAL_MS) {
    digitalWrite(PIN_REQ_IN, HIGH);
    digitalWrite(REQ_STATUS_LED, HIGH); 
    delay(3);         
    digitalWrite(PIN_REQ_IN, LOW);
    digitalWrite(REQ_STATUS_LED, LOW);  

    lastRequestTime = currentMillis;
    Serial.println("[REQ SENT]");
  }

  // Read and print incoming P1 data
  while (mySerial.available()) {
    int byteRead = mySerial.read();
    if (byteRead != -1) {
      Serial.write((char)byteRead);
    }
  }
}