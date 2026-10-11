/*
 * Program Uji Coba Pompa Air Akuarium Mini 5V
 * Menggunakan ESP32 + Modul JZ MOS
 */

// Tentukan pin GPIO ESP32 yang terhubung ke pin TRIG / SIG pada modul JZ MOS
const int PUMP_PIN = 23; 

void setup() {
  // Inisialisasi Serial Monitor untuk debugging
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=========================================");
  Serial.println("  Pengujian Kontrol Pompa Air - ESP32    ");
  Serial.println("=========================================");

  // Set pin GPIO sebagai OUTPUT
  pinMode(PUMP_PIN, OUTPUT);
  
  // Memastikan pompa dalam keadaan MATI saat awal menyala
  digitalWrite(PUMP_PIN, LOW);
  Serial.println("Status: System Ready. Pompa OFF.");
  delay(2000);
}

void loop() {
  // 1. Menyalakan Pompa
  Serial.println("--> Menyalakan Pompa (HIGH)...");
  digitalWrite(PUMP_PIN, HIGH);
  delay(3000); // Nyala selama 3 detik

  // 2. Mematikan Pompa
  Serial.println("--> Mematikan Pompa (LOW)...");
  digitalWrite(PUMP_PIN, LOW);
  delay(5000); // Mati selama 5 detik
}