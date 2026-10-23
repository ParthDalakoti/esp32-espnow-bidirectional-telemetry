#include <WiFi.h> 
#include <esp_now.h> 
 
// Transmitter MAC: 00:70:07:83:4B:8C 
uint8_t transmitterMAC[] = { 
  0x00, 0x70, 0x07, 0x83, 0x4B, 0x8C 
}; 
 
#define ENA 25 
#define IN1 26 
#define IN2 27 
#define ENB 14 
#define IN3 32 
#define IN4 33 
 
const int PWM_FREQ = 1000; 
const int PWM_RES  = 8; 
const int FULL_PWM = 255; 
const int TRIGGER  = 20; 
 
typedef struct { 
  int16_t throttle; 
  int16_t steer; 
} Packet; 
 
volatile int rxThrottle = 0; 
volatile int rxSteer = 0; 
volatile unsigned long lastRecv = 0; 
 
void setMotor(int pwmPin, int inA, int inB, int dir) { 
  if (dir > 0) { 
    digitalWrite(inA, HIGH); 
    digitalWrite(inB, LOW); 
    ledcWrite(pwmPin, FULL_PWM); 
  }  
  else if (dir < 0) { 
    digitalWrite(inA, LOW); 
    digitalWrite(inB, HIGH); 
    ledcWrite(pwmPin, FULL_PWM); 
  }  
  else { 
    digitalWrite(inA, LOW); 
    digitalWrite(inB, LOW); 

    ledcWrite(pwmPin, 0); 
  } 
} 
 
void stopCar() { 
  setMotor(ENA, IN1, IN2, 0); 
  setMotor(ENB, IN3, IN4, 0); 
} 
 
int toDir(int v) { 
  if (v > TRIGGER)  return 1; 
  if (v < -TRIGGER) return -1; 
  return 0; 
} 
 
void onRecv(const esp_now_recv_info_t *info, 
            const uint8_t *incoming, int len) { 
  if (len != sizeof(Packet)) return; 
 
  if (memcmp(info->src_addr, transmitterMAC, 6) != 0) 
    return; 
 
  Packet p; 
  memcpy(&p, incoming, sizeof(p)); 
 
  rxThrottle = p.throttle; 
  rxSteer = p.steer; 
  lastRecv = millis(); 
} 
 
void setup() { 
  Serial.begin(115200); 
 
  pinMode(IN1, OUTPUT); 
  pinMode(IN2, OUTPUT); 
  pinMode(IN3, OUTPUT); 
  pinMode(IN4, OUTPUT); 
 
  ledcAttach(ENA, PWM_FREQ, PWM_RES); 
  ledcAttach(ENB, PWM_FREQ, PWM_RES); 
 
  stopCar(); 
 
  WiFi.mode(WIFI_STA); 
 
  if (esp_now_init() != ESP_OK) { 
    Serial.println("ESP-NOW init fail"); 
    return; 

  } 
 
  esp_now_register_recv_cb(onRecv); 
 
  Serial.println("Receiver ready"); 
} 
 
void loop() { 
  // Signal 300 ms tak nahi aaya to car stop 
  if (millis() - lastRecv > 300) { 
    stopCar(); 
    return; 
  } 
 
  int t = toDir(rxThrottle); 
  int s = toDir(rxSteer); 
 
  int left  = constrain(t + s, -1, 1); 
  int right = constrain(t - s, -1, 1); 
 
  setMotor(ENA, IN1, IN2, left); 
  setMotor(ENB, IN3, IN4, right); 
 
  delay(10); 
} 
 
 
THIS IS BASICALLY MAC ADDRESSES BASED COMMUNICATION SYSTEM BETWEEN 
2 ESP.