#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include "config.h"

#define USE_ARDUINO_INTERRUPTS false
#include <PulseSensorPlayground.h>

PulseSensorPlayground pulseSensor;
const int PULSE_INPUT = A0;
const int PULSE_BLINK = LED_BUILTIN;    // Pin 13 is the on-board LED
const int THRESHOLD = 550;   // Adjust this number to avoid noise when idle

byte samplesUntilReport;
const byte SAMPLES_PER_SERIAL_SAMPLE = 10;
WebSocketsClient webSocket;
long delayToSend = 500;
long lastSend = 0;

void setup() {
  Serial.begin(115200);
  //while(!Serial.available());
    Serial.println("Connecting");  
  pinMode(PULSE_INPUT, INPUT);
  samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

  WiFi.begin(ssid, wifiPwd);
    Serial.println("Connecting");  
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }
    Serial.println("Connection established!");  

  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.blinkOnPulse(PULSE_BLINK);

  pulseSensor.setSerial(Serial);
  pulseSensor.setThreshold(THRESHOLD);

  if (!pulseSensor.begin()) {
    Serial.println("Error init pulse sensor");
  }

  tryToConnectWS();
}

void loop() {
	webSocket.loop();
  if (pulseSensor.sawNewSample()) {
    if (--samplesUntilReport == (byte) 0) {
      samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

      int value = pulseSensor.getBeatsPerMinute();
      if (millis() - lastSend > delayToSend) {
        String data = "{\"BPM\": " + String(value) + "}";
        Serial.println(data);
        webSocket.sendTXT(data);
        lastSend = millis();
      }
    }
  }
}

void tryToConnectWS() {
	webSocket.begin("192.168.1.198", 8181, "/");
	webSocket.setReconnectInterval(5000);
	// event handler
	webSocket.onEvent(webSocketEvent);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED: 
			Serial.printf("[WSc] Connected to url: %s\n", payload);
			break;
    }
}