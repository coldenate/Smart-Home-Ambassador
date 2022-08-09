#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <string>
WiFiServer server(80);
WiFiServer server2(79);
String header;

// Auxiliar variables to store the current output state
int lightstate = 0;
// int garageDoorState = 1;
int currentGarageDoorState = 1;      // 0 is open, 1 is closed, 2 is opening, and 3 is closing
int currentGarageDoorStateLocal = 1; // 0 is open, 1 is closed, 2 is opening, and 3 is closing
int targetGarageDoorState = 1;
// Assign output variables to GPIO pins
const int fanLight = 5;
const int garageDoor = 4;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
int ipayload;
void buttonPress(int pin)
{
  digitalWrite(pin, HIGH);
  delay(800);
  digitalWrite(pin, LOW);
}

void setup()
{
  Serial.begin(9600);
  pinMode(fanLight, OUTPUT);
  pinMode(garageDoor, OUTPUT);
  // set outputs to low
  digitalWrite(fanLight, LOW);
  digitalWrite(garageDoor, LOW);
  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  wifiManager.setSTAStaticIPConfig(IPAddress(10, 0, 0, 101), IPAddress(10, 0, 0, 1), IPAddress(255, 255, 255, 0)); // optional DNS 4th argument
  wifiManager.autoConnect("Fan Bridge");
  Serial.println("Connected!");
  server.begin();
  server2.begin();
}

void loop()
{
  // Serial.println(targetGarageDoorState);
  HTTPClient http;
  WiFiClient client = server.available();   // Listen for incoming clients
  WiFiClient client2 = server2.available(); // Listen for incoming clients
  // run pre-client http services

  // targetGarageDoorState = doc["targetDoorState"];   // 1
  // Serial.println out the two states

  // int ipayload = payload.toInt();
  // currentGarageDoorState = ipayload;

  HTTPClient althttp;

  if (client)
  {

    // If a new client connects,
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    DynamicJsonDocument doc(1024);
    String json;
    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        // Serial.write(c);        // print it out the serial monitor
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            http.begin(client2, "http://10.0.0.102:80/status");
            // Serial.println("Requesting /status...");
            http.GET();

            String payload = http.getString();

            http.end(); // this has gotten the currentgarage door state from a watchdog service

            StaticJsonDocument<96> doc;

            DeserializationError error = deserializeJson(doc, payload);

            if (error)
            {
              Serial.print(F("deserializeJson() failed: "));
              Serial.println(error.f_str());
              return;
            }

            currentGarageDoorState = doc["currentDoorState"]; // 0 or 1
            // targetGarageDoorState = currentGarageDoorState;   // 0 or 1
            if (currentGarageDoorState == 1)
            {
              targetGarageDoorState = 1;
            }
            if (currentGarageDoorState == 0)
            {
              targetGarageDoorState = 0;
            }
            Serial.println("Setting target door state to:");
            Serial.println(targetGarageDoorState);
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:

            // make sure to process state changes up here to include in the return before client disconnection

            // if (header.startsWith("POST /status/update"))
            // {
            //   String input = client.readString();
            //   // String input;

            //   StaticJsonDocument<96> doc;

            //   DeserializationError error = deserializeJson(doc, input);

            //   if (error)
            //   {
            //     Serial.print(F("deserializeJson() failed: "));
            //     Serial.println(error.f_str());
            //     return;
            //   }

            //   currentGarageDoorState = doc["currentDoorState"]; // 0 or 1
            //   // targetGarageDoorState = currentGarageDoorState;   // 0 or 1
            //   if (currentGarageDoorState == 1)
            //   {
            //     targetGarageDoorState = 1;
            //   }
            //   if (currentGarageDoorState == 0)
            //   {
            //     targetGarageDoorState = 0;
            //   }
            //   Serial.println("Setting target door state to:");
            //   Serial.println(targetGarageDoorState);
            //   // targetGarageDoorState = doc["targetDoorState"];   // 1
            //   // Serial.println out the two states
            // }
            if (header.startsWith("GET /status"))
            {
              Serial.println("GETing /status... haha");
              client.println("HTTP/1.1 200 OK");

              client.println("Content-type:text/json");
              client.println("Connection: close");

              Serial.println(ipayload);
              doc["currentDoorState"] = currentGarageDoorState;
              doc["targetDoorState"] = targetGarageDoorState;
              client.println();
              serializeJson(doc, json);
              client.println(json);
              Serial.println("closing connection");
            }
            else
            {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
            }
            if (header.startsWith("GET /light/state"))
            {
              http.begin(client, "http://10.0.0.105:8080/fanlight");
              http.addHeader("Content-Type", "text/json");
              // Serial.println("sending state");
              doc["characteristic"] = "On";
              if (lightstate == 0)
              {
                doc["value"] = "true";
              }
              else
              {
                doc["value"] = "false";
              }
              serializeJson(doc, json);
              http.POST(json);
              // client.println("<!DOCTYPE html><html>");
              http.end();
            }
            if (header.startsWith("GET /garagedoor/state"))
            {
              http.begin(client, "http://10.0.0.105:8080/garage");
              http.addHeader("Content-Type", "text/json");
              doc["characteristic"] = "On";
              if (currentGarageDoorState == 0)
              {
                doc["value"] = "true";
              }
              else
              {
                doc["value"] = "false";
              }
              serializeJson(doc, json);
              http.POST(json);
              http.end();
            }

            // Serial.println("Did not ask for state, moving on.");
            if (!header.startsWith("GET /light/state"))
            {

              // turns the GPIOs on and off
              if (header.indexOf("GET /light/on") >= 0)
              {
                // Serial.println("GPIO 5 on");
                lightstate = 0;
                digitalWrite(fanLight, HIGH);
                delay(500);
                digitalWrite(fanLight, LOW);
              }
              else if (header.indexOf("GET /light/off") >= 0)
              {
                // Serial.println("GPIO 5 off");
                lightstate = 1;
                digitalWrite(fanLight, HIGH);
                delay(500);
                digitalWrite(fanLight, LOW);
              }
              else if (header.indexOf("GET /setTargetDoorState") >= 0)
              {
                char a = header.charAt(header.indexOf("/") + 1);
                int ia = a - '0'; // converts char to int!! but only works for one char
                targetGarageDoorState = ia;
                buttonPress(garageDoor);
                while (true)
                {
                  althttp.begin(client, "http://10.0.0.102:80/status");

                  althttp.GET();

                  String payload = althttp.getString();

                  althttp.end(); // this has gotten the currentgarage door state from a watchdog service

                  int ipayload = payload.toInt();
                  currentGarageDoorStateLocal = ipayload;
                  ipayload += 2;
                  // if it is 0 (meaning open), then we set it to 2 meaning it is opening
                  // if it is 1 (meaning closed), then we set it to 3 meaning it is closing
                  payload = String(ipayload);
                  http.begin(client, "http://10.0.0.105:2000/currentDoorState?value=" + payload);
                  http.POST(" ");
                  http.end();
                  if (currentGarageDoorStateLocal == targetGarageDoorState)
                  {
                    break;
                  }
                  delay(600);
                }
                http.begin(client, "http://10.0.0.105:2000/targetDoorState?value=" + targetGarageDoorState);
              }
              // else if (header.indexOf("GET /setTargetDoorState/0") >= 0)
              // {
              //   Serial.println("GPIO 4 on");
              //   garageDoorState = 1;
              //   digitalWrite(garageDoor, HIGH);
              //   delay(800);
              //   digitalWrite(garageDoor, LOW);
              // }
              // else if (header.indexOf("GET /currentDoorState?") >= 0)
              // {
              //   Serial.println("GPIO 4 on");
              //   int x = header.charAt(header.indexOf("=") + 1);
              //   garageDoorState = x;
              //   Serial.println(x);
              // }

              else if (header.indexOf("GET /garagedoor/on") >= 0)
              {
                Serial.println("GPIO 4 on");
                // garageDoorState = 0;
                digitalWrite(garageDoor, HIGH);
                delay(800);
                digitalWrite(garageDoor, LOW);
              }
              else if (header.indexOf("GET /garagedoor/off") >= 0)
              {
                Serial.println("GPIO 4 off");
                // garageDoorState = 1;
                digitalWrite(garageDoor, HIGH);
                delay(800);
                digitalWrite(garageDoor, LOW);
              }
              else if (header.indexOf("GET /garagedoor/toggle") >= 0)
              {
                Serial.println("GPIO 4 toggle");
                // inverse state
                // if (garageDoorState == 1)
                // {
                //   garageDoorState = 0;
                // }
                // else if (garageDoorState == 0)
                // {
                //   garageDoorState = 1;
                // }

                digitalWrite(garageDoor, HIGH);
                delay(800);
                digitalWrite(garageDoor, LOW);
              }
              else
              {
                Serial.println("Unknown command");
              }

              // else if (header.indexOf("GET /garagedoor/toggle") >= 0)
              // {
              //   Serial.println("GPIO 4 off");
              //   output4State = 0;
              //   digitalWrite(output4, HIGH);
              //   delay(500);
              //   digitalWrite(output4, LOW);
              // }
            }

            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    // Serial.println("Client disconnected.");
    // Serial.println("");
  }
}