# JWS SYNCHRO CORE v3 - MASTER SIDE
**Sistem Jam Waktu Sholat Terintegrasi (ESP8266 + MIT App Inventor)**

[![Version](https://img.shields.io/badge/Version-v3.0--Stable-green)](https://github.com/elindsmkn1magelang-dev/JWS_SYNCHRO_CORE_v3_MIT_Apps)
[![Platform](https://img.shields.io/badge/Platform-ESP8266-blue)](https://github.com/esp8266/Arduino)

## 📌 Deskripsi Proyek
Proyek ini adalah sistem kontrol utama (Master Side) untuk Jam Waktu Sholat (JWS) yang dirancang khusus untuk kebutuhan edukasi di jurusan Teknik Elektronika (SMK). Versi 3 ini sudah mendukung sinkronisasi koordinat GPS melalui aplikasi Android (MIT App Inventor) dan penyimpanan permanen pada memori EEPROM.

## 🚀 Fitur Utama
* **GPS Location Sync**: Update koordinat Lintang & Bujur secara real-time via aplikasi Android.
* **Dual Time Sync**: Menggunakan RTC DS3231 untuk waktu lokal dan NTP Client untuk sinkronisasi otomatis via Internet.
* **Persistent Storage**: Koordinat tersimpan aman di EEPROM (tidak reset saat mati lampu).
* **Visual Feedback**: Indikator NeoPixel untuk status sinkronisasi (Hijau: Berhasil, Merah: Gagal).
* **Audio Automation**: Integrasi DFPlayer Mini untuk suara Tarhim otomatis sebelum waktu sholat.

## 🛠️ Skema Pinout (Wiring)
| Komponen | Pin ESP8266 (Wemos D1) | Deskripsi |
| :--- | :--- | :--- |
| **RTC DS3231** | D1 (SCL), D2 (SDA) | Komunikasi I2C Waktu |
| **NeoPixel Strip** | D6 (Data) | Output Display / Status |
| **DFPlayer Mini** | D7 (RX), D5 (TX) | Modul Suara (Software Serial) |
| **Power Supply** | 5V & G | Pastikan Amperage cukup untuk LED |

## 📦 Instalasi & Penggunaan
1. **Persiapan Library**:
   Pastikan Arduino IDE Bapak sudah terinstall library berikut:
   * `ESP8266WiFi` & `ESP8266WebServer`
   * `RTClib` (Adafruit)
   * `NTPClient`
   * `PrayerTimes`
   * `Adafruit_NeoPixel`
   * `DFRobotDFPlayerMini`

2. **Konfigurasi WiFi**:
   Ubah SSID dan Password pada baris berikut di kode utama:
   ```cpp
   const char *ssid_ntp = "Nama_WiFi_Bapak";
   const char *pass_ntp = "Password_WiFi_Bapak";
