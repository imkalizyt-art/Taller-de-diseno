# Dashboard personalizado — Sistema de humedad del suelo

Aplicación web desarrollada por el equipo para visualizar las mediciones
del ESP8266 sin depender del dashboard genérico de ThingSpeak.

## Funciones

- Humedad actual y valor ADC.
- Estado del suelo: seco o humedad adecuada.
- Recomendación de riego.
- Promedio, mínimo y máximo de las últimas 24 horas.
- Gráfico histórico con el umbral de 35%.
- Estado de conexión del dispositivo.
- Persistencia de datos en PostgreSQL de Aiven.
- Actualización automática cada 10 segundos.

## Variables de entorno necesarias en Railway

| Variable | Contenido |
|---|---|
| `DATABASE_URL` | URI completa entregada por Aiven |
| `DEVICE_API_KEY` | Clave privada compartida solo con el ESP8266 |
| `HUMIDITY_THRESHOLD` | `35` |
| `ONLINE_WINDOW_MINUTES` | `15` |

No subir un archivo `.env` ni credenciales reales al repositorio.

## Rutas de la aplicación

- `GET /` — dashboard.
- `GET /health` — comprueba la aplicación y la base de datos.
- `POST /api/mediciones` — recibe datos del ESP8266.
- `GET /api/mediciones?limit=120` — entrega el historial.
- `GET /api/resumen` — entrega la medición actual y estadísticas.

## Formato que envía el ESP8266

Encabezado:

```text
X-API-Key: la-misma-clave-configurada-en-railway
Content-Type: application/json
```

Cuerpo mínimo:

```json
{
  "humedad": 42.5,
  "adc": 638,
  "suelo_seco": false,
  "dispositivo": "ESP8266"
}
```

También acepta, de forma opcional:

```json
{
  "temperatura": 21.4,
  "lluvia_24h": 0.0
}
```

## Prueba manual

Después de desplegar, reemplazar los valores:

```bash
curl -X POST "https://TU-DOMINIO.up.railway.app/api/mediciones" \
  -H "Content-Type: application/json" \
  -H "X-API-Key: TU_CLAVE_PRIVADA" \
  -d '{"humedad":42.5,"adc":638,"suelo_seco":false,"dispositivo":"ESP8266"}'
```

## Configuración de Railway en un repositorio con varias carpetas

Como el repositorio completo contiene firmware, hardware y diseño 3D,
configurar el **Root Directory** del servicio de Railway como:

```text
/dashboard
```

Luego crear un dominio público desde la sección Networking del servicio.
