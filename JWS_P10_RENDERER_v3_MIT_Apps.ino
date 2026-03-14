#include <DMDESP.h>
#include <fonts/SystemFont5x7Ramping.h>
#include <fonts/Arial_Black_16.h>

// --- KONFIGURASI PANEL ---
int pnlX = 1; // UBAH KE 2 JIKA BAPAK PAKAI 2 PANEL BERJEJER
int pnlY = 1; 
DMDESP Disp(pnlX, pnlY);

// --- VARIABEL DATA ---
int jam, menit, detik, tgl, bln, thn;
String jadwal[6] = {"00:00","00:00","00:00","00:00","00:00","00:00"};
String namaSholat[6] = {"IMSAK","SUBUH","DZUHUR","ASHAR","MAGRIB","ISYA"};
String pesan = "JWS SMK ELECTRONICS";
char frame[160]; // Ukuran buffer disesuaikan
byte idx = 0;
bool mulai = false;
unsigned long lastScroll, lastMode, lastRX = 0;
int scrollX;
byte mode = 0; // 0: Jam, 1: Running Text

byte checksum(String s) {
  byte cs = 0;
  for (int i = 0; i < s.length(); i++) cs ^= s[i];
  return cs;
}

// --- FUNGSI PARSING DATA DARI MASTER ---
void parseData(char* data) {
  // Pisahkan isi dengan checksum yang dipisahkan tanda '*'
  char* star = strchr(data, '*');
  if (!star) return; // Jika tidak ada tanda *, abaikan
  
  *star = '\0'; // Potong string di posisi '*'
  byte csDiterima = atoi(star + 1); // Ambil angka setelah '*'
  byte csHitung = checksum(String(data)); // Hitung ulang checksum dari data depan
  
  if (csHitung != csDiterima) return; // Jika tidak cocok, data rusak, abaikan!

  // Jika lolos verifikasi, baru lakukan parsing koma
  lastRX = millis();
  char* p = data;
  char* str;
  byte i = 0;
  while ((str = strtok_r(p, ",", &p)) != NULL) {
    if (i == 0) jam = atoi(str);
    else if (i == 1) menit = atoi(str);
    else if (i == 2) detik = atoi(str);
    else if (i == 3) tgl = atoi(str);
    else if (i == 4) bln = atoi(str);
    else if (i == 5) thn = atoi(str);
    else if (i >= 6 && i <= 11) {
      jadwal[i-6] = String(str);
      jadwal[i-6].trim();
    }
    i++;
  }
}

void bacaSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '<') { 
      mulai = true; 
      idx = 0; 
    }
    else if (c == '>') { 
      frame[idx] = '\0'; 
      parseData(frame); 
      mulai = false; 
    }
    else if (mulai && idx < 159) {
      frame[idx++] = c;
    }
    yield(); // Menjaga kestabilan ESP8266
  }
}

void setup() {
  Serial.begin(9600);
  Disp.start();
  Disp.setBrightness(40);
  scrollX = pnlX * 32;
  lastRX = 0; // Inisialisasi awal
}

void loop() {
  bacaSerial();
  Disp.loop();

  // 1. CEK KONEKSI KE MASTER (TIMEOUT 10 DETIK)
  if (millis() - lastRX > 10000) {
    Disp.clear();
    Disp.setFont(SystemFont5x7Ramping);
    Disp.drawText((pnlX * 32 - Disp.textWidth("MENCARI")) / 2, 0, "MENCARI");
    Disp.drawText((pnlX * 32 - Disp.textWidth("MASTER")) / 2, 8, "MASTER");
  } 
  
  // 2. JIKA KONEKSI OK
  else {
    // MODE 0: TAMPILAN JAM DIGITAL
    if (mode == 0) { 
      Disp.setFont(SystemFont5x7Ramping);
      char s[15]; 
      sprintf(s, "%02d:%02d:%02d", jam, menit, detik);
      
      Disp.clear(); 
      Disp.drawText((pnlX * 32 - Disp.textWidth(s)) / 2, 4, s);
      
      // Pindah ke Running Text setelah 15 detik
      if (millis() - lastMode > 15000) { 
        mode = 1; 
        lastMode = millis(); 
        scrollX = pnlX * 32; 
      } 
    }
    
    // MODE 1: RUNNING TEXT (PESAN/MOTIVASI)
    else if (mode == 1) { 
      Disp.setFont(Arial_Black_16);
      if (millis() - lastScroll > 35) { // Kecepatan scroll
        Disp.clear(); 
        Disp.drawText(scrollX, 0, pesan);
        scrollX--;
        
        // Jika teks habis, kembali ke mode Jam
        if (scrollX < -Disp.textWidth(pesan)) { 
          mode = 0; 
          lastMode = millis(); 
        }
        lastScroll = millis();
      }
    }
  }
}