#include "USB.h"
#include "USBHIDKeyboard.h"

USBHIDKeyboard Keyboard;

// --- CONFIGURATION ---
const int NOISE_GATE = 100;      
const float SWIPE_DIST = 0.6;    
const int SWIPE_WINDOW = 500;    
const int COOL_DOWN = 600;       

// History for tracking movement
float historyX[20];
float historyY[20];
unsigned long historyTime[20];
int historyIdx = 0;
unsigned long lastTriggerTime = 0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 0, 1); 
  Keyboard.begin();
  USB.begin();
}

void loop() {
  if (Serial1.available()) {
    String incoming = Serial1.readStringUntil('\n');
    incoming.trim();

    if (incoming.length() > 0) {
      int v1, v2, v3, v4;
      if (sscanf(incoming.c_str(), "%d,%d,%d,%d", &v1, &v2, &v3, &v4) == 4) {
        
        // --- PRINT RAW DATA TO MONITOR ---
        Serial.print(v1); Serial.print(",");
        Serial.print(v2); Serial.print(",");
        Serial.print(v3); Serial.print(",");
        Serial.println(v4);

        // --- SWIPE LOGIC ---
        float pTL = abs(v1); float pTR = abs(v2);
        float pBL = abs(v3); float pBR = abs(v4);
        float total = pTL + pTR + pBL + pBR;

        if (total > NOISE_GATE) {
          float touchX = (((pTR + pBR) / total) * 2.0) - 1.0;
          float touchY = (((pTL + pTR) / total) * 2.0) - 1.0;

          unsigned long now = millis();
          historyX[historyIdx] = touchX;
          historyY[historyIdx] = touchY;
          historyTime[historyIdx] = now;
          historyIdx = (historyIdx + 1) % 20;

          if (now - lastTriggerTime > COOL_DOWN) {
            checkSwipe(now);
          }
        }
      }
    }
  }
}

void checkSwipe(unsigned long currentTime) {
  int oldestIdx = historyIdx; 
  float moveX = historyX[(historyIdx + 19) % 20] - historyX[oldestIdx];
  float moveY = historyY[(historyIdx + 19) % 20] - historyY[oldestIdx];
  unsigned long duration = currentTime - historyTime[oldestIdx];

  if (duration < SWIPE_WINDOW && duration > 50) {
    if (abs(moveX) > abs(moveY) * 1.5) {
      if (moveX > SWIPE_DIST) triggerSwipe("RIGHT");
      else if (moveX < -SWIPE_DIST) triggerSwipe("LEFT");
    } 
    else if (abs(moveY) > abs(moveX) * 1.5) {
      if (moveY > SWIPE_DIST) triggerSwipe("UP");
      else if (moveY < -SWIPE_DIST) triggerSwipe("DOWN");
    }
  }
}

void triggerSwipe(String direction) {
  lastTriggerTime = millis();
  Serial.print(">>> [DETECTED SWIPE]: "); 
  Serial.println(direction);

  if (direction == "RIGHT") Keyboard.write(KEY_RIGHT_ARROW);
  if (direction == "LEFT")  Keyboard.write(KEY_LEFT_ARROW);
  if (direction == "UP")    Keyboard.write(KEY_UP_ARROW);
  if (direction == "DOWN")  Keyboard.write(KEY_DOWN_ARROW);
}