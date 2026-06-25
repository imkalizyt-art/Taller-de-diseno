# Taller-de-diseno

Sistema IoT que monitorea la humedad del suelo y alerta a un jardinero mediante Telegram, con registro histórico en ThingSpeak y pronóstico de lluvia integrado.

**Integrantes:** Enzo Quiñones, Agustín Lastra, Matías Pineda, Carlos León  
**Ramo:** Taller de Diseño en Ingeniería — TEI 201  
**Profesor:** Sebastián Duarte

## ¿Qué hace el sistema?

1. El sensor capacitivo mide la humedad del suelo cada cierto intervalo
2. Si la humedad cae bajo el 35%, el ESP8266 envía una alerta por Telegram al jardinero
3. El mensaje incluye: porcentaje de humedad, valor ADC crudo, timestamp real (NTP), pronóstico de lluvia de los próximos 5 días (OpenWeatherMap) y enlace al historial
4. Si el suelo sigue seco, se envía un recordatorio cada 10 minutos
5. Cuando el suelo vuelve a estar húmedo, se confirma por Telegram
6. Todas las mediciones se registran continuamente en ThingSpeak para análisis histórico


## Hardware utilizado

 Componente  Modelo 

 Microcontrolador  ESP8266 NodeMCU V3 (ESP-12E, CP2102) 
 Sensor de humedad  Capacitive Soil Moisture Sensor V1.2 
 Módulo de carga  TP4056 con protección 
 Batería | Li-Po 3.7V 2000mAh 
 Panel solar  6V 1W 
 Placa de pruebas  Protoboard 

---

## Software y plataformas

 Herramienta  Uso 

 Arduino IDE  Desarrollo y carga del firmware 
 ThingSpeak  Dashboard y registro histórico de mediciones 
 Telegram Bot API  Envío de alertas al jardinero 
 OpenWeatherMap API  Pronóstico de lluvia 5 días 
 NTP  Timestamp real en los mensajes 

**Librerías principales:** `ESP8266WiFi`, `ThingSpeak`, `UniversalTelegramBot`, `ArduinoJson`, `NTPClient`


## Estructura del repositorio

Taller-de-diseno/
 firmware/        # Código fuente Arduino (main.ino)
 hardware/        # Esquema de conexiones y lista de componentes
diseno-3d/       # Archivos de diseño de carcasa (si aplica)
 docs/            # Documentación, capturas e informe
 testing/         # Resultados de pruebas del sistema
 FUENTES.md       # Librerías, fuentes externas e IA utilizadas
 README.md        # Este archivo

## Cómo configurar el proyecto

### 1. Instalar librerías en Arduino IDE

Ir a **Herramientas → Administrar bibliotecas** e instalar:
- `ThingSpeak` (MathWorks)
- `UniversalTelegramBot` (Brian Lough)
- `ArduinoJson` (Benoit Blanchon)
- `NTPClient` (Fabrice Weinberg)

### 2. Configurar credenciales en `firmware/main.ino`
 WiFi
const char* ssid     = "TU_RED_WIFI";
const char* password = "TU_CONTRASEÑA";

 Telegram
#define BOT_TOKEN   "TOKEN_DE_BOTFATHER"
#define CHAT_ID     "TU_CHAT_ID"

 ThingSpeak
unsigned long channelID = TU_CHANNEL_ID;
const char* writeAPIKey = "TU_API_KEY";

 OpenWeatherMap
String apiKey   = "TU_API_KEY";
String city     = "Santiago";
String countryCode = "CL";

 Calibración del sensor (ajustar según medición real)
#define SENSOR_SECO 820   // Valor ADC en aire
#define SENSOR_AGUA 390   // Valor ADC sumergido en agua


### 3. Cargar el firmware

1. Conectar el NodeMCU V3 por USB
2. Seleccionar placa: **NodeMCU 1.0 (ESP-12E Module)**
3. Puerto: el que aparezca con CP2102
4. Cargar (`Ctrl+U`)
5. Verificar en Monitor Serial (115200 baudios)

## Calibración del sensor

El sensor capacitivo entrega voltaje **inverso** a la humedad (más húmedo = menor voltaje = menor valor ADC). La calibración usa interpolación lineal:

- `SENSOR_SECO = 820` → 0% humedad (sensor en aire)
- `SENSOR_AGUA = 390` → 100% humedad (sensor en agua)

El umbral de alerta está configurado en **35%** — bajo ese valor el sistema considera que el suelo necesita riego.

## Dashboard ThingSpeak

Las mediciones se registran en tres campos:
- **Field 1:** Valor ADC crudo (0–1023)
- **Field 2:** Porcentaje de humedad calculado (0–100%)
- **Field 3:** Estado de alerta (0 = húmedo, 1 = seco)
