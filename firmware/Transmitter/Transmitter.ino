#include <WiFi.h> 
#include <esp_now.h> 
 
// Receiver MAC: 70:4B:CA:6E:4D:CC 
uint8_t receiverMAC[] = {0x70, 0x4B, 0xCA, 0x6E, 0x4D, 0xCC}; 
 
#define THROTTLE_PIN 34   // right joystick Y (aage/peeche) 
#define STEER_PIN    35   // left joystick X (left/right) 
 
typedef struct { 
  int16_t throttle; 
  int16_t steer; 
} Packet; 
 
Packet data; 
int cT = 2048, cS = 2048;                 // center 
int minT, maxT, minS, maxS;               // auto-learned range 
const int DEADZONE = 8; 
 
int readAvg(int pin) { 
  analogRead(pin);              // dummy read (crosstalk kam) 
  delayMicroseconds(200); 
  long s = 0; 
  for (int i = 0; i < 16; i++) { 
    s += analogRead(pin); 
    delayMicroseconds(50); 
  } 
  return s / 16; 
} 
 
int mapAxis(int raw, int center, int mn, int mx) { 
  int v; 
  if (raw >= center) { 
    if (mx - center < 100) return 0; 
    v = map(raw, center, mx - 30, 0, 100); 
  } else { 
    if (center - mn < 100) return 0; 
    v = map(raw, mn + 30, center, -100, 0); 
  } 
  v = constrain(v, -100, 100); 
  if (abs(v) < DEADZONE) v = 0; 

  return v; 
} 
 
void setup() { 
  Serial.begin(115200); 
  analogReadResolution(12); 
  analogSetPinAttenuation(THROTTLE_PIN, ADC_11db); 
  analogSetPinAttenuation(STEER_PIN, ADC_11db); 
 
  delay(500);  // joystick beech me chhodo 
  cT = readAvg(THROTTLE_PIN); 
  cS = readAvg(STEER_PIN); 
  minT = cT - 600; maxT = cT + 600; 
  minS = cS - 600; maxS = cS + 600; 
 
  WiFi.mode(WIFI_STA); 
  if (esp_now_init() != ESP_OK) { 
    Serial.println("ESP-NOW init fail"); 
    return; 
  } 
 
  esp_now_peer_info_t peer = {}; 
  memcpy(peer.peer_addr, receiverMAC, 6); 
  peer.channel = 0; 
  peer.encrypt = false; 
  if (esp_now_add_peer(&peer) != ESP_OK) { 
    Serial.println("Peer add fail"); 
  } 
} 
 
void loop() { 
  int rawT = readAvg(THROTTLE_PIN); 
  int rawS = readAvg(STEER_PIN); 
 
  // range auto-learn 
  if (rawT < minT) minT = rawT; 
  if (rawT > maxT) maxT = rawT; 
  if (rawS < minS) minS = rawS; 
  if (rawS > maxS) maxS = rawS; 
 
  data.throttle = -mapAxis(rawT, cT, minT, maxT);   // forward/back: same 
  data.steer    =  mapAxis(rawS, cS, minS, maxS);   // left/right: reverse kiya 
 
  esp_now_send(receiverMAC, (uint8_t *)&data, sizeof(data)); 
 
  Serial.printf("rawT:%d rawS:%d | T:%d S:%d\n", rawT, rawS, data.throttle, data.steer); 
  delay(20); 
}