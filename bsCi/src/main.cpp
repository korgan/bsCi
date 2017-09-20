/*
 * This sketch reads data from the command line and makes a http GET request
 * to save the data to a website.
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <Ticker.h>

/**
  * WIFI
  */
  //no need to make the WiFiManager global as we only need the WiFiMulti after it has done its job.
ESP8266WiFiMulti WiFiMulti;


/**
  * Web parameters
  */
const char* HOST = "brieger-ultraschallhandel.de";
const char* FINGERPRINT = "‎‎BD 7F 5D b7 FA 95 9A FF CD C8 3D B9 73 EA 87 E6 6C 37 69 D6"; //!< Fingerprints are constant on the ESP as parsing the certificate is too expensive.
const uint16_t HTTPS_PORT = 443;

/**
  * Sense input
  */
Ticker senseTicker;
bool bSenseEnabled = false;
const uint8_t SENSE_INTERVAL_S = 10;

/**
  * Firmware update
  */
Ticker updateCheckTicker;
bool bUpdateEnabled = false;
const uint8_t UPDATE_CHECK_INTERVAL_S = 60;
const String FIRMWARE_UPDATE_URL = "/BrandschutzBoard/firmware.php?tag=";
//this macro allows the compiler to put the git hash into the code
#define TEXTIFY(A) #A
#define ESCAPEQUOTE(A) TEXTIFY(A)
String sBuildTag = ESCAPEQUOTE(BUILD_TAG);

/**
  * PIN DEFINITIONS
  */
const uint8_t LED_RGB_RED   = 12;
const uint8_t LED_RGB_BLUE  = 13;
const uint8_t LED_RGB_GREEN = 15;

const uint8_t LED_1 = 16;
const uint8_t LED_2 = 14;
const uint8_t LED_3 = 2;
const uint8_t LED_4 = 0;
const uint8_t LED_5 = 4;
const uint8_t LED_6 = 5;

const uint8_t SENSE_OUT  = 5;
const uint8_t SENSE_IN_0 = 4;
const uint8_t SENSE_IN_1 = 16;
const uint8_t SENSE_IN_2 = 14;
const uint8_t SENSE_IN_3 = 13;

/**
  * Forward Declarations
  */
String urlencode(String str); //!< URL encodes a string.
uint8_t getCurrentState(); //!< Reads the current state of the break sense inputs.
void enableSense()  { bSenseEnabled  = true; }  //!< Enables the sensing.
void enableUpdate() { bUpdateEnabled = true; } //!< Enables the update flag;
void setRgbLed(uint8_t puiRed, uint8_t puiGreen, uint8_t puiBlue); //!< Set the RGB LED.
WiFiClientSecure getSecureClient(); //!< Returns a secure connection to the HOST defined above.

void setup() {
  //setup leds
  pinMode(LED_RGB_RED, OUTPUT);
  //we have to re-use the blue led as an input as some other inputs on the ESP are wonky
  //pinMode(LED_RGB_BLUE, OUTPUT);
  pinMode(LED_RGB_GREEN, OUTPUT);

  pinMode(SENSE_OUT, OUTPUT);
  pinMode(SENSE_IN_0, INPUT);
  pinMode(SENSE_IN_1, INPUT);
  pinMode(SENSE_IN_2, INPUT);
  pinMode(SENSE_IN_3, INPUT);

  setRgbLed(127,127,0);

  digitalWrite(SENSE_OUT, LOW);

  Serial.begin(115200);

  WiFiManager wifiManager;
  wifiManager.autoConnect();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //update time
  configTime(3 * 3600, 0, "pool.ntp.org");

  //test the input
  uint8_t uiValue = getCurrentState();
  Serial.print("Current value is ");
  Serial.println(uiValue);

  senseTicker.attach(SENSE_INTERVAL_S, enableSense);
  updateCheckTicker.attach(UPDATE_CHECK_INTERVAL_S, enableUpdate);
}

void loop() {
  if(bSenseEnabled)
  {
    uint8_t uiValue = getCurrentState();
    Serial.print("Current value is ");
    Serial.println(uiValue);

    //if everything is okay, the LED is green
    if(uiValue == 15)
    {
      setRgbLed(0,255,0);
    }
    else
    {
      setRgbLed(255,0,0);
    }

    if((WiFiMulti.run() == WL_CONNECTED))
    {
      WiFiClientSecure client = getSecureClient();

      if(client != NULL)
      {
        String baseUrl = "/BrandschutzReport/status.php?";
        String macParam = "HeadMac=";
        String macValue = urlencode(WiFi.macAddress());
        String statusParam = "StatusReport=";
        String statusValue = String(uiValue, DEC);
        String url = baseUrl + macParam + macValue + "&" + statusParam + statusValue;
        //String url = "/BrandschutzReport/status.php?HeadMac=00%3A80%3A41%3Aae%3Afd%3A7e&StatusReport=1";

        Serial.print("requesting URL: ");
        Serial.println(url);

        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + HOST + "\r\n" +
                     "User-Agent: ESP8266\r\n" +
                     "Connection: close\r\n\r\n");
        delay(50);
        Serial.println("reply was:");
        Serial.println("==========");

        while(client.available())
        {
          String line = client.readStringUntil('\r');
          Serial.println(line);
        }
        Serial.println("==========");
      }
    }

    bSenseEnabled = false;
  }

  //update check
  if(bUpdateEnabled)
  {
    Serial.println("Going to update firmware...");
    if((WiFiMulti.run() == WL_CONNECTED))
    {

        Serial.println("Checking for Update. Current version: " + sBuildTag);
        String url = FIRMWARE_UPDATE_URL + sBuildTag;

        //    t_httpUpdate_return update(const String& host, uint16_t port, const String& url,
        //                       const String& currentVersion, const String& httpsFingerprint);
        t_httpUpdate_return ret = ESPhttpUpdate.update(HOST, HTTPS_PORT, url, String(""), FINGERPRINT);

        switch(ret)
        {
            case HTTP_UPDATE_FAILED:
                Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                break;
            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("HTTP_UPDATE_NO_UPDATES");
                break;
            case HTTP_UPDATE_OK:
                Serial.println("HTTP_UPDATE_OK");
                break;
            default:
              Serial.println("UPDATE ERROR");
              break;
        }
      }
      bUpdateEnabled = false;
  }
}

/**
* Reads the current state of the break sense inputs.
*/
uint8_t getCurrentState()
{
  uint8_t uiSense0 = 0;
  uint8_t uiSense1 = 0;
  uint8_t uiSense2 = 0;
  uint8_t uiSense3 = 0;

  digitalWrite(SENSE_OUT, HIGH);
  //Wait until voltage has stabilized.
  delay(100);
  uiSense0 = digitalRead(SENSE_IN_0);
  uiSense1 = digitalRead(SENSE_IN_1);
  uiSense2 = digitalRead(SENSE_IN_2);
  uiSense3 = digitalRead(SENSE_IN_3);
  digitalWrite(SENSE_OUT, LOW);

  uint8_t uiValue = 0;

  uiValue = (uiSense0 << 0) | (uiSense1 << 1) | (uiSense2 << 2) | (uiSense3 << 3);
  return uiValue;
};


/**
* This function takes a string and makes it URL-compatible (https://en.wikipedia.org/wiki/Percent-encoding)
*/
String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++) {
      c=str.charAt(i);
      if (c == ' ') {
        encodedString+= '+';
      } else if (isalnum(c)) {
        encodedString+=c;
      } else {
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9) {
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9) {
            code0=c - 10 + 'A';
        }

        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
      }
      yield();
    }
    return encodedString;

}

/**
* Set the RGB LED.
* ATTN: Blue is ignored!
*/
void setRgbLed(uint8_t puiRed, uint8_t puiGreen, uint8_t puiBlue)
{
  analogWrite(LED_RGB_RED,   puiRed);
  analogWrite(LED_RGB_GREEN, puiGreen);
  analogWrite(LED_RGB_BLUE,  0);
}

/**
* Returns a secure connection to the HOST defined above.
* @todo We should really not give back the client if it failed to establish a connection...
*/
WiFiClientSecure getSecureClient()
{
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(HOST);

  if (!client.connect(HOST, HTTPS_PORT))
  {
    Serial.println("connection failed");
    return client;
  }

  if (client.verify(FINGERPRINT, HOST))
  {
    Serial.println("certificate matches");
    return client;
  }
  else
  {
    Serial.println("certificate doesn't match");
    return client;
  }



}
