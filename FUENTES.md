# FUENTES DEL PROYECTO

Este archivo registra las librerías, servicios, códigos externos adaptados y herramientas de inteligencia artificial utilizadas en el desarrollo del sistema de monitoreo de humedad del suelo.

---

## 1. Librerías utilizadas

| Librería, dependencia o servicio | Versión | Enlace oficial | Uso en el proyecto |
|---|---:|---|---|
| ESP8266 Arduino Core (`ESP8266WiFi`, `ESP8266HTTPClient`, `WiFiClient`, `WiFiClientSecure` y `WiFiUDP`) | 3.1.2 | https://github.com/esp8266/Arduino | Conexión Wi-Fi del NodeMCU, solicitudes HTTP/HTTPS, comunicación con el dashboard y conexión NTP |
| ThingSpeak | 2.0.0 | https://github.com/mathworks/thingspeak-arduino | Respaldo secundario de las mediciones de humedad, estado del suelo y lluvia pronosticada |
| UniversalTelegramBot | 1.3.0 | https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot | Envío de alertas, recordatorios y confirmaciones de riego por Telegram |
| ArduinoJson | 6.21.5 | https://arduinojson.org | Lectura de respuestas de OpenWeatherMap y creación del JSON enviado a Railway |
| NTPClient | 3.2.1 | https://github.com/arduino-libraries/NTPClient | Obtención de fecha y hora para los mensajes del sistema |
| Flask | `>=3.0,<4` | https://flask.palletsprojects.com | Backend del dashboard, recepción de mediciones y creación de las rutas de la API |
| Psycopg | `>=3.1,<4` | https://www.psycopg.org/psycopg3/docs/ | Conexión entre la aplicación Flask y PostgreSQL de Aiven |
| Gunicorn | `>=22,<24` | https://gunicorn.org | Ejecución del backend Flask en Railway |
| Chart.js | 4.4.7 | https://www.chartjs.org/docs/latest/ | Gráfico histórico de humedad y línea del umbral de riego |
| Railway | Servicio web | https://docs.railway.com | Hosting del backend y del dashboard personalizado |
| Aiven for PostgreSQL | PostgreSQL 17 | https://aiven.io/docs/products/postgresql | Almacenamiento persistente del historial de mediciones |
| OpenWeatherMap | 5 Day / 3 Hour Forecast API | https://openweathermap.org/forecast5 | Consulta de temperatura y lluvia pronosticada |

---

## 2. Código externo adaptado

### Conexión Wi-Fi y solicitudes de red

- **Fuente:** documentación oficial de ESP8266 Arduino Core.  
  https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
- **Adaptación realizada:** se incorporó la conexión del NodeMCU a la red Wi-Fi, control del estado de conexión y uso de clientes HTTP y HTTPS para comunicarse con ThingSpeak, Telegram, OpenWeatherMap y Railway.

### Lectura y calibración del sensor capacitivo

- **Fuente:** documentación del Capacitive Soil Moisture Sensor V1.2.  
  https://www.sigmaelectronica.net/manuals/SMTMS-2-4.pdf
- **Adaptación realizada:** la lectura ADC se transforma en porcentaje mediante interpolación lineal entre `SENSOR_SECO = 850` y `SENSOR_AGUA = 380`. El umbral de riego se fijó en 35 % según las pruebas realizadas con el sensor.

### Envío de datos a ThingSpeak

- **Fuente:** documentación oficial de ThingSpeak para Arduino.  
  https://www.mathworks.com/help/thingspeak/use-arduino-client-to-write-to-channel.html
- **Adaptación realizada:** se configuró el envío de humedad, estado del suelo y lluvia pronosticada. ThingSpeak se conserva como respaldo secundario y ya no corresponde al dashboard principal del proyecto.

### Alertas mediante Telegram y consulta del clima

- **Fuentes:** ejemplos de UniversalTelegramBot y documentación de OpenWeatherMap.  
  https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/tree/master/examples  
  https://openweathermap.org/forecast5
- **Adaptación realizada:** se implementaron alertas de suelo seco, recordatorios cada 10 minutos y confirmación cuando el suelo vuelve a estar húmedo. Los mensajes incluyen humedad, lectura ADC, hora, temperatura, pronóstico de lluvia, recomendación y enlace al dashboard personalizado.

### Dashboard personalizado, API y base de datos

- **Fuentes:** documentación oficial de Flask, Psycopg, Chart.js, Railway y Aiven.  
  https://flask.palletsprojects.com  
  https://www.psycopg.org/psycopg3/docs/  
  https://www.chartjs.org/docs/latest/  
  https://docs.railway.com  
  https://aiven.io/docs/products/postgresql
- **Adaptación realizada:** se creó una página propia para mostrar humedad actual, ADC, estado del suelo, recomendación, última actualización, promedio, mínimo, máximo y gráfico histórico. Flask recibe las mediciones del ESP8266, valida la clave `DEVICE_API_KEY` y las almacena en PostgreSQL de Aiven. La aplicación se encuentra desplegada en Railway.

### Integración del ESP8266 con el dashboard

- **Fuente:** documentación de `ESP8266HTTPClient`, `WiFiClientSecure` y ArduinoJson.
- **Adaptación realizada:** se agregó al firmware la función `enviarADashboard()`, que crea un objeto JSON con humedad, ADC, estado seco/húmedo, temperatura, lluvia y nombre del dispositivo. Luego envía una solicitud HTTPS `POST` a la API de Railway utilizando el encabezado privado `X-API-Key`.
- **Estado de validación:** el código fue integrado al firmware. El envío desde el dispositivo debe verificarse nuevamente al cargar la versión final en el ESP8266.

---

## 3. Uso de Inteligencia Artificial

### Diseño inicial y desarrollo del firmware

- **Herramienta:** Claude, de Anthropic.
- **Fecha:** junio de 2026.
- **Prompt utilizado:** se solicitó plantear y explicar un sistema de monitoreo de humedad con ESP8266, sensor capacitivo, ThingSpeak, Telegram, NTP y OpenWeatherMap.
- **Uso:** apoyo en la arquitectura inicial, migración a ESP8266, generación y depuración del firmware, mensajes de Telegram y control anti-spam con `millis()`.
- **Adaptación:** el equipo configuró el hardware real, las credenciales, la ubicación de OpenWeatherMap, los tiempos de medición, los valores de calibración y el umbral de 35 %.
- **Comprensión:** el equipo comprende la lectura ADC, la conversión a porcentaje, el control del LED, la lógica de alertas y la consulta del clima.

### Desarrollo del dashboard personalizado

- **Herramienta:** ChatGPT, de OpenAI.
- **Fecha:** julio de 2026.
- **Prompt utilizado:** “Crear una página web propia como dashboard para el sistema de humedad, alojarla en Railway y guardar las mediciones en PostgreSQL de Aiven, sin depender de los gráficos genéricos de ThingSpeak”.
- **Uso:** generación de la estructura inicial de `app.py`, HTML, CSS, JavaScript, conexión con PostgreSQL, rutas de la API, estadísticas y gráfico histórico.
- **Adaptación:** se utilizaron los datos reales del proyecto: humedad, ADC, estado del suelo, temperatura y lluvia. Se fijó el umbral en 35 %, se configuró el dominio público de Railway y se conectó la base de datos creada por el equipo en Aiven.
- **Comprensión:** el equipo comprende que el ESP8266 envía un JSON por HTTPS, Flask valida la clave, PostgreSQL guarda la medición y la página consulta las rutas de la API para actualizar las tarjetas y el gráfico.

### Integración del firmware y actualización de documentación

- **Herramienta:** ChatGPT, de OpenAI.
- **Fecha:** julio de 2026.
- **Prompts utilizados:** se solicitó integrar el firmware existente con la nueva API sin eliminar las configuraciones originales, y actualizar `README.md` y `FUENTES.md` de acuerdo con la rúbrica.
- **Uso:** incorporación de `enviarADashboard()`, cambio de los enlaces de Telegram al dashboard propio y actualización de la documentación del repositorio.
- **Adaptación:** se mantuvieron la calibración, los pines, los tiempos, Telegram, OpenWeatherMap y ThingSpeak del código base. Solo se agregó la comunicación con Railway/Aiven y se actualizó la documentación para reflejar el sistema actual.
- **Comprensión:** el equipo puede explicar la estructura JSON, el encabezado `X-API-Key`, las rutas de la API, el uso de variables privadas en Railway y la función de Aiven como repositorio persistente.

---

## 4. Hardware utilizado

| Componente | Modelo o especificación | Datasheet o referencia |
|---|---|---|
| Microcontrolador | ESP8266 NodeMCU V3, ESP-12E, CP2102 | https://www.nodemcu.com/index_en.html |
| Sensor de humedad | Capacitive Soil Moisture Sensor V1.2 | https://www.sigmaelectronica.net/manuals/SMTMS-2-4.pdf |
| Módulo de carga | TP4056 con protección | https://dlnmh9ip6v2uc.cloudfront.net/datasheets/Prototyping/TP4056.pdf |
| Batería | Li-Po 3.7 V, 2000 mAh | Componente adquirido por el equipo |
| Panel solar | 6 V, 1 W | Componente adquirido por el equipo |
| Placa de pruebas | Protoboard | Componente utilizado en el montaje |
