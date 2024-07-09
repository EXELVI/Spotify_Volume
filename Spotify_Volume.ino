#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include "LiquidCrystal_I2C.h"

ArduinoLEDMatrix matrix;
// 20x4 LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);

int pinA = 3;  // CLK
int pinB = 4;  // DT
int pinSW = 2; // SW
int encoderPosCount = 0;

int pinALast;
int aVal;
boolean bCW;

#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <Arduino_JSON.h>
#include "WiFiSSLClient.h"

const char *host = "api.spotify.com";
#include "arduino_secrets.h"

int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp;

WiFiServer server(80);

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 30 * 1000; // 30 seconds
JSONVar myObject;

byte barStartEmpty[] = {B01111, B10000, B10000, B10000, B10000, B10000, B10000, B01111};
byte barStartFull[] = {B01111, B11111, B11111, B11111, B11111, B11111, B11111, B01111};
byte full[] = {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111};
byte barEmpty[] = {B11111, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
byte barEndEmpty[] = {B11110, B00001, B00001, B00001, B00001, B00001, B00001, B11110};
byte barEndFull[] = {B11110, B11111, B11111, B11111, B11111, B11111, B11111, B11110};

byte lock[] = {B00000, B01110, B01010, B01010, B11111, B11011, B11111, B11111};

String device_name = "Device";
bool support_volume = true;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String client_id = SECRET_CLIENT_ID;
String client_secret = SECRET_CLIENT_SECRET;
String base64auth = SECRET_BASE64;
int keyIndex = 0; // your network key index number (needed only for WEP)

String code = "";
String token = "";
bool requestSent = false;
bool waitingCallback = true;

bool editing = false;

int status = WL_IDLE_STATUS;

WiFiSSLClient client;

unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500;

String getRandomString(int length)
{
  String randomString;
  char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int n = 0; n < length; n++)
  {
    randomString += charset[random(0, sizeof(charset) - 1)];
  }
  return randomString;
}

String getSpotifyRequestUrl()
{
  String redirect_uri = "http://" + WiFi.localIP().toString() + "/callback";
  String scope = "user-read-playback-state user-modify-playback-state";
  String state = getRandomString(16);

  String url = "https://accounts.spotify.com/authorize?" + String("client_id=") + client_id + String("&response_type=code") + String("&redirect_uri=") + redirect_uri + String("&scope=") + scope + String("&state=") + state;

  Serial.println(url);
  return url;
}

void printToMatrix(char text[])
{

  matrix.beginDraw();
  matrix.clear();
  matrix.textScrollSpeed(100);

  matrix.stroke(0xFFFFFFFF);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText(SCROLL_LEFT);

  matrix.endDraw();
}

void printIntToMatrix(int number)
{

  matrix.beginDraw();
  matrix.clear();

  matrix.stroke(0xFFFFFFFF);
  char cstr[16];
  itoa(number, cstr, 10);
  matrix.textFont(number < 100 ? Font_5x7 : Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(cstr);
  matrix.endText(NO_SCROLL);

  matrix.endDraw();
}

bool blinked = false;

void printBar(int percentual, String uppertext)
{
  // uppertext on the middle pos 0y
  // bar take all screen pos 1y
  // percentual in the middle of the bar

  int barSize = 20;
  int barStart = 0;
  int barEnd = 20;

  int barStartFullSize = 1;
  int barEndFullSize = 1;

  int barEmptySize = 18;
  int barFullSize = 18;

  int barFull = (percentual * barFullSize) / 100;
  int barEmpty = barEmptySize - barFull;

  // clear
  lcd.setCursor(0, 0);
  lcd.print("                    ");
  lcd.setCursor(0, 1);
  lcd.print("                    ");

  int uppertextpos = (20 - uppertext.length()) / 2;
  lcd.setCursor(uppertextpos, 0);
  lcd.print(uppertext);

  lcd.setCursor(0, 1);
  lcd.write(percentual == 0 ? byte(0) : byte(1));
  for (int i = 0; i < barFull; i++)
  {
    lcd.write(byte(2));
  }
  for (int i = 0; i < barEmpty; i++)
  {
    lcd.write(byte(3));
  }
  lcd.write(percentual == 100 ? byte(5) : byte(4));

  // percentual in the midlle of the bar
  int percentualpos = (barSize - 2) / 2;
  lcd.setCursor(percentualpos, 1);
  lcd.print(percentual);
}

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ;
  }
  matrix.begin();

  lcd.init();

  lcd.backlight();

  lcd.createChar(0, barStartEmpty);
  lcd.createChar(1, barStartFull);
  lcd.createChar(2, full);
  lcd.createChar(3, barEmpty);
  lcd.createChar(4, barEndEmpty);
  lcd.createChar(5, barEndFull);
  lcd.createChar(6, lock);

  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);
  pinMode(pinSW, INPUT_PULLUP);

  pinALast = digitalRead(pinA);

  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }

  printWifiStatus();
  delay(5000);

  server.begin();

  getSpotifyRequestUrl();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connect to:");
  lcd.setCursor(0, 1);
  lcd.print("http://" + WiFi.localIP().toString() + "/");
  lcd.setCursor(0, 2);
  lcd.print("to authorize.");
  lcd.setCursor(0, 3);
  lcd.print("Awaiting callback...");
}

void loop()
{
  if (client.available())
  {
    if (token == "")
    {
      String line = client.readStringUntil('\r');
      Serial.println(line);
      if (line.indexOf("access_token") != -1)
      {
        int tokenIndex = line.indexOf("access_token") + 15;
        int tokenEndIndex = line.indexOf("\"", tokenIndex);
        token = line.substring(tokenIndex, tokenEndIndex);
        Serial.println(token);

        lcd.setCursor(0, 3);
        lcd.print("Token Received");

        delay(1000);
        client.stop();

        delay(1000);
        Serial.print("Starting connection to server... " + String(host) + ": ");

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Starting connection to ");
        lcd.setCursor(0, 1);
        lcd.print(host);

        if (client.connect(host, 443))
        {
          Serial.println("connected to server");
          lcd.setCursor(0, 2);
          lcd.print("Connected to server");
          client.println("GET /v1/me/player HTTP/1.1");
          client.println("Host: api.spotify.com");
          client.println("Authorization: Bearer " + token);
          client.println("Content-Type: application/json");
          client.println("Accept: application/json");
          client.println("Connection: close");
          client.println();
        }
        else
        {
          Serial.println("connection failed");
          lcd.setCursor(0, 2);
          lcd.print("Connection failed");
        }
      }
    }
    else
    {
      String line = client.readStringUntil('\r');
      Serial.println(line);
      // get song name, volume,
      if (line.indexOf("volume_percent") != -1)
      {
        int volumeIndex = line.indexOf("volume_percent") + 17;
        int volumeEndIndex = line.indexOf(",", volumeIndex);
        int volume = line.substring(volumeIndex, volumeEndIndex).toInt();
        Serial.println(volume);
        encoderPosCount = volume;
        printIntToMatrix(encoderPosCount);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print("Volume:");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print(String(volume));
      }
      if (line.indexOf("name") != -1)
      {
        int nameIndex = line.indexOf("\"name\"") + 10;
        int nameEndIndex = line.indexOf(",", nameIndex);
        String name = line.substring(nameIndex, nameEndIndex - 1);
        device_name = name;
        Serial.println(name);
        lcd.setCursor(0, 0);
        lcd.print("                    ");
        lcd.setCursor(0, 0);
        lcd.print("Device:");
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 1);
        lcd.print(name);
      }
      if (line.indexOf("supports_volume") != -1)
      {
        int supportsVolumeIndex = line.indexOf("supports_volume") + 19;
        int supportsVolumeEndIndex = line.indexOf(",", supportsVolumeIndex);
        String supportsVolume = line.substring(supportsVolumeIndex, supportsVolumeEndIndex);
        support_volume = supportsVolume == "true";
        Serial.println(support_volume);
        lcd.setCursor(8, 2);
        if (!support_volume)
        {
          lcd.write(byte(6));
        }
      }
    }
  }

  if (waitingCallback)
  {
    WiFiClient clientServer = server.available();

    if (clientServer)
    { // callback
      Serial.println("new client");
      String currentLine = "";
      while (clientServer.connected())
      {
        if (clientServer.available())
        {
          char c = clientServer.read();
          Serial.write(c);
          if (c == '\n')
          {
            if (currentLine.length() == 0)
            {
              String curl = getSpotifyRequestUrl();
              clientServer.println("HTTP/1.1 200 OK");
              clientServer.println("Content-type:text/html");
              clientServer.println();
              clientServer.println("<!doctype html>");
              clientServer.println("<html data-bs-theme=\"dark\" lang=\"en\">");
              clientServer.println("  <head>");
              clientServer.println("    <meta charset=\"utf-8\">");
              clientServer.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              clientServer.println("    <title>Spotify Volume</title>");
              clientServer.println("    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH\" crossorigin=\"anonymous\">");
              clientServer.println("  </head>");
              clientServer.println("  <body>");
              clientServer.println("    <div class=\"container-md text-center\">");
              clientServer.println("      <h1 class=\"display-5\">Spotify Volume</h1>");
              clientServer.println("      <a class=\"btn btn-primary\" role=\"button\" href=\"" + curl + "\">Callback Url</a>");
              clientServer.println("     </div>");
              clientServer.println("    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz\" crossorigin=\"anonymous\"></script>");
              clientServer.println("  </body>");
              clientServer.println("</html>");
              clientServer.println();

              break;
            }
            else
            {
              currentLine = "";
            }
          }
          else if (c != '\r')
          {
            currentLine += c;
          }
          if (currentLine.indexOf("GET /callback?code=") != -1 && currentLine.indexOf("HTTP/1.1") != -1)
          {
            int codeIndex = currentLine.indexOf("GET /callback?code=") + 20;
            int stateIndex = currentLine.indexOf("&state=");
            code = currentLine.substring(codeIndex - 1, stateIndex);
            waitingCallback = false;
            Serial.println(code);

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Callback Received");
            clientServer.println("HTTP/1.1 200 OK");
            clientServer.println("Content-type:text/html");
            clientServer.println();
            clientServer.println("<!doctype html>");
            clientServer.println("<html data-bs-theme=\"dark\" lang=\"en\">");
            clientServer.println("  <head>");
            clientServer.println("    <meta charset=\"utf-8\">");
            clientServer.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            clientServer.println("    <title>Spotify Volume</title>");
            clientServer.println("    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH\" crossorigin=\"anonymous\">");
            clientServer.println("  </head>");
            clientServer.println("  <body>");
            clientServer.println("    <div class=\"container-md text-center\">");
            clientServer.println("      <h1 class=\"display-5\">Spotify Volume</h1>");
            clientServer.println("      <h2 class=\"text-success\">Callback Received</h2>");
            clientServer.println("      <a class=\"btn btn-primary\" role=\"button\" href=\"/\">Home</a>");
            clientServer.println("     </div>");
            clientServer.println("    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz\" crossorigin=\"anonymous\"></script>");
            clientServer.println("  </body>");
            clientServer.println("</html>");
            clientServer.println();

            break;
          }
        }
      }
    }
  }
  else if (token == "" && !requestSent)
  {

    // get spotify user token from code
    String redirect_uri = "http://" + WiFi.localIP().toString() + "/callback";

    String body = "code=" + code + "&redirect_uri=" + redirect_uri + "&grant_type=authorization_code";
    String auth = base64auth;
    String headers = "content-type: application/x-www-form-urlencoded\nAuthorization: Basic " + auth;

    // request
    client.stop();

    Serial.print("\nStarting connection to server accounts.spotify.com: ");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Starting connection to ");
    lcd.setCursor(0, 1);
    lcd.print("accounts.spotify.com");

    if (client.connect("accounts.spotify.com", 443))
    {
      Serial.println("connected to server");
      lcd.setCursor(0, 2);
      lcd.print("Connected to server");
      client.println("POST /api/token HTTP/1.1");
      client.println("Host: accounts.spotify.com");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Authorization: Basic " + auth);
      client.print("Content-Length: ");
      client.println(body.length());
      client.println();
      client.println(body);

      lastConnectionTime = millis();
      requestSent = true;
    }
    else
    {
      Serial.println("connection failed");
      lcd.setCursor(0, 2);
      lcd.print("Connection failed");
    }
  }
  else if (token != "")
  {
    aVal = digitalRead(pinA);

    if (aVal != pinALast)
    {
      if (!support_volume)
      {
        // repeat 3 times
        for (int i = 0; i < 3; i++)
        {
          lcd.setCursor(8, 2);
          lcd.print(" ");
          delay(500);
          lcd.setCursor(8, 2);
          lcd.write(byte(6));
          delay(500);
        }  
      }
      else
      {

        editing = true;
        if (digitalRead(pinB) != aVal)
        {
          encoderPosCount++;
          encoderPosCount++;
          bCW = true;
          if (encoderPosCount > 100)
          {
            encoderPosCount = 100;
          }
        }
        else
        {
          encoderPosCount--;
          encoderPosCount--;
          bCW = false;

          if (encoderPosCount < 0)
          {
            encoderPosCount = 0;
          }
        }
        Serial.print("Rotated: ");
        if (bCW)
        {
          Serial.println("clockwise");
        }
        else
        {
          Serial.println("counterclockwise");
        }
        Serial.print("Encoder Position: ");
        Serial.println(encoderPosCount);
        printIntToMatrix(encoderPosCount);
        printBar(encoderPosCount, "Volume");
      }
    }

    if ((millis() - lastConnectionTime > postingInterval) && requestSent && !editing)
    {
      lastConnectionTime = millis();

      Serial.print("Starting connection to server api.spotify.com: ");

      // update
      client.stop();

      Serial.print("Starting connection to server... " + String(host) + ": ");

      if (client.connect(host, 443))
      {
        Serial.println("connected to server");
        client.println("GET /v1/me/player HTTP/1.1");
        client.println("Host: api.spotify.com");
        client.println("Authorization: Bearer " + token);
        client.println("Content-Type: application/json");
        client.println("Accept: application/json");
        client.println("Connection: close");
        client.println();
      }
      else
      {
        Serial.println("connection failed");
      }
    }

    if (editing)
    {
      if (millis() - lastBlinkTime > blinkInterval)
      {
        if (blinked)
        {
          printIntToMatrix(encoderPosCount);
          blinked = false;
        }
        else
        {
          matrix.clear();

          blinked = true;
        }
      }
    }

    if (digitalRead(pinSW) == LOW)
    {
      if (editing)
      {
        editing = false;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Device: ");
        lcd.setCursor(0, 1);
        lcd.print(device_name);
        lcd.setCursor(0, 2);
        lcd.print("Volume: ");
        if (!support_volume)
        {
          lcd.write(byte(6));
        }
        lcd.setCursor(0, 3);
        lcd.print(String(encoderPosCount));

        // request
        client.stop();

        Serial.print("\nStarting connection to server api.spotify.com: ");

        if (client.connect("api.spotify.com", 443))
        {
          Serial.println("connected to server");
          client.println("PUT /v1/me/player/volume?volume_percent=" + String(encoderPosCount) + " HTTP/1.1");
          client.println("Host: api.spotify.com");
          client.println("Authorization: Bearer " + token);
          client.println("Content-Type: application/json");
          client.println("Content-Length: 0");
          client.println("Accept: application/json");
          client.println("Connection: close");
          client.println();
        }
        else
        {
          Serial.println("connection failed");
        }
        printIntToMatrix(encoderPosCount);
        delay(1000);
      }
    }
  }
}

void printWifiStatus()
{

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  lcd.clear();
  lcd.setCursor(0, 2);
  lcd.print("SSID: " + String(WiFi.SSID()));

  int signalStrenght = map(WiFi.RSSI(), -105, -40, 0, 100);

  printBar(signalStrenght, "Signal Strength");

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  String ipString = WiFi.localIP().toString();
  char ipCh[ipString.length() + 1];
  strcpy(ipCh, ipString.c_str());

  printToMatrix(ipCh);

  lcd.setCursor(0, 2);
  lcd.print("IP: " + ipString);

  long rssi = WiFi.RSSI();

  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  lcd.setCursor(0, 0);
}
