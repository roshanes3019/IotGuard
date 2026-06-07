#include <ESP8266WiFi.h>
#include <espnow.h>
#include <FS.h>

// MAC addresses of both devices
uint8_t allowedDevices[][6] = {
  {0xC4, 0xD8, 0xD5, 0x38, 0x7E, 0xE0}, // Device 1 MAC
  {0xC4, 0xD8, 0xD5, 0x38, 0x73, 0x13}  // Device 2 MAC
};

// 16-byte encryption key (same on both devices; optional)
uint8_t encryptionKey[16] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA7, 0xB8, 
                             0xC9, 0xDA, 0xEB, 0xFC, 0xAB, 0xBC, 0xCD, 0xDE};

// Maximum chunk size for ESP-NOW messages
#define CHUNK_SIZE 240  // 240 bytes for payload (max 250 bytes for ESP-NOW)

// Message structure
typedef struct struct_message {
  uint8_t messageType;      // 0 for text, 1 for image
  uint8_t chunkID;          // Chunk ID to track the order of chunks
  uint8_t totalChunks;      // Total number of chunks
  char text[CHUNK_SIZE];    // Text data or chunk of binary image data
} struct_message;

struct_message outgoingMessage;
struct_message incomingMessage;

// Variables for reassembling text and image
String reassembledText = "";
String reassembledImageFilePath = "/received_image.bin"; // Path for storing received image
File receivedImageFile;

// Helper function: Check if a MAC address is in the whitelist
bool isAllowedDevice(uint8_t *mac_addr) {
  for (int i = 0; i < sizeof(allowedDevices) / sizeof(allowedDevices[0]); i++) {
    if (memcmp(mac_addr, allowedDevices[i], 6) == 0) {
      return true; // MAC address matches
    }
  }
  return false; // MAC address not in whitelist
}

// Helper function: Log traffic
void logTraffic(uint8_t *mac_addr, bool allowed) {
  String macStr = "";
  for (int i = 0; i < 6; i++) {
    macStr += String(mac_addr[i], HEX);
    if (i < 5) macStr += ":";
  }
  if (allowed) {
    Serial.println("Allowed communication from: " + macStr);
  } else {
    Serial.println("Blocked communication from: " + macStr);
  }
}

// Callback when data is sent
void onSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(sendStatus == 0 ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void onReceive(uint8_t *mac_addr, uint8_t *incomingData, uint8_t len) {
  // Check if the device is allowed
  bool allowed = isAllowedDevice(mac_addr);
  logTraffic(mac_addr, allowed);

  if (!allowed) {
    Serial.println("Unauthorized device tried to connect!");
    return; // Ignore the packet
  }

  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));

  if (incomingMessage.messageType == 0) {
    // Handle text message
    Serial.print("Received Text Chunk ");
    Serial.print(incomingMessage.chunkID);
    Serial.print(" of ");
    Serial.println(incomingMessage.totalChunks);

    // Append text chunk to reassembledText
    reassembledText += incomingMessage.text;

    // If all chunks are received, print the full message
    if (incomingMessage.chunkID == incomingMessage.totalChunks - 1) {
      Serial.println("Reassembled Text Message: ");
      Serial.println(reassembledText);
      reassembledText = ""; // Clear for next message
    }
  } else if (incomingMessage.messageType == 1) {
    // Handle image chunk
    Serial.print("Received Image Chunk ");
    Serial.print(incomingMessage.chunkID);
    Serial.print(" of ");
    Serial.println(incomingMessage.totalChunks);

    // Open file for appending
    if (incomingMessage.chunkID == 0) {
      SPIFFS.remove(reassembledImageFilePath); // Remove any previous file
      receivedImageFile = SPIFFS.open(reassembledImageFilePath, "w");
    } else {
      receivedImageFile = SPIFFS.open(reassembledImageFilePath, "a");
    }

    if (receivedImageFile) {
      receivedImageFile.write((uint8_t *)incomingMessage.text, len - 3); // Write chunk data
      receivedImageFile.close();
    }

    // If all chunks are received, finalize image
    if (incomingMessage.chunkID == incomingMessage.totalChunks - 1) {
      Serial.println("Image Received and Saved to SPIFFS.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Initialize SPIFFS for saving images
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed.");
    return;
  }

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

  // Add peer device with specified MAC address and set encryption key (optional)
  esp_now_add_peer(allowedDevices[0], ESP_NOW_ROLE_COMBO, 1, encryptionKey, 16);
}

void sendLongMessage(String message) {
  int totalChunks = ceil((float)message.length() / CHUNK_SIZE);

  for (uint8_t i = 0; i < totalChunks; i++) {
    // Extract a chunk of the message
    String chunk = message.substring(i * CHUNK_SIZE, (i + 1) * CHUNK_SIZE);

    // Prepare outgoing message
    outgoingMessage.messageType = 0; // Text
    outgoingMessage.chunkID = i;
    outgoingMessage.totalChunks = totalChunks;
    chunk.toCharArray(outgoingMessage.text, CHUNK_SIZE);

    // Send the chunk
    esp_now_send(allowedDevices[0], (uint8_t *)&outgoingMessage, sizeof(outgoingMessage));

    Serial.print("Sent Text Chunk ");
    Serial.print(i);
    Serial.print(" of ");
    Serial.println(totalChunks);

    delay(100);  // Small delay to avoid overwhelming ESP-NOW
  }
}

void sendImage(String imagePath) {
  File imageFile = SPIFFS.open(imagePath, "r");
  if (!imageFile) {
    Serial.println("Image file not found.");
    return;
  }

  int totalChunks = ceil((float)imageFile.size() / CHUNK_SIZE);
  uint8_t chunkID = 0;

  while (imageFile.available()) {
    // Read a chunk of data from the file
    size_t bytesRead = imageFile.readBytes(outgoingMessage.text, CHUNK_SIZE);

    // Prepare outgoing message
    outgoingMessage.messageType = 1; // Image
    outgoingMessage.chunkID = chunkID++;
    outgoingMessage.totalChunks = totalChunks;

    // Send the chunk
    esp_now_send(allowedDevices[0], (uint8_t *)&outgoingMessage, bytesRead + 3);

    Serial.print("Sent Image Chunk ");
    Serial.print(chunkID - 1);
    Serial.print(" of ");
    Serial.println(totalChunks);

    delay(100);  // Small delay to avoid overwhelming ESP-NOW
  }

  imageFile.close();
}

void loop() {
  // Check if data is available in the Serial Monitor
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');  // Read the input until newline
    input.trim();                                // Remove any leading/trailing whitespace

    if (input.startsWith("text:")) {
      String message = input.substring(5);  // Extract text after "text:"
      Serial.println("Sending Text Message...");
      sendLongMessage(message);
    } else if (input.startsWith("image:")) {
      String imagePath = input.substring(6);  // Extract path after "image:"
      Serial.println("Sending Image...");
      sendImage(imagePath);
    }
  }
}
