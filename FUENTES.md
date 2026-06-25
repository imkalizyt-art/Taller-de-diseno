# FUENTES DEL PROYECTO

Este archivo registra las librerías, códigos externos y herramientas de inteligencia artificial utilizadas en el desarrollo del proyecto.

---

## 1. Librerías utilizadas

 Librería  Versión  Enlace oficial  Uso en el proyecto 
 ESP8266WiFi  integrada en ESP8266 Arduino Core 3.1.2  https://github.com/esp8266/Arduino  Conexión del NodeMCU V3 a red WiFi para envío de datos 
 ThingSpeak  2.0.0  https://github.com/mathworks/thingspeak-arduino  Envío y visualización histórica de lecturas de humedad del suelo 
 UniversalTelegramBot  1.3.0  https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot  Envío de alertas con porcentaje de humedad, timestamp NTP y recomendación de riego 
 ArduinoJson  6.21.5  https://arduinojson.org  Requerida por UniversalTelegramBot para parsear respuestas de la API de Telegram 
 ESP8266HTTPClient  integrada en ESP8266 Arduino Core 3.1.2  https://github.com/esp8266/Arduino  Solicitudes HTTP a la API de OpenWeatherMap para pronóstico de lluvia 
 NTPClient  3.2.1  https://github.com/arduino-libraries/NTPClient  Obtención de timestamp UTC real para incluir en mensajes de alerta 
 WiFiUdp  integrada en ESP8266 Arduino Core 3.1.2  https://github.com/esp8266/Arduino  Requerida por NTPClient para comunicación UDP con servidor de tiempo 

---

## 2. Código externo adaptado

### Conexión WiFi con reintentos

 **Fuente:** documentación oficial de ESP8266 Arduino Core  
  https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
 **Adaptación realizada:** se implementó un bucle de reintento con límite de 20 intentos y mensaje de error por Serial. Si no conecta, el sistema no intenta enviar datos para evitar bloqueos.

### Lectura del sensor capacitivo y calibración

 **Fuente:** datasheet Capacitive Soil Moisture Sensor V1.2  
  https://www.sigmaelectronica.net/manuals/SMTMS-2-4.pdf
 **Adaptación realizada:** se utilizó interpolación lineal entre dos valores de calibración (`SENSOR_SECO = 820`, `SENSOR_AGUA = 390`) obtenidos midiendo el sensor en aire y sumergido en agua. La fórmula convierte el valor ADC (0–1023) a porcentaje de humedad (0–100%).

### Envío de datos a ThingSpeak

- **Fuente:** documentación oficial ThingSpeak para Arduino  
  https://www.mathworks.com/help/thingspeak/use-arduino-client-to-write-to-channel.html
- **Adaptación realizada:** se modificó para enviar tres campos: valor ADC crudo (Field 1), porcentaje de humedad calculado (Field 2) y estado de alerta en binario 0/1 (Field 3). Se agregó manejo de error cuando el canal no está disponible.

### Envío de alertas por Telegram

 **Fuente:** repositorio y ejemplos de UniversalTelegramBot  
  https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/tree/master/examples
 **Adaptación realizada:** se implementaron tres tipos de mensaje: alerta inicial de suelo seco (incluye % humedad, valor ADC, timestamp NTP, pronóstico de lluvia OpenWeatherMap, recomendación de riego y enlace a ThingSpeak), recordatorio periódico cada 10 minutos mientras el suelo sigue seco, y confirmación cuando el suelo vuelve a estar húmedo. Se agregó guardia anti-spam con `millis()` para evitar inundación de mensajes.

### Consulta de pronóstico de lluvia

 **Fuente:** documentación de OpenWeatherMap API 5-day forecast  
  https://openweathermap.org/forecast5
 **Adaptación realizada:** se consulta el endpoint `/forecast` con la ubicación del jardín y se parsea la respuesta JSON para extraer la probabilidad de lluvia (`pop`) de los próximos 5 días. El sistema recomienda regar solo en días sin lluvia prevista.

---

# 3. Uso de Inteligencia Artificial

 Diseño de arquitectura del sistema

 *Herramienta:** Claude (Anthropic) — claude.ai
 *Fecha:* junio de 2026
 *Uso:* plantear la arquitectura de comunicación entre sensor capacitivo, ESP8266, ThingSpeak, OpenWeatherMap y Telegram Bot.
 *Adaptación:* el equipo ajustó la estructura según el hardware real disponible (NodeMCU V3, sensor V1.2, batería Li-Po con TP4056 y panel solar 6V 1W).
 *Comprensión:* cada componente de la arquitectura fue revisado y comprendido antes de implementarlo en el prototipo físico.

# Desarrollo, depuración y explicación del código

 **Herramienta:** Claude (Anthropic) — claude.ai
 **Fecha:** junio de 2026
 **Uso:** generación, explicación y corrección del código principal (`main.ino`), incluyendo migración de ESP32 a ESP8266, implementación del sistema de alertas Telegram con tres estados, integración NTP, y sistema anti-spam con `millis()`.
- **Adaptación:** el equipo definió los valores de calibración (`SENSOR_SECO`, `SENSOR_AGUA`) mediante medición real, ajustó el umbral de humedad al 35% según las necesidades del cultivo, y configuró las credenciales WiFi, tokens de Telegram, ThingSpeak y OpenWeatherMap.
 **Comprensión:** el funcionamiento fue verificado mediante Monitor Serial de Arduino IDE, dashboard de ThingSpeak y recepción real de alertas en Telegram.



# 4. Hardware utilizado

Componente  Modelo / Especificación  Datasheet / Referencia 

 Microcontrolador  ESP8266 NodeMCU V3 (ESP-12E, CP2102)  https://www.nodemcu.com/index_en.html 
 Sensor de humedad  Capacitive Soil Moisture Sensor V1.2  https://www.sigmaelectronica.net/manuals/SMTMS-2-4.pdf 
 Módulo de carga  TP4056 con protección  https://dlnmh9ip6v2uc.cloudfront.net/datasheets/Prototyping/TP4056.pdf 
 Batería  Li-Po 3.7V 2000mAh 
 Panel solar  6V 1W 
 Placa de pruebas  Protoboard 830 puntos 
