/** ************************************************************************
 * File Name: M5_Access_Switch_Output_Software.ino 
 * Title: M5 Switch Output Interface Software
 * Developed by: Milador
 * Version Number: 1.0 (24/9/2022)
 * Github Link: https://github.com/milador/M5-Access-Switch-Output
 ***************************************************************************/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <M5StickC.h>

#define SWITCH_A_PIN         26
#define SWITCH_B_PIN         25
#define TO_SLEEP_TIME        60        //Go to sleep if no data was sent in 60 seconds
#define TO_WAKE_TIME         180       //Wake up every 3 minutes 
#define S_TO_MS_FACTOR       1000
#define US_TO_S_FACTOR       1000000

// Set these to your desired credentials.
const char *ssid = "M5SwitchOutput";
const char *password = "123456";

String g_switchMessage;        //Custom message 
int g_pageNumber;
unsigned long currentTime = 0, switchDataTime = 0;


WiFiServer server(80);


void setup() {
  
  pinMode(SWITCH_A_PIN, OUTPUT);
  pinMode(SWITCH_B_PIN, OUTPUT);

  Serial.begin(115200);
  M5.begin();                                                                  //Starts M5Stack
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");
  showIntro();                                                                 //Show intro page
  delay(5);   
  showMode(myIP);
  delay(5000);
  initBatterySaver();
}

void loop() {

  batterySaver();
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");          // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print("<head><style>.center {display: flex; justify-content: center; align-items: center; height: 200px; border: 3px solid blue; }</style></head>");
            // the content of the HTTP response follows the header:
            //client.print("Click <a href=\"/A\">here</a> to activate Switch A.<br>");
            //client.print("Click <a href=\"/B\">here</a> to activate Switch B.<br>");
            client.print("<h2 style=\"text-align:center\">M5StickC Switch Output Interface</h2><br>");
            client.print("<div class=\"center\"><button onclick=\"location.href='/A'\" style=\"height:200px;width:200px\" type=\"button\">Switch A</button>");
            client.print("<button onclick=\"location.href='/B'\" style=\"height:200px;width:200px\" type=\"button\">Switch B</button></div><br>");
            client.print("<p style=\"text-align:center\">More information at milador.ca</p><br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /A" or "GET /B":
        if (currentLine.endsWith("GET /A")) {
          digitalWrite(SWITCH_A_PIN, HIGH);
          delay(100);
          digitalWrite(SWITCH_A_PIN, LOW);
        }
        if (currentLine.endsWith("GET /B")) {
          digitalWrite(SWITCH_B_PIN, HIGH);
          delay(100);
          digitalWrite(SWITCH_B_PIN, LOW);
        }
        switchDataTime = millis();  
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

//***INITIALIZE BATTERY SAVER FUNCTION***//
void initBatterySaver() {
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_37,LOW);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_39,LOW);
  currentTime = switchDataTime = millis();
}

//***BATTERY SAVER FUNCTION***//
void batterySaver() {
  currentTime = millis();
  if (currentTime > (switchDataTime + (TO_SLEEP_TIME * S_TO_MS_FACTOR))) {
    M5.Axp.DeepSleep(SLEEP_SEC(TO_WAKE_TIME));
  }
}

//*** SHOW INTRO PAGE***//
void showIntro() {

  g_pageNumber = 0;                                  //Set intro page number 

  M5.Lcd.setRotation(3);

  M5.Lcd.fillScreen(WHITE);
  delay(200);
  M5.Lcd.fillScreen(RED);
  delay(200);
  M5.Lcd.fillScreen(GREEN);
  delay(200);
  M5.Lcd.fillScreen(BLUE);
  delay(200);
  M5.Lcd.fillScreen(BLACK);
  
  M5.Lcd.setTextColor(WHITE);

  M5.Lcd.setTextSize(2);                        
  M5.Lcd.drawCentreString("Milador.ca",80,20,2);

  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCentreString("M5Stick Switch Output",80,50,1);

  delay(3000);
}

//*** SHOW MODE PAGE***//
void showMode(IPAddress ip){

  g_pageNumber = 1;

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);                      //Black background
  M5.Lcd.drawRect(1, 1, 159, 20, BLUE);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCentreString(ipToString(ip),80,2,2);
  
  showModeInfo();
  showMessage();
}

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

//*** SHOW MODE INFO***//
void showModeInfo() {

  String switchAText = "Swich A";
  String switchBText = "Swich B";
  M5.Lcd.setRotation(3);
  M5.Lcd.drawRect(1, 23, 159, 41, WHITE);
  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1);

  M5.Lcd.drawCentreString(switchAText,80,28,2);
  M5.Lcd.drawCentreString(switchBText,80,43,2);
}

//*** SHOW CUSTOM MESSAGE***//
void showMessage() {

  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(1);                    // Select the font
  //Display connection status based on code
  M5.Lcd.drawRect(1, 65, 159, 15, BLUE);
  M5.Lcd.setTextColor(BLUE); 
  g_switchMessage = "Switch Output";
  Serial.println(g_switchMessage);
  M5.Lcd.drawCentreString(g_switchMessage,80,69,1);// Display connection state
    
}
