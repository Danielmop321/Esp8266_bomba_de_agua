# Sistema domótico de control de bomba con ESP8266 y ESP-NOW

Este proyecto consiste en un sistema distribuido de dos nodos diseñado para el control inalámbrico de una bomba de agua. Utiliza el protocolo **ESP-NOW** para una comunicación rápida y segura, integrando una interfaz OLED para monitoreo en tiempo real y una aplicación web progresiva (PWA) para el control móvil.

## Características principales del sistema
* **Comunicación de baja latencia:** Uso de ESP-NOW para evitar la dependencia total del router Wi-Fi en tareas críticas.
* **Seguridad integrada (Failsafe):** Apagado automático de la bomba si se pierde la conexión con el maestro por más de 5 segundos.
* **Interfaz enriquecida:** Pantalla OLED con reloj sincronizado por NTP, clima actual y estados de conexión mediante iconos personalizados.
* **Control remoto dual:** Operación mediante botones físicos en el hardware y a través de una interfaz web (PWA) accesible desde el celular.

## Diagrama de la arquitectura


## Requisitos de hardware
* 2x NodeMCU 1.0 (ESP-12E / ESP8266MOD).
* 1x Pantalla OLED SSD1306 (I2C).
* 1x Módulo de relevador de 5V (Active Low).
* Sensores y botones de navegación.

## Instrucciones de instalación
1. **Configuración del esclavo:** Cargar el código `Bomba_Esclavo.ino`. Asegurarse de anotar la dirección MAC del dispositivo.
2. **Configuración del maestro:** * Editar la variable `macBomba[]` con la dirección del esclavo.
   * Configurar las credenciales de Wi-Fi y la API de OpenWeatherMap.
   * Cargar el código `Bomba_Maestro.ino`.

## Bibliografía y referencias
* Arduino. (2025). *ESP8266 Arduino core’s documentation*.
* Espressif Systems. (2023). *ESP8266 hardware design guidelines*.
* Mocencahua Parraguirre, D. (2026). *Manual de usuario y documentación del sistema domótico*.
* ThingPulse. (2024). *ESP8266 OLED driver for SSD1306 displays library documentation*.
