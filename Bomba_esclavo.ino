/* === CÓDIGO ESCLAVO: BOMBA CON LÓGICA INVERTIDA === */
#include <ESP8266WiFi.h>
#include <espnow.h>

const char* ssid = "INFINITUM2D38_2.4";
const char* password = "Elgatofeo2588";

// Pin del relé (D1 / GPIO 5)
#define RELAY_PIN 5 

// Definición de la lógica invertida (Active Low)
#define RELAY_ON LOW
#define RELAY_OFF HIGH

typedef struct struct_mensaje {
  bool encender;
} struct_mensaje;

struct_mensaje mensajeRecibido;

unsigned long ultimoPaqueteRecibido = 0;
const unsigned long TIEMPO_SEGURIDAD = 5000; 

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&mensajeRecibido, incomingData, sizeof(mensajeRecibido));
  ultimoPaqueteRecibido = millis();
  
  // Aplicamos la lógica correcta de encendido/apagado
  if (mensajeRecibido.encender) {
     digitalWrite(RELAY_PIN, RELAY_ON);
  } else {
     digitalWrite(RELAY_PIN, RELAY_OFF);
  }
}

void setup() {
  Serial.begin(115200);
  
  // 1. IMPORTANTE: Definir el estado apagado ANTES de configurar el pin como salida
  digitalWrite(RELAY_PIN, RELAY_OFF); 
  pinMode(RELAY_PIN, OUTPUT);
  
  // 2. Asegurar el estado nuevamente por redundancia
  digitalWrite(RELAY_PIN, RELAY_OFF);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  if (esp_now_init() != 0) return;
  
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
  
  ultimoPaqueteRecibido = millis(); 
}

void loop() {
  unsigned long ahora = millis();

  // Protocolo de seguridad (Failsafe)
  if (digitalRead(RELAY_PIN) == RELAY_ON) {
    if (ahora - ultimoPaqueteRecibido > TIEMPO_SEGURIDAD) {
      digitalWrite(RELAY_PIN, RELAY_OFF);
    }
  }
}
