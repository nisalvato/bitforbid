#include <Adafruit_BMP085.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>

/**
 * An example showing how to put ESP8266 into Deep-sleep mode
 * GPI00 to ground during flashing (without VCC)
 * GPI00 free and vcc during execution
 * flashing phase: 
 *            1- OFF VCC ESP
 *            2- GP00 to ground
 *            3- plug USB adaptor 
 *            4- FLash
 * execution PHASE
 *            1- OFF VCC ESP
 *            2- GP00 Free
 *            3- ON VCC ESP
 *            4- PLUG ESP Adaptor
 */
// Time to sleep (in minutes):
//const int sleepTimeS = 60;

//Wi Fi
const char* ssid     = "SSID";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "Wifipwd";  

//DomoticZ
const char* host = "***.***.***.***"; //Domoticz IP
const int httpPort1 = 8080; //DomoticZ Port
const int httpPort2 = 80; //Web Meteo station port

//Static IP address configuration
IPAddress staticIP(192, 168, 1, 41); //ESP static ip
IPAddress gateway(192, 168, 1, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(8, 8, 8, 8);  //DNS

//Barometer
Adafruit_BMP085 bmp;

//flag della previsione
int prev=0;

void setup() {

  //Force Sleep
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay( 1 );
  
  //Serial.begin(115200);
  delay(100);
  // Wait for serial to initialize.
  //while(!//Serial) { }
  delay(100);
  //Serial.println('\n');
  //Serial.println("I'm awake.");

}

void loop() {
  //Hey sensor, give me the data!
    if (!bmp.begin()) {
        //Serial.println("Could not find a valid BMP085/BMP180 sensor, check wiring!");
  }
  delay(1); 
  //Serial.println("Temperature = ");
  //Serial.println(bmp.readTemperature());
  //Serial.println(" *C");
  //Serial.print("Pressure = ");
  //Serial.print(bmp.readPressure());
  //Serial.println(" Pa");
    
  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  //Serial.print("Altitude = ");
  //Serial.print(bmp.readAltitude());
  //Serial.println(" meters");

  //Serial.print("Pressure at sealevel (calculated) = ");
  //Serial.print(bmp.readSealevelPressure());
  //Serial.println(" Pa");

  //Serial.print("connecting to ");
  //Serial.println(host);
  float temperature = bmp.readTemperature();
  int pressure = bmp.readPressure()/100;
  int altitude = bmp.readAltitude();

  //umbrella or not umbrella?
  forecast(pressure);

  // Bring up the WiFi connection
  WiFi.forceSleepWake();
  delay( 1 );
  WiFi.mode( WIFI_STA );
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);             // Connect to the network
  //Serial.print("Connecting to ");
  //Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    //Serial.print(++i); Serial.print(' ');
  }

  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  if (!client.connect(host, httpPort1)) {
    //Serial.println("connection failed");
    sleepWell();
  }

  // We now create a URI for the request for Domoticz
  String url = "/json.htm?type=command&param=udevice&idx=29&nvalue=0&svalue=";
  url += String(temperature, 1);
  url += ";";
  url += String(pressure);
  url += ";";
  url += String(prev);
  url += ";";
  url += "0";

  //json string to domoticz http://192.168.1.112:8080/json.htm?type=command&param=udevice&idx=29&nvalue=0&svalue=65;800;4;0
  //Serial.print("Requesting URL: ");
  //Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      //Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to //Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.print(line);
  }

  //Serial.println();
  //Serial.println("closing connection");


  WiFiClient client2;
//Meteo data archive
  if (!client2.connect(host, httpPort2)) {
    sleepWell();
  }

  // We now create a URI for the request for Domoticz
  url = "/meteostation/insertmeteostation.php?idx=29&tempdata=";
  url += String(temperature, 1);
  url += "&pressdata=";
  url += String(pressure);


  //Serial.print("Requesting URL: ");
  //Serial.println(url);

  // This will send the request to the server
  client2.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  timeout = millis();
  while (client2.available() == 0) {
    if (millis() - timeout > 5000) {
      //Serial.println(">>> Client Timeout !");
      client2.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to //Serial
  while (client2.available()) {
    String line = client2.readStringUntil('\r');
    //Serial.print(line);
  }

  //Serial.println();
  //Serial.println("closing connection");
 
  sleepWell();
                
}

void forecast(int analogTemp){
      delay(1);
      //Serial.println('\n');
      //Serial.println("looping");
      EEPROM.begin(512);
      delay(1);
      //Serial.println('\n');
      int val0 = int(EEPROM.read(0));
      int val1 = int(EEPROM.read(1));
      int compare=0;
      if (val0==0){
        //Scrivi su 0  
           //Serial.println("Scrivo su 0");   
           EEPROM.write(0, int(analogTemp/4.2));    
      }else{
            if (val1==0){
              //Scrivi su 1
              //Serial.println("Scrivo su 1");
              EEPROM.write(1, int(analogTemp/4.2));       
            }else{
              //previsione
              compare= analogTemp-(int(EEPROM.read(0)))*4.2;
              if(compare <=-20){
               //Serial.println("PEGGIORAMENTO: "+String(compare));
               prev= 4;
              }else{
                if((compare<=-10)&&(compare>-20)){
                   //Serial.println("PEGGIORAMENTO LIEVE: "+String(compare));
                   prev= 3;
                }else{
                  if((compare<=0)&&(compare>-10)){
                   //Serial.println("PEGGIORAMENTO  molto LIEVE: "+String(compare));
                   prev= 2;
                }else{
                      if(compare>0){
                        //Serial.println("miglioramento: "+String(compare));
                        prev= 1;
                      }
                  }
                }
              }
              
              //Shift
              //Serial.println("Shift");
              EEPROM.write(0, int(val1)); 
              EEPROM.commit();
              EEPROM.write(1, int(analogTemp/4.2)); 
              //Serial.println("memory");
              //Serial.println("ind 0: "+ String((int(EEPROM.read(0)))*4.2));
              //Serial.println("ind 1: "+ String((int(EEPROM.read(1)))*4.2)); 
            }
      } 
      EEPROM.commit();     
}

void sleepWell(){
    //Serial.println("Going into deep sleep");
    WiFi.disconnect( true );
    delay(100 );
    ESP.deepSleep(3740000000,WAKE_RF_DISABLED); //
  }

