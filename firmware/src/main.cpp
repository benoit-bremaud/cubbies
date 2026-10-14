// cubbies firmware entry point.
// Placeholder until the UML study of `cube-core` is validated (docs/architecture/README.md).
// Adapters (display, audio, touch, radio, sensors) will live next to this file; behaviour logic
// lives in lib/ and never includes Arduino.
#include <Arduino.h>

#include "pins.h"
#include "version.h"

void setup() {
  Serial.begin(115200);
  pinMode(cubbies::pins::kBuzzer, OUTPUT);
  digitalWrite(cubbies::pins::kBuzzer, LOW);
  Serial.printf("cubbies %s\n", cubbies::kFirmwareVersion);
}

void loop() {
  delay(1000);
}
