/*
  =========================================================
  KODE INTEGRASI: Sensor pH + TDS, OLED SH1106 1.3", 
                  2 Servo 360° DS04-NFC, 2 Button, 
                  & Water Pump (MOSFET JZ MOS)
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
#define PIN_PUMP            23  // Pin TRIG / SIG Modul JZ MOS
#define PIN_BTN_PILIH       27
#define PIN_BTN_OK          14

// Konfigurasi OLED SH1106
#define LEBAR_OLED  128
#define TINGGI_OLED 64
#define OLED_ALAMAT 0x3C // Ganti ke 0x3D jika OLED tidak menyala
Adafruit_SH1106G oled(LEBAR_OLED, TINGGI_OLED, &Wire, -1);

// Konfigurasi Servo Continuous 360° DS04-NFC
Servo servoMineral;
Servo servoPHBuffer;

// Nilai Kontrol Servo 360
const int KECEPATAN_TERBUKA = 0;   // Putar CCW
const int KECEPATAN_DIAM    = 90;  // Stop

// =========================================================
// 2. ENUMERASI & STATE MACHINE
// =========================================================
enum ModeAir { MODE_FISH = 0, MODE_PLANT = 1, MODE_MONITORING = 2 };
enum AppState { MENU, MONITORING };
enum DosingState { IDLE, DOSING_MINERAL, DOSING_PHBUFFER, PUMPING, COOLDOWN };

ModeAir modeTerpilih = MODE_FISH;
AppState appState = MENU;
DosingState dosingState = IDLE;

// =========================================================
// 3. PARAMETER & TARGET DOSING
// =========================================================
// Index [0] = FISH, Index [1] = PLANT, Index [2] = MONITORING (Dummy)
const float TDS_MIN[3]             = { 190.0, 130.0, 0.0 };
const float TDS_MAX[3]             = { 300.0, 300.0, 9999.0 };
const float PH_MIN[3]              = { 2.0,   2.0,   0.0 };
const float PH_MAX[3]              = { 12.0,  12.0,  14.0 };
const float TARGET_PPM_DOSING[3]   = { 207.5, 150.0, 0.0 };
const unsigned long DURASI_PHBUFFER_MS[3] = { 300, 400, 0 };

// Parameter Kalibrasi & Durasi Pompa
const float VOLUME_AIR_LITER            = 0.63; 
const float GRAM_PER_PPM_PER_LITER      = 0.6 / 218.0;
const float LAJU_ALIR_MINERAL_G_PER_MS  = 0.0012; 
const unsigned long DURASI_PUMP_MS       = 5000;  // Durasi pompa air menyala (5000 ms = 5 detik)

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

// Variable Dosing & Pompa
unsigned long waktuMulaiAksi = 0;
unsigned long durasiMineralSekarang = 0;
unsigned long durasiPHBufferSekarang = 0;
const unsigned long COOLDOWN_SETELAH_DOSING = 2500;

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

      // Sorting bubble sort
      for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 10; j++) {
          if (sortedBuf[i] > sortedBuf[j]) {
            int temp = sortedBuf[i];
            sortedBuf[i] = sortedBuf[j];
            sortedBuf[j] = temp;
          }
        }
      }

      // Rata-rata nilai tengah
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
  else return (bTab[iFilterLen / 2] + bTab[(iFilterLen / 2) - 1]) / 2;
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
// 7. KONTROL DOSING, SERVO, & POMPA (MOSFET)
// =========================================================
void tampilkanMenu(); // Forward declaration

void hentikanSemuaSistem() {
  servoMineral.write(KECEPATAN_DIAM);
  servoPHBuffer.write(KECEPATAN_DIAM);
  digitalWrite(PIN_PUMP, LOW); // Matikan pompa air seketika
  dosingState = IDLE;
}

void hitungDosingMineral() {
  durasiMineralSekarang  = 5000; // Manual set 5 detik
  durasiPHBufferSekarang = DURASI_PHBUFFER_MS[modeTerpilih];
}

void mulaiDosing() {
  if (modeTerpilih == MODE_MONITORING) return;

  hitungDosingMineral();
  servoMineral.write(KECEPATAN_TERBUKA);
  waktuMulaiAksi = millis();
  dosingState = DOSING_MINERAL;
  Serial.println(">> Status: Mulai Dosing Mineral");
}

void kelolaDosingOtomatis() {
  if (modeTerpilih == MODE_MONITORING) {
    hentikanSemuaSistem();
    return;
  }

  int tdsInt = (int)(tdsValue + 0.5); // Pembulatan integer TDS

  bool phAman    = (ph_act >= PH_MIN[modeTerpilih] && ph_act <= PH_MAX[modeTerpilih]);
  bool tdsAman   = (tdsInt >= (int)TDS_MIN[modeTerpilih] && tdsInt <= (int)TDS_MAX[modeTerpilih]);
  bool airSesuai = phAman && tdsAman;

  switch (dosingState) {
    case IDLE:
      if (airSesuai) {
        // Jika parameter air sudah sesuai, langsung jalankan pompa!
        waktuMulaiAksi = millis();
        digitalWrite(PIN_PUMP, HIGH); // Nyalakan Pompa Air
        dosingState = PUMPING;
        Serial.println(">> Status: Air Sesuai! Memulai Pompa Air...");
      } 
      else {
        mulaiDosing();
      }
      break;

    case DOSING_MINERAL:
      if (millis() - waktuMulaiAksi >= durasiMineralSekarang) {
        servoMineral.write(KECEPATAN_DIAM);
        servoPHBuffer.write(KECEPATAN_TERBUKA);
        waktuMulaiAksi = millis();
        dosingState = DOSING_PHBUFFER;
        Serial.println(">> Status: Lanjut Dosing pH Buffer");
      }
      break;

    case DOSING_PHBUFFER:
      if (millis() - waktuMulaiAksi >= durasiPHBufferSekarang) {
        servoPHBuffer.write(KECEPATAN_DIAM);
        waktuMulaiAksi = millis();
        dosingState = COOLDOWN;
        Serial.println(">> Status: Dosing Selesai, Memasuki Cooldown");
      }
      break;

    case PUMPING:
      if (millis() - waktuMulaiAksi >= DURASI_PUMP_MS) {
        // SETELAH PUMP SELESAI -> LANGSUNG RESET KEMBALI KE MENU AWAL
        hentikanSemuaSistem();
        appState = MENU;
        tampilkanMenu();
        Serial.println(">> Status: Pompa Selesai -> Langsung Kembali ke Menu Awal!");
      }
      break;

    case COOLDOWN:
      if (millis() - waktuMulaiAksi >= COOLDOWN_SETELAH_DOSING) {
        dosingState = IDLE;
        Serial.println(">> Status: Standby kembali");
      }
      break;
  }
}

String namaStatusDosing() {
  if (modeTerpilih == MODE_MONITORING) return "MONITOR ONLY";

  switch (dosingState) {
    case DOSING_MINERAL:  return "Dosing Mineral";
    case DOSING_PHBUFFER: return "Dosing pHBuf";
    case PUMPING:         return "PUMPING...";
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
  oled.drawLine(0, 9, 128, 9, SH110X_WHITE);

  oled.setTextSize(1);
  oled.setCursor(5, 15);
  oled.print(modeTerpilih == MODE_FISH ? "> 1. Mode Fish" : "  1. Mode Fish");
  
  oled.setCursor(5, 28);
  oled.print(modeTerpilih == MODE_PLANT ? "> 2. Mode Plant" : "  2. Mode Plant");

  oled.setCursor(5, 41);
  oled.print(modeTerpilih == MODE_MONITORING ? "> 3. Only Monitor" : "  3. Only Monitor");

  oled.setCursor(0, 56);
  oled.print("PILIH=ganti OK=mulai");
  oled.display();
}

void tampilkanMonitoring() {
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);

  // Tampilan saat sedang memompa air
  if (dosingState == PUMPING && modeTerpilih != MODE_MONITORING) {
    oled.setTextSize(1);
    oled.setCursor(10, 8);
    oled.println("AIR SUDAH SESUAI");
    oled.drawLine(0, 20, 128, 20, SH110X_WHITE);

    oled.setTextSize(1);
    oled.setCursor(5, 32);
    oled.println("SEDANG PROSES PUMP");
    oled.setCursor(10, 48);
    oled.println("PIN27 = BATAL / MENU");
  } 
  else {
    // Tampilan standar monitoring pH & TDS
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.print("Mode: ");
    if (modeTerpilih == MODE_FISH) oled.println("FISH");
    else if (modeTerpilih == MODE_PLANT) oled.println("PLANT");
    else oled.println("MONITOR ONLY");
    
    oled.drawLine(0, 10, 128, 10, SH110X_WHITE);

    // Nilai pH & TDS Live
    oled.setTextSize(2);
    oled.setCursor(0, 16);
    oled.print("pH  :");
    oled.println(ph_act, 2);

    oled.setCursor(0, 38);
    oled.print("TDS :");
    oled.println(tdsValue, 0);

    // Status Bawah
    oled.setTextSize(1);
    oled.setCursor(0, 56);
    oled.print("ST : ");
    oled.print(namaStatusDosing());
  }

  oled.display();

  // Log ke Serial Monitor
  Serial.print("pH: "); Serial.print(ph_act, 2);
  Serial.print(" | TDS: "); Serial.print(tdsValue, 0);
  Serial.print(" | State: "); Serial.println(namaStatusDosing());
}

// =========================================================
// 9. SETUP & LOOP MAIN
// =========================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  // Inisialisasi Pin Input & Output
  pinMode(PIN_BTN_PILIH, INPUT_PULLUP);
  pinMode(PIN_BTN_OK, INPUT_PULLUP);
  pinMode(PIN_TDS_SENSOR, INPUT);
  
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW); // Memastikan pompa MATI saat booting

  // Inisialisasi ADC ESP32
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Inisialisasi Sensor pH
  hitungKalibrasiPH();

  // Inisialisasi OLED
  if (!oled.begin(OLED_ALAMAT, true)) {
    Serial.println("OLED Gagal Ditemukan!");
    while (true) delay(1000);
  }

  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  oled.display();

  // Inisialisasi Servo Continuous 360°
  servoMineral.setPeriodHertz(50);
  servoMineral.attach(PIN_SERVO_MINERAL, 500, 2500);

  servoPHBuffer.setPeriodHertz(50);
  servoPHBuffer.attach(PIN_SERVO_PHBUFFER, 500, 2500);

  hentikanSemuaSistem();
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
      if (modeTerpilih == MODE_FISH) modeTerpilih = MODE_PLANT;
      else if (modeTerpilih == MODE_PLANT) modeTerpilih = MODE_MONITORING;
      else modeTerpilih = MODE_FISH;

      tampilkanMenu();
    }
    if (okValid) {
      waktuTekanOkTerakhir = millis();
      appState = MONITORING;
      dosingState = IDLE;
      Serial.print("Masuk Mode: ");
      if (modeTerpilih == MODE_FISH) Serial.println("FISH");
      else if (modeTerpilih == MODE_PLANT) Serial.println("PLANT");
      else Serial.println("ONLY MONITORING");
    }
  } 
  else if (appState == MONITORING) {
    // ABORT KEMBALI KE MENU (PIN 27)
    if (pilihValid) {
      waktuTekanPilihTerakhir = millis();
      hentikanSemuaSistem();
      appState = MENU;
      tampilkanMenu();
      Serial.println(">> Diinterupsi via PIN 27: Kembali ke Menu Utama");
    }
    // Trigger Manual Dosing via OK (Kecuali Mode Only Monitoring)
    else if (okValid && dosingState == IDLE && modeTerpilih != MODE_MONITORING) {
      waktuTekanOkTerakhir = millis();
      mulaiDosing();
    }

    // Kelola Dosing & Memompa Otomatis
    kelolaDosingOtomatis();

    // Refresh Display (Hanya jika masih dalam mode MONITORING)
    if (appState == MONITORING && (millis() - tampilTimepoint >= TAMPIL_INTERVAL)) {
      tampilTimepoint = millis();
      tampilkanMonitoring();
    }
  }
}