/*
  =========================================================
  ESP32 - Test OLED SSD1306 (SAJA, tanpa sensor/servo)
  =========================================================
  Kode simpel buat cek OLED nyala & bisa nampilin teks/angka
  sebelum digabung ke sistem lengkap.

  LIBRARY YANG PERLU DIINSTALL (Arduino Library Manager):
  - "Adafruit SSD1306" by Adafruit
  - "Adafruit GFX Library" by Adafruit

  WIRING:
  - OLED VCC -> 3V3
  - OLED GND -> GND
  - OLED SDA -> GPIO21
  - OLED SCL -> GPIO22
  =========================================================
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LEBAR_OLED 128
#define TINGGI_OLED 64
#define OLED_ALAMAT 0x3C   // Ganti ke 0x3D kalau OLED tidak terdeteksi

Adafruit_SSD1306 oled(LEBAR_OLED, TINGGI_OLED, &Wire, -1);

int hitungan = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ALAMAT)) {
    Serial.println("OLED tidak terdeteksi! Cek wiring SDA/SCL atau alamat I2C.");
    while (true) delay(1000); // Berhenti di sini kalau OLED gagal
  }

  Serial.println("OLED terdeteksi, mulai test...");

  // Test 1: Layar penuh nyala (cek semua pixel jalan)
  oled.clearDisplay();
  oled.fillRect(0, 0, LEBAR_OLED, TINGGI_OLED, SSD1306_WHITE);
  oled.display();
  delay(1000);

  // Test 2: Teks statis
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("OLED Test Berhasil!");
  oled.setCursor(0, 15);
  oled.println("128x64 SSD1306");
  oled.setTextSize(2);
  oled.setCursor(0, 35);
  oled.println("Halo ESP32");
  oled.display();
  delay(2000);
}

void loop() {
  // Test 3: Angka berjalan (cek layar bisa update terus-menerus)
  oled.clearDisplay();

  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("Menghitung...");
  oled.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  oled.setTextSize(3);
  oled.setCursor(30, 25);
  oled.println(hitungan);

  oled.display();

  Serial.print("Tampil di OLED: ");
  Serial.println(hitungan);

  hitungan++;
  delay(500);
}
