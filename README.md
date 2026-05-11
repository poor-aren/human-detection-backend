# KIPAS PINTAR 🎯

Sistem deteksi jumlah manusia menggunakan ESP32-CAM + Roboflow YOLO + Flask.

---

## Struktur File

```
human-detection-backend/
├── app.py              ← Server utama (Flask)
├── requirements.txt    ← Library Python
├── Procfile            ← Konfigurasi Render
├── .gitignore
├── templates/
│   └── index.html      ← Tampilan web
└── esp32cam/
    └── esp32cam.ino    ← Kode Arduino untuk ESP32-CAM
```

---

## Tutorial Deploy

### 1. Upload ke GitHub

```bash
git clone https://github.com/poor-aren/human-detection-backend.git
cd human-detection-backend

# Salin semua file ke folder ini, lalu:
git add .
git commit -m "first commit"
git push origin main
```

### 2. Deploy ke Render

1. Buka [render.com](https://render.com) → daftar/login
2. Klik **New +** → pilih **Web Service**
3. Pilih **Connect a repository** → pilih repo `human-detection-backend`
4. Isi form:
   - **Name**: `kipas-pintar` (bebas)
   - **Runtime**: `Python 3`
   - **Build Command**: `pip install -r requirements.txt`
   - **Start Command**: `gunicorn app:app`
5. Klik **Create Web Service**
6. Tunggu deploy selesai (~2 menit)
7. Catat URL-nya, contoh: `https://kipas-pintar.onrender.com`

### 3. Setting ESP32-CAM

Buka file `esp32cam/esp32cam.ino` di Arduino IDE, ubah bagian ini:

```cpp
const char* WIFI_SSID     = "NAMA_WIFI_KAMU";
const char* WIFI_PASSWORD = "PASSWORD_WIFI_KAMU";
const char* SERVER_URL    = "https://kipas-pintar.onrender.com/upload";
//                           ↑ ganti dengan URL Render kamu
```

### 4. Library Arduino yang Dibutuhkan

Install via **Tools → Manage Libraries**:
- `ESP32` board package (dari Boards Manager)

Untuk board ESP32-CAM, pastikan sudah install ESP32 board:
- File → Preferences → Additional Boards Manager URLs:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Tools → Board → Boards Manager → cari `esp32` → Install

### 5. Upload ke ESP32-CAM

1. Tools → Board → ESP32 Arduino → **AI Thinker ESP32-CAM**
2. Tools → Port → pilih port yang muncul
3. Klik **Upload**
4. Buka Serial Monitor (baud: 115200)

---

## Output Serial Monitor

Normal:
```
=== KIPAS PINTAR ===
Menginisialisasi kamera...
Kamera siap.
Menghubungkan ke WiFi: NamaWifi
....
WiFi terhubung. IP: 192.168.1.x
mengirim data
2 manusia
mengirim data
0 manusia
```

Error:
```
mengirim data
error: Roboflow timeout
```

---

## Endpoint API

| Method | URL | Fungsi |
|--------|-----|--------|
| POST | `/upload` | Terima gambar dari ESP32 |
| GET | `/status` | Ambil data terbaru (dipake web) |
| GET | `/` | Halaman web dashboard |

---

## Alur Sistem

```
ESP32-CAM
  → POST /upload (gambar JPEG)
    → Flask forward ke Roboflow
      → Roboflow balik hasil deteksi
    → Flask hitung jumlah manusia
  → Response JSON ke ESP32
    → Serial Monitor tampilkan jumlah

Web Browser
  → GET /status tiap 3 detik
    → Tampilkan gambar + log + jumlah
```
