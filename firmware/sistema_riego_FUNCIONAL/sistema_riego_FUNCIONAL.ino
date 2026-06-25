// ============================================================
//  SISTEMA INTELIGENTE DE RIEGO — TEI201 — AVANCE #3
//  ESP8266 NodeMCU V3 + Sensor Capacitivo V1.2 + Li-Po + Solar
//
//  Ciclo del sistema:
//    Sensor detecta suelo seco
//    → Alerta por Telegram al jardinero (con humedad, hora, clima)
//    → Datos enviados a ThingSpeak (dashboard e historial)
//    → Pronóstico de lluvia para recomendar días de riego
//
//  Hardware:
//    - ESP8266 NodeMCU V3 (ESP-12E, CP2102)
//    - Capacitive Soil Moisture Sensor V1.2
//    - Li-Po 3.7V 2000mAh (103450) + Módulo TP4056
//    - Panel Solar 6V 1W
//
//  Librerías necesarias (instalar desde Arduino IDE):
//    - ThingSpeak by MathWorks       (v2.0.0+)
//    - ArduinoJson by bblanchon      (v6.x)
//    - UniversalTelegramBot by Brian Lough (v1.3.0+)
//    - WiFiClientSecure (incluida en ESP8266 board package)
//    - NTPClient by Fabrice Weinberg (v3.2.1+)
// ============================================================

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include "ThingSpeak.h"

// ── CREDENCIALES WiFi ──────────────────────────────────────
const char* WIFI_SSID     = "iPhone de Agustin";
const char* WIFI_PASSWORD = "lastra10";

// ── TELEGRAM ───────────────────────────────────────────────
// 1. Habla con @BotFather en Telegram → /newbot → copia el token
// 2. Habla con @userinfobot → copia tu chat_id numérico
const char* TELEGRAM_TOKEN   = "8388957685:AAHvpDwxRTstP5iNCmUD0ooh8jR0Jnyo0Wc";
const char* TELEGRAM_CHAT_ID = "1428052556";  // ID del jardinero

// ── THINGSPEAK ─────────────────────────────────────────────
// Crea canal en thingspeak.com con 3 fields:
//   Field 1: Humedad Suelo (%)
//   Field 2: Estado Riego (0/1)
//   Field 3: Lluvia 24
//   Field 4: Temp. Ambiente (°C)
unsigned long TS_CHANNEL_ID = 3416600;          // Tu Channel ID
const char*   TS_WRITE_KEY  = "WV3IN2BZ99MKBBLT"; // Write API Key
// URL pública del canal para incluir en mensajes de Telegram
const char*   TS_CANAL_URL  = "https://thingspeak.com/channels/3416600"; // Reemplaza con tu Channel ID

// ── OPENWEATHERMAP ─────────────────────────────────────────
// Plan gratuito en openweathermap.org/api → "5 Day / 3 Hour Forecast"
const char* OWM_API_KEY = "5d91382e693ec863e947ad57fbcc002a";
const char* OWM_CITY    = "Santiago";
const char* OWM_COUNTRY = "CL";

// ── PINES (ESP8266 NodeMCU V3) ─────────────────────────────
// El NodeMCU solo tiene UN pin analógico: A0
// El sensor capacitivo se conecta directamente a A0
const int PIN_SENSOR = A0;

// Pin digital para LED indicador (opcional, pin D5 = GPIO14)
const int PIN_LED = D5;

// ── CALIBRACIÓN DEL SENSOR CAPACITIVO V1.2 ─────────────────
// IMPORTANTE: Estos valores varían según tu sensor.
// Procedimiento de calibración:
//   1. Con sensor al aire (seco) → anota el valor del Serial Monitor → SENSOR_SECO
//   2. Con sensor en agua       → anota el valor del Serial Monitor → SENSOR_AGUA
// El ESP8266 tiene ADC de 10 bits → rango 0–1023
const int SENSOR_SECO  = 850;   // Valor ADC con sensor al aire
const int SENSOR_AGUA  = 380;   // Valor ADC con sensor en agua
const int UMBRAL_RIEGO = 35;    // % de humedad bajo el cual el suelo se considera seco

// ── TIEMPOS ────────────────────────────────────────────────
const unsigned long INTERVALO_SENSOR = 30000;    // Leer sensor cada 30 seg
const unsigned long INTERVALO_CLIMA  = 3600000;  // Consultar clima cada 1 hora
// Anti-spam: mínimo 10 min entre alertas Telegram para evitar notificaciones repetidas
const unsigned long MIN_ENTRE_ALERTAS = 600000;

// ── ZONA HORARIA ───────────────────────────────────────────
// Chile Continental (UTC-3 en verano / UTC-4 en invierno)
// Ajusta el offset según la época del año
const long UTC_OFFSET_SEGUNDOS = -3 * 3600; // UTC-3

// ── ESTADO GLOBAL ──────────────────────────────────────────
bool   estadoAnterior        = true;   // true=húmedo al inicio (evita falsa alerta al arrancar)
float  lluviaProxima24h      = 0.0;   // mm de lluvia proyectados próximas 24h
String diasRecomendadosRiego = "";    // Texto con días sin lluvia → buenos para regar
String climaDescripcion      = "";    // Descripción del clima actual (ej. "Cielo despejado")
float  tempActual            = 0.0;   // Temperatura actual en °C

unsigned long ultimaLecturaSensor = 0;
unsigned long ultimaConsultaClima = 0;
unsigned long ultimaAlertaTelegram = 0;

// ── CLIENTES DE RED ────────────────────────────────────────
WiFiClient        clienteHTTP;       // Para ThingSpeak (HTTP)
WiFiClientSecure  clienteSeguro;     // Para Telegram (HTTPS)

// Bot de Telegram
UniversalTelegramBot bot(TELEGRAM_TOKEN, clienteSeguro);

// Cliente NTP para obtener hora real
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_SEGUNDOS, 60000);

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // ESP8266 requiere deshabilitar verificación SSL para Telegram
  // (limitación de memoria del chip; aceptable en contexto académico)
  clienteSeguro.setInsecure();

  Serial.println("\n╔══════════════════════════════╗");
  Serial.println("║  SISTEMA DE RIEGO INTELIGENTE  ║");
  Serial.println("╚══════════════════════════════╝");

  conectarWiFi();
  ThingSpeak.begin(clienteHTTP);

  // Iniciar cliente de hora
  timeClient.begin();
  timeClient.update();

  // Consultar clima al arrancar
  consultarClima();

  Serial.println("[SISTEMA] Listo. Umbral de riego: " + String(UMBRAL_RIEGO) + "%");
  Serial.println("[SISTEMA] Monitoreando cada " + String(INTERVALO_SENSOR / 1000) + " segundos");
}

// ============================================================
//  LOOP PRINCIPAL
// ============================================================
void loop() {
  unsigned long ahora = millis();

  // ── 1. Leer sensor y procesar ───────────────────────────
  if (ahora - ultimaLecturaSensor >= INTERVALO_SENSOR) {
    ultimaLecturaSensor = ahora;
    timeClient.update();

    int   rawADC  = analogRead(PIN_SENSOR);
    float humedad = calcularHumedad(rawADC);
    bool  esSeco  = (humedad < UMBRAL_RIEGO);

    // Actualizar LED: encendido = seco, apagado = húmedo
    digitalWrite(PIN_LED, esSeco ? HIGH : LOW);

    // Log en Serial Monitor
    Serial.println("──────────────────────────────────");
    Serial.println("[SENSOR] ADC      : " + String(rawADC));
    Serial.println("[SENSOR] Humedad  : " + String(humedad, 1) + "%");
    Serial.println("[SENSOR] Estado   : " + String(esSeco ? "SECO ⚠️" : "HÚMEDO ✅"));
    Serial.println("[HORA]   " + obtenerHoraFormateada());

    // Enviar datos a ThingSpeak siempre (para historial continuo)
    enviarAThingSpeak(humedad, esSeco ? 0 : 1, lluviaProxima24h);

    // ── Detectar cambio de estado húmedo → seco ──────────
    bool alertaPermitida = (ahora - ultimaAlertaTelegram >= MIN_ENTRE_ALERTAS);

    if (esSeco && !estadoAnterior && alertaPermitida) {
      // Suelo acaba de secarse → enviar alerta al jardinero
      enviarAlertaTelegram(humedad, rawADC);
      ultimaAlertaTelegram = ahora;
    } else if (esSeco && estadoAnterior && alertaPermitida) {
      // Suelo sigue seco pero aún no se ha regado → recordatorio cada 10 min
      enviarRecordatorioTelegram(humedad);
      ultimaAlertaTelegram = ahora;
    } else if (!esSeco && estadoAnterior) {
      // Suelo pasó de seco a húmedo → confirmar al jardinero
      enviarConfirmacionRiego(humedad);
    }

    estadoAnterior = esSeco;
  }

  // ── 2. Actualizar pronóstico del clima (cada hora) ──────
  if (ahora - ultimaConsultaClima >= INTERVALO_CLIMA || ultimaConsultaClima == 0) {
    ultimaConsultaClima = ahora;
    consultarClima();
  }
}

// ============================================================
//  FUNCIONES DE TELEGRAM
// ============================================================

/**
 * enviarAlertaTelegram(humedad, rawADC)
 * Mensaje principal cuando el suelo detecta por primera vez que está seco.
 * Incluye: humedad, hora, clima, pronóstico y link al dashboard.
 */
void enviarAlertaTelegram(float humedad, int rawADC) {
  Serial.println("[Telegram] Enviando alerta de suelo seco...");

  String mensaje = "🌱 *ALERTA DE RIEGO*\n";
  mensaje += "──────────────────\n";
  mensaje += "⚠️ El suelo está *SECO* y necesita agua\n\n";

  mensaje += "📊 *Estado del suelo:*\n";
  mensaje += "   💧 Humedad actual: *" + String(humedad, 1) + "%*\n";
  mensaje += "   📉 Umbral de riego: " + String(UMBRAL_RIEGO) + "%\n";
  mensaje += "   🔢 Lectura sensor (ADC): " + String(rawADC) + "\n\n";

  mensaje += "🕐 *Hora de detección:*\n";
  mensaje += "   " + obtenerHoraFormateada() + "\n\n";

  mensaje += "🌤 *Clima actual (" + String(OWM_CITY) + "):*\n";
  if (climaDescripcion.length() > 0) {
    mensaje += "   " + climaDescripcion + " — " + String(tempActual, 1) + "°C\n";
    mensaje += "   🌧 Lluvia próximas 24h: *" + String(lluviaProxima24h, 1) + " mm*\n\n";
  } else {
    mensaje += "   (sin datos de clima disponibles)\n\n";
  }

  // Recomendación inteligente basada en lluvia esperada
  mensaje += "💡 *Recomendación:*\n";
  if (lluviaProxima24h >= 5.0) {
    mensaje += "   ⏳ Esperar — lluvia prevista de " + String(lluviaProxima24h, 1) + " mm en 24h\n";
    mensaje += "   Puedes postergar el riego.\n\n";
  } else if (lluviaProxima24h > 0 && lluviaProxima24h < 5.0) {
    mensaje += "   🤔 Lluvia leve prevista (" + String(lluviaProxima24h, 1) + " mm)\n";
    mensaje += "   Insuficiente — se recomienda regar igualmente.\n\n";
  } else {
    mensaje += "   ✅ Regar lo antes posible — sin lluvia prevista.\n\n";
  }

  // Días buenos para regar (sin lluvia) en los próximos 5 días
  if (diasRecomendadosRiego.length() > 0) {
    mensaje += "📅 *Mejores días para regar (próx. 5 días):*\n";
    mensaje += "   " + diasRecomendadosRiego + "\n\n";
  }

  mensaje += "📈 *Ver historial de humedad:*\n";
  mensaje += "   [Dashboard ThingSpeak](" + String(TS_CANAL_URL) + ")\n";
  mensaje += "──────────────────";

  bool enviado = bot.sendMessage(TELEGRAM_CHAT_ID, mensaje, "Markdown");

  if (enviado) {
    Serial.println("[Telegram] ✅ Alerta enviada correctamente");
  } else {
    Serial.println("[Telegram] ❌ Error al enviar alerta");
  }
}

/**
 * enviarRecordatorioTelegram(humedad)
 * Recordatorio periódico si el suelo sigue seco después del primer aviso.
 */
void enviarRecordatorioTelegram(float humedad) {
  Serial.println("[Telegram] Enviando recordatorio...");

  String mensaje = "🔁 *RECORDATORIO — Suelo aún seco*\n";
  mensaje += "──────────────────\n";
  mensaje += "💧 Humedad: *" + String(humedad, 1) + "%* (bajo umbral de " + String(UMBRAL_RIEGO) + "%)\n";
  mensaje += "🕐 Hora: " + obtenerHoraFormateada() + "\n";

  if (lluviaProxima24h >= 5.0) {
    mensaje += "⏳ Lluvia prevista: " + String(lluviaProxima24h, 1) + " mm en 24h\n";
  } else {
    mensaje += "⚠️ Sin lluvia prevista — regar a la brevedad.\n";
  }

  mensaje += "\n📈 [Ver historial](" + String(TS_CANAL_URL) + ")";

  bot.sendMessage(TELEGRAM_CHAT_ID, mensaje, "Markdown");
}

/**
 * enviarConfirmacionRiego(humedad)
 * Mensaje de confirmación cuando el suelo vuelve a estar húmedo.
 */
void enviarConfirmacionRiego(float humedad) {
  Serial.println("[Telegram] Enviando confirmación de riego...");

  String mensaje = "✅ *Suelo hidratado correctamente*\n";
  mensaje += "──────────────────\n";
  mensaje += "💧 Humedad actual: *" + String(humedad, 1) + "%*\n";
  mensaje += "🕐 Hora: " + obtenerHoraFormateada() + "\n";
  mensaje += "👍 El suelo ya no necesita riego.\n\n";
  mensaje += "📈 [Ver historial](" + String(TS_CANAL_URL) + ")";

  bot.sendMessage(TELEGRAM_CHAT_ID, mensaje, "Markdown");
}

// ============================================================
//  FUNCIONES DE SENSOR Y DATOS
// ============================================================

/**
 * calcularHumedad(rawADC)
 * Convierte el valor ADC del sensor a porcentaje de humedad (0–100%).
 * Usa interpolación lineal entre los valores de calibración.
 * El sensor capacitivo V1.2 entrega MAYOR voltaje cuando está SECO
 * y MENOR voltaje cuando está HÚMEDO (lógica inversa al resistivo).
 */
float calcularHumedad(int rawADC) {
  rawADC = constrain(rawADC, SENSOR_AGUA, SENSOR_SECO);
  // SECO (valor alto) → 0% | HÚMEDO (valor bajo) → 100%
  float humedad = (float)(SENSOR_SECO - rawADC) / (float)(SENSOR_SECO - SENSOR_AGUA) * 100.0;
  return constrain(humedad, 0.0, 100.0);
}

/**
 * enviarAThingSpeak(humedad, estado, lluvia)
 * Sube los 3 campos al canal ThingSpeak para dashboard e historial.
 * Límite plan gratuito: mínimo 15 segundos entre envíos.
 */
void enviarAThingSpeak(float humedad, int estado, float lluvia) {
  ThingSpeak.setField(1, humedad);  // % de humedad del suelo
  ThingSpeak.setField(2, estado);   // 0 = seco / 1 = húmedo
  ThingSpeak.setField(3, lluvia);   // mm de lluvia proyectados 24h

  int resultado = ThingSpeak.writeFields(TS_CHANNEL_ID, TS_WRITE_KEY);

  if (resultado == 200) {
    Serial.println("[ThingSpeak] ✅ Datos enviados");
  } else {
    Serial.println("[ThingSpeak] ❌ Error. Código: " + String(resultado));
  }
}

// ============================================================
//  FUNCIÓN DE CLIMA
// ============================================================

/**
 * consultarClima()
 * Consulta el pronóstico de 5 días (intervalos de 3h) de OpenWeatherMap.
 * Actualiza: lluviaProxima24h, diasRecomendadosRiego, climaDescripcion, tempActual.
 */
void consultarClima() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Clima] Sin WiFi, omitiendo consulta");
    return;
  }

  Serial.println("[Clima] Consultando pronóstico...");

  // Endpoint gratuito: forecast cada 3 horas, 5 días (40 intervalos)
  String url = "http://api.openweathermap.org/data/2.5/forecast?"
               "q=" + String(OWM_CITY) + "," + String(OWM_COUNTRY) +
               "&appid=" + String(OWM_API_KEY) +
               "&units=metric"
               "&lang=es"     // Descripciones en español
               "&cnt=40";

  HTTPClient http;
  http.begin(clienteHTTP, url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.println("[Clima] ❌ Error HTTP: " + String(httpCode));
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  // Parsear JSON — 6KB suficiente para los campos relevantes del forecast
  DynamicJsonDocument doc(6144);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.println("[Clima] ❌ Error JSON: " + String(error.c_str()));
    return;
  }

  // ── Clima actual (primer intervalo) ─────────────────────
  tempActual       = doc["list"][0]["main"]["temp"].as<float>();
  climaDescripcion = doc["list"][0]["weather"][0]["description"].as<String>();
  // Capitalizar primera letra
  if (climaDescripcion.length() > 0) {
    climaDescripcion[0] = toupper(climaDescripcion[0]);
  }

  // ── Lluvia próximas 24h (8 intervalos × 3h = 24h) ──────
  lluviaProxima24h = 0.0;
  for (int i = 0; i < 8 && i < (int)doc["list"].size(); i++) {
    if (doc["list"][i].containsKey("rain") &&
        doc["list"][i]["rain"].containsKey("3h")) {
      lluviaProxima24h += doc["list"][i]["rain"]["3h"].as<float>();
    }
  }

  // ── Días recomendados para regar (sin lluvia significativa) ─
  // Un día es candidato si la lluvia total del día < 3mm
  diasRecomendadosRiego = "";
  String diasSemana[] = {"Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"};

  String fechaActual = "";
  float  lluviaDia   = 0.0;
  float  tempMaxDia  = 0.0;

  for (int i = 0; i < (int)doc["list"].size(); i++) {
    String dtTxt = doc["list"][i]["dt_txt"].as<String>();
    String fecha = dtTxt.substring(0, 10); // "YYYY-MM-DD"
    float  temp  = doc["list"][i]["main"]["temp"].as<float>();

    float lluviaItem = 0.0;
    if (doc["list"][i].containsKey("rain") &&
        doc["list"][i]["rain"].containsKey("3h")) {
      lluviaItem = doc["list"][i]["rain"]["3h"].as<float>();
    }

    if (fecha != fechaActual && fechaActual.length() > 0) {
      if (lluviaDia < 3.0) {
        // Obtener día de la semana a partir del timestamp Unix
        long ts     = doc["list"][i - 1]["dt"].as<long>();
        int  diaSem = ((ts / 86400L) + 4) % 7;
        diasRecomendadosRiego += diasSemana[diaSem];
        diasRecomendadosRiego += " (" + String((int)round(tempMaxDia)) + "°C)  ";
      }
      lluviaDia  = 0.0;
      tempMaxDia = 0.0;
    }

    fechaActual = fecha;
    lluviaDia  += lluviaItem;
    if (temp > tempMaxDia) tempMaxDia = temp;
  }

  Serial.println("[Clima] ✅ " + climaDescripcion + " — " + String(tempActual, 1) + "°C");
  Serial.println("[Clima] Lluvia 24h : " + String(lluviaProxima24h, 1) + " mm");
  Serial.println("[Clima] Días riego : " + (diasRecomendadosRiego.length() > 0 ? diasRecomendadosRiego : "ninguno (lluvia todos los días)"));
}

// ============================================================
//  UTILIDADES
// ============================================================

/**
 * conectarWiFi()
 * Conecta al WiFi con hasta 30 reintentos y reinicia si falla.
 */
void conectarWiFi() {
  Serial.print("[WiFi] Conectando a " + String(WIFI_SSID));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(500);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Conectado — IP: " + WiFi.localIP().toString());
    Serial.println("[WiFi] RSSI: " + String(WiFi.RSSI()) + " dBm");
  } else {
    Serial.println("\n[WiFi] ERROR: No se pudo conectar. Reiniciando...");
    ESP.restart();
  }
}

/**
 * obtenerHoraFormateada()
 * Retorna la hora actual como string "HH:MM:SS — DD/MM/YYYY".
 * Usa NTP para sincronizar con servidores de tiempo reales.
 */
String obtenerHoraFormateada() {
  time_t tiempoEpoch = timeClient.getEpochTime();
  struct tm* tiempoInfo = localtime(&tiempoEpoch);

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%H:%M:%S — %d/%m/%Y", tiempoInfo);
  return String(buffer);
}
