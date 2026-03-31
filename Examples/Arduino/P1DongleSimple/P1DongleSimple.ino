#include <HardwareSerial.h>

// P1 IN
#define PIN_DATA_IN     1
#define PIN_REQ_IN      2

// Status LED 
#define REQ_STATUS_LED  9

HardwareSerial mySerial(1);

unsigned long lastRequestTime = 0;
unsigned long requestStartTime = 0;

const unsigned long REQUEST_INTERVAL_MS = 1000; // Request interval time (ms)
const unsigned long REQUEST_TIMEOUT_MS  = 5000; // Request timeout time (ms)

bool requestActive = false;
bool checksumStarted = false;
int checksumBytesReceived = 0;

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

  // Start new request
  if (!requestActive && (currentMillis - lastRequestTime >= REQUEST_INTERVAL_MS)) {

    digitalWrite(PIN_REQ_IN, HIGH);
    digitalWrite(REQ_STATUS_LED, HIGH);

    requestActive = true;
    requestStartTime = currentMillis;
    lastRequestTime = currentMillis;

    checksumStarted = false;
    checksumBytesReceived = 0;

    Serial.println("[REQ HIGH]");
  }

  // Read incoming P1 data
  while (mySerial.available()) {

    int byteRead = mySerial.read();

    if (byteRead != -1) {

      char c = (char)byteRead;

      Serial.write(c);

      // Detect start of checksum
      if (requestActive) {
        if (!checksumStarted) {
          if (c == '!') {
            checksumStarted = true;
            checksumBytesReceived = 0;
          }
        } else {
          checksumBytesReceived++;
          if (checksumBytesReceived >= 4) {
            digitalWrite(PIN_REQ_IN, LOW);
            digitalWrite(REQ_STATUS_LED, LOW);
            requestActive = false;
            checksumStarted = false;
            Serial.println("\n[CHECKSUM DETECTED - REQ LOW]");
          }
        }
      }
    }
  }

  // Request timeout
  if (requestActive && (currentMillis - requestStartTime > REQUEST_TIMEOUT_MS)) {
    digitalWrite(PIN_REQ_IN, LOW);
    digitalWrite(REQ_STATUS_LED, LOW);
    requestActive = false;
    checksumStarted = false;
    Serial.println("[TIMEOUT - REQ LOW]");
  }
}
