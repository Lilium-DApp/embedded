#include "DHT.h"

#include <MQUnifiedsensor.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
// DHT11 settings
#define DHTPIN 2     
#define DHTTYPE DHT11 

// MQ135 setting
#define Board ("ESP32")
#define Pin (1) 
#define Type ("MQ-135") 
#define Voltage_Resolution (3.3) 
#define ADC_Bit_Resolution (32) 
#define RatioMQ2CleanAir (3.6) //RS / R0 = 3.6 ppm

// Wifi setting

#define WIFI_SSID "---"
#define WIFI_PASSWORD "---"

// MQTT settings
#define BROKER_SERVER "---"
#define BROKER_PORT 8883
#define BROKER_USER "---"
#define BROKER_PASSWORD "---"

struct Package {
	float temperature_celsius;
	float humidity_percent;
	float light_lux;
	float co_ppm;
};

DHT dht(DHTPIN, DHTTYPE);
MQUnifiedsensor MQ2(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type);

WiFiClientSecure espClient;
PubSubClient client(espClient);

void connectWIFI() {
	Serial.print("Connecting to WiFi...");
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("  done!.");
}

void connectMQTT() {
	Serial.print("Connecting to MQTT broker...");
	espClient.setInsecure();
	client.setServer(BROKER_SERVER, BROKER_PORT);
	while (!client.connected()) {
		if (client.connect("MicrocontrollerClient", BROKER_USER, BROKER_PASSWORD)) {
			Serial.println("  done!.");
		} else {
			Serial.print("failed with state ");
			Serial.print(client.state());
			delay(2000);
		}
	}
}

void mq135_calibrate() {

  MQ2.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ2.setA(36974); MQ2.setB(-3.109);

  /*
    Exponential regression:
    Gas    | a      | b
    H2     | 987.99 | -2.162
    LPG    | 574.25 | -2.222
    CO     | 36974  | -3.109
    Alcohol| 3616.1 | -2.675
    Propane| 658.71 | -2.168
  */

  MQ2.init(); 

  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ2.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ2.calibrate(RatioMQ2CleanAir);
    Serial.print(".");
  }
  MQ2.setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}

}

float get_co() {
  MQ2.update(); 
  return  MQ2.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
}

void setup() {

  Serial.begin(115200);

  connectWIFI();
	connectMQTT();

  mq135_calibrate(); 

  dht.begin();
}

void publish(Package package){

  client.publish("Lilium/temperature", String(package.temperature_celsius).c_str());
	client.publish("Lilium/humidity", String(package.humidity_percent).c_str());
	client.publish("Lilium/CO", String(package.co_ppm).c_str());

}

void loop() {

  Package package;
  package.temperature_celsius = dht.readTemperature();
	package.humidity_percent = dht.readHumidity();
	package.co_ppm = get_co();

  Serial.println("Temperature: " + String(package.temperature_celsius) + " C");
	Serial.println("Humidity: " + String(package.humidity_percent) + " %");
	Serial.println("CO: " + String(package.co_ppm) + " ppm");
	Serial.println();

  publish(package);
  delay(1000);
  
}