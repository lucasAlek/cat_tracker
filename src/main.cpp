#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "BELL487";
const char* password = "C6E7335AFC99";

const char* scriptURL = "https://script.google.com/macros/s/AKfycbxnjRpXuyTHRHWxDqlg31IhDuTAXuqRe3JgoftoDtAKj4Bco2iKGJ2_MavsBy9bxoxj/exec";

const int pirPin = 17;
const int redPin = 13;
const int greenPin = 12;
const int bluePin = 14;

// Motion filtering and timing values
const unsigned long motionHoldMillis = 12000; // keep green active long enough for the cat
const unsigned long motionCheckInterval = 200; // sample PIR every 200 ms
const int motionStableThreshold = 4; // require several HIGH readings before confirming motion

int consecutiveHighReadings = 0; // stable motion sample counter
bool motionActive = false; // whether motion is currently considered active
unsigned long motionActiveUntil = 0; // timestamp to keep the active period alive

// put function declarations here:
// int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(pirPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password, 6);

   while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  delay(20000);
}

void loop() {
  // put your main code here, to run repeatedly:
  int motionDetected = digitalRead(pirPin); // read PIR sensor state
  unsigned long now = millis(); // current time for tracking active hold

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

      // show motion detected with green LED
      digitalWrite(greenPin, HIGH);
      digitalWrite(redPin, LOW);
      digitalWrite(bluePin, LOW);

      Serial.println("Motion detected on pin 17!");

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

  delay(motionCheckInterval); // pause before next PIR sample
}

// put function definitions here:
// int myFunction(int x, int y) {
//   return x + y;
// }