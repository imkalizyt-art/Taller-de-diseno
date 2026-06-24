# FUENTES DEL PROYECTO

Este archivo registra las librerías, códigos externos y herramientas de inteligencia artificial utilizadas en el desarrollo del proyecto.

## 1. Librerías utilizadas

Esta sección se completará cuando se confirmen las librerías definitivas.

Librerías consideradas actualmente:

* **WiFi:** conexión de la placa ESP8266 a internet.
* **ThingSpeak:** envío, almacenamiento y visualización de las mediciones de humedad.
* **UniversalTelegramBot:** envío de alertas al jardinero por Telegram.

## 2. Código externo adaptado

### Conexión WiFi

Conexion mediante ESP8266.

**Adaptación realizada:** se agregarán reintentos de conexión y comprobación del estado de la red antes de enviar datos.

### Envío de datos al dashboard

Se utilizará como referencia la documentación de ThingSpeak.

**Adaptación realizada:** el código será modificado para enviar el valor del sensor, el porcentaje de humedad y el estado de alerta.

### Envío de alertas por Telegram

Se utilizará como referencia la documentación de Telegram Bot API o de la librería seleccionada.

**Adaptación realizada:** el mensaje incluirá el sector medido, la humedad actual, el umbral mínimo y un enlace al historial de mediciones.

## 3. Uso de Inteligencia Artificial

### Organización del diseño inicial del sistema

* **Herramienta:** ChatGPT.
* **Fecha:** junio de 2026.
* **Uso:** plantear la comunicación entre el sensor, la placa ESP y Telegram.
* **Adaptación:** el equipo ajustará la estructura según los componentes, archivos y pruebas reales del proyecto.
* **Comprensión:** el equipo revisará y probará cada parte antes de incorporarla al prototipo.

### Desarrollo y revisión del código

* **Herramienta:** ChatGPT.
* **Fecha:** junio de 2026.
* **Uso:** apoyo para generar, explicar y corregir partes del código del sistema.
* **Adaptación:** el equipo modificará los pines, valores de calibración, frecuencia de muestreo, umbrales de humedad y credenciales.
* **Comprensión:** el funcionamiento será comprobado mediante el Monitor Serial, el dashboard y las alertas recibidas.

## 4. Actualizaciones pendientes

Este archivo será actualizado cuando se definan:

* Librerías y versiones utilizadas.
* Enlaces oficiales de las fuentes.
* Fragmentos de código finalmente incorporados.
* Pruebas y modificaciones realizadas por el equipo.
