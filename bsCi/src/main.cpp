/*
 * This sketch reads data from the command line and makes a http GET request
 * to save the data to a website.
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

//const char* ssid = "Kraftwerk";
//const char* password = "schokovulkan";
const char* ssid = "HUAWEI-E5220-25dc";
const char* password = "GD02ERQ6";

const char* host = "brieger-ultraschallhandel.de";
const char* fingerprint = "BD 7F 5D B7 FA 95 9A FF CD C8 3D B9 73 EA 87 E6 6C 37 69 D6";
const int httpsPort = 443;

const int LED_RGB_RED = 12;
const int LED_RGB_BLUE = 13;
const int LED_RGB_GREEN = 15;

const int LED_1 = 16;
const int LED_2 = 14;
const int LED_3 = 2;
const int LED_4 = 0;
const int LED_5 = 4;
const int LED_6 = 5;

const int SENSE_OUT = 5;
const int SENSE_IN_0 = 4;
const int SENSE_IN_1 = 16;
const int SENSE_IN_2 = 14;
const int SENSE_IN_3 = 13;

//function forward declaration
String urlencode(String str);

void setup() {
  //setup leds
  pinMode(LED_RGB_RED, OUTPUT);
  //pinMode(LED_RGB_BLUE, OUTPUT);
  pinMode(LED_RGB_GREEN, OUTPUT);

  pinMode(SENSE_OUT, OUTPUT);
  pinMode(SENSE_IN_0, INPUT);
  pinMode(SENSE_IN_1, INPUT);
  pinMode(SENSE_IN_2, INPUT);
  pinMode(SENSE_IN_3, INPUT);

  analogWrite(LED_RGB_RED,127);
  analogWrite(LED_RGB_BLUE,0);
  analogWrite(LED_RGB_GREEN,127);

  /*analogWrite(LED_1, 0);
  analogWrite(LED_2, 0);
  analogWrite(LED_3, 0);
  analogWrite(LED_4, 0);
  analogWrite(LED_5, 0);
  analogWrite(LED_6, 0);*/

  digitalWrite(SENSE_OUT, LOW);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting..\r\n");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Current sense input: ");
  Serial.println(digitalRead(SENSE_IN_0));
  digitalWrite(SENSE_OUT, HIGH);
  delay(50);
  Serial.print("Sense input after output HIGH: ");
  Serial.println(digitalRead(SENSE_IN_0));
  digitalWrite(SENSE_OUT, LOW);
  delay(50);
  Serial.print("Sense input after output LOW: ");
  Serial.println(digitalRead(SENSE_IN_0));

}

void loop() {
  uint8_t uiSense0 = 0;
  uint8_t uiSense1 = 0;
  uint8_t uiSense2 = 0;
  uint8_t uiSense3 = 0;

  digitalWrite(SENSE_OUT, HIGH);
  delay(50);
  uiSense0 = digitalRead(SENSE_IN_0);
  uiSense1 = digitalRead(SENSE_IN_1);
  uiSense2 = digitalRead(SENSE_IN_2);
  uiSense3 = digitalRead(SENSE_IN_3);
  digitalWrite(SENSE_OUT, LOW);

  int value = 0;

  value = (uiSense0 << 0) | (uiSense1 << 1) | (uiSense2 << 2) | (uiSense3 << 3);
  Serial.print("Current BS values are: ");
  Serial.print(uiSense0);
  Serial.print(" | ");
  Serial.print(uiSense1);
  Serial.print(" | ");
  Serial.print(uiSense2);
  Serial.print(" | ");
  Serial.print(uiSense3);
  Serial.print(", resulting value is ");
  Serial.println(value);

  if(value == 15)
  {
    analogWrite(LED_RGB_RED,0);
    analogWrite(LED_RGB_BLUE,0);
    analogWrite(LED_RGB_GREEN,255);
  }
  else
  {
    analogWrite(LED_RGB_RED,255);
    analogWrite(LED_RGB_BLUE,0);
    analogWrite(LED_RGB_GREEN,0);
  }

  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }


  String baseUrl = "/BrandschutzReport/status.php?";
  String macParam = "HeadMac=";
  String macValue = urlencode(WiFi.macAddress());
  String statusParam = "StatusReport=";
  String statusValue = String(value, DEC);
  String url = baseUrl + macParam + macValue + "&" + statusParam + statusValue;
  //String url = "/BrandschutzReport/status.php?HeadMac=00%3A80%3A41%3Aae%3Afd%3A7e&StatusReport=1";

  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");
  delay(500);
  Serial.println("reply was:");
  Serial.println("==========");

  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.println(line);
  }

  Serial.println("==========");
  Serial.println("closing connection");
  delay(10000);
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;

}
