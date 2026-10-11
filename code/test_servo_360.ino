#include <ESP32Servo.h>

#define PIN_SERVO 26 // Bebas ganti ke GPIO pilihan kamu

Servo servo360;

void setup() {
  Serial.begin(115200);
  delay(500);

  // Kebanyakan servo DS04-NFC memiliki lebar pulsa neutral ~1500us (500us - 2500us)
  servo360.setPeriodHertz(50);
  servo360.attach(PIN_SERVO, 500, 2500);

  Serial.println("=== TEST SERVO 360 DS04-NFC ===");
}

void loop() {

  // 3. Putar Berlawanan Arah Jarum Jam (Full Speed)
  Serial.println("3. Putar Kiri (Full Speed)");
  servo360.write(0);
  delay(5000);

  // 4. Berhenti (Stop)
  Serial.println("4. Diam / Stop");
  servo360.write(90);
  delay(1000);
}