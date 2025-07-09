#include <WebServer.h>
#include <ESP32Servo.h>
#include <cstdint>  // para uint8_t
#include "functions.h"

// Definições reais das variáveis globais
WebServer server(80);
Servo servo;
uint8_t *last_image = nullptr;
size_t last_image_size = 0;

void accessPoint() {

  const char *ssid = "Auto Access";
  const char *password = "12345678";

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.setSleep(false);

  Serial.print("Access Point iniciado. IP: ");
  Serial.println(WiFi.softAPIP());

  return;
}

void configInitCamera() {

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
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro ao iniciar a câmera: 0x%x\n", err);
    return;
  } else {
    Serial.println("Câmera iniciada com sucesso");
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }

  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_special_effect(s, 2);
    s->set_framesize(s, FRAMESIZE_QVGA);
  }
}

float readDistanceCM() {
  
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // timeout 30ms
  float distance = duration * SOUND_SPEED / 2.0;
  if (duration == 0) return -1;  // falha
  return distance;
}

void initSensors() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  servo.attach(SERVO_PIN);
  servo.write(0);

}

void setupRotasHTTP() {
  server.on("/", []() {
    CORS();
    server.send(200, "application/json", "{\"message\": \"Hello from ESP32!\"}");
  });

  server.on("/set_flash_on", []() {
    CORS();
    digitalWrite(LED, HIGH);
    server.send(200, "application/json", "{\"message\": \"Flash ON\"}");
  });

  server.on("/set_flash_off", []() {
    CORS();
    digitalWrite(LED, LOW);
    server.send(200, "application/json", "{\"message\": \"Flash OFF\"}");
  });

  server.on("/debug_capture", HTTP_GET, []() {
    CORS();
    tirarFoto();  // nova função modularizada (ver abaixo)
  });

  server.on("/debug_capture_2", HTTP_GET, []() {
    CORS();
    capturarComSensor();  // nova função modularizada
  });

  server.on("/last_capture", HTTP_GET, []() {
    CORS();
    if (!last_image) {
      server.send(404, "application/json", "{\"message\": \"Nenhuma imagem capturada\"}");
      return;
    }
    server.send_P(200, "image/jpeg", (const char *)last_image, last_image_size);
  });

  server.on("/debug_camera_view", HTTP_GET, []() {
    CORS();
    enviarCameraAoVivo();  // nova função
  });

  server.onNotFound([]() {
    server.send(404, "application/json", "{\"message\": \"Rota não encontrada\"}");
  });
}

void tirarFoto() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "application/json", "{\"error\": \"Falha ao capturar imagem\"}");
    return;
  }

  if (last_image) {
    free(last_image);
    last_image = nullptr;
  }

  last_image_size = fb->len;
  last_image = (uint8_t *)malloc(last_image_size);
  if (!last_image) {
    esp_camera_fb_return(fb);
    server.send(500, "application/json", "{\"error\": \"Memória insuficiente\"}");
    return;
  }

  memcpy(last_image, fb->buf, last_image_size);
  esp_camera_fb_return(fb);
  server.send(200, "application/json", "{\"message\": \"Imagem capturada com sucesso\"}");
}

void capturarComSensor() {
  float distance = readDistanceCM();

  if (distance > 0 && distance < DIST_THRESHOLD_CM) {
    digitalWrite(LED, HIGH);
    delay(200);

    tirarFoto();  // reutiliza função
    digitalWrite(LED, LOW);
  } else {
    server.send(200, "application/json", "{\"message\": \"Objeto não detectado!!!!!!\"}");
  }
}

void enviarCameraAoVivo() {
  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) continue;

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.println();

    esp_camera_fb_return(fb);
    delay(100);
  }
}

void CORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}