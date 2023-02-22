#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include "Colors.h"

namespace Connect
{
    namespace Wifi
    {

        class ESP32Wifi
        {
        public:
            /**
             *
             */

            // String dId = "C-0002";
            // String webhook_pass = "YeNA8fMt5Y";

            // dId = "C-0001";
            // webhook_pass = "utW-ATFgEV";

            ESP32Wifi()
            {
                dId = "C-0001";
                webhook_pass = "utW-ATFgEV";
                webhook_endpoint = "http://192.168.0.111:3030/api/getdevicecredentials";
                //WiFi
                // wifi_ssid = "HTC";
                wifi_ssid = "Flybox_B96F";
                wifi_password = "Tr0p1c0420";
            }

            bool wifi_status()
            {

                if (WiFi.status() != WL_CONNECTED)
                    return false;
                else
                    return true;
            }

            void setup_wifi()
            {
                delay(10);
                Serial.print(underlinePurple + "\n\n\nWiFi Connection in Progress" + fontReset + Purple);
                // WiFi.begin(wifi_ssid, wifi_password);
                WiFi.mode(WIFI_STA);
                WiFi.begin(wifi_ssid, wifi_password);

                int counter = 0;

                while (WiFi.status() != WL_CONNECTED)
                {
                    delay(500);
                    Serial.print(".");
                    counter++;

                    if (counter > 10)
                    {
                        Serial.print("  ⤵" + fontReset);
                        Serial.print(Red + "\n\n         Ups WiFi Connection FaiLED :( ");
                        Serial.println(" -> Restarting..." + fontReset);
                        delay(2000);
                        ESP.restart();
                    }
                }

                Serial.print("  ⤵" + fontReset);

                //Printing local ip
                Serial.println(boldGreen + "\n\n         WiFi Connection -> SUCCESS :)" + fontReset);
                Serial.print("\n         Local IP -> ");
                Serial.print(boldBlue);
                Serial.print(WiFi.localIP());
                Serial.println(fontReset);
            }

            String get_mqtt_credentials()
            {

                Serial.print(underlinePurple + "\n\n\nGetting MQTT Credentials from WebHook" + fontReset + Purple + "  ⤵");
                delay(1000);

                String toSend = "dId=" + dId + "&password=" + webhook_pass;

                HTTPClient http;
                http.begin(webhook_endpoint);
                http.addHeader("Content-Type", "application/x-www-form-urlencoded");

                int response_code = http.POST(toSend);

                if (response_code < 0)
                {
                    Serial.print(boldGreen + "\n\n  " + webhook_endpoint);
                    Serial.print(boldGreen + "\n\n  " + toSend);
                    Serial.print(boldRed + "\n\n         Error Sending Post Request :( " + fontReset);

                    Serial.print(response_code);
                    http.end();
                    return "false";
                }

                if (response_code != 200)
                {
                    Serial.print(boldRed + "\n\n         Error in response :(   e-> " + fontReset + " " + response_code);
                    http.end();
                    return "false";
                }

                if (response_code == 200)
                {
                    String responseBody = http.getString();

                    Serial.print(boldGreen + "\n\n         Mqtt Credentials Obtained Successfully :) " + fontReset);

                    http.end();
                    delay(1000);

                    return responseBody;
                }

                return "false";
            }

            /**
             *
             * @return
             */

        protected:
            String dId;
            String webhook_pass;
            String webhook_endpoint;
            //WiFi
            const char *wifi_ssid;
            const char *wifi_password;
        };
    }
}