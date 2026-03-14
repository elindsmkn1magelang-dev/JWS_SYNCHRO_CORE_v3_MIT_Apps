/*
 * =======================================================================================
 * PROJECT: JWS SYNCHRO CORE - MASTER SIDE
 * VERSION: v1.2 (Stable, GPS Sync & EEPROM Storage)
 * AUTHOR: Bapak Guru - SMK Electronics Engineering
 * =======================================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <RTClib.h>
#include <PrayerTimes.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <EEPROM.h>  // Tambahan untuk memori permanen

// --- KONFIGURASI PERMANEN ---
struct ConfigLoc {
  double lat;
  double lon;
  uint32_t valid;
};
ConfigLoc userLoc;
#define EEPROM_ADDR 0
#define EEPROM_VALID_KEY 12345

// --- KONFIGURASI PIN & OBJEK ---
#define PIXEL_PER_SEGMENT 2
#define PIXEL_DIGITS 4
#define PIXEL_PIN D6
#define PIXEL_DASH 1
#define SDA_PIN D2
#define SCL_PIN D1

SoftwareSerial mp3Serial(D7, D5);
DFRobotDFPlayerMini myDFPlayer;
Adafruit_NeoPixel strip = Adafruit_NeoPixel((PIXEL_PER_SEGMENT * 7 * PIXEL_DIGITS) + (PIXEL_DASH * 2), PIXEL_PIN, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);
RTC_DS3231 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.nist.gov", 25200, 60000);

const char *ssid_ntp = "SSID KAMU";
const char *pass_ntp = "PASSWORD SSIDMU";

// --- VARIABEL GLOBAL ---
int Hour, Minute, Second, Day, Month, Year;
double prayerTimes[7];
byte wheelPos = 0;
byte digits[] = { 0b1111110, 0b0011000, 0b0110111, 0b0111101, 0b1011001, 0b1101101, 0b1101111, 0b0111000, 0b1111111, 0b1111101 };

// --- 1. FUNGSI UPDATE JADWAL SHOLAT (MENGGUNAKAN KOORDINAT EEPROM) ---
void update_Prayers() {
  DateTime now = rtc.now();
  set_calc_method(Karachi);
  set_asr_method(Shafii);
  set_fajr_angle(20.0);
  set_isha_angle(18.0);
  // Gunakan userLoc.lat dan userLoc.lon dari EEPROM
  get_prayer_times(now.year(), now.month(), now.day(), userLoc.lat, userLoc.lon, 7, prayerTimes);
}

// --- 2. HANDLER TERIMA DATA DARI APK MIT APP INVENTOR ---
void handleUpdateLocation() {
  if (server.hasArg("Lat") && server.hasArg("Lon")) {
    userLoc.lat = server.arg("Lat").toDouble();
    userLoc.lon = server.arg("Lon").toDouble();
    userLoc.valid = EEPROM_VALID_KEY;

    // Simpan permanen
    EEPROM.put(EEPROM_ADDR, userLoc);
    EEPROM.commit();

    update_Prayers();  // Hitung ulang jadwal sholat

    server.send(200, "text/plain", "Update Berhasil Tersimpan!");

    // Feedback visual: Hijau kedip
    for (int i = 0; i < 3; i++) {
      strip.fill(strip.Color(0, 255, 0));
      strip.show();
      delay(150);
      strip.clear();
      strip.show();
      delay(100);
    }
  } else {
    server.send(400, "text/plain", "Gagal: Data Tidak Lengkap");
  }
}

// --- FUNGSI ANIMASI & NTP (TETAP SAMA) ---
void aniSync() {
  static int pos = 0;
  strip.clear();
  for (int i = 0; i < 3; i++) {
    int p = (pos + i) % strip.numPixels();
    strip.setPixelColor(p, strip.Color(0, 150, 255));
  }
  strip.show();
  pos++;
  delay(25);
}

void syncTimeNTP() {
  WiFi.begin(ssid_ntp, pass_ntp);
  unsigned long startWait = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWait < 15000) {
    aniSync();
    yield();
  }
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.begin();
    if (timeClient.update()) {
      unsigned long t = timeClient.getEpochTime();
      rtc.adjust(DateTime(year(t), month(t), day(t), hour(t), minute(t), second(t)));
      for (int i = 0; i < 2; i++) {
        strip.fill(strip.Color(0, 255, 0));
        strip.show();
        delay(300);
        strip.clear();
        strip.show();
        delay(200);
      }
    }
    timeClient.end();
  } else {
    for (int i = 0; i < 2; i++) {
      strip.fill(strip.Color(255, 0, 0));
      strip.show();
      delay(300);
      strip.clear();
      strip.show();
      delay(200);
    }
  }
  WiFi.mode(WIFI_AP_STA);  // Set kembali ke mode AP agar APK bisa terhubung
  WiFi.softAP("JWS-SMK-ENGINEERING", "12345678");
}

void sendDataToSlave(int h, int m, int s) {
  DateTime now = rtc.now();
  int sh, sm, dh, dm, ah, am, mh, mm, ih, im;
  get_float_time_parts(prayerTimes[0], sh, sm);
  get_float_time_parts(prayerTimes[2], dh, dm);
  get_float_time_parts(prayerTimes[3], ah, am);
  get_float_time_parts(prayerTimes[5], mh, mm);
  get_float_time_parts(prayerTimes[6], ih, im);

  // Kalibrasi Ikhtiyat sesuai standar Bapak
  sm += 3; dm += 4; am += 3; mm += 6; im += 2;
  if (sm >= 60) { sh++; sm -= 60; }
  if (dm >= 60) { dh++; dm -= 60; }
  if (am >= 60) { ah++; am -= 60; }
  if (mm >= 60) { mh++; mm -= 60; }
  if (im >= 60) { ih++; im -= 60; }

  // 1. Buat isi data ke dalam buffer
  char content[150];
  sprintf(content, "%d,%d,%d,%d,%d,%d,%02d:%02d,%02d:%02d,%02d:%02d,%02d:%02d,%02d:%02d,%02d:%02d",
          h, m, s, now.day(), now.month(), now.year(),
          sh, (sm - 10 < 0 ? sm + 50 : sm - 10), // Imsak
          sh, sm, dh, dm, ah, am, mh, mm, ih, im);

  // 2. Hitung Checksum dari 'content' menggunakan logika XOR
  byte cs = 0;
  for (int i = 0; content[i] != '\0'; i++) {
    cs ^= content[i];
  }

  // 3. Kirim ke Slave dengan format: <DATA*CHECKSUM>
  Serial.print("<");
  Serial.print(content);
  Serial.print("*");
  Serial.print(cs);
  Serial.println(">");
}

uint32_t colorWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void writeDigitRainbow(int index, int val) {
  int margin = (index >= 2) ? (PIXEL_DASH * 2) : 0;
  int offset = index * (PIXEL_PER_SEGMENT * 7) + margin;
  byte digit = digits[val];
  for (int i = 0; i < 7; i++) {
    uint32_t segCol = colorWheel((wheelPos + (i * 15) + (index * 40)) & 255);
    if (digit & (0x40 >> i)) {
      for (int j = 0; j < PIXEL_PER_SEGMENT; j++) strip.setPixelColor(offset + (i * PIXEL_PER_SEGMENT) + j, segCol);
    }
  }
}

// --- 6. SETUP ---
void setup() {
  Serial.begin(9600);
  mp3Serial.begin(9600);

  // EEPROM Load
  EEPROM.begin(512);
  EEPROM.get(EEPROM_ADDR, userLoc);
  if (userLoc.valid != EEPROM_VALID_KEY) {
    userLoc.lat = -**********;  // Sesuaikan dengan lat wilayahmu
    userLoc.lon = ***********;  // Sesuaikan dengan long wilayahmu
    userLoc.valid = EEPROM_VALID_KEY;
  }

  // Web Server & AP Mode
  WiFi.softAP("JWS-SMK-ENGINEERING", "12345678");
  server.on("/set", handleUpdateLocation);
  server.begin();

  if (!myDFPlayer.begin(mp3Serial)) {
    strip.fill(strip.Color(255, 100, 0));
    strip.show();
    delay(1000);
  }
  myDFPlayer.volume(25);
  strip.begin();
  strip.setBrightness(120);
  strip.show();

  for (int i = 0; i < 50; i++) { aniSync(); }

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!rtc.begin()) {
    strip.fill(strip.Color(255, 0, 0));
    strip.show();
    while (1)
      ;
  }
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  update_Prayers();
  syncTimeNTP();
}

void loop() {
  server.handleClient();  // Pantau request dari APK setiap saat
  DateTime now = rtc.now();

  if (now.year() < 2025) return;

  if (Second != now.second()) {
    Second = now.second();
    Minute = now.minute();
    Hour = now.hour();
    sendDataToSlave(Hour, Minute, Second);
    checkTarhimAll();
    if (Hour == 0 && Minute == 0 && Second == 0) update_Prayers();
    if (Hour == 2 && Minute == 0 && Second == 0) syncTimeNTP();
  }

  wheelPos++;
  strip.clear();
  writeDigitRainbow(0, Hour / 10);
  writeDigitRainbow(1, Hour % 10);
  writeDigitRainbow(2, Minute / 10);
  writeDigitRainbow(3, Minute % 10);

  if (Second % 2 == 0) {
    int base = 2 * (7 * PIXEL_PER_SEGMENT);
    for (int i = 0; i < PIXEL_DASH * 2; i++) strip.setPixelColor(base + i, colorWheel(wheelPos + 128));
  }
  strip.show();
  delay(20);
}

void checkTarhimAll() {
  int waktuSholatIdx[] = { 0, 2, 3, 5, 6 };
  for (int i = 0; i < 5; i++) {
    int sh, sm;
    get_float_time_parts(prayerTimes[waktuSholatIdx[i]], sh, sm);
    if (waktuSholatIdx[i] == 0) sm += 3;
    else if (waktuSholatIdx[i] == 2) sm += 4;
    else if (waktuSholatIdx[i] == 3) sm += 3;
    else if (waktuSholatIdx[i] == 5) sm += 6;
    else if (waktuSholatIdx[i] == 6) sm += 2;

    if (sm >= 60) {
      sh++;
      sm -= 60;
    }

    int tarhimH = sh;
    int tarhimM = sm - 10;
    if (tarhimM < 0) {
      tarhimH--;
      tarhimM += 60;
    }

    if (Hour == tarhimH && Minute == tarhimM && Second == 0) {
      myDFPlayer.play(1);
    }
  }
}
