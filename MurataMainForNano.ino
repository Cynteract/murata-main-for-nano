#include "USB.h"
#include "USBHIDKeyboard.h"

// --- CONSTANTS & THRESHOLDS ---
const int NOISE_GATE = 100;
const int TRIGGER_THREASHOLD =  50;
const unsigned long CALIBRATION_DURATION_MILLISECONDS = 5000;
const unsigned long SWIPE_WINDOW_MILLISECONDS = 300;
const unsigned long SWIPE_PAUSE_MILLISECONDS = 200;

// --- CALIBRATION STATE ---
float averageBottom = 0, averageTop = 0;
bool isCurrentlyCalibrating = true;
unsigned long calibrationStartTime;
// --- SWIPE DETECTION ---
unsigned long lastTopTriggerTime = 0, lastBottomTriggerTime = 0;
USBHIDKeyboard Keyboard;
bool blockTopSwipe = false;
bool blockBottomSwipe = false;
unsigned long lastSwipeTime=0;


void executeSwipeAction(String direction) {
  Serial.print("EVENT_TRIGGERED: ");
  Serial.println(direction);

  if (direction == "UP") {
    // Serial.println("UP SWIPE Detected");
    //  You can change these to KEY_PAGE_UP or other keys as needed
    Keyboard.press('a');
    delay(50);
    Keyboard.releaseAll();
  } else if (direction == "DOWN") {
    // Serial.println("DOWN SWIPE Detected");
    Keyboard.press('s');
    delay(50);
    Keyboard.releaseAll();
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 0, 1);
  Keyboard.begin();
  USB.begin();
  delay(2000);
  calibrationStartTime = millis();
  Serial.println("Vertical Mode Online: Starting 5s Calibration...");
}

void loop() {
  if (Serial1.available()) {
    String incoming = Serial1.readStringUntil('\n');
    incoming.trim();

    if (incoming.length() > 0) {
      // v1: yellow
      // v2: red
      // v3: blue
      // v4: green
      int v1, v2, v3, v4;
      if (sscanf(incoming.c_str(), "%d,%d,%d,%d", &v1, &v2, &v3, &v4) == 4) {
        int rawPressureTop = -v2;//Touch results in negative values, release in positive
        int rawPressureBottom = -v3;
        // 2. Calibration Logic
        if (isCurrentlyCalibrating) {
          static long sampleCount = 0;
          averageTop += rawPressureTop;
          averageBottom += rawPressureBottom;
          sampleCount++;

          if (millis() - calibrationStartTime > CALIBRATION_DURATION_MILLISECONDS) {
            averageTop /= sampleCount;
            averageBottom /= sampleCount;
            isCurrentlyCalibrating = false;
            Serial.println("Calibration Complete. Vertical Tracking Active.");
          }
          return;
        }

        // 3. Apply Calibration Offsets and Noise Gate
        float adjustedTop = rawPressureTop - averageTop;
        float adjustedBottom = rawPressureBottom - averageBottom;

        if (adjustedTop < NOISE_GATE)
          adjustedTop = 0;
        if (adjustedBottom < NOISE_GATE)
          adjustedBottom = 0;
        
        if (adjustedTop > adjustedBottom)
          adjustedBottom = 0;
        if (adjustedBottom > adjustedTop)
          adjustedTop = 0;
        // 4. Calculate Vertical Coordinate
        // Positive Y = Top pressure | Negative Y = Bottom pressure
        float currentTouchY = adjustedTop - adjustedBottom;

        if (adjustedTop > TRIGGER_THREASHOLD) {
          lastTopTriggerTime = millis();
          blockBottomSwipe = false;
          if (!blockTopSwipe&&millis()>lastSwipeTime+SWIPE_PAUSE_MILLISECONDS) {
            blockTopSwipe = true;
            lastSwipeTime=millis();
            if (millis() < lastBottomTriggerTime + SWIPE_WINDOW_MILLISECONDS) {
              executeSwipeAction("UP");
            }
          }
        }
        if (adjustedBottom > TRIGGER_THREASHOLD) {
          lastBottomTriggerTime = millis();
          blockTopSwipe = false;
          if (!blockBottomSwipe&&millis()>lastSwipeTime+SWIPE_PAUSE_MILLISECONDS) {
            blockBottomSwipe = true;
            lastSwipeTime=millis();
            if (millis() < lastTopTriggerTime + SWIPE_WINDOW_MILLISECONDS) {
              executeSwipeAction("DOWN");
            }
          }
        }

        // // 7. Data Output (Simplified for Top/Bottom only)
        // Serial.print(adjustedTop);
        // Serial.print(",");
        // Serial.println(adjustedBottom);
      }
      delay(10);
    }
  }
}
