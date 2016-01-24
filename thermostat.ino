
/***************************************************
 * Themostat d'ambiance
 * Connexion Wifi 
 * Connexion MQTT (IO Adafruit)
 * Connexion NTP 
 * Capteur DHT11
 ****************************************************/

// Libraries
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"

#include <ntp-client.h>

// Config DHT 11 sensor
#define DHTPIN 5
#define DHTTYPE DHT11 

// Config WiFi parameters
#define WLAN_SSID       "Livebox-79ca"
#define WLAN_PASS       "AD12566717FADDD9D4E9545412"

// Config NTP  server 
ntp* ntpServer; 
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server

// Config Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "jpinon"
#define AIO_KEY         "78f2cc8dde492223bcb1d15fc24b0d4da0df66e0"

// DHT sensor
DHT dht(DHTPIN, DHTTYPE, 15);

// Functions
void connect();

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Store the MQTT server, client ID, username, and password in flash memory.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;

// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = AIO_KEY __DATE__ __TIME__;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);/****************************** Feeds ***************************************/

// Setup feeds for temperature & humidity
const char TEMPERATURE_FEED[] PROGMEM = AIO_USERNAME "/feeds/temp_ch1";
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, TEMPERATURE_FEED);

const char HUMIDITY_FEED[] PROGMEM = AIO_USERNAME "/feeds/humid_ch1";
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, HUMIDITY_FEED);

// Setup a feed called 'thermostat' for subscribing to changes.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char THERMOSTAT_FEED[] PROGMEM = AIO_USERNAME "/feeds/thermostat";
Adafruit_MQTT_Subscribe thermostat = Adafruit_MQTT_Subscribe(&mqtt, THERMOSTAT_FEED);

/*************************** Sketch Code ************************************/

void setup() {

  // Init sensor
  dht.begin();

  Serial.begin(115200);
  delay(1000);
  Serial.println(F("Adafruit IO Example"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  delay(10);
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();

  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
  Serial.println("Request for Internet time");
  ntpServer = new ntp(localPort,timeServer);
  Serial.println (ntpServer->epochTimeString());
  //delay(1000);
  // connect to adafruit io
  connect();
  // listen for events on the lamp feed
  mqtt.subscribe(&thermostat);
  
}

unsigned int t=1000000; // index of mesures
// log every (in millis)
#define INTERVAL (2*60*1000) 


void loop() {
  Adafruit_MQTT_Subscribe *subscription;
  
  // ping adafruit io a few times to make sure we remain connected
  if(! mqtt.ping(3)) {
    // reconnect to adafruit io
    if(! mqtt.connected())
      connect();
  }
  
  // 
  // this is our 'wait for incoming subscription packets' busy subloop
  while (subscription = mqtt.readSubscription(1000)) {

    // we only care about the thermostat events
    if (subscription == &thermostat) {

      // convert mqtt ascii payload to int
      char *value = (char *)thermostat.lastread;
      Serial.print(F("Received: "));
      Serial.println(value);
      
    }

  }
    
  // every interval acquire temp for logs purposes
  if ((millis()/INTERVAL)!=t) {
    Serial.println (ntpServer->epochTimeString());
    t=millis()/INTERVAL;
  
    // Grab the current state of the sensor
    int humidity_data = (int)dht.readHumidity();
    Serial.print(F("Humidity:"));
    Serial.println(humidity_data);
    int temperature_data = (int)dht.readTemperature();
    Serial.print(F("Temperature:"));
    Serial.println(temperature_data);
  
    if ((temperature_data>-20) && (temperature_data<100)) {
      // Publish data
      if (! temperature.publish(temperature_data))
        Serial.println(F("Failed to publish temperature"));
      else
        Serial.println(F("Temperature published!"));
    }
      
    if ((humidity_data>=0) && (temperature_data<=100)) {
      if (! humidity.publish(humidity_data))
        Serial.println(F("Failed to publish humidity"));
      else
        Serial.println(F("Humidity published!"));    
    }
  }
}

// connect to adafruit io via MQTT
void connect() {

  Serial.print(F("Connecting to Adafruit IO... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {

    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if(ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(5000);

  }

  Serial.println(F("Adafruit IO Connected!"));

}
