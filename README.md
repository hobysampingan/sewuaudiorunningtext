# 🔊 SEWU AUDIO - Running Text Display System

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP8266-blue?style=for-the-badge&logo=espressif" alt="ESP8266">
  <img src="https://img.shields.io/badge/Display-P10%20LED-red?style=for-the-badge" alt="P10 LED">
  <img src="https://img.shields.io/badge/Version-2.1-green?style=for-the-badge" alt="Version 2.1">
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" alt="MIT License">
</p>

<p align="center">
  <b>Pure Running Text Display Controller untuk LED Matrix P10</b><br>
  <i>Tanpa RTC • 5 Info Text • WiFi AP Control • Mobile-First UI</i>
</p>

---

## ✨ Fitur Utama

| Fitur | Deskripsi |
|-------|-----------|
| 📝 **5 Running Text** | Lima slot info text dengan toggle ON/OFF individual |
| 🎚️ **Speed Control** | Kecepatan scroll 15ms - 80ms (adjustable) |
| 🔤 **2 Font Size** | Pilihan font kecil (8px) atau besar (16px) |
| ✦ **Separator** | 5 opsi pemisah antar teks (bintang, plus, garis, jeda, none) |
| 📱 **Mobile-First UI** | Interface modern dengan desain responsif |
| 📶 **WiFi AP Mode** | Access Point standalone, tidak perlu router |
| 🔄 **Auto Recovery** | WiFi watchdog dengan auto-restart |
| 💾 **Persistent Storage** | Semua setting tersimpan di SPIFFS |

---

## 🔌 Wiring Diagram

### LED P10 → NodeMCU ESP8266

```
┌─────────────────────────────────────────────────────┐
│  LED P10 Panel          NodeMCU ESP8266             │
├─────────────────────────────────────────────────────┤
│  Pin 2  (A)      →      D0  (GPIO16)                │
│  Pin 4  (B)      →      D6  (GPIO12)                │
│  Pin 8  (CLK)    →      D5  (GPIO14)                │
│  Pin 10 (SCK)    →      D3  (GPIO0)                 │
│  Pin 12 (R)      →      D7  (GPIO13)                │
│  Pin 1  (NOE)    →      D8  (GPIO15)                │
│  Pin 3  (GND)    →      GND                         │
└─────────────────────────────────────────────────────┘
```

### Power Supply

```
┌─────────────────────────────────────────────────────┐
│  ⚡ PENTING: LED P10 membutuhkan power 5V eksternal │
│                                                     │
│  Power Supply 5V 4A+ → LED P10 VCC & GND            │
│  NodeMCU             → USB atau VIN 5V              │
│                                                     │
│  ⚠️  Pastikan GND terhubung bersama!                │
└─────────────────────────────────────────────────────┘
```

---

## 📦 Library yang Dibutuhkan

Install melalui Arduino Library Manager:

| Library | Versi | Fungsi |
|---------|-------|--------|
| **HJS589** | Latest | DMD P10 driver untuk ESP8266 |
| **ArduinoJson** | 6.x | JSON parser untuk config |
| **ESP8266WiFi** | Built-in | WiFi Access Point |
| **ESP8266WebServer** | Built-in | Web server |
| **DNSServer** | Built-in | Captive portal |

### Install HJS589

```
1. Download dari: [https://github.com/nicklfretz/HJS589](https://github.com/hobysampingan/sewuaudiorunningtext/tree/main/HJS589)
2. Sketch → Include Library → Add .ZIP Library
3. Pilih file ZIP yang didownload
```

---

## 🚀 Cara Instalasi

### 1. Persiapan Arduino IDE

```
Board Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json

Board Settings:
├── Board: NodeMCU 1.0 (ESP-12E Module)
├── Upload Speed: 115200
├── CPU Frequency: 80 MHz
├── Flash Size: 4MB (FS:2MB OTA:~1019KB)
└── Port: [Pilih COM port]
```

### 2. Upload Sketch

```
1. Buka file running.ino
2. Pastikan sewuwebpage.h ada di folder yang sama
3. Klik Upload (→)
4. Tunggu hingga selesai
```

### 3. Upload File System (SPIFFS)

```
Tools → ESP8266 Sketch Data Upload
(Opsional - untuk file config awal)
```

---

## 📱 Cara Penggunaan

### 1. Koneksi WiFi

```
┌─────────────────────────────────────┐
│  📶 WiFi: SEWU AUDIO                │
│  🔑 Pass: sewuaudio123              │
│  🌐 URL:  http://192.168.4.1        │
└─────────────────────────────────────┘
```

### 2. Buka Web Interface

Setelah terhubung ke WiFi, buka browser dan akses:

```
http://192.168.4.1
```

### 3. Konfigurasi

| Tab | Fungsi |
|-----|--------|
| 📝 **Teks** | Edit nama sistem dan 5 running text |
| 🖥️ **Display** | Panel count, font, speed, brightness, separator |
| 📶 **WiFi** | Ubah SSID dan password Access Point |

---

## 🔄 Flow Display

```
┌─────────────────────────────────────────────────────┐
│                    STARTUP                          │
│                                                     │
│    ┌─────────┐         ┌─────────┐                  │
│    │  SEWU   │  2 sec  │ READY!  │  1 sec           │
│    │  AUDIO  │ ──────► │         │ ──────►          │
│    └─────────┘         └─────────┘                  │
└─────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────┐
│                  LOOP (Forever)                     │
│                                                     │
│   ★ NAMA ★ ──► ✦ ──► INFO 1 ──► ✦ ──► INFO 2       │
│       │                                    │        │
│       │         ◄── ✦ ◄── INFO 5 ◄── ✦ ◄──┘        │
│       │              │                              │
│       └──────────────┘ (loop)                       │
│                                                     │
│   ✦ = Separator (jika diaktifkan)                   │
│   INFO yang OFF akan di-skip                        │
└─────────────────────────────────────────────────────┘
```

---

## ⚙️ Konfigurasi Default

```json
{
  "nama": "SEWU AUDIO",
  "info1": "SELAMAT DATANG DI HAJATAN KAMI",
  "info2": "SELAMAT MENIKMATI HIDANGAN",
  "info3": "TERIMA KASIH ATAS KEHADIRANNYA",
  "info4": "SEWU AUDIO SOUND SYSTEM",
  "info5": "PROFESSIONAL LIGHTING SERVICE",
  "panelCount": 2,
  "speed": 35,
  "brightness": 150,
  "fontSize": 0,
  "separator": 0
}
```

---

## 🛠️ Troubleshooting

### LED P10 Tidak Menyala

```
✓ Cek power supply 5V (min 4A untuk 2 panel)
✓ Cek koneksi kabel data
✓ Pastikan GND terhubung semua
✓ Coba reset NodeMCU
```

### WiFi AP Tidak Muncul

```
✓ Tunggu 10 detik setelah power on
✓ Tekan tombol RST pada NodeMCU
✓ Cek Serial Monitor untuk log error
✓ Sistem memiliki auto-recovery (watchdog)
```

### Web Interface Tidak Bisa Diakses

```
✓ Pastikan sudah konek ke WiFi "SEWU AUDIO"
✓ Buka http://192.168.4.1 (bukan https)
✓ Clear browser cache
✓ Coba browser lain
```

### Teks Tidak Scroll

```
✓ Pastikan minimal 1 info text ON
✓ Cek setting font size
✓ Restart device
```

---

## 📁 Struktur File

```
running/
├── running.ino        # Main sketch
├── sewuwebpage.h      # Web interface (minified)
└── README.md          # Dokumentasi
```

---

## 📋 Spesifikasi Teknis

| Parameter | Nilai |
|-----------|-------|
| MCU | ESP8266 (NodeMCU) |
| Display | P10 LED Matrix (1-4 panel) |
| Resolution | 32×16 per panel |
| Color | Single color (Red/Green/Blue) |
| WiFi Mode | Access Point (AP) |
| Web Interface | Responsive HTML5 |
| Storage | SPIFFS (JSON config) |
| Power | 5V DC |

---

## 🎨 Web Interface Preview

```
┌─────────────────────────────────────┐
│  🔊 SEWU AUDIO                      │
│  ● Aktif: 4/5 │ Panel: 2 │ 35ms    │
├─────────────────────────────────────┤
│  [📝 Teks] [🖥️ Display] [📶 WiFi]  │
├─────────────────────────────────────┤
│  ● URUTAN TAMPIL                    │
│  📢NAMA ✦ 1 ✦ 2 ✦ 3 ✦ 4 ✦ 5        │
├─────────────────────────────────────┤
│  🏷️ Nama Sistem                    │
│  ┌───────────────────────────────┐  │
│  │ SEWU AUDIO                    │  │
│  └───────────────────────────────┘  │
│         [💾 Simpan Nama]            │
├─────────────────────────────────────┤
│  📝 Running Text                    │
│  ┌───────────────────────────────┐  │
│  │ 1  Info 1              [ON]  │  │
│  ├───────────────────────────────┤  │
│  │ Selamat datang...            │  │
│  │                       45/199 │  │
│  └───────────────────────────────┘  │
│  ...                                │
│         [💾 Simpan Semua Teks]      │
└─────────────────────────────────────┘
```

---

## 📜 Changelog

### v2.1 (2024-12-19)
- ✅ Minified HTML/CSS/JS untuk optimasi memory
- ✅ Separator antar teks (5 opsi)
- ✅ 5 slot info text dengan toggle ON/OFF
- ✅ Mobile-first responsive design
- ✅ Textarea input untuk teks panjang
- ✅ Character counter
- ✅ Live preview urutan teks

### v2.0
- ✅ Dihapus RTC (running text only)
- ✅ Font size selector
- ✅ Speed control
- ✅ Panel count 1-4

### v1.0
- 🚀 Initial release

---

## 👨‍💻 Author

**SEWU AUDIO**  
*Sound System & Lighting Professional*

---

## 📄 License

```
MIT License

Copyright (c) 2024 SEWU AUDIO

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

---

<p align="center">
  Made with ❤️ by <b>SEWU AUDIO</b><br>
  <i>Sound System & Lighting</i>
</p>
