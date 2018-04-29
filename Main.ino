#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

#define SIZE 12500 //Size of data array

//Global variables
ESP8266WebServer server(80);
unsigned short data[SIZE]; //Array with all points from marker, stored in the following format: [x1] [y1] [x2] [y2] ... [xn] [yn] [0] [x1] [y1] ...
int n = 0; //Index of element to insert into data array

//Constants
const char *ssid = "ECEN1400"; //Name of access point
const char *password = ""; //Password for access point (no password)

void handleRoot() { //This function is called when a webpage is requested, creates and sends the html file, which is the "text" String
  String text = "<!DOCTYPE html>\n<html>\n<body>\n\n";
         text += outOfRam();
         text += "<svg  style=\"fill:none;stroke:blue;stroke-width:3;stroke-linecap:round;width:900;height:500\">\n";
         text += updateData();
         text += "\n<polyline points=\"1,1 898,1 898,498 1,498 1,1\" style=\"stroke:black;\"></polyline>";
         text += "\n</svg>\n\n";
         text += "<script> location.reload();</script>\n";
         text += "</body>\n</html>";
  server.send(200, "text/html", text); //Sends "text" String to client, 200 is status code for OK
}

String updateData(){ //This function converts the data array a set of "polylines" to be displayed as an svg (scalable vector graphic)
  String svg = "<polyline points=\"";
         int i = 0;
         while(1){
          if(data[i]){
            svg += String(data[i]) + "," + String(data[++i]) + " ";
          } else if(data[i+1]){
            svg += "\"></polyline>"
                   "\n<polyline points=\" ";
          } else {
            break;
          }
          i++;
         }
         svg += "\"></polyline>";
  return svg;
}

String outOfRam(){ //This adds red "OUT OF RAM" text at the top of the page when the data array is filled, there is a 3 element buffer on the end of the array
  if(n >= SIZE-4){
    return "<p style=\"color:red\">OUT OF RAM</p>";
  }
  return "";
}

void undo(){ //This deletes the last set of points from the data array, called when the undo button is pushed
  while(n > 0 && !data[--n]);
  while(n > 0 && data[--n]) data[n] = 0;
  if(n > 0) n++;
}

void setup() {
  delay(1000);
  Serial.begin(115200); //Start Serial connection with Pro Mini

  WiFi.mode(WIFI_AP); //Configures access point
  WiFi.softAP(ssid, password);
  WiFi.getAutoConnect();
  server.on("/", handleRoot); //Sets "handleRoot" as function to be called when a webpage is requested
  server.begin();

  for(int j = 0; j < SIZE; j++) data[j] = 0; //Fills the data array with zeroes to initialize the array
}


void loop() {
  server.handleClient(); //Checks for new client connections/requests, calls appropriate functions
  
  int pointsAdded = 0; //This reads the new data from the serial (10 at a time) and puts it in the data array
                       //New data points are represented by "[x],[y]", end of line is represented by "0", and undo is represented as "1"
  while(Serial.available()){
    if(n < SIZE-4){
    char input[17];
    int a = 0;
    int c;
    while((c = Serial.read()) != '\n' && a < 16) if(c != -1)input[a++] = (char)c; //Read in from Serial one char at a time into "input" char array
    input[a] = '\0'; //Add null terminator to the end of "input" so it can be treated a string in subsequent functions
    char * separator = strchr(input,','); //Looks for comma character in "input" string, sets "separator" to address of the comma if found, NULL if not found
      if(separator){ //Comma found, converts String before and after the comma into a short to be placed into "data" array
        *separator = '\0';
        data[n++] = (short)atoi(input);
        data[n++] = (short)atoi(separator+1);
      } else { //Comma not found, check for "0" for end of line or "1" for undo
        if(input[0] == '0') data[n++] = 0;
        if(input[0] == '1') undo();
      }
    }
    pointsAdded++; //Increment how many Strings have been read from Serial, stops after 10 Strings
    if(pointsAdded > 10)break;
  }
}
