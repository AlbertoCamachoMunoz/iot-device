
// contactor para motor trifasico https://www.youtube.com/watch?v=wGYn83sksgE&ab_channel=iQuimiCAD

// *************************************
// *************  CAMARA DEFINE
// *************************************
#define CAMERA_MODEL_AI_THINKER

#include <FS.h>
#include <SPIFFS.h>
#include "EloquentVision.h"

#define FRAME_SIZE FRAMESIZE_QVGA
#define SOURCE_WIDTH 320
#define SOURCE_HEIGHT 240
#define CHANNELS 1
#define DEST_WIDTH 32
#define DEST_HEIGHT 24
#define BLOCK_VARIATION_THRESHOLD 0.3
#define MOTION_THRESHOLD 0.2

using namespace Eloquent::Vision;
using namespace Eloquent::Vision::IO;
using namespace Eloquent::Vision::ImageProcessing;
using namespace Eloquent::Vision::ImageProcessing::Downscale;
using namespace Eloquent::Vision::ImageProcessing::DownscaleStrategies;

// an easy interface to capture images from the camera
ESP32Camera camera;
// the buffer to store the downscaled version of the image
uint8_t resized[DEST_HEIGHT][DEST_WIDTH];
// the downscaler algorithm
// for more details see https://eloquentarduino.github.io/2020/05/easier-faster-pure-video-esp32-cam-motion-detection
// Cross<SOURCE_WIDTH, SOURCE_HEIGHT, DEST_WIDTH, DEST_HEIGHT> crossStrategy;
Center<SOURCE_WIDTH, SOURCE_HEIGHT, DEST_WIDTH, DEST_HEIGHT> crossStrategy;
// the downscaler container
Downscaler<SOURCE_WIDTH, SOURCE_HEIGHT, CHANNELS, DEST_WIDTH, DEST_HEIGHT> downscaler(&crossStrategy);
// the motion detection algorithm
MotionDetection<DEST_WIDTH, DEST_HEIGHT> motion;

// *************************************
// *************  END CAMARA DEFINE
// *************************************

#include "ConnectWifi.h"

using namespace Connect::Wifi;
ESP32Wifi ESP32_Wifi;

#include <Arduino.h>
#include "Colors.h"
#include "mbedtls/base64.h"
#include <base64.h>

#include "IoTicosSplitter.h"
#include "Ultrasound.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>

const char *mqtt_server = "192.168.0.111";

//PINS
#define LED 4
#define FAN_DEFINE 2
#define LIGHT_DEFINE 12

// const char *wifi_password = "Tesla208";

//Functions definitions
void check_mqtt_connection();
bool reconnect();
void process_sensors();
void process_actuators();
void send_data_input_to_broker();
void send_topic_data(int positionArray);
void callback(char *topic, byte *payload, unsigned int length);
void process_incoming_msg(String topic, String incoming);
void onOffLed();
void onOffLight(bool signal, int positionArray);
void onOffFan(bool signal, int positionArray);

void sendHumidity(int i);
void sendTemperature(int i);

void print_stats();
void clear();

//Global Vars
String dId = "C-0001";

WiFiClient espclient;
PubSubClient client(espclient);
IoTicosSplitter splitter;
Ultrasound radar;
long lastReconnectAttemp = 0;
long varsLastSend[20];
String last_received_msg = "";
String last_received_topic = "";
int prev_temp = 0;
int prev_hum = 0;

// CORE 0
TaskHandle_t Task1;

// Ultrasound variable
int cm = 0;
bool useUltraSound = false;
int positionAlarm = -1;
bool sendAlarm = false;
int lastMsgAlarm = 0;
camera_fb_t *fb = NULL;
sensor_t *sensor = NULL;
DynamicJsonDocument mqtt_data_doc(2048);

//***   TAREA OTRO NUCLEO   ***
int lastMsgTask = 0;
void codeForTask1(void *parameter)
{
	for (;;)
	{

		long now = millis();
		if (now - lastMsgTask > 500)
		{
			lastMsgTask = millis();

			if (useUltraSound == true && positionAlarm != -1)
			{
				cm = radar.distance();
				mqtt_data_doc["variables"][positionAlarm]["last"]["value"] = cm;
			}
		}

		vTaskDelay(100);
	}
}

void setup()
{

	SPIFFS.begin(true);
	camera.begin(FRAME_SIZE, PIXFORMAT_GRAYSCALE);
	motion.setBlockVariationThreshold(BLOCK_VARIATION_THRESHOLD);
	Serial.begin(115200);

	//***   TAREA OTRO NUCLEO   ***
	xTaskCreatePinnedToCore(
		codeForTask1, /* Task function. */
		"Task_1",	  /* name of task. */
		10000,		  /* Stack size of task */
		NULL,		  /* parameter of the task */
		1,			  /* priority of the task */
		&Task1,		  /* Task handle to keep track of created task */
		0);

	pinMode(LED, OUTPUT);
	pinMode(FAN_DEFINE, OUTPUT);
	pinMode(LIGHT_DEFINE, OUTPUT);
	clear();

	ESP32_Wifi.setup_wifi();
	client.setCallback(callback);
}

void loop()
{
	fb = camera.capture();
	check_mqtt_connection();
}

void onOffLed()
{
	delay(400);
	digitalWrite(LED, LOW);
	delay(400);
	digitalWrite(LED, HIGH);
	delay(400);
	digitalWrite(LED, LOW);
}

/******************************/
/*****DATA FROM TEMPLATE*******/
/******************************/

void callback(char *topic, byte *payload, unsigned int length)
{

	String incoming = "";

	for (int i = 0; i < length; i++)
	{
		incoming += (char)payload[i];
	}

	incoming.trim();

	process_incoming_msg(String(topic), incoming);
}

//TEMPLATE ⤵
void process_incoming_msg(String topic, String incoming)
{
	Serial.print("\n         PROCESANDO MENSAJE -> ");
	last_received_topic = topic;
	last_received_msg = incoming;

	String variable = splitter.split(topic, '/', 2);

	for (int i = 0; i < mqtt_data_doc["variables"].size(); i++)
	{

		if (mqtt_data_doc["variables"][i]["variable"] == variable)
		{

			DynamicJsonDocument doc(256);
			deserializeJson(doc, incoming);
			mqtt_data_doc["variables"][i]["last"] = doc;

			long counter = mqtt_data_doc["variables"][i]["counter"];
			counter++;
			mqtt_data_doc["variables"][i]["counter"] = counter;
		}
	}

	process_actuators();
}

void process_actuators()
{
	Serial.print("\n         RECIBIDO A ACTUATOR -> ");
	String last = mqtt_data_doc["variables"][3]["last"];
	String value = mqtt_data_doc["variables"][3]["last"]["value"];

	Serial.print("\n " + last);
	Serial.print("\n " + value);

	for (int i = 0; i < mqtt_data_doc["variables"].size(); i++)
	{
		if (mqtt_data_doc["variables"][i]["variableType"] == "input")
		{
			continue;
		}

		if (mqtt_data_doc["variables"][i]["variableFullName"] == "on/off")
		{
			mqtt_data_doc["variables"][i]["last"]["value"] = true;
		}

		if (mqtt_data_doc["variables"][i]["variableFullName"] == "fan")
		{
			Serial.print("\n         FAN -> ");
			onOffFan(mqtt_data_doc["variables"][i]["last"]["value"], i);
		}

		if (mqtt_data_doc["variables"][i]["variableFullName"] == "light")
		{
			Serial.print("\n         LIGHT -> ");
			onOffLight(mqtt_data_doc["variables"][i]["last"]["value"], i);
		}
	}
}

// LA LOGICA DEL VENTILADOR ESTÁ INVERTIDA POR LOS RELES
void onOffFan(bool signal, int positionArray)
{
	if (signal == true)
	{
		Serial.print("\n        FAN SI -> ");
		delay(10);
		digitalWrite(FAN_DEFINE, LOW);
		onOffLed();

		varsLastSend[positionArray] = 0;
	}
	else if (signal == false)
	{
		Serial.print("\n        FAN NO -> ");
		delay(10);
		digitalWrite(FAN_DEFINE, HIGH);
		onOffLed();
		varsLastSend[positionArray] = 0;
	}
	send_topic_data(positionArray);
}

void onOffLight(bool signal, int positionArray)
{
	if (signal == true)
	{
		Serial.print("\n        LIGHT SI -> ");
		delay(10);
		digitalWrite(LIGHT_DEFINE, LOW);
		digitalWrite(LED, HIGH);
		varsLastSend[positionArray] = 0;
	}
	else if (signal == false)
	{
		Serial.print("\n       LIGHT  NO -> ");
		delay(10);
		digitalWrite(LIGHT_DEFINE, HIGH);
		digitalWrite(LED, LOW);
		varsLastSend[positionArray] = 0;
	}
	send_topic_data(positionArray);
}

/******************************/
/*****  EXECUTE IN LOOP *******/
/******************************/

void check_mqtt_connection()
{

	if (ESP32_Wifi.wifi_status() != true)
	{
		Serial.print(Red + "\n\n         Ups WiFi Connection FaiLED :( ");
		Serial.println(" -> Restarting..." + fontReset);
		onOffLed();
		delay(7000);
		ESP.restart();
	}

	if (!client.connected())
	{
		onOffLed();
		delay(1400);

		long now = millis();

		if (now - lastReconnectAttemp > 5000)
		{
			lastReconnectAttemp = millis();
			if (reconnect())
			{
				lastReconnectAttemp = 0;
			}
		}
	}
	else
	{
		client.loop();
		process_sensors();
		send_data_input_to_broker();
		// print_stats();
	}
}

bool reconnect()
{

	if (ESP32_Wifi.get_mqtt_credentials() == "false")
		return false;

	deserializeJson(mqtt_data_doc, ESP32_Wifi.get_mqtt_credentials());

	// Setting up Mqtt Server
	client.setServer(mqtt_server, 1883);

	Serial.print(underlinePurple + "\n\n\nTrying MQTT Connection" + fontReset + Purple + "  ⤵");

	String str_client_id = "device_" + dId + "_" + random(1, 9999);

	const char *username = mqtt_data_doc["username"];
	const char *password = mqtt_data_doc["password"];
	String str_topic = mqtt_data_doc["topic"];

	if (client.connect(str_client_id.c_str(), username, password))
	{
		Serial.print(boldGreen + "\n\n         Mqtt Client Connected :) " + fontReset);
		delay(2000);

		client.subscribe((str_topic + "+/actdata").c_str());
	}
	else
	{
		Serial.print(boldRed + "\n\n         Mqtt Client Connection FaiLED :( " + fontReset);
		return false;
	}
	return true;
}

/******************************/
/*****  DATA TO BROKER  *******/
/******************************/

//USER FUNTIONS ⤵
void process_sensors()
{

	for (int i = 0; i < mqtt_data_doc["variables"].size(); i++)
	{
		if (mqtt_data_doc["variables"][i]["variableType"] == "output")
		{
			continue;
		}
		// ESTA IMPLEMENTACION LA HARÉ EN UN FUTURO
		// if (mqtt_data_doc["variables"][i]["variableFullName"] == "humidity")
		// {
		// 	sendHumidity(i);
		// }

		// if (mqtt_data_doc["variables"][i]["variableFullName"] == "temperature")
		// {
		// 	sendTemperature(i);
		// }
		if (mqtt_data_doc["variables"][i]["variableFullName"] == "camera__")
		{
			if (sendAlarm != false)
				return;

			if (fb->format != PIXFORMAT_GRAYSCALE)
				camera.changeFormat(PIXFORMAT_GRAYSCALE);

			// resize image and detect motion
			downscaler.downscale(fb->buf, resized);
			motion.update(resized);
			Serial.println(motion.detect());
			if (motion.detect() > 110)
			{
				Serial.println("Motion detected");
				if (positionAlarm != -1)
					mqtt_data_doc["variables"][positionAlarm]["last"]["value"] = 100;
			}
			else
			{
				mqtt_data_doc["variables"][positionAlarm]["last"]["value"] = -1;
			}
		}
		if (mqtt_data_doc["variables"][i]["variableFullName"] == "alarm")
		{
			useUltraSound = true;
			positionAlarm = i;
		}
	}
}

void sendHumidity(int i)
{
	//get humidity simulation
	// int hum = random(1, 50);
	// mqtt_data_doc["variables"][i]["last"]["value"] = hum;

	// //save hum?
	// int dif = hum - prev_hum;
	// if (dif < 0)
	// {
	// 	dif *= -1;
	// }

	// if (dif >= 20)
	// {
	// 	mqtt_data_doc["variables"][i]["last"]["save"] = 1;
	// }
	// else
	// {
	// 	mqtt_data_doc["variables"][i]["last"]["save"] = 1;
	// }

	// prev_hum = hum;
	int chicle = 0;
}

void sendTemperature(int i)
{
	int chicle = 0;
}

void send_data_input_to_broker()
{

	long now = millis();

	for (int i = 0; i < mqtt_data_doc["variables"].size(); i++)
	{

		if (mqtt_data_doc["variables"][i]["variableType"] == "output")
		{
			continue;
		}

		int freq = mqtt_data_doc["variables"][i]["variableSendFreq"];

		if (now - varsLastSend[i] > freq * 1000)
		{
			varsLastSend[i] = millis();

			send_topic_data(i);

			//STATS
			long counter = mqtt_data_doc["variables"][i]["counter"];
			counter++;
			mqtt_data_doc["variables"][i]["counter"] = counter;
		}
	}
}

void send_topic_data(int i)
{
	String str_root_topic = mqtt_data_doc["topic"];
	String str_variable = mqtt_data_doc["variables"][i]["variable"];
	String str_variableFullName = mqtt_data_doc["variables"][i]["variableFullName"];
	String topic = "";

	topic = str_root_topic + str_variable + "/sdata";
	String toSend = "";
	serializeJson(mqtt_data_doc["variables"][i]["last"], toSend);

	if (str_variableFullName == "alarm")
	{
		if (sendAlarm == false)
		{
			if (mqtt_data_doc["variables"][i]["last"]["value"] < 150 && mqtt_data_doc["variables"][i]["last"]["value"] > 5)
			{

				Serial.println("Enviando distancia");
				Serial.println(cm);

				client.publish(topic.c_str(), toSend.c_str());
				sendAlarm = true;
			}
		}
		else
		{

			long now = millis();
			if (now - lastMsgAlarm > 10000)
			{
				lastMsgAlarm = millis();
				sendAlarm = false;
			}
		}
	}

	else if (str_variableFullName == "camera")
	{
		topic = str_root_topic + str_variable + "/camera";
		Serial.println("send image black and white:  ");

		size_t _jpg_buf_len = 0;
		uint8_t *_jpg_buf = NULL;

		if (fb->format != PIXFORMAT_JPEG)
		{
			Serial.println("formateando");
			bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
			if (!jpeg_converted)
			{
				Serial.println("JPEG compression failed");
				return;
			}
		}
		else
		{
			Serial.println("NO necesita formatear");
			_jpg_buf_len = fb->len;
			_jpg_buf = fb->buf;
		}

		delay(500);

		String encoded = base64::encode(_jpg_buf, _jpg_buf_len);
		Serial.println("encoded");
		Serial.println(encoded);
		if (encoded != "")
			client.publish(topic.c_str(), encoded.c_str());
		else
			Serial.println("encoded failed");
	}
	else
		client.publish(topic.c_str(), toSend.c_str());
}

void clear()
{
	Serial.write(27);	 // ESC command
	Serial.print("[2J"); // clear screen command
	Serial.write(27);
	Serial.print("[H"); // cursor to home command
}

long lastStats = 0;

void print_stats()
{
	long now = millis();

	if (now - lastStats > 2000)
	{
		lastStats = millis();
		clear();

		Serial.print("\n");
		Serial.print(Purple + "\n╔══════════════════════════╗" + fontReset);
		Serial.print(Purple + "\n║       SYSTEM STATS       ║" + fontReset);
		Serial.print(Purple + "\n╚══════════════════════════╝" + fontReset);
		Serial.print("\n\n");
		Serial.print("\n\n");

		Serial.print(boldCyan + "#" + " \t Name" + " \t\t Var" + " \t\t Type" + " \t\t Count" + " \t\t Last V" + fontReset + "\n\n");

		for (int i = 0; i < mqtt_data_doc["variables"].size(); i++)
		{

			String variableFullName = mqtt_data_doc["variables"][i]["variableFullName"];
			String variable = mqtt_data_doc["variables"][i]["variable"];
			String variableType = mqtt_data_doc["variables"][i]["variableType"];
			String lastMsg = mqtt_data_doc["variables"][i]["last"];
			long counter = mqtt_data_doc["variables"][i]["counter"];

			Serial.println(String(i) + " \t " + variableFullName.substring(0, 5) + " \t\t " + variable.substring(0, 10) + " \t " + variableType.substring(0, 5) + " \t\t " + String(counter).substring(0, 10) + " \t\t " + lastMsg);
		}

		Serial.print(boldGreen + "\n\n Free RAM -> " + fontReset + ESP.getFreeHeap() + " Bytes");

		Serial.print(boldGreen + "\n\n Last Incomming Msg -> " + fontReset + last_received_msg);
	}
}