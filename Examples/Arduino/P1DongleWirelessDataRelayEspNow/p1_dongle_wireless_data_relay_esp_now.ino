#include <esp_now.h>
#include <WiFi.h>
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

// MAC address of peer device
uint8_t peerMac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; // Insert MAC address of peer device

unsigned long lastSendTime = 0;
const int SEND_INTERVAL_MS = 10; // UART buffer polling interval (ms)

// Structure for UART data transfer
typedef struct struct_data {
  uint8_t length;
  uint8_t buffer[512]; // Buffer size
} DataPacket;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_REQ_OUT, INPUT);
  pinMode(PIN_REQ_IN, OUTPUT);
  pinMode(REQ_STATUS_LED, OUTPUT);
  digitalWrite(PIN_REQ_IN, LOW);
  digitalWrite(REQ_STATUS_LED, LOW);

  mySerial.begin(115200, SERIAL_8N1, PIN_DATA_IN, PIN_DATA_OUT);

  // Initialize Wi-Fi for ESP-NOW 
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50); // Give Wi-Fi hardware time to start

  // Get the MAC address of this device
  String macAddress = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(macAddress); // Use this MAC address in the peer device code to set up a connection between devices

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    while (true);
  }

  esp_now_register_recv_cb(onDataReceived);

  // Register peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(peerMac)) { 
    esp_now_add_peer(&peerInfo); 
  }

  Serial.println("Setup completed");
}

void loop() {
  static int lastReqState = -1;
  int currentReqState = digitalRead(PIN_REQ_OUT);

  // Send REQ state on change
  if (currentReqState != lastReqState) {
    esp_now_send(peerMac, (uint8_t*)&currentReqState, sizeof(currentReqState));
    lastReqState = currentReqState;
  }

  // Check UART buffer and send data
  if (millis() - lastSendTime > SEND_INTERVAL_MS && mySerial.available()) {
    DataPacket packet;
    packet.length = 0;

    while (mySerial.available() && packet.length < sizeof(packet.buffer)) {
      int byteRead = mySerial.read();
      if (byteRead == -1) {
        Serial.println("[UART ERROR] Could not read a byte from mySerial!");
        break;
      }
      if (packet.length < sizeof(packet.buffer)) {
        packet.buffer[packet.length++] = (uint8_t)byteRead;
      } else {
        Serial.println("[BUFFER WARNING] Buffer limit reached, data is being ignored!");
        break;
      }
    }

    esp_err_t result = esp_now_send(peerMac, (uint8_t*)&packet, sizeof(uint8_t) + packet.length);
    if (result == ESP_OK) {
      Serial.print("Sent UART data, bytes: ");
      Serial.println(packet.length);
    } else {
      Serial.println("Failed to send UART data");
    }

    lastSendTime = millis();
  }
}

// Handle incoming ESP-NOW data
void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len == sizeof(int)) {
    int state = !(*data); // Invert REQ signal on receiver
    digitalWrite(PIN_REQ_IN, state);
    digitalWrite(REQ_STATUS_LED, state);
  } else if (len >= 1) {
    const DataPacket* packet = (const DataPacket*)data;

    // Display the received data as text
    for (int i = 0; i < packet->length; i++) {
      Serial.print((char)packet->buffer[i]);
    }

    for (int i = 0; i < packet->length; i++) {
      mySerial.write(packet->buffer[i]);
    }
  }
}
