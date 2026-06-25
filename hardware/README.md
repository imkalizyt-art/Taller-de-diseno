# Lista de Materiales y Costos — Hardware

**Proyecto:** Sistema IoT de Monitoreo de Humedad del Suelo  
**Ramo:** TEI 201 — Taller de Diseño en Ingeniería  
**Integrantes:** Enzo Quiñones, Agustín Lastra, Matías Pineda, Carlos León

---

## Lista de Componentes

| N° | Componente | Modelo / Especificación | Cantidad | Precio unit. (CLP) | Total (CLP) | Fuente de referencia |
| 1 | Microcontrolador WiFi | ESP8266 NodeMCU V3 (ESP-12E, CP2102) | 1 | $5.090 | $5.090 | mechatronicstore.cl |
| 2 | Sensor de humedad del suelo | Capacitive Soil Moisture Sensor V1.2 | 1 | $2.990 | $2.990 | maxelectronica.cl |
| 3 | Módulo elevador de voltaje | XL6009 DC-DC Step-Up Boost Converter | 1 | $2.490 | $2.490 | maxelectronica.cl |
| 4 | Módulo cargador de batería | TP4056 con protección integrada (USB-C) | 1 | $2.490 | $2.490 | maxelectronica.cl |
| 5 | Batería recargable | Li-Po 3.7V 2000mAh modelo 103450 | 1 | $5.990 | $5.990 | makerschile.cl (ref.) |
| 6 | Panel solar | 6V 1W (policristalino) | 1 | $3.490 | $3.490 | mechatronicstore.cl (ref.) |
| 7 | Placa de pruebas | Protoboard 830 puntos | 1 | $2.490 | $2.490 | maxelectronica.cl |
| 8 | Cables de conexión | Jumper macho-macho / macho-hembra (x40) | 1 pack | $1.990 | $1.990 | mechatronicstore.cl |

**Costo total del prototipo: $27.020 CLP**

## Justificación de Componentes

### 1. ESP8266 NodeMCU V3 (ESP-12E, CP2102)

El ESP8266 fue elegido como cerebro del sistema por tres razones principales:

- **WiFi integrado:** a diferencia de un Arduino UNO que requeriría un módulo WiFi externo adicional, el ESP8266 integra conectividad WiFi 802.11 b/g/n en el mismo chip, reduciendo componentes y costos.
- **Suficiente capacidad para el proyecto:** con 80 MHz de procesador, 4 MB de memoria flash y 1 pin ADC de 10 bits (0–1023), cubre exactamente las necesidades del sistema: leer el sensor analógico, hacer solicitudes HTTP a OpenWeatherMap y ThingSpeak, y enviar mensajes por Telegram.
- **Compatibilidad directa:** el sensor capacitivo V1.2 opera a 3.3V–5V, compatible con los GPIOs del NodeMCU sin necesidad de adaptadores de nivel.

### 2. Capacitive Soil Moisture Sensor V1.2

El sensor capacitivo fue elegido por sobre el sensor resistivo (tipo FC-28) por razones técnicas y de durabilidad:

- **Principio capacitivo vs. resistivo:** el sensor resistivo hace pasar corriente por el suelo entre dos electrodos metálicos, lo que provoca electrólisis y corrosión en pocas semanas. El sensor capacitivo mide el cambio en la constante dieléctrica del suelo sin contacto eléctrico directo, eliminando este problema.
- **Mayor vida útil:** el fabricante especifica una vida útil mínima de 3 años, adecuada para un sistema de monitoreo permanente en jardín.
- **Salida analógica directa:** entrega una señal analógica de 0–3V proporcional a la humedad, compatible con el único pin ADC del ESP8266 (A0). No requiere conversión adicional.
- **Recubrimiento anticorrosión:** el electrodo tiene una capa protectora que lo hace apto para estar enterrado permanentemente en tierra húmeda.
- **Compatibilidad de voltaje:** opera entre 3.3V y 5.5V, compatible directamente con la salida del ESP8266.

---

### 3. XL6009 DC-DC Step-Up Boost Converter

El módulo XL6009 se incorporó para resolver un problema de voltaje en el sistema de alimentación:

- **Problema:** la batería Li-Po entrega 3.7V nominales (3.2V–4.2V según su estado de carga). El ESP8266 requiere 5V en su pin VIN para operar correctamente a través de su regulador interno de 3.3V.
- **Solución:** el XL6009 eleva el voltaje de la batería (3.2V–4.2V) a un voltaje de salida estable de 5V, garantizando operación continua del ESP8266 independientemente del nivel de carga de la batería.
- **Por qué XL6009 y no LM2596:** el LM2596 es un convertidor reductor (buck), no elevador. El XL6009 es específicamente un elevador de voltaje (boost), necesario para subir de 3.7V a 5V.
- **Eficiencia:** con una eficiencia de conversión de hasta 94%, minimiza las pérdidas energéticas, lo cual es importante para prolongar la autonomía con batería solar.

---

### 4. TP4056 con Protección Integrada

El módulo TP4056 gestiona la carga segura de la batería Li-Po desde el panel solar:

- **Protección contra sobrecargas:** el chip TP4056 controla el proceso de carga mediante el método CC/CV (corriente constante / voltaje constante), cortando la carga cuando la batería alcanza 4.2V para prevenir daños.
- **Protección integrada:** se eligió la versión con DW01A + FS8205A (protección adicional), que añade protección contra sobredescarga, sobrecorriente y cortocircuito — esencial para un sistema autónomo sin supervisión constante.
- **Por qué no conectar el panel solar directo a la batería:** un panel solar sin regulación puede sobrecargar la batería Li-Po con voltajes superiores a 4.2V, lo que genera riesgo de hinchamiento, daño permanente o incendio.
- **Compatibilidad:** acepta entrada de 4.5V–5.5V, compatible con el panel solar de 6V (que en condiciones normales entrega ~5V bajo carga).

---

### 5. Batería Li-Po 3.7V 2000mAh (modelo 103450)

- **Capacidad elegida:** 2000mAh permite autonomía estimada de 8–12 horas de operación continua del sistema, suficiente para días con poca luz solar (nublado) donde el panel no recarga completamente.
- **Química Li-Po vs. alcalina:** las baterías alcalinas (AA/AAA) no son recargables por panel solar. El Li-Po permite ciclos continuos de carga/descarga sin efecto memoria y entrega alta densidad de energía en formato compacto.
- **Voltaje nominal 3.7V:** compatible con el módulo TP4056 (diseñado para celdas Li-Po de una celda a 3.7V) y con el XL6009 (rango de entrada mínimo 3.5V).
- **Formato físico:** el modelo 103450 (10mm × 34mm × 50mm) es compacto y fácil de integrar en el protoboard o carcasa del prototipo.

---

### 6. Panel Solar 6V 1W

- **Fuente de energía renovable:** permite que el sistema opere de forma autónoma en un jardín sin acceso a red eléctrica, cumpliendo con el requisito de instalación permanente.
- **Potencia elegida (1W / ~166mA a 6V):** el ESP8266 consume aproximadamente 80mA en operación WiFi. Con 5–6 horas de sol útil diario, el panel entrega energía suficiente para recargar la batería y compensar el consumo del sistema.
- **Voltaje de 6V:** entrega ~5V bajo carga, compatible con el rango de entrada del TP4056 (4.5V–5.5V). No se conecta directamente a la batería — siempre pasa por el TP4056.
- **Formato pequeño:** un panel de 1W es compacto y discreto para instalación en jardín, sin requerir estructura de soporte especial.

---

### 7. Protoboard 830 puntos

- **Prototipado sin soldadura:** permite modificar el circuito rápidamente durante el desarrollo, depuración y ajuste de parámetros (pines, resistencias, reordenamiento de componentes) sin necesidad de herramientas de soldadura.
- **Capacidad suficiente:** 830 puntos es el estándar para proyectos de complejidad media; permite ubicar el NodeMCU, el TP4056, el XL6009 y los cables del sensor con espacio disponible.
- **Etapa de prototipo:** para una versión final de producción, el circuito sería transferido a una PCB, pero en etapa de prototipo universitario la protoboard es la solución estándar.
