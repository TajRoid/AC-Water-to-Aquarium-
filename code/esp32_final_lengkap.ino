/*
  =========================================================
  KODE REFACTORED: Sensor pH + TDS, OLED SH1106 1.3", 2 Servo, 2 Button
  =========================================================
  Library yang dibutuhkan:
  - Adafruit SH110X (by Adafruit)
  - Adafruit GFX Library (by Adafruit)
  - ESP32Servo (by Kevin Harrington)
  =========================================================
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ESP32Servo.h>

// =========================================================
// 1. PIN & KONFIGURASI HARDWARE
// =========================================================
#define PIN_PH_SENSOR       35
#define PIN_TDS_SENSOR      34
#define PIN_SERVO_MINERAL   25
#define PIN_SERVO_PHBUFFER  26
#define PIN_BTN_PILIH       27
#define PIN_BTN_OK          14

// Konfigurasi OLED SH1106
#define LEBAR_OLED  128
#define TINGGI_OLED 64
#define OLED_ALAMAT 0x3C // Ganti ke 0x3D jika OLED tidak menyala
Adafruit_SH1106G oled(LEBAR_OLED, TINGGI_OLED, &Wire, -1);

// Konfigurasi Servo
Servo servoMineral;
Servo servoPHBuffer;
const int SUDUT_TERTUTUP = 0;
const int SUDUT_TERBUKA  = 90;

// =========================================================
// 2. ENUMERASI & STATE MACHINE
// =========================================================
enum ModeAir { MODE_FISH = 0, MODE_PLANT = 1 };
enum AppState { MENU, MONITORING };
enum DosingState { IDLE, DOSING_MINERAL, DOSING_PHBUFFER, COOLDOWN };

ModeAir modeTerpilih = MODE_FISH;
AppState appState = MENU;
DosingState dosingState = IDLE;

// =========================================================
// 3. PARAMETER & TARGET DOSING
// =========================================================
// Index [0] = FISH, Index [1] = PLANT
const float TDS_MIN[2]             = { 190.0, 130.0 };
const float TDS_MAX[2]             = { 225.0, 170.0 };
const float PH_MIN[2]              = { 6.5,   5.5   };
const float PH_MAX[2]              = { 7.5,   6.5   };
const float TARGET_PPM_DOSING[2]   = { 207.5, 150.0 };
const unsigned long DURASI_PHBUFFER_MS[2] = { 300, 400 };

// Parameter Kalibrasi & Fisik
const float VOLUME_AIR_LITER            = 0.63; 
const float GRAM_PER_PPM_PER_LITER      = 0.6 / (218.0 * 0.1);
const float LAJU_ALIR_MINERAL_G_PER_MS  = 0.0012; // Wajib kalibrasi ulang secara fisik

// Kalibrasi Sensor pH (3 Titik Voltage)
float VOLTAGE_PH4_01 = 3.300;
float VOLTAGE_PH6_86 = 2.540;
float VOLTAGE_PH9_18 = 2.115;

const float PH_REF_1 = 4.01;
const float PH_REF_2 = 6.86;
const float PH_REF_3 = 9.18;

// =========================================================
// 4. VARIABEL GLOBAL & TIMING
// =========================================================
// Variable Sensor pH
float ph_slope = 0.0, ph_offset = 0.0, ph_act = 0.0;
int phBuffer[10];
int phBufferIndex = 0;
unsigned long phSampleTimepoint = 0;
const unsigned long PH_SAMPLE_INTERVAL = 30;

// Variable Sensor TDS
#define SCOUNT 30
int tdsAnalogBuffer[SCOUNT];
int tdsAnalogBufferTemp[SCOUNT];
int tdsBufferIndex = 0;
float tdsValue = 0.0;
float temperature = 25.0; // Asumsi suhu air
unsigned long tdsSampleTimepoint = 0;
unsigned long tdsCalcTimepoint = 0;

// Variable Dosing
unsigned long waktuMulaiAksi = 0;
unsigned long durasiMineralSekarang = 0;
unsigned long durasiPHBufferSekarang = 0;
const unsigned long COOLDOWN_SETELAH_DOSING = 15000;

// Variable Display & Tombol
unsigned long tampilTimepoint = 0;
const unsigned long TAMPIL_INTERVAL = 500;
const unsigned long DEBOUNCE_DELAY = 200;
unsigned long waktuTekanPilihTerakhir = 0;
unsigned long waktuTekanOkTerakhir = 0;

// =========================================================
// 5. FUNGSI SENSOR pH
// =========================================================
void hitungKalibrasiPH() {
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

  ph_slope  = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
  ph_offset = (sumY - ph_slope * sumX) / n;
}

void updateSensorPH() {
  if (millis() - phSampleTimepoint >= PH_SAMPLE_INTERVAL) {
    phSampleTimepoint = millis();
    phBuffer[phBufferIndex++] = analogRead(PIN_PH_SENSOR);

    if (phBufferIndex >= 10) {
      phBufferIndex = 0;
      int sortedBuf[10];
      for (int i = 0; i < 10; i++) sortedBuf[i] = phBuffer[i];

      // Sorting bubble sort sederhana
      for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 10; j++) {
          if (sortedBuf[i] > sortedBuf[j]) {
            int temp = sortedBuf[i];
            sortedBuf[i] = sortedBuf[j];
            sortedBuf[j] = temp;
          }
        }
      }

      // Rata-rata dari nilai tengah (index 2-7)
      unsigned long avgval = 0;
      for (int i = 2; i < 8; i++) avgval += sortedBuf[i];

      float volt = (float)avgval * 3.3 / 4095.0 / 6.0;
      ph_act = ph_slope * volt + ph_offset;
    }
  }
}

// =========================================================
// 6. FUNGSI SENSOR TDS
// =========================================================
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++) bTab[i] = bArray[i];

  for (int j = 0; j < iFilterLen - 1; j++) {
    for (int i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        int temp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = temp;
      }
    }
  }

  if ((iFilterLen & 1) > 0) return bTab[(iFilterLen - 1) / 2];
  else return (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
}

void updateSensorTDS() {
  if (millis() - tdsSampleTimepoint > 40U) {
    tdsSampleTimepoint = millis();
    tdsAnalogBuffer[tdsBufferIndex++] = analogRead(PIN_TDS_SENSOR);
    if (tdsBufferIndex >= SCOUNT) tdsBufferIndex = 0;
  }

  if (millis() - tdsCalcTimepoint > 800U) {
    tdsCalcTimepoint = millis();
    for (int i = 0; i < SCOUNT; i++) tdsAnalogBufferTemp[i] = tdsAnalogBuffer[i];

    float averageVoltage = getMedianNum(tdsAnalogBufferTemp, SCOUNT) * 3.3 / 4096.0;
    float compCoef = 1.0 + 0.02 * (temperature - 25.0);
    float compVolt = averageVoltage / compCoef;

    tdsValue = (133.42 * compVolt * compVolt * compVolt 
               - 255.86 * compVolt * compVolt 
               + 857.39 * compVolt) * 0.5;

    if (tdsValue < 0) tdsValue = 0;
  }
}

// =========================================================
// 7. KONTROL DOSING & SERVO
// =========================================================
void hentikanSemuaDosing() {
  servoMineral.write(SUDUT_TERTUTUP);
  servoPHBuffer.write(SUDUT_TERTUTUP);
  dosingState = IDLE;
}

void hitungDosingMineral() {
  float targetPpm = TARGET_PPM_DOSING[modeTerpilih];
  float gramMineral = targetPpm * VOLUME_AIR_LITER * GRAM_PER_PPM_PER_LITER;

  durasiMineralSekarang  = (unsigned long)(gramMineral / LAJU_ALIR_MINERAL_G_PER_MS);
  durasiPHBufferSekarang = DURASI_PHBUFFER_MS[modeTerpilih];
}

void mulaiDosing() {
  hitungDosingMineral();
  servoMineral.write(SUDUT_TERBUKA);
  waktuMulaiAksi = millis();
  dosingState = DOSING_MINERAL;
  Serial.println(">> Status: Mulai Dosing Mineral");
}

void kelolaDosingOtomatis() {
  switch (dosingState) {
    case IDLE:
      if (tdsValue < TDS_MIN[modeTerpilih] || ph_act < PH_MIN[modeTerpilih] || ph_act > PH_MAX[modeTerpilih]) {
        mulaiDosing();
      }
      break;

    case DOSING_MINERAL:
      if (millis() - waktuMulaiAksi >= durasiMineralSekarang) {
        servoMineral.write(SUDUT_TERTUTUP);
        servoPHBuffer.write(SUDUT_TERBUKA);
        waktuMulaiAksi = millis();
        dosingState = DOSING_PHBUFFER;
        Serial.println(">> Status: Lanjut Dosing pH Buffer");
      }
      break;

    case DOSING_PHBUFFER:
      if (millis() - waktuMulaiAksi >= durasiPHBufferSekarang) {
        servoPHBuffer.write(SUDUT_TERTUTUP);
        waktuMulaiAksi = millis();
        dosingState = COOLDOWN;
        Serial.println(">> Status: Dosing Selesai, Memasuki Cooldown");
      }
      break;

    case COOLDOWN:
      if (millis() - waktuMulaiAksi >= COOLDOWN_SETELAH_DOSING) {
        dosingState = IDLE;
        Serial.println(">> Status: Cooldown Selesai, Standby");
      }
      break;
  }
}

String namaStatusDosing() {
  switch (dosingState) {
    case DOSING_MINERAL:  return "Dosing Mineral";
    case DOSING_PHBUFFER: return "Dosing pHBuf";
    case COOLDOWN:        return "Cooldown";
    default:              return "Standby";
  }
}

// =========================================================
// 8. TAMPILAN OLED
// =========================================================
void tampilkanMenu() {
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);

  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("Pilih Mode:");
  oled.drawLine(0, 10, 128, 10, SH110X_WHITE);

  oled.setTextSize(2);
  oled.setCursor(10, 20);
  oled.print(modeTerpilih == MODE_FISH ? "> Fish" : "  Fish");
  oled.setCursor(10, 42);
  oled.print(modeTerpilih == MODE_PLANT ? "> Plant" : "  Plant");

  oled.setTextSize(1);
  oled.setCursor(0, 56);
  oled.print("PILIH=ganti OK=mulai");
  oled.display();
}

void tampilkanMonitoring() {
  bool phAman    = (ph_act >= PH_MIN[modeTerpilih] && ph_act <= PH_MAX[modeTerpilih]);
  bool tdsAman   = (tdsValue >= TDS_MIN[modeTerpilih] && tdsValue <= TDS_MAX[modeTerpilih]);
  bool semuaAman = phAman && tdsAman;

  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);

  // Baris Header: Nama Mode
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("Mode: ");
  oled.println(modeTerpilih == MODE_FISH ? "FISH" : "PLANT");
  oled.drawLine(0, 10, 128, 10, SH110X_WHITE);

  // Nilai pH & TDS Live
  oled.setTextSize(2);
  oled.setCursor(0, 16);
  oled.print("pH  :");
  oled.println(ph_act, 2);

  oled.setCursor(0, 38);
  oled.print("TDS :");
  oled.println(tdsValue, 0);

  // Baris Status Paling Bawah
  oled.setTextSize(1);
  oled.setCursor(0, 56);

  if (semuaAman) {
    // Jika pH dan TDS sudah masuk rentang target
    if (modeTerpilih == MODE_FISH) {
      oled.print("IDEAL FOR FISH");
    } else {
      oled.print("IDEAL FOR PLANT");
    }
  } else {
    // Jika belum ideal, tampilkan peringatan dan status dosing/standby
    oled.print("CEK! | ");
    oled.print(namaStatusDosing());
  }

  oled.display();

  // Log ke Serial Monitor
  Serial.print("pH: "); Serial.print(ph_act, 2);
  Serial.print(" | TDS: "); Serial.print(tdsValue, 0);
  Serial.print(" | Mode: "); Serial.print(modeTerpilih == MODE_FISH ? "FISH" : "PLANT");
  Serial.print(" | Status: "); 
  if (semuaAman) {
    Serial.println(modeTerpilih == MODE_FISH ? "IDEAL FOR FISH" : "IDEAL FOR PLANT");
  } else {
    Serial.println(namaStatusDosing());
  }
}

// =========================================================
// 9. SETUP & LOOP MAIN
// =========================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  // Inisialisasi Pin Input
  pinMode(PIN_BTN_PILIH, INPUT_PULLUP);
  pinMode(PIN_BTN_OK, INPUT_PULLUP);
  pinMode(PIN_TDS_SENSOR, INPUT);

  // Inisialisasi ADC ESP32
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Inisialisasi Sensor pH
  hitungKalibrasiPH();
  Serial.println("=== Kalibrasi pH Berhasil ===");
  Serial.print("Slope: "); Serial.println(ph_slope, 4);
  Serial.print("Offset: "); Serial.println(ph_offset, 4);

  // Inisialisasi OLED
  if (!oled.begin(OLED_ALAMAT, true)) {
    Serial.println("OLED Gagal Ditemukan! Cek Alamat I2C / wiring.");
    while (true) delay(1000);
  }

  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  oled.display();

  // Inisialisasi Servo
  servoMineral.setPeriodHertz(50);
  servoMineral.attach(PIN_SERVO_MINERAL, 500, 2400);

  servoPHBuffer.setPeriodHertz(50);
  servoPHBuffer.attach(PIN_SERVO_PHBUFFER, 500, 2400);

  hentikanSemuaDosing();

  // Tampilkan UI awal
  tampilkanMenu();
}

void loop() {
  // 1. Pembacaan Sensor berkala
  updateSensorPH();
  updateSensorTDS();

  // 2. Pembacaan Input Tombol
  bool btnPilihPressed = (digitalRead(PIN_BTN_PILIH) == LOW);
  bool btnOkPressed    = (digitalRead(PIN_BTN_OK) == LOW);

  bool pilihValid = btnPilihPressed && (millis() - waktuTekanPilihTerakhir > DEBOUNCE_DELAY);
  bool okValid    = btnOkPressed && (millis() - waktuTekanOkTerakhir > DEBOUNCE_DELAY);

  // 3. Logic Alur Aplikasi
  if (appState == MENU) {
    if (pilihValid) {
      waktuTekanPilihTerakhir = millis();
      modeTerpilih = (modeTerpilih == MODE_FISH) ? MODE_PLANT : MODE_FISH;
      tampilkanMenu();
    }
    if (okValid) {
      waktuTekanOkTerakhir = millis();
      appState = MONITORING;
      dosingState = IDLE;
      Serial.print("Masuk Mode Monitoring: ");
      Serial.println(modeTerpilih == MODE_FISH ? "FISH" : "PLANT");
    }
  } 
  else if (appState == MONITORING) {
    // Navigasi Kembali ke Menu Utama
    if (pilihValid) {
      waktuTekanPilihTerakhir = millis();
      hentikanSemuaDosing();
      appState = MENU;
      tampilkanMenu();
    }
    // Dosing Manual (Trigger tombol OK saat Idle)
    else if (okValid && dosingState == IDLE) {
      waktuTekanOkTerakhir = millis();
      Serial.println(">> Trigger Manual Dosing via Tombol OK");
      mulaiDosing();
    }

    // Dosing Otomatis
    kelolaDosingOtomatis();

    // Refresh Tampilan Display Monitoring
    if (millis() - tampilTimepoint >= TAMPIL_INTERVAL) {
      tampilTimepoint = millis();
      tampilkanMonitoring();
    }
  }
}