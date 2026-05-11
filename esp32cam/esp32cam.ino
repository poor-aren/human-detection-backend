/*
  KIPAS PINTAR - ESP32-CAM
  Kirim gambar ke server tiap 10 detik
  Tampilkan jumlah manusia di Serial Monitor
*/

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// ==============================
// KONFIGURASI - UBAH DI SINI
// ==============================
const char* WIFI_SSID     = "NAMA_WIFI_KAMU";
const char* WIFI_PASSWORD = "PASSWORD_WIFI_KAMU";
const char* SERVER_URL    = "https://NAMA-APP-KAMU.onrender.com/upload";
// ==============================

// Pin kamera AI-Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== KIPAS PINTAR ===");
  Serial.println("Menginisialisasi kamera...");

  // Konfigurasi kamera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("error: kamera gagal init (0x%x)\n", err);
    return;
  }

  Serial.println("Kamera siap.");

  // Koneksi WiFi
  Serial.printf("Menghubungkan ke WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi terhubung. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("error: gagal konek WiFi");
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("error: WiFi terputus, mencoba reconnect...");
    WiFi.reconnect();
    delay(5000);
    return;
  }

  // Ambil foto
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("error: gagal ambil gambar dari kamera");
    delay(10000);
    return;
  }

  Serial.println("mengirim data");

  // Kirim ke server
  HTTPClient http;
  http.begin(SERVER_URL);
  http.setTimeout(20000);

  String boundary = "----ESP32Boundary";
  String bodyStart = "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"image\"; filename=\"frame.jpg\"\r\n"
    "Content-Type: image/jpeg\r\n\r\n";
  String bodyEnd = "\r\n--" + boundary + "--\r\n";

  int totalLen = bodyStart.length() + fb->len + bodyEnd.length();

  uint8_t* fullBody = (uint8_t*)malloc(totalLen);
  if (!fullBody) {
    Serial.println("error: memori tidak cukup");
    esp_camera_fb_return(fb);
    delay(10000);
    return;
  }

  memcpy(fullBody, bodyStart.c_str(), bodyStart.length());
  memcpy(fullBody + bodyStart.length(), fb->buf, fb->len);
  memcpy(fullBody + bodyStart.length() + fb->len, bodyEnd.c_str(), bodyEnd.length());

  esp_camera_fb_return(fb);

  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  int httpCode = http.POST(fullBody, totalLen);
  free(fullBody);

  if (httpCode == 200) {
    String response = http.getString();

    // Parse jumlah manusia dari response JSON
    // Response format: {"count": 2, "message": "2 manusia"}
    int msgIdx = response.indexOf("\"message\":\"");
    if (msgIdx != -1) {
      int start = msgIdx + 11;
      int end = response.indexOf("\"", start);
      String msg = response.substring(start, end);
      Serial.println(msg);
    } else {
      Serial.println("error: format response tidak dikenal");
    }

  } else if (httpCode > 0) {
    String errBody = http.getString();
    int errIdx = errBody.indexOf("\"error\":\"");
    if (errIdx != -1) {
      int start = errIdx + 9;
      int end = errBody.indexOf("\"", start);
      Serial.println("error: " + errBody.substring(start, end));
    } else {
      Serial.printf("error: HTTP %d\n", httpCode);
    }
  } else {
    Serial.printf("error: koneksi gagal (%s)\n", http.errorToString(httpCode).c_str());
  }

  http.end();

  delay(10000); // 10 detik
}
