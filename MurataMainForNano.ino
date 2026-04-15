#include "USB.h"
#include "USBHIDKeyboard.h"

// --- CONSTANTS & THRESHOLDS ---
const unsigned long CALIBRATION_DURATION_MILLISECONDS = 5000;
const int NOISE_GATE = 100;
const int SWIPE_DISTANCE_THRESHOLD = 50;
const float SWIPE_TIME_WINDOW_SECONDS = 0.5;
const unsigned long COOLDOWN_WAIT_TIME_MILLISECONDS = 600;

// --- CALIBRATION STATE ---
float averageBottom = 0, averageTop = 0;
bool isCurrentlyCalibrating = true;
unsigned long calibrationStartTime;
unsigned long lastSwipeTriggerTime = 0;

// --- GESTURE HISTORY (Circular Buffer) ---
const int HISTORY_BUFFER_SIZE = 50;
float historyPositionY[HISTORY_BUFFER_SIZE];
unsigned long historyTimestamp[HISTORY_BUFFER_SIZE];
int currentHistoryIndex = 0;

USBHIDKeyboard Keyboard;

// --- GESTURE ACTIONS ---
void executeSwipeAction(String direction) {
  Serial.print("EVENT_TRIGGERED: ");
  Serial.println(direction);

  if (direction == "UP") {
    Serial.println("UP SWIPE Detected");
    // You can change these to KEY_PAGE_UP or other keys as needed
    Keyboard.press('s');
    delay(50);
    Keyboard.releaseAll();
  } else if (direction == "DOWN") {
    Serial.println("DOWN SWIPE Detected");
    Keyboard.press('a');
    delay(50);
    Keyboard.releaseAll();
  }

  lastSwipeTriggerTime = millis();
}
void checkForVerticalSwipe() {
  if (millis() - lastSwipeTriggerTime < COOLDOWN_WAIT_TIME_MILLISECONDS) return;

  // Identify the oldest data point to calculate movement over time
  int oldestIndex = (currentHistoryIndex + 1) % HISTORY_BUFFER_SIZE;

  float movementY = historyPositionY[currentHistoryIndex] - historyPositionY[oldestIndex];
  unsigned long timeDelta = historyTimestamp[currentHistoryIndex] - historyTimestamp[oldestIndex];
  Serial.println(movementY);
  // Check if movement occurred within the speed window
  // Vertical Swipe Detection
  if (movementY > SWIPE_DISTANCE_THRESHOLD) {

    executeSwipeAction("UP");
  } else if (movementY < -SWIPE_DISTANCE_THRESHOLD) {
    executeSwipeAction("DOWN");
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
      int v1, v2, v3, v4;
      if (sscanf(incoming.c_str(), "%d,%d,%d,%d", &v1, &v2, &v3, &v4) == 4) {
        int rawPressureTop = abs(v2);
        int rawPressureBottom = abs(v3);
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

        if (adjustedTop < NOISE_GATE) adjustedTop = 0;
        if (adjustedBottom < NOISE_GATE) adjustedBottom = 0;

        // 4. Calculate Vertical Coordinate
        // Positive Y = Top pressure | Negative Y = Bottom pressure
        float currentTouchY = adjustedTop - adjustedBottom;

        // 5. Update History
        historyPositionY[currentHistoryIndex] = currentTouchY;
        historyTimestamp[currentHistoryIndex] = millis();

        // 6. Process Vertical Gestures
        checkForVerticalSwipe();

        // Increment buffer index
        currentHistoryIndex = (currentHistoryIndex + 1) % HISTORY_BUFFER_SIZE;

        // // 7. Data Output (Simplified for Top/Bottom only)
        // Serial.print(adjustedTop);
        // Serial.print(",");
        // Serial.println(adjustedBottom);
      }
      delay(10);
    }
  }
}