# Dispositivos IoT con ESP32 conectados vía MQTT

**Nombre del módulo:** Dispositivos conectados por ESP32 + MQTT  
**Protocolo de comunicación:** MQTT  
**Hardware base:** ESP32 (Arduino compatible)  
**Objetivo:** Monitoreo y control de sensores, actuadores y cámaras mediante broker MQTT

---

## Descripción General

Este proyecto define la arquitectura y el funcionamiento de dispositivos IoT basados en **ESP32**, los cuales se comunican a través del **protocolo MQTT** con un panel de control central. Los dispositivos pueden ser sensores, actuadores o cámaras, y están diseñados para operar en una red local WiFi, publicando o recibiendo mensajes a través de un broker MQTT.

El objetivo es implementar una infraestructura de comunicación ligera, robusta y en tiempo real entre los dispositivos físicos y una plataforma de supervisión/control.

---

## Dispositivos soportados

### 1. **Cámara ESP32-CAM**
- Módulo con capacidad de transmisión de imágenes (baja tasa de frames).
- Publica notificaciones al detectar movimiento (eventos tipo `motion-detected`).
- Opcionalmente puede enviar imágenes o snapshots codificados.

### 2. **Sensores de movimiento**
- Sensores PIR, ultrasónicos u otros conectados a pines digitales del ESP32.
- Publican eventos `movement/on` o `distance/<valor>` en temas MQTT específicos.

### 3. **Actuadores (Relés)**
- Controlan el paso de corriente eléctrica.
- Reciben comandos por MQTT (`relay/on`, `relay/off`) desde el panel.

### 4. **Sensores ambientales**
- Posible integración con sensores de humedad, temperatura, gas, etc.
- Publican valores periódicos en formato JSON.

---

## Estructura MQTT

### Ejemplos de temas utilizados

```plaintext
esp32/camera_01/motion
esp32/sensor_01/distance
esp32/relay_01/state
esp32/relay_01/set
esp32/perrobot_01/status
```

### Formato de mensaje típico

```json
{
  "device_id": "camera_01",
  "event": "motion-detected",
  "timestamp": "2025-05-29T14:00:00"
}
```

---

## Flujo de trabajo

1. **Inicialización del ESP32:**
   - Conexión al WiFi local.
   - Conexión al broker MQTT.
   - Suscripción a los temas relevantes.

2. **Publicación y recepción de datos:**
   - Cada dispositivo publica sus eventos/sensores.
   - Los actuadores escuchan comandos del broker.

3. **Interacción con panel central (Vue.js):**
   - El panel se suscribe a los temas de interés.
   - Puede enviar comandos o visualizar eventos en tiempo real.

---

## Ventajas de MQTT

- Protocolo ligero, ideal para redes con recursos limitados
- Comunicación en tiempo real
- Escalabilidad a decenas o cientos de dispositivos
- Modelo pub/sub desacoplado

---

## Estado del proyecto

**Implementación estable**  
Actualmente en uso con:
- Cámaras ESP32-CAM
- Sensores de movimiento por ultrasonido
- Módulos de relé activados remotamente

---

## Licencia

Este proyecto está en fase de pruebas experimentales.  
Para implementación profesional o colaboración, contactar al desarrollador.

