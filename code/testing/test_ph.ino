#include <Wire.h>
///#include <LiquidCrystal_I2C.h>

// Inisialisasi LCD I2C (alamat umum 0x27, bisa 0x3F tergantung modul)
//LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============================================================
// LANGKAH KALIBRASI 3 TITIK (WAJIB sebelum pakai sensor ini):
// 1. Siapkan 3 larutan buffer: pH 4.01, pH 6.86, dan pH 9.18/10.01
// 2. Celupkan probe ke larutan pH 4.01
//    -> Buka Serial Monitor, tunggu voltase stabil, catat angkanya
//    -> Masukkan ke VOLTAGE_PH4_01 di bawah
// 3. Bilas probe dengan air suling, keringkan dengan tisu
// 4. Ulangi untuk pH 6.86 -> catat ke VOLTAGE_PH6_86
// 5. Bilas lagi, ulangi untuk pH 9.18 (atau 10.01) -> catat ke VOLTAGE_PH9_18
// 6. Upload ulang kode ini, sensor siap dipakai
// ============================================================

// GANTI 3 nilai ini sesuai hasil pengukuran voltase kamu:
float VOLTAGE_PH4_01 = 3.300;
float VOLTAGE_PH6_86 = 2.540;
float VOLTAGE_PH9_18 = 2.115;

// Nilai pH referensi buffer (SESUAIKAN kalau kamu pakai 10.01, bukan 9.18)
const float PH_REF_1 = 4.01;
const float PH_REF_2 = 6.86;
const float PH_REF_3 = 9.18;

// Variabel hasil regresi linear (slope & offset)
float ph_slope;
float ph_offset;

unsigned long int avgval;
int buffer_arr[10], temp;
float ph_act;

// Tentukan pin analog untuk sensor pH
#define PH_SENSOR_PIN 34   // GPIO34 contoh, bisa ganti ke pin analog lain

// Fungsi hitung regresi linear least-squares dari 3 titik (x = voltase, y = pH)
void hitungKalibrasi() {
  float x[3] = { VOLTAGE_PH4_01, VOLTAGE_PH6_86, VOLTAGE_PH9_18 };
  float y[3] = { PH_REF_1, PH_REF_2, PH_REF_3 };

  float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
  int n = 3;

  for (int i = 0; i < n; i++) {
    sumX  += x[i];
    sumY  += y[i];
    sumXY += x[i] * y[i];
    sumX2 += x[i] * x[i];
  }

  // Rumus least squares: slope = (n*ΣXY - ΣX*ΣY) / (n*ΣX² - (ΣX)²)
  ph_slope  = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
  ph_offset = (sumY - ph_slope * sumX) / n;
}

void setup() {
  Wire.begin();
  Serial.begin(115200);   // Baudrate umum ESP32
  //lcd.init();
  // lcd.backlight();
  // lcd.setCursor(0, 0);
  // lcd.print("pH Sensor Ready");

  hitungKalibrasi();

  Serial.println("=== Hasil Kalibrasi 3 Titik ===");
  Serial.print("Slope: ");  Serial.println(ph_slope, 4);
  Serial.print("Offset: "); Serial.println(ph_offset, 4);
  Serial.println("================================");

  delay(2000);
  //lcd.clear();
}

void loop() {
  // Baca data analog
  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(PH_SENSOR_PIN);
    delay(30);
  }

  // Urutkan data (bubble sort)
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }

  // Ambil rata-rata dari data tengah (buang 2 terendah & 2 tertinggi)
  avgval = 0;
  for (int i = 2; i < 8; i++)
    avgval += buffer_arr[i];

  // Konversi ke voltase ESP32 (ADC 12-bit, referensi 3.3V, dibagi 6 karena rata-rata 6 sampel)
  float volt = (float)avgval * 3.3 / 4095.0 / 6;

  // Hitung pH pakai hasil regresi linear 3 titik
  ph_act = ph_slope * volt + ph_offset;

  // Tampilkan di Serial Monitor
  Serial.print("Voltase: ");
  Serial.print(volt, 3);
  Serial.print(" V | pH Value: ");
  Serial.println(ph_act, 2);

  // Tampilkan di LCD
  // lcd.clear();
  // lcd.setCursor(0, 0);
  // lcd.print("pH Value:");
  // lcd.setCursor(0, 1);
  // lcd.print(ph_act, 2);    // 2 angka di belakang koma

  delay(1000); // Delay untuk kestabilan pembacaan
}
