#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "functions.h"

void setup() {
  Serial.begin(115200);

  // Inicializa o mDNS (hostname.local)
  MDNS.begin("autoaccess");

  // Configura Access Point e periféricos
  accessPoint();
  configInitCamera();
  initSensors();

  // Configura rotas HTTP
  setupRotasHTTP();

  // Inicia o servidor
  server.begin();
}

void loop() {
  server.handleClient();
  delay(2);
}


