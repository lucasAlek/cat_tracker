#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <U8g2lib.h>
#include "secrets.h" // contains ssid and password - excluded from git

const char* scriptURL = "https://script.google.com/macros/s/AKfycbxnjRpXuyTHRHWxDqlg31IhDuTAXuqRe3JgoftoDtAKj4Bco2iKGJ2_MavsBy9bxoxj/exec";

const int pirPin = 17;
const int redPin = 13;
const int greenPin = 12;
const int bluePin = 14;
const int resetButtonPin = 15; // button to reset the counter

// OLED display setup (I2C on pins 1=SCL, 2=SDA - try swapping if display doesn't work)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 1, /* data=*/ 2);

// Motion filtering and timing values
const unsigned long motionHoldMillis = 45000; // keep green active long enough for the cat (45 seconds to complete and leave)
const unsigned long motionCheckInterval = 200; // sample PIR every 200 ms
const int motionStableThreshold = 5; // require several HIGH readings before confirming motion

int consecutiveHighReadings = 0; // stable motion sample counter
bool motionActive = false; // whether motion is currently considered active
unsigned long motionActiveUntil = 0; // timestamp to keep the active period alive
int visitCounter = 0; // count of cat visits since last reset
bool lastButtonState = HIGH; // track previous button state for edge detection
unsigned long lastButtonPressTime = 0; // debounce timer for button
const unsigned long buttonDebounceDelay = 50; // 50ms debounce

// put function declarations here:
// int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(pirPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(resetButtonPin, INPUT_PULLUP); // reset button with internal pull-up

  // initialize OLED display
  Serial.println("Initializing OLED display...");
  u8g2.begin();
  Serial.println("OLED display initialized");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr); // larger font for better visibility
  u8g2.drawStr(10, 30, "Initializing...");
  u8g2.sendBuffer();
  Serial.println("OLED display updated");

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password, 6);

   while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  delay(20000);

  // display ready message
  u8g2.clearBuffer();
  u8g2.drawStr(15, 30, "Litter Box");
  u8g2.drawStr(30, 50, "Tracker");
  u8g2.sendBuffer();
  delay(2000);
}

void loop() {
  // put your main code here, to run repeatedly:
  int motionDetected = digitalRead(pirPin); // read PIR sensor state
  unsigned long now = millis(); // current time for tracking active hold

  // check for reset button press
  bool currentButtonState = digitalRead(resetButtonPin);
  if (currentButtonState == LOW && lastButtonState == HIGH && (now - lastButtonPressTime) > buttonDebounceDelay) {
    lastButtonPressTime = now;
    visitCounter = 0; // reset counter
    Serial.println("Counter reset!");
  }
  lastButtonState = currentButtonState;

  // if the sensor reads HIGH, increase the stability counter
  if (motionDetected == HIGH) {
    consecutiveHighReadings++;
  } else {
    consecutiveHighReadings = 0; // reset if the reading is not stable
  }

  // if motion is not already active, check for a stable trigger
  if (!motionActive) {
    if (consecutiveHighReadings >= motionStableThreshold) {
      motionActive = true;
      motionActiveUntil = now + motionHoldMillis; // keep motion active for a set period
      visitCounter++; // increment the visit counter

      // show motion detected with green LED
      digitalWrite(greenPin, HIGH);
      digitalWrite(redPin, LOW);
      digitalWrite(bluePin, LOW);

      Serial.print("Motion detected on pin 17! Visit count: ");
      Serial.println(visitCounter);

      // attempt to log the motion event over WiFi
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(scriptURL) + "?event=Gouch_in_the_Gouch_box";
        http.begin(url);

        int code = http.GET();
        if (code > 0) {
          Serial.println("Logged motion event!");
        } else {
          Serial.println("Error sending event");
        }

        http.end();
      }
    } else {
      // not enough stable readings yet, keep red LED on
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, LOW);
    }
  } else {
    // motion is currently active, keep it active until timeout
    if (now >= motionActiveUntil) {
      motionActive = false; // active period finished
      consecutiveHighReadings = 0; // reset counter so we require a fresh trigger
    }

    // maintain green LED while motion period is active
    digitalWrite(greenPin, HIGH);
    digitalWrite(redPin, LOW);
    digitalWrite(bluePin, LOW);
  }

  // update OLED display with counter
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(15, 30, "Visits:");
  
  char counterStr[10];
  sprintf(counterStr, "%d", visitCounter);
  u8g2.setFont(u8g2_font_ncenB24_tr); // larger font for the number
  u8g2.drawStr(25, 65, counterStr);
  
  u8g2.sendBuffer();

  delay(motionCheckInterval); // pause before next PIR sample
}

// put function definitions here:
// int myFunction(int x, int y) {
//   return x + y;
// }