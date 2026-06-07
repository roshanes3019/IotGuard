#include <ESP8266WiFi.h>
#include <espnow.h>

// MAC addresses of both devices
uint8_t peerAddress1[] = {0xBC, 0xDD, 0xC2, 0x16, 0xA1, 0x4E}; // Device 1 MAC address
uint8_t peerAddress2[] = {0xDC, 0x4F, 0x22, 0x6C, 0x0C, 0xDE}; // Device 2 MAC address

// 16-byte encryption key (must be the same on both devices)
uint8_t encryptionKey[16] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA7, 0xB8,
                             0xC9, 0xDA, 0xEB, 0xFC, 0xAB, 0xBC, 0xCD, 0xDE};

// Message structure
typedef struct struct_message {
  char text[250];
} struct_message;

struct_message outgoingMessage;
struct_message incomingMessage;

// Callback when data is sent
void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("\nLast Packet Send Status: ");
  Serial.println(sendStatus == 0 ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void onReceive(uint8_t *mac_addr, uint8_t *incomingData, uint8_t len) {
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
  Serial.print("Received Message: ");
  Serial.println(incomingMessage.text);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }
  
  // Set device role as both sender and receiver
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  
  // Register callback functions
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);

  // Add peer device with specified MAC address and set encryption key
  esp_now_add_peer(peerAddress1, ESP_NOW_ROLE_COMBO, 1, encryptionKey, 16);
}

void loop() {
  // Check if data is available in the serial monitor
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');  // Read the input until newline
    input.trim();  // Remove any leading/trailing whitespace

    // Convert the input message to char array and copy to outgoingMessage.text
    input.toCharArray(outgoingMessage.text, 250);

    // Send message to the peer device
    esp_now_send(peerAddress1, (uint8_t *)&outgoingMessage, sizeof(outgoingMessage));

    Serial.println("Message Sent: " + input);
  }
}
