# Manual de Uso — Sistema de Riego Inteligente IoT

**Proyecto:** Sistema IoT de monitoreo de humedad del suelo  
**Ramo:** TEI 201 — Taller de Diseño en Ingeniería  
**Equipo:** Enzo Quiñones, Agustín Lastra, Matías Pineda, Carlos León


## 1. Descripción general

El sistema monitorea la humedad del suelo de forma autónoma mediante conexion a wifi, notifica al jardinero por Telegram cuando el suelo necesita riego. Funciona con batería recargable y panel solar, por lo que no requiere conexión a la red eléctrica.


## 2. Componentes del sistema

 Componente  Función 

 ESP8266 NodeMCU V3  Cerebro del sistema: lee el sensor, procesa datos y envía alertas 
 Sensor Capacitivo V1.2  Mide la humedad del suelo 
 Batería Li-Po 3.7V 2000mAh  Alimentación del sistema 
 Panel solar 6V 1W  Recarga la batería durante el día 
 Módulo TP4056  Controla la carga de la batería de forma segura 



## 3. Encendido del sistema

1. Asegurarse de que la batería esté cargada (el LED del TP4056 debe estar azul o verde)
2. Conectar la batería al módulo TP4056
3. El ESP8266 enciende automáticamente y se conecta al WiFi configurado
4. Esperar aproximadamente **10–15 segundos** para que establezca conexión
5. El sistema comienza a medir y enviar datos a ThingSpeak de inmediato

> **Indicador de funcionamiento:** si el LED del NodeMCU parpadea brevemente cada cierto tiempo, el sistema está operando correctamente.


## 4. Mensajes de Telegram

El sistema envía tres tipos de mensajes automáticos:

### 🔴 Alerta de suelo seco
Se envía cuando la humedad baja del 35%.

🚨 ALERTA: Suelo seco detectado

💧 Humedad: 22%
📊 Valor ADC: 714
🕐 Hora: 14:32:05 UTC
🌧️ Pronóstico lluvia próximos 5 días: No se esperan lluvias
✅ Recomendación: Regar hoy
📈 Ver historial: https://thingspeak.mathworks.com/channels/3416600
```

### 🔁 Recordatorio periódico
Si el suelo sigue seco después de 10 minutos, se envía un recordatorio con los datos actualizados.

### 🟢 Confirmación de suelo húmedo
Se envía automáticamente cuando la humedad vuelve a superar el 35% tras haber estado seca.

```
✅ Suelo húmedo — No se requiere riego

💧 Humedad actual: 61%
🕐 Hora: 15:10:22 UTC
```

---

## 5. Ver el historial de datos

1. Abrir el navegador e ir a:  
   👉 https://thingspeak.mathworks.com/channels/3416600
2. El dashboard muestra tres gráficos en tiempo real:
   - **Field 1:** Valor ADC crudo del sensor
   - **Field 2:** Porcentaje de humedad calculado
   - **Field 3:** Estado de alerta (0 = húmedo, 1 = seco)
3. Es posible filtrar por rango de fechas usando los controles del canal

---

## 6. Interpretación de la humedad

 Rango  Estado, Acción recomendada 

 - **0% – 35%**    Suelo seco  Regar — el sistema ya habrá alertado 
-  **35% – 60%**   Humedad moderada  Monitorear 
- **60% – 100%**   Suelo húmedo  No requiere riego 

## 7. Mantenimiento

### Revisión semanal
- Verificar que el panel solar esté expuesto al sol sin obstrucciones
- Revisar que el sensor esté correctamente insertado en el suelo (hasta la línea marcada, sin sumergir la electrónica)
- Confirmar que la conexion desde el dispostivo al celular esten funcionando de manera correcta

### Revisión mensual
- Revisar conexiones en el protoboard
- Verificar nivel de carga de la batería
- Limpiar el sensor o la parte que cubre el sensor con paño húmedo si hay acumulación de tierra

### Recalibración del sensor
Si las lecturas de humedad parecen incorrectas:
1. Medir el valor ADC con el sensor en aire → actualizar `SENSOR_SECO`
2. Medir el valor ADC con el sensor en agua → actualizar `SENSOR_AGUA`
3. Cargar el firmware actualizado al ESP8266


## 8. Solución de problemas

Síntoma  Causa probable  Solución 

- No llegan mensajes a Telegram  Sin conexión WiFi  
- Humedad siempre en 0% o 100%  Sensor descalibrado  Recalibrar con mediciones reales 
- El sistema no enciende  Batería descargada  Conectar al cargador o esperar carga solar 
- Mensajes repetidos muy seguidos  Bug en anti-spam  Revisar valor de `millis()` en el código 


## 9. Apagar el sistema

Desconectar la batería del módulo TP4056. El sistema no tiene interruptor físico; simplemente se desconecta la alimentación.

## 10. Contacto del equipo

Para dudas sobre el sistema, contactar a cualquier integrante del equipo a través del curso TEI 201.
