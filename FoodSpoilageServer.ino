

/********************************************************
 * ESP32-CAM Food Spoilage Detection
 * Camera: AI-THINKER ESP32-CAM
 * Blynk 2.0 + Flask + TensorFlow Lite
 ********************************************************/

#define BLYNK_TEMPLATE_ID   "TMPL6hEpzyt5M"
#define BLYNK_TEMPLATE_NAME "Food Spoilage Detection"
#define BLYNK_AUTH_TOKEN    "pSat8aStEhdlVT7Ek2T6I_yunAweOqud"

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <ArduinoJson.h>

// ---------------- WIFI ----------------
const char* ssid     = "add your WIFI name";
const char* password = "add your WIFI passowrd";

// ---------------- SERVER ----------------
const char* serverURL = "http://192.168.0.104:5000/upload";
unsigned long lastCapture = 0;
const unsigned long captureInterval = 60000; // 1 minute

// ---------------- CAMERA MODEL ----------------
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
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
#endif

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Food Spoilage Detection...");

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  // Camera
  if (!initCamera()) {
    Serial.println("Camera Failed");
    ESP.restart();
  }

  Serial.println("System Ready");
}

// ---------------- LOOP ----------------
void loop() {
  Blynk.run();

  if (millis() - lastCapture > captureInterval) {
    lastCapture = millis();
    captureAndSend();
  }
}

// ---------------- CAMERA INIT ----------------
bool initCamera() {
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
  config.pin_reset   = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  return esp_camera_init(&config) == ESP_OK;
}

// ---------------- CAPTURE & SEND ----------------
void captureAndSend() {
  Serial.println("Capturing image...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Capture failed");
    return;
  }

  HTTPClient http;
  WiFiClient client;

  http.begin(client, serverURL);
  http.addHeader("Content-Type", "image/jpeg");

  int code = http.POST(fb->buf, fb->len);
  if (code > 0) {
    String response = http.getString();
    Serial.println("Server: " + response);
    handlePrediction(response);
  } else {
    Serial.println("HTTP Failed");
  }

  http.end();
  esp_camera_fb_return(fb);
}

// ---------------- HANDLE RESPONSE ----------------
void handlePrediction(String json) {
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, json)) return;

  String cls = doc["predicted_class"];
  String status, recipe;
  int severity = 0;

  if (cls == "Spoiled_Apples") {
    status = "Spoiled Apple";
    severity = 2;
    Blynk.logEvent("food_spoilage_alert", "Spoiled Apple detected!");

  } else if (cls == "Slightly_Spoiled_Apples") {
    status = "Slightly Spoiled Apple";
    recipe = "Make jam / pie / smoothie";
    severity = 1;
    Blynk.logEvent("food_warning", "Apple slightly spoiled. Cook it!");

  } else if (cls == "Fresh_Apples") {
    status = "Fresh Apple";

  } else if (cls == "Spoiled_Banana") {
    status = "Spoiled Banana";
    severity = 2;
    Blynk.logEvent("food_spoilage_alert", "Spoiled Banana detected!");

  } else if (cls == "Slightly_Spoiled_Banana") {
    status = "Slightly Spoiled Banana";
    recipe = "Banana bread / smoothie";
    severity = 1;
    Blynk.logEvent("food_warning", "Banana slightly spoiled. Use it!");

  } else if (cls == "Fresh_Banana") {
    status = "Fresh Banana";
  }

  Blynk.virtualWrite(V0, status);
  Blynk.virtualWrite(V1, recipe);
  Blynk.virtualWrite(V2, severity);

  Serial.println("Updated Blynk");
}
