import hmac
import logging
import os
from datetime import datetime, timedelta, timezone
from threading import Lock

from flask import Flask, jsonify, render_template, request
from psycopg import connect
from psycopg.rows import dict_row

app = Flask(__name__)

logging.basicConfig(
    level=os.getenv("LOG_LEVEL", "INFO"),
    format="%(asctime)s %(levelname)s %(message)s",
)
logger = logging.getLogger(__name__)

DATABASE_URL = os.getenv("DATABASE_URL", "").strip()
DEVICE_API_KEY = os.getenv("DEVICE_API_KEY", "").strip()
HUMIDITY_THRESHOLD = float(os.getenv("HUMIDITY_THRESHOLD", "35"))
ONLINE_WINDOW_MINUTES = int(os.getenv("ONLINE_WINDOW_MINUTES", "15"))

_schema_lock = Lock()
_schema_ready = False


def get_connection():
    """Abre una conexión segura con PostgreSQL usando DATABASE_URL."""
    if not DATABASE_URL:
        raise RuntimeError("Falta configurar la variable DATABASE_URL.")
    return connect(
        DATABASE_URL,
        row_factory=dict_row,
        connect_timeout=10,
    )


def ensure_schema():
    """Crea la tabla la primera vez que la aplicación logra conectarse."""
    global _schema_ready

    if _schema_ready:
        return

    with _schema_lock:
        if _schema_ready:
            return

        with get_connection() as conn:
            conn.execute(
                """
                CREATE TABLE IF NOT EXISTS mediciones (
                    id BIGSERIAL PRIMARY KEY,
                    humedad NUMERIC(5, 2) NOT NULL
                        CHECK (humedad >= 0 AND humedad <= 100),
                    adc INTEGER NOT NULL
                        CHECK (adc >= 0 AND adc <= 1023),
                    suelo_seco BOOLEAN NOT NULL,
                    temperatura NUMERIC(5, 2),
                    lluvia_24h NUMERIC(7, 2),
                    dispositivo VARCHAR(60) NOT NULL DEFAULT 'ESP8266',
                    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                """
            )
            conn.execute(
                """
                CREATE INDEX IF NOT EXISTS idx_mediciones_created_at
                ON mediciones (created_at DESC);
                """
            )

        _schema_ready = True
        logger.info("Esquema de base de datos verificado.")


def parse_number(payload, field, minimum, maximum, required=True):
    value = payload.get(field)

    if value is None and not required:
        return None
    if value is None:
        raise ValueError(f"Falta el campo '{field}'.")

    try:
        number = float(value)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"El campo '{field}' debe ser numérico.") from exc

    if not minimum <= number <= maximum:
        raise ValueError(
            f"El campo '{field}' debe estar entre {minimum} y {maximum}."
        )
    return number


def serialize_measurement(row):
    if row is None:
        return None

    return {
        "id": row["id"],
        "humedad": float(row["humedad"]),
        "adc": int(row["adc"]),
        "suelo_seco": bool(row["suelo_seco"]),
        "temperatura": (
            float(row["temperatura"]) if row["temperatura"] is not None else None
        ),
        "lluvia_24h": (
            float(row["lluvia_24h"]) if row["lluvia_24h"] is not None else None
        ),
        "dispositivo": row["dispositivo"],
        "created_at": row["created_at"].isoformat(),
    }


@app.after_request
def add_security_headers(response):
    response.headers["X-Content-Type-Options"] = "nosniff"
    response.headers["X-Frame-Options"] = "SAMEORIGIN"
    response.headers["Referrer-Policy"] = "strict-origin-when-cross-origin"
    return response


@app.get("/")
def index():
    return render_template(
        "index.html",
        humidity_threshold=HUMIDITY_THRESHOLD,
        refresh_seconds=10,
    )


@app.get("/health")
def health():
    try:
        ensure_schema()
        with get_connection() as conn:
            conn.execute("SELECT 1").fetchone()
        return jsonify({"status": "ok"}), 200
    except Exception as exc:
        logger.exception("Error en healthcheck")
        return jsonify({"status": "error", "detail": str(exc)}), 503


@app.post("/api/mediciones")
def create_measurement():
    if not DEVICE_API_KEY:
        return jsonify({"error": "DEVICE_API_KEY no está configurada."}), 500

    received_key = request.headers.get("X-API-Key", "")
    if not hmac.compare_digest(received_key, DEVICE_API_KEY):
        return jsonify({"error": "No autorizado."}), 401

    payload = request.get_json(silent=True)
    if not isinstance(payload, dict):
        return jsonify({"error": "El cuerpo debe ser JSON."}), 400

    try:
        humedad = parse_number(payload, "humedad", 0, 100)
        adc = int(parse_number(payload, "adc", 0, 1023))
        temperatura = parse_number(
            payload, "temperatura", -50, 80, required=False
        )
        lluvia_24h = parse_number(
            payload, "lluvia_24h", 0, 1000, required=False
        )

        raw_dry = payload.get("suelo_seco")
        if raw_dry is None:
            suelo_seco = humedad < HUMIDITY_THRESHOLD
        elif isinstance(raw_dry, bool):
            suelo_seco = raw_dry
        elif raw_dry in (0, 1, "0", "1"):
            suelo_seco = str(raw_dry) == "1"
        else:
            raise ValueError("El campo 'suelo_seco' debe ser booleano.")

        dispositivo = str(payload.get("dispositivo", "ESP8266")).strip()[:60]
        if not dispositivo:
            dispositivo = "ESP8266"

        ensure_schema()
        with get_connection() as conn:
            row = conn.execute(
                """
                INSERT INTO mediciones (
                    humedad,
                    adc,
                    suelo_seco,
                    temperatura,
                    lluvia_24h,
                    dispositivo
                )
                VALUES (%s, %s, %s, %s, %s, %s)
                RETURNING *
                """,
                (
                    humedad,
                    adc,
                    suelo_seco,
                    temperatura,
                    lluvia_24h,
                    dispositivo,
                ),
            ).fetchone()

        return jsonify(
            {
                "message": "Medición guardada correctamente.",
                "medicion": serialize_measurement(row),
            }
        ), 201

    except ValueError as exc:
        return jsonify({"error": str(exc)}), 400
    except Exception:
        logger.exception("No se pudo guardar la medición")
        return jsonify({"error": "No se pudo guardar la medición."}), 500


@app.get("/api/mediciones")
def list_measurements():
    try:
        limit = int(request.args.get("limit", 120))
    except ValueError:
        return jsonify({"error": "El parámetro limit debe ser entero."}), 400

    limit = min(max(limit, 1), 500)

    try:
        ensure_schema()
        with get_connection() as conn:
            rows = conn.execute(
                """
                SELECT *
                FROM mediciones
                ORDER BY created_at DESC
                LIMIT %s
                """,
                (limit,),
            ).fetchall()

        # Se invierte para que el gráfico quede en orden cronológico.
        return jsonify([serialize_measurement(row) for row in reversed(rows)])
    except Exception:
        logger.exception("No se pudo consultar el historial")
        return jsonify({"error": "No se pudo consultar el historial."}), 500


@app.get("/api/resumen")
def summary():
    try:
        ensure_schema()
        since = datetime.now(timezone.utc) - timedelta(hours=24)

        with get_connection() as conn:
            latest = conn.execute(
                """
                SELECT *
                FROM mediciones
                ORDER BY created_at DESC
                LIMIT 1
                """
            ).fetchone()

            stats = conn.execute(
                """
                SELECT
                    AVG(humedad) AS promedio,
                    MIN(humedad) AS minimo,
                    MAX(humedad) AS maximo,
                    COUNT(*) AS cantidad
                FROM mediciones
                WHERE created_at >= %s
                """,
                (since,),
            ).fetchone()

        latest_serialized = serialize_measurement(latest)

        online = False
        recommendation = "Esperando la primera medición."
        state = "Sin datos"

        if latest:
            online = latest["created_at"] >= (
                datetime.now(timezone.utc)
                - timedelta(minutes=ONLINE_WINDOW_MINUTES)
            )

            if latest["suelo_seco"]:
                state = "Suelo seco"
                if (
                    latest["lluvia_24h"] is not None
                    and float(latest["lluvia_24h"]) >= 2
                ):
                    recommendation = (
                        "El suelo está seco, pero se pronostica lluvia. "
                        "Revisar antes de regar."
                    )
                else:
                    recommendation = "Se recomienda regar."
            else:
                state = "Humedad adecuada"
                recommendation = "No es necesario regar."

        return jsonify(
            {
                "actual": latest_serialized,
                "estado": state,
                "recomendacion": recommendation,
                "dispositivo_online": online,
                "umbral": HUMIDITY_THRESHOLD,
                "periodo_horas": 24,
                "estadisticas": {
                    "promedio": (
                        round(float(stats["promedio"]), 1)
                        if stats["promedio"] is not None
                        else None
                    ),
                    "minimo": (
                        float(stats["minimo"])
                        if stats["minimo"] is not None
                        else None
                    ),
                    "maximo": (
                        float(stats["maximo"])
                        if stats["maximo"] is not None
                        else None
                    ),
                    "cantidad": int(stats["cantidad"]),
                },
            }
        )
    except Exception:
        logger.exception("No se pudo construir el resumen")
        return jsonify({"error": "No se pudo construir el resumen."}), 500


@app.errorhandler(404)
def not_found(_error):
    return jsonify({"error": "Ruta no encontrada."}), 404


if __name__ == "__main__":
    port = int(os.getenv("PORT", "5000"))
    app.run(host="0.0.0.0", port=port, debug=False)
