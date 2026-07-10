# Taller-de-diseno

Sistema IoT que monitorea la humedad del suelo y alerta a un jardinero mediante Telegram. Las mediciones se envían a un dashboard web personalizado alojado en Railway y se almacenan en una base de datos PostgreSQL de Aiven. El sistema también utiliza OpenWeatherMap para consultar el pronóstico de lluvia y mantiene ThingSpeak como respaldo del historial.

**Integrantes:** Enzo Quiñones, Agustín Lastra, Matías Pineda, Carlos León  
**Ramo:** Taller de Diseño en Ingeniería — TEI 201  
**Profesor:** Sebastián Duarte  

## ¿Qué hace el sistema?

**Problema que resuelve:** permite conocer la humedad real del suelo y entregar información útil para decidir cuándo regar, evitando depender solamente de la observación visual o de horarios fijos.

1. El sensor capacitivo mide la humedad del suelo cada 30 segundos.
2. El ESP8266 convierte la lectura ADC en un porcentaje de humedad.
3. Si la humedad cae bajo el 35 %, el suelo se considera seco.
4. Cada medición se envía al dashboard personalizado mediante una API alojada en Railway.
5. Railway guarda los datos en una base PostgreSQL de Aiven para mantener un historial.
6. El dashboard muestra la humedad actual, el valor ADC, el estado del suelo, estadísticas y un gráfico histórico.
7. Cuando el suelo está seco, el ESP8266 envía una alerta por Telegram al jardinero.
8. El mensaje incluye el porcentaje de humedad, el valor ADC, la hora, la temperatura, el pronóstico de lluvia y un enlace al dashboard.
9. Si el suelo continúa seco, se envía un recordatorio cada 10 minutos.
10. Cuando el suelo vuelve a estar húmedo, se envía una confirmación por Telegram.
11. ThingSpeak se mantiene como respaldo secundario de las mediciones.

## Hardware utilizado

| Componente | Modelo |
|---|---|
| Microcontrolador | ESP8266 NodeMCU V3 (ESP-12E, CP2102) |
| Sensor de humedad | Capacitive Soil Moisture Sensor V1.2 |
| Módulo de carga | TP4056 con protección |
| Batería | Li-Po 3.7 V, 2000 mAh |
| Panel solar | 6 V, 1 W |
| Placa de pruebas | Protoboard |
| Indicador | LED conectado al pin D5 |

## Software y plataformas

| Herramienta | Uso |
|---|---|
| Arduino IDE | Desarrollo y carga del firmware |
| Railway | Hosting del backend y del dashboard personalizado |
| Aiven PostgreSQL | Almacenamiento persistente de las mediciones |
| Flask | Backend y API del dashboard |
| Chart.js | Gráfico histórico de humedad |
| Telegram Bot API | Envío de alertas al jardinero |
| OpenWeatherMap API | Pronóstico de lluvia |
| ThingSpeak | Respaldo secundario del historial |
| NTP | Fecha y hora utilizadas por el sistema |

Librerías principales del firmware: `ESP8266WiFi`, `ESP8266HTTPClient`, `WiFiClientSecure`, `ThingSpeak`, `UniversalTelegramBot`, `ArduinoJson` y `NTPClient`.

Dependencias principales del dashboard: `Flask`, `gunicorn` y `psycopg`.

## Estructura del repositorio

```text
Taller-de-diseno/
├── dashboard/          # Página web, API y conexión con Aiven
├── firmware/           # Código fuente del ESP8266
├── hardware/           # Esquema de conexiones y lista de componentes
├── diseno-3d/          # Archivos de Fusion 360, planos y renders
├── docs/               # Documentación y evidencias
├── testing/            # Resultados y protocolo de pruebas
├── FUENTES.md          # Librerías, fuentes externas e IA utilizadas
└── README.md           # Descripción e instrucciones del proyecto
```

## Cómo configurar el proyecto

### 1. Instalar librerías en Arduino IDE

Ir a **Herramientas → Administrar bibliotecas** e instalar:

- `ThingSpeak` de MathWorks.
- `UniversalTelegramBot` de Brian Lough.
- `ArduinoJson` de Benoit Blanchon.
- `NTPClient` de Fabrice Weinberg.

Las librerías `ESP8266WiFi`, `ESP8266HTTPClient` y `WiFiClientSecure` se incluyen con el paquete de placas ESP8266.

### 2. Configurar credenciales en el firmware

En el archivo del firmware deben completarse las siguientes variables:

```cpp
const char* WIFI_SSID = "TU_RED_WIFI";
const char* WIFI_PASSWORD = "TU_CONTRASENA_WIFI";

const char* TELEGRAM_TOKEN = "TU_TOKEN_TELEGRAM";
const char* TELEGRAM_CHAT_ID = "TU_CHAT_ID";

unsigned long TS_CHANNEL_ID = TU_CHANNEL_ID;
const char* TS_WRITE_KEY = "TU_API_KEY_THINGSPEAK";

const char* OWM_API_KEY = "TU_API_KEY_OPENWEATHERMAP";

const char* DEVICE_API_KEY = "TU_DEVICE_API_KEY_DE_RAILWAY";
```

La clave `DEVICE_API_KEY` debe ser exactamente igual a la variable configurada en Railway. Las credenciales reales no deben subirse al repositorio público.

El firmware envía las mediciones a:

```text
https://taller-de-diseno-production.up.railway.app/api/mediciones
```

### 3. Configurar el dashboard

El dashboard se encuentra en la carpeta `dashboard/`.

En Railway se deben configurar estas variables:

```text
DATABASE_URL
DEVICE_API_KEY
HUMIDITY_THRESHOLD=35
ONLINE_WINDOW_MINUTES=15
```

- `DATABASE_URL` corresponde a la URI privada entregada por Aiven.
- `DEVICE_API_KEY` debe coincidir con la clave escrita en el firmware.
- El directorio raíz del servicio en Railway debe ser `/dashboard`.

Dashboard público:

**https://taller-de-diseno-production.up.railway.app**

### 4. Cargar el firmware

1. Conectar el NodeMCU V3 mediante USB.
2. Seleccionar la placa **NodeMCU 1.0 (ESP-12E Module)**.
3. Seleccionar el puerto correspondiente al conversor CP2102.
4. Verificar y cargar el código.
5. Abrir el Monitor Serial a 115200 baudios.
6. Comprobar que aparezcan las lecturas del sensor y el mensaje de envío correcto al dashboard.
7. Abrir el dashboard y verificar que la medición nueva aparezca en la página.

## Calibración del sensor

El sensor capacitivo entrega una lectura inversa a la humedad:

- Un valor ADC mayor representa un suelo más seco.
- Un valor ADC menor representa un suelo más húmedo.

La configuración actual es:

```cpp
const int SENSOR_SECO = 850;
const int SENSOR_AGUA = 380;
const int UMBRAL_RIEGO = 35;
```

La conversión se realiza mediante interpolación lineal:

- `SENSOR_SECO = 850` corresponde a 0 % de humedad.
- `SENSOR_AGUA = 380` corresponde a 100 % de humedad.
- Bajo 35 %, el sistema considera que el suelo necesita riego.

Para recalibrar el sensor se debe medir el valor ADC al aire y luego con la zona sensible en agua, reemplazando los valores anteriores por los resultados obtenidos.

## Dashboard personalizado

El dashboard fue desarrollado por el equipo y es la interfaz principal del sistema:

**https://taller-de-diseno-production.up.railway.app**

La página muestra:

- Humedad actual del suelo.
- Valor ADC.
- Estado seco o humedad adecuada.
- Recomendación de riego.
- Fecha y hora de la última medición.
- Promedio, mínimo y máximo de las últimas 24 horas.
- Cantidad de mediciones registradas.
- Gráfico histórico de humedad.
- Línea de referencia del umbral del 35 %.
- Estado de conexión del ESP8266.

El flujo principal de datos es:

```text
Sensor capacitivo → ESP8266 → API en Railway → PostgreSQL en Aiven → Dashboard personalizado
```

ThingSpeak no se utiliza como dashboard principal; se conserva únicamente como respaldo secundario del historial.
