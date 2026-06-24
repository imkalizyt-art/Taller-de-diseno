// =============================================================================
//  SISTEMA DE ALERTA DE RIEGO IoT - PROTOTIPO UNIVERSITARIO
//  Plataforma : ESP8266 (NodeMCU)
//  IDE        : Arduino IDE
//  Autor      : [Tu nombre]
//  Versión    : 1.0
//
//  DESCRIPCIÓN:
//  Lee el sensor capacitivo de humedad de suelo en A0 y determina si la
//  tierra está seca o húmeda. Indica el estado con un LED RGB y, en caso
//  de sequedad, envía UNA sola notificación a Telegram hasta que la
//  condición cambie (evita spam de mensajes).
// =============================================================================


// =============================================================================
//  BLOQUE 1 — LIBRERÍAS
// =============================================================================

// Librería principal del ESP8266 para gestionar la conexión WiFi
#include <ESP8266WiFi.h>

// Cliente WiFi seguro (SSL/TLS) necesario para conectarse a la API de Telegram
// (que usa HTTPS). Si solo usas HTTP básico, puedes reemplazar por WiFiClient.
#include <WiFiClientSecure.h>

// Librería para comunicarse con la API de Telegram mediante un bot.
// Instálala desde el Gestor de Librerías: busca "UniversalTelegramBot"
#include <UniversalTelegramBot.h>


// =============================================================================
//  BLOQUE 2 — CREDENCIALES Y CONFIGURACIÓN  ← RELLENA ESTOS CAMPOS
// =============================================================================

// --- Credenciales de tu red WiFi ---
const char* WIFI_SSID     = "HONOR PRUEBA";   // <-- Escribe tu SSID aquí
const char* WIFI_PASSWORD = "prueba123";   // <-- Escribe tu contraseña aquí

// --- Credenciales del Bot de Telegram ---
// Para obtenerlos:
//   1. Abre Telegram y busca "@BotFather".
//   2. Escribe /newbot, sigue los pasos y copia el token que te da.
//   3. Para el CHAT_ID: escribe un mensaje a tu bot y luego visita en tu
//      navegador: https://api.telegram.org/bot<TU_TOKEN>/getUpdates
//      Busca el campo "chat":{"id": XXXXXXX} y copia ese número.
const char* BOT_TOKEN = "8388957685:AAHvpDwxRTstP5iNCmUD0ooh8jR0Jnyo0Wc"; // <-- Tu token
const char* CHAT_ID   = "1428052556";                                     // <-- Tu chat ID


// =============================================================================
//  BLOQUE 3 — PINES DE HARDWARE
// =============================================================================

// Pin analógico del ESP8266. Es el ÚNICO pin analógico disponible.
// Aquí se conecta la señal de salida del sensor capacitivo de humedad.
const int PIN_SENSOR    = A0;

// Pines digitales para cada canal de color del LED RGB (cátodo común).
// En un LED de CÁTODO COMÚN, el pin negativo (-) va a GND.
// Para encender un color, se escribe HIGH en su pin.
const int PIN_LED_ROJO  = D1; // Canal rojo   → pin D1 del NodeMCU
const int PIN_LED_VERDE = D2; // Canal verde  → pin D2 del NodeMCU
const int PIN_LED_AZUL  = D3; // Canal azul   → pin D3 (no se usa en la lógica
                               // principal, pero se declara para referencia y
                               // para asegurarnos de apagarlo al inicio)


//============================================================================
//  BLOQUE 4 — UMBRALES DE CALIBRACIÓN DEL SENSOR
// =============================================================================

// Ajustado para tu sensor: Aire = ~700, Agua = ~620
// Dejamos márgenes más estrictos porque la diferencia es poca.

const int UMBRAL_SECO = 680; // Si el valor SUBE de 680, está en el aire (seco).
const int UMBRAL_HUMEDO = 640; // Si el valor BAJA de 640, tocó el agua (húmedo).


// =============================================================================
//  BLOQUE 5 — VARIABLES DE ESTADO GLOBAL
// =============================================================================

// Variable booleana (bandera) que controla si ya enviamos la alerta de Telegram.
// Se inicializa en 'false' (no enviada).
// Su propósito: enviar el mensaje SOLO UNA VEZ cuando la condición de sequedad
// comienza, y no repetirlo en cada ciclo del loop (cada ~1 segundo sería spam).
// Se resetea a 'false' cuando la tierra vuelve a estar húmeda, permitiendo
// que la próxima sequedad genere una nueva alerta.
bool alertaEnviada = false;

// Almacena el valor crudo (raw) leído del pin analógico A0.
// Rango de valores posibles: 0 (0V) a 1023 (3.3V en el ESP8266).
int valorSensor = 0;

// Objeto cliente WiFi seguro requerido por UniversalTelegramBot.
// 'X509List' vacío y setInsecure() se usan para simplificar la conexión SSL
// en un prototipo (sin verificación de certificado). En producción, usar
// verificación de certificado completa.
WiFiClientSecure clienteSSL;

// Objeto del bot de Telegram.
// Se inicializa con el token del bot y el cliente SSL.
UniversalTelegramBot bot(BOT_TOKEN, clienteSSL);


// =============================================================================
//  BLOQUE 6 — DECLARACIÓN DE FUNCIONES AUXILIARES (prototipos)
//  (Se definen en detalle al final del archivo)
// =============================================================================
void conectarWiFi();          // Gestiona la conexión a la red WiFi
void encenderLedRojo();       // Apaga todos los canales y enciende solo el rojo
void encenderLedVerde();      // Apaga todos los canales y enciende solo el verde
void apagarLed();             // Apaga los tres canales del LED RGB
void enviarAlertaTelegram();  // Envía un mensaje de alerta al bot de Telegram


// =============================================================================
//  BLOQUE 7 — SETUP (se ejecuta UNA sola vez al arrancar)
// =============================================================================

void setup() {

  // --- Inicializar el Monitor Serie ---
  // Se usa para depuración (debugging). Fijamos la velocidad en 115200 baudios
  // (debe coincidir con la velocidad elegida en el Monitor Serie del IDE).
  Serial.begin(115200);
  // Pequeña pausa para que el puerto serie se estabilice antes de imprimir.
  delay(100);

  Serial.println();
  Serial.println("=== SISTEMA DE ALERTA DE RIEGO - INICIANDO ===");

  // --- Configurar pines del LED RGB como SALIDAS ---
  // pinMode establece la dirección del pin: OUTPUT significa que el ESP8266
  // controlará el voltaje en ese pin (no lo leerá).
  pinMode(PIN_LED_ROJO,  OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AZUL,  OUTPUT);

  // --- Estado inicial: todos los LEDs apagados ---
  // Es buena práctica asegurarse de que los actuadores empiecen en un estado
  // conocido y seguro al arrancar el sistema.
  apagarLed();
  Serial.println("[HARDWARE] Pines del LED RGB configurados y apagados.");

  // --- Configurar el cliente SSL para Telegram ---
  // setInsecure() deshabilita la verificación del certificado SSL.
  // Es adecuado para prototipos universitarios. Para producción, usar
  // clienteSSL.setTrustAnchors() con el certificado raíz de Telegram.
  clienteSSL.setInsecure();

  // --- Conectar al WiFi ---
  // Llamamos a nuestra función auxiliar que gestiona el proceso de conexión.
  conectarWiFi();

  Serial.println("=== SETUP COMPLETO — INICIANDO MONITOREO DE SUELO ===");
  Serial.println(); // Línea en blanco para separar la salida del setup del loop
}


// =============================================================================
//  BLOQUE 8 — LOOP PRINCIPAL (se ejecuta en bucle infinito)
// =============================================================================

void loop() {

  valorSensor = analogRead(PIN_SENSOR);

  Serial.print("[SENSOR] Humedad del suelo: ");
  Serial.print(valorSensor);
  Serial.print(" | Estado: ");

  // CONDICIÓN A: TIERRA SECA → ROJO
  // ¡CORREGIDO! Si el valor es MAYOR (>) que 680, significa que está seco
  if (valorSensor > UMBRAL_SECO) { 

    Serial.println("SECO - ¡ALERTA!");
    encenderLedRojo();

    if (!alertaEnviada) {
      Serial.println("[TELEGRAM] Condición de sequedad detectada. Enviando alerta...");
      enviarAlertaTelegram();   
      alertaEnviada = true;     
    }
  }

  // CONDICIÓN B: TIERRA HÚMEDA → VERDE
  // ¡CORREGIDO! Si el valor es MENOR (<) que 640, significa que está en agua
  else if (valorSensor < UMBRAL_HUMEDO) { 

    Serial.println("HÚMEDO - OK");
    encenderLedVerde();

    if (alertaEnviada) {
      alertaEnviada = false;
      Serial.println("[SISTEMA] Humedad restaurada.");
    }
  }

  else {
    Serial.println("TRANSICIÓN (zona intermedia)");
  }

  delay(1000);
}

// =============================================================================
//  BLOQUE 9 — FUNCIONES AUXILIARES (definiciones completas)
// =============================================================================


// -----------------------------------------------------------------------------
// conectarWiFi()
// Propósito: Gestiona el proceso completo de conexión a la red WiFi.
//   1. Inicia la conexión con las credenciales definidas arriba.
//   2. Espera activamente hasta que el ESP8266 obtenga una IP del router.
//   3. Imprime la IP asignada en el Monitor Serie.
// Se llama desde setup(). Si el WiFi se desconecta durante la operación,
// el código no reconecta automáticamente (para un sistema robusto, añadir
// comprobación en el loop o usar WiFi.setAutoReconnect(true)).
// -----------------------------------------------------------------------------
void conectarWiFi() {

  Serial.print("[WIFI] Conectando a la red: ");
  Serial.println(WIFI_SSID);

  // Modo estación (STA): el ESP8266 actúa como cliente de un router WiFi.
  // (Alternativas: AP para crear su propio hotspot, o AP+STA para ambos.)
  WiFi.mode(WIFI_STA);

  // Inicia el proceso de conexión WiFi con el SSID y contraseña.
  // Este comando es no-bloqueante; la conexión ocurre en segundo plano.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Bucle de espera activa: seguimos en este bucle mientras el estado
  // de la conexión WiFi NO sea WL_CONNECTED (conectado).
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);             // Esperamos 500ms entre comprobaciones
    Serial.print(".");      // Imprimimos un punto para mostrar progreso
  }

  // Si llegamos aquí, la conexión fue exitosa.
  Serial.println(); // Salto de línea después de los puntos
  Serial.print("[WIFI] Conectado! Dirección IP asignada: ");
  Serial.println(WiFi.localIP()); // Imprime la IP local (ej: 192.168.1.42)
}


// -----------------------------------------------------------------------------
// encenderLedRojo()
// Propósito: Configura el LED RGB para emitir luz roja.
// En un LED de cátodo común: HIGH = encendido, LOW = apagado.
// Primero apaga todos los canales para garantizar un estado limpio,
// luego enciende únicamente el canal rojo.
// -----------------------------------------------------------------------------
void encenderLedRojo() {
  digitalWrite(PIN_LED_ROJO,  HIGH); // Enciende el canal rojo
  digitalWrite(PIN_LED_VERDE, LOW);  // Apaga el canal verde
  digitalWrite(PIN_LED_AZUL,  LOW);  // Apaga el canal azul
}


// -----------------------------------------------------------------------------
// encenderLedVerde()
// Propósito: Configura el LED RGB para emitir luz verde.
// Apaga los otros canales para evitar mezclas de color indeseadas.
// -----------------------------------------------------------------------------
void encenderLedVerde() {
  digitalWrite(PIN_LED_ROJO,  LOW);  // Apaga el canal rojo
  digitalWrite(PIN_LED_VERDE, HIGH); // Enciende el canal verde
  digitalWrite(PIN_LED_AZUL,  LOW);  // Apaga el canal azul
}


// -----------------------------------------------------------------------------
// apagarLed()
// Propósito: Apaga los tres canales del LED RGB simultáneamente.
// Se usa al inicio del setup para garantizar un estado inicial conocido.
// También puede usarse si se necesita un estado "inactivo" o de espera.
// -----------------------------------------------------------------------------
void apagarLed() {
  digitalWrite(PIN_LED_ROJO,  LOW); // Apaga canal rojo
  digitalWrite(PIN_LED_VERDE, LOW); // Apaga canal verde
  digitalWrite(PIN_LED_AZUL,  LOW); // Apaga canal azul
}


// -----------------------------------------------------------------------------
// enviarAlertaTelegram()
// Propósito: Compone y envía un mensaje de texto al chat de Telegram
//   especificado en CHAT_ID, usando el bot identificado por BOT_TOKEN.
//
// Funcionamiento interno:
//   - bot.sendMessage() realiza una petición HTTPS POST a la API de Telegram:
//     https://api.telegram.org/bot<TOKEN>/sendMessage
//   - El segundo argumento es el cuerpo del mensaje (texto plano o Markdown).
//   - El tercer argumento es el modo de parseo ("" = sin formato,
//     "Markdown" = negritas/cursivas, "HTML" = etiquetas HTML).
//
// Prerequisito: El WiFi debe estar conectado cuando se llama a esta función.
// Se recomienda comprobar WiFi.status() == WL_CONNECTED antes de llamarla.
// -----------------------------------------------------------------------------
void enviarAlertaTelegram() {

  // Verificación de seguridad: asegurarse de que el WiFi sigue conectado
  // antes de intentar hacer una petición de red.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TELEGRAM] ERROR: WiFi no conectado. No se puede enviar el mensaje.");
    return; // Salir de la función sin hacer nada
  }

  // Componer el texto del mensaje de alerta.
  // Puedes personalizar este texto con emojis y detalles del parque/sensor.
  String mensaje = "🌵 ALERTA DE RIEGO - Sistema IoT Universitario\n\n";
  mensaje += "⚠️ Se requiere riego en el parque.\n\n";
  mensaje += "📊 Valor del sensor (A0): " + String(valorSensor) + "\n";
  mensaje += "🔴 Estado: TIERRA SECA (valor > " + String(UMBRAL_SECO) + ")\n";
  mensaje += "🕐 Acción requerida: activar el sistema de riego.";

  // Enviar el mensaje usando la librería UniversalTelegramBot.
  // Parámetros:
  //   - CHAT_ID  : destinatario (tu ID de chat o grupo)
  //   - mensaje  : texto del mensaje
  //   - ""       : modo de parseo (vacío = texto plano simple)
  bool resultado = bot.sendMessage(CHAT_ID, mensaje, "");

  // Comprobamos si el mensaje se envió correctamente.
  if (resultado) {
    Serial.println("[TELEGRAM] ✓ Mensaje enviado correctamente.");
  } else {
    Serial.println("[TELEGRAM] ✗ Error al enviar el mensaje. Comprueba el token y el chat ID.");
  }
}


// =============================================================================
//  FIN DEL CÓDIGO
//
//  NOTAS DE CALIBRACIÓN:
//  1. Abre el Monitor Serie a 115200 baudios.
//  2. Observa los valores que reporta el sensor en tierra COMPLETAMENTE SECA.
//     Ese valor (aprox.) es tu candidato para UMBRAL_SECO.
//  3. Observa los valores con la tierra bien regada.
//     Ese valor (aprox.) es tu candidato para UMBRAL_HUMEDO.
//  4. Deja un margen de al menos 50-100 puntos entre ambos umbrales
//     para crear la zona de histéresis y evitar parpadeos del LED.
//
//  CONEXIONES FÍSICAS RESUMIDAS:
//  ┌─────────────────────┬───────────────┬──────────────────────────┐
//  │ Componente          │ Pin sensor    │ Pin NodeMCU              │
//  ├─────────────────────┼───────────────┼──────────────────────────┤
//  │ Sensor humedad      │ AOUT          │ A0                       │
//  │ Sensor humedad      │ VCC           │ 3.3V                     │
//  │ Sensor humedad      │ GND           │ GND                      │
//  │ LED RGB (cátodo -)  │ GND (cátodo)  │ GND                      │
//  │ LED RGB             │ R (rojo)      │ D1 (con resistencia 220Ω)│
//  │ LED RGB             │ G (verde)     │ D2 (con resistencia 220Ω)│
//  │ LED RGB             │ B (azul)      │ D3 (con resistencia 220Ω)│
//  └─────────────────────┴───────────────┴──────────────────────────┘
//  IMPORTANTE: Coloca siempre una resistencia de 220Ω en serie con
//  cada pin del LED para limitar la corriente y no dañar el ESP8266.
// =============================================================================
