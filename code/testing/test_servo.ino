/*
  =========================================================
  ESP32 - Test Servo Mineral & pH Buffer (SAJA, tanpa sensor)
  =========================================================
  Kode simpel buat cek servo bergerak normal sebelum digabung
  ke sistem lengkap. Servo akan bolak-balik buka-tutup terus
  menerus supaya kamu bisa lihat & dengar gerakannya langsung.

  LIBRARY: Install "ESP32Servo" by Kevin Harrington lewat
  Arduino Library Manager (Tools > Manage Libraries).

  WIRING:
  - Servo Mineral   -> Signal ke GPIO25
  - Servo pH Buffer -> Signal ke GPIO26
  - Servo VCC -> 5V (idealnya dari power eksternal, bukan dari ESP32
                 kalau servo agak besar/dipakai bersamaan)
  - Servo GND -> GND (disatukan dengan GND ESP32)
  =========================================================
*/

#include <ESP32Servo.h>

#define PIN_SERVO_MINERAL 25
#define PIN_SERVO_PHBUFFER 26

Servo servoMineral;
Servo servoPHBuffer;

const int SUDUT_TERTUTUP = 0;
const int SUDUT_TERBUKA  = 90;

void setup() {
  Serial.begin(115200);
  delay(500);

  servoMineral.setPeriodHertz(50);
  servoMineral.attach(PIN_SERVO_MINERAL, 500, 2400);

  servoPHBuffer.setPeriodHertz(50);
  servoPHBuffer.attach(PIN_SERVO_PHBUFFER, 500, 2400);

  // Posisi awal: tertutup
  servoMineral.write(SUDUT_TERTUTUP);
  servoPHBuffer.write(SUDUT_TERTUTUP);

  Serial.println("=== Test Servo Mineral & pH Buffer ===");
  delay(1000);
}

void loop() {
  // ---- Test Servo Mineral ----
  Serial.println("Servo MINERAL: buka (90 derajat)");
  servoMineral.write(SUDUT_TERBUKA);
  delay(1000);

  Serial.println("Servo MINERAL: tutup (0 derajat)");
  servoMineral.write(SUDUT_TERTUTUP);
  delay(1000);

  // ---- Test Servo pH Buffer ----
  Serial.println("Servo PH BUFFER: buka (90 derajat)");
  servoPHBuffer.write(SUDUT_TERBUKA);
  delay(1000);

  Serial.println("Servo PH BUFFER: tutup (0 derajat)");
  servoPHBuffer.write(SUDUT_TERTUTUP);
  delay(1000);

  Serial.println("--- Ulangi lagi ---");
  delay(1000);
}
