## Taller-de-diseno
- **Sistema IoT** que monitorea la humedad del suelo y alerta al usuario.

- **Integrantes:** Enzo Quiñones, Agustin Lastra, Matias Pineda, Carlos León.

- **Ramo:** Taller de diseño en ingenieria, TEI 201.

- **Profesor:** Sebastian Duarte.
# ¿Qué hace el sistema?
- 1. El sensor capacitivo mide la humedad del suelo cada cierto intervalo
- 2. Si la humedad cae bajo el 35%, el ESP8266 envía una alerta por Telegram al jardinero
- 3. El mensaje incluye: porcentaje de humedad, valor ADC crudo, timestamp real (NTP), pronóstico de lluvia de los próximos 5 días (OpenWeatherMap) y enlace al historial
- 4. Si el suelo sigue seco, se envía un recordatorio cada 10 minutos
- 5. Cuando el suelo vuelve a estar húmedo, se confirma por Telegram
- 6. Todas las mediciones se registran continuamente en ThingSpeak para análisis histórico
