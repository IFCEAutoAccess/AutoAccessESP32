#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "esp_camera.h"
#include "camera_pins.h"
#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>

#define LED 4
#define DIST_THRESHOLD_CM 100
#define TRIG_PIN 15
#define ECHO_PIN 14
#define SOUND_SPEED 0.034
#define SERVO_PIN 13

extern WebServer server;
extern Servo servo;
extern uint8_t *last_image;
extern size_t last_image_size;

void accessPoint();
void configInitCamera();
void initSensors();
float readDistanceCM();
void CORS();
void setupRotasHTTP();
void tirarFoto();
void capturarComSensor();
void enviarCameraAoVivo();

#endif
