#include "esp_camera.h"
#include "camera_pins.h"
#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

const int flashPin = 4;

const char *ssid = "CCJ2G";
const char *password = "Cmj106815#";

WebServer server(80);
void CORS();

void setup(void) {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // CAMERA SETUP
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;
  // Local dos buffers e modo de captura
    // Inicializa a câmera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro ao iniciar a camera: 0x%x\n", err);
  } else {
    Serial.println("Câmera iniciada com sucesso");
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

  // PIN SETUP
  pinMode(flashPin, OUTPUT);


  // WIFI SETUP
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("autoaccess")) {
    Serial.println("MDNS responder started");
  }

  // GET ( "/" ")
  server.on("/", []() {
    CORS();
    server.send(200, "application/json", "{\"message\": \"Hello from ESP32!\"}");
  });

  server.on("/set_flash_on", []() {
    CORS();
    server.send(200, "application/json", "{\"message\": \"Flash pin is ON\"}");
    digitalWrite(flashPin, HIGH);
  });
  
  server.on("/set_flash_off", []() {
    CORS();
    server.send(200, "application/json", "{\"message\": \"Flash pin is OFF\"}");
    digitalWrite(flashPin, LOW);
  });

  server.on("/camera", HTTP_GET, []() {
    CORS();
    camera_fb_t *fb = esp_camera_fb_get();

    if(!fb) {
      server.send(500, "application/json", "{\"error\": \"Failed to capture image\"}");
      return;
    }

    server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  });

  // GET ( "*" )
  server.onNotFound([]() {
    server.send(404, "application/json", "{\"message\": \"Page Not Found\"}");
  });

  // BEGIN SERVER
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient(); // LISTEN REQUESTS
  delay(2);
}

void CORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}