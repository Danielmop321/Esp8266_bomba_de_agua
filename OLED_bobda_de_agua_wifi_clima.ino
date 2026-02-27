/* === CÓDIGO MAESTRO DEFINITIVO: ICONOS DE ESTADO + WEB + SEGURIDAD ===
   Hardware: ESP8266 (NodeMCU 1.0 / ESP-12E)
   Librería: esp8266-oled-ssd1306 (ThingPulse)
   Novedad: Ícono "Ok" integrado en la esquina superior derecha (121, 2).
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>
#include <espnow.h>

#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"

/* ================= CONFIGURACIÓN HARDWARE ================= */
#define SDA_PIN 12
#define SCL_PIN 14
#define BTN_NAV_PIN 13
#define BTN_BOMBA_PIN 0

/* ================= DATOS WIFI Y API ================= */
const char* ssid = "INFINITUM2D38_2.4";
const char* password = "Elgatofeo2588";
const char* apiKey = "a227163bb3d00750ee0927a108f5b1e2";
const float LAT = 19.0433;
const float LON = -98.2019;

const char* ntp1 = "pool.ntp.org";
const char* ntp2 = "time.nist.gov";
const char* tzInfo = "MST6MDT,M3.2.0/2,M11.1.0/2";

/* ================= OBJETOS ================= */
SSD1306Wire display(0x3c, SDA_PIN, SCL_PIN);
OLEDDisplayUi ui(&display);
ESP8266WebServer server(80);

/* ================= VARIABLES ================= */
unsigned long ultimaHoraSync = 0;
unsigned long ultimoClimaSync = 0;
const unsigned long INTERVALO_HORA  = 86400000UL;
const unsigned long INTERVALO_CLIMA = 3600000UL;

float temperatura = 0.0;
int humedad = 0;
bool climaValido = false;
bool estadoAgua = false;

unsigned long ultimaInteraccion = 0;
const unsigned long TIEMPO_APAGADO = 60000;
bool pantallaEncendida = true;

uint8_t macBomba[] = {0xCC, 0x50, 0xE3, 0x18, 0xCC, 0x42};

typedef struct struct_mensaje {
  bool encender;
} struct_mensaje;
struct_mensaje mensaje;

unsigned long ultimoHeartbeat = 0;
const unsigned long INTERVALO_HEARTBEAT = 2000;
bool espNowConnected = false;

int ultimoEstadoNav = HIGH;
unsigned long ultimoReboteNav = 0;
int ultimoEstadoBomba = HIGH;
unsigned long ultimoReboteBomba = 0;
const unsigned long delayRebote = 50;

/* ================= ICONOS (BITMAPS) ================= */
const uint8_t activeSymbol[] PROGMEM = {B00000000, B00011000, B00111100, B01111110, B01111110, B00111100, B00011000, B00000000};
const uint8_t inactiveSymbol[] PROGMEM = {B00000000, B00011000, B00100100, B01000010, B01000010, B00100100, B00011000, B00000000};

// Iconos Lopaka
const unsigned char image_BLE_beacon_bits[] PROGMEM = {0x22, 0x49, 0x55, 0x49, 0x2a, 0x08, 0x08, 0x3e};
const unsigned char image_Pin_star_bits[] PROGMEM = {0x49, 0x2a, 0x1c, 0x7f, 0x1c, 0x2a, 0x49};
const unsigned char image_Ok_btn_bits[] PROGMEM = {0x0e,0x15,0x1f,0x15,0x0e};

/* ================= PÁGINA WEB (HTML / PWA) ================= */
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="theme-color" content="#121212">
  <title>Bomba App</title>
  <style>
    body { font-family: sans-serif; text-align: center; background: #121212; color: #eee; padding-top: 40px; margin: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh;}
    .btn { background: #007bff; border: none; color: white; padding: 25px 50px; font-size: 24px; border-radius: 50px; cursor: pointer; box-shadow: 0 4px 15px rgba(0,123,255,0.4); transition: transform 0.1s; }
    .btn:active { transform: scale(0.95); }
    .status { font-size: 28px; margin-bottom: 40px; color: #0f0; }
    h1 { margin-bottom: 10px; }
  </style>
</head>
<body>
  <h1>Panel Domótico</h1>
  <div class="status">Estado: %STATE%</div>
  <a href="/toggle"><button class="btn">ACTIVAR BOMBA</button></a>
</body>
</html>)rawliteral";

/* ================= DECLARACIONES ================= */
void frameReloj(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void frameClima(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void frameAgua(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void frameInfo(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void overlayHeader(OLEDDisplay *display, OLEDDisplayUiState* state);
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus); 

void leerBotones();
void despertarPantalla();
void enviarComandoBomba(bool toggle);
void conectarWiFi();
void sincronizarClima();
void handleRoot();
void handleToggle();

FrameCallback frames[] = { frameReloj, frameClima, frameAgua, frameInfo };
int frameCount = 4;
OverlayCallback overlays[] = { overlayHeader };
int overlaysCount = 1;

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  pinMode(BTN_NAV_PIN, INPUT_PULLUP);
  pinMode(BTN_BOMBA_PIN, INPUT_PULLUP);

  ui.setTargetFPS(60);
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(TOP);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.disableAutoTransition();
  ui.setFrames(frames, frameCount);
  ui.setOverlays(overlays, overlaysCount);
  ui.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 25, "Conectando...");
  display.display();

  conectarWiFi();

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();

  if (esp_now_init() == 0) {
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    esp_now_add_peer(macBomba, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  }

  configTime(0, 0, ntp1, ntp2);
  setenv("TZ", tzInfo, 1);
  tzset();
  sincronizarClima();

  ultimaInteraccion = millis();
}

/* ================= LOOP ================= */
void loop() {
  server.handleClient();

  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    unsigned long ahora = millis();
    leerBotones();

    if (pantallaEncendida && (ahora - ultimaInteraccion > TIEMPO_APAGADO)) {
      display.displayOff();
      pantallaEncendida = false;
    }

    if (ahora - ultimaHoraSync >= INTERVALO_HORA) { ultimaHoraSync = ahora; }
    if (ahora - ultimoClimaSync >= INTERVALO_CLIMA) {
      sincronizarClima();
      ultimoClimaSync = ahora;
    }

    if (ahora - ultimoHeartbeat >= INTERVALO_HEARTBEAT) {
      enviarComandoBomba(false);
      ultimoHeartbeat = ahora;
    }
    delay(remainingTimeBudget);
  }
}

/* ================= LÓGICA Y CALLBACKS ================= */
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  espNowConnected = (sendStatus == 0);
}

void handleRoot() {
  String html = index_html;
  html.replace("%STATE%", estadoAgua ? "ON" : "OFF");
  server.send(200, "text/html", html);
}

void handleToggle() {
  despertarPantalla();
  enviarComandoBomba(true);
  server.sendHeader("Location", "/");
  server.send(303);
}

void despertarPantalla() {
  ultimaInteraccion = millis();
  if (!pantallaEncendida) {
    display.displayOn();
    pantallaEncendida = true;
  }
}

void leerBotones() {
  int lecturaNav = digitalRead(BTN_NAV_PIN);
  if (lecturaNav != ultimoEstadoNav) ultimoReboteNav = millis();
  if ((millis() - ultimoReboteNav) > delayRebote) {
    static int estableNav = HIGH;
    if (lecturaNav != estableNav) {
      estableNav = lecturaNav;
      if (estableNav == LOW) { despertarPantalla(); ui.nextFrame(); }
    }
  }
  ultimoEstadoNav = lecturaNav;

  int lecturaBomba = digitalRead(BTN_BOMBA_PIN);
  if (lecturaBomba != ultimoEstadoBomba) ultimoReboteBomba = millis();
  if ((millis() - ultimoReboteBomba) > delayRebote) {
    static int estableBomba = HIGH;
    if (lecturaBomba != estableBomba) {
      estableBomba = lecturaBomba;
      if (estableBomba == LOW) { despertarPantalla(); enviarComandoBomba(true); }
    }
  }
  ultimoEstadoBomba = lecturaBomba;
}

void enviarComandoBomba(bool toggle) {
  if (toggle) estadoAgua = !estadoAgua;
  mensaje.encender = estadoAgua;
  esp_now_send(macBomba, (uint8_t *) &mensaje, sizeof(mensaje));
}

/* ================= FRAMES (PANTALLAS) ================= */
void frameReloj(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  char timeStr[10];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", t->tm_hour, t->tm_min);
  display->drawString(64 + x, 18 + y, timeStr);
  display->setFont(ArialMT_Plain_16);
  char dateStr[12];
  snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
  display->drawString(64 + x, 44 + y, dateStr);
}

void frameClima(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  if (!climaValido) {
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 25 + y, "Conectando...");
    return;
  }
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 18 + y, String(temperatura, 1) + "°C");
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 44 + y, "Humedad: " + String(humedad) + "%");
}

void frameAgua(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 18 + y, estadoAgua ? "ON" : "OFF");
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 44 + y, "Bomba Agua");
}

void frameInfo(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 18 + y, "Direccion IP:");
  if (WiFi.status() == WL_CONNECTED) {
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 40 + y, WiFi.localIP().toString());
  } else {
    display->drawString(64 + x, 40 + y, "Sin Conexion");
  }
}

// --- OVERLAY PARA LA BARRA DE ESTADO ---
void overlayHeader(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setColor(WHITE);

  // Ícono ESP-NOW (Izquierda)
  if (espNowConnected) {
    display->drawXbm(1, 1, 7, 8, image_BLE_beacon_bits);
  }

  // Ícono WiFi (Junto al anterior)
  if (WiFi.status() == WL_CONNECTED) {
    display->drawXbm(11, 2, 7, 7, image_Pin_star_bits);
  }

  // Ícono OK (Esquina superior derecha) indicando estado de la bomba
  if (estadoAgua) {
    display->drawXbm(121, 2, 5, 5, image_Ok_btn_bits);
  }
}

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
}

void sincronizarClima() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    char url[200];
    snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/weather?lat=%.4f&lon=%.4f&units=metric&lang=es&appid=%s", LAT, LON, apiKey);
    if (http.begin(client, url)) {
      if (http.GET() == 200) {
         StaticJsonDocument<384> doc;
         if(!deserializeJson(doc, http.getStream())) {
           temperatura = doc["main"]["temp"];
           humedad = doc["main"]["humidity"];
           climaValido = true;
         }
      }
      http.end();
    }
  }
}
