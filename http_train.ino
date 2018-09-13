#include "WEMOS_Motor.h"
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>

const int timeout = 1000; //max time before emergency break; set to 0 to deactivate

uint8_t direction = _CW;
int speed = 0;
long last_update;
String dir_text[3] = {"", "CCW", "CW"};
Motor M1(0x30,_MOTOR_A, 1000);

ESP8266WebServer server(80);

WiFiManager wifiManager;

void handleRoot() {
  if (server.hasArg("d")){ //use d get argument for direction: 1 or 0
    int d = server.arg("d").toInt();
    if(d==1){
      direction = _CCW;
      Serial.println("_CCW ");
    }else{
      direction = _CW;
      Serial.println("_CW  ");
    }
    server.send(200, "text/html", String(dir_text[direction]));
  }  
  else if (server.hasArg("c")){ //use c get argument to change speed about given value: 0-100
    int c = server.arg("c").toInt();
    speed += c;
    if(speed > 100){
      speed = 100;
    }
    else if(speed < 0){
      speed = 0;
    }
    Serial.println(speed);
    last_update = millis();
    M1.setmotor(direction, speed);      
    server.send(200, "text/html", String(speed));
  }  
  else if (server.hasArg("s")){ //use s get argument for speed: 0-100
    int s = server.arg("s").toInt();
    speed = s;
    if(speed > 100){
      speed = 100;
    }
    else if(speed < 0){
      speed = 0;
    }
    Serial.println(speed);
    last_update = millis();
    M1.setmotor(direction, speed);      
    server.send(200, "text/html", String(speed));
  }  
  else if (server.hasArg("t")){ //use t get argument to reset the timeout
    last_update = millis();
    server.send(200, "text/html", String(speed));
  }  
  else {

    server.send(200, "text/html",
      "<html>\
        <head>\
          <title>TRAIN (ESP8266)</title>\
          <style>\
            body { background-color: #000000; font-family: Arial, Helvetica, Sans-Serif; Color: #ffffff; text-align: center; user-select: none;}\
            a:link.on_Link , a:visited.on_Link {background-color: #43f436; color: black; padding: 14px 25px; text-align: center; text-decoration: none; display: inline-block;}\
            a:link.off_Link, a:visited.off_Link {background-color: #f44336; color: white; padding: 14px 25px; text-align: center; text-decoration: none; display: inline-block;}\
            a:link.neut_Link, a:visited.neut_Link {background-color: #f4f4f4; color: black; padding: 14px 25px; text-align: center; text-decoration: none; display: inline-block;}\
            a:hover.on_Link , a:active.on_Link {background-color: #35BF2A;}\
            a:hover.off_Link, a:active.off_Link {background-color: #BF352A;}\
            a:hover.neut_Link, a:active.neut_Link {background-color: #BFBFBF;}\
          </style>\
        </head>\
        <body>\
          <h1>TRAIN</h1>\
          <p>\
            </br> timeout=   "+ String(timeout) +" speed=<b id=\"speed\">" + String(speed) + "</b>   dir=<b id=\"direction\">"+ dir_text[direction] +"</b>\
            </br></br>\
            <a href=\"#ccw\" class=\"neut_Link\" data-command=\"d\" data-target=\"/?d=1\">CCW</a>\
            <a href=\"#cw\" class=\"neut_Link\" data-command=\"d\" data-target=\"/?d=0\">CW</a>\
            </br></br>\
            <a href=\"#off\" class=\"off_Link\" data-command=\"s\" data-target=\"/?c=-100\">off</a>\
            <a href=\"#dm\" class=\"off_Link\" data-command=\"s\" data-target=\"/?c=-10\">--</a>\
            <a href=\"#m\" class=\"off_Link\" data-command=\"s\" data-target=\"/?c=-1\">-</a>\
            <a href=\"#p\" class=\"on_Link\" data-command=\"s\" data-target=\"/?c=+1\">+</a>\
            <a href=\"#dp\" class=\"on_Link\" data-command=\"s\" data-target=\"/?c=+10\">++</a>\
            <a href=\"#max\" class=\"on_Link\" data-command=\"s\" data-target=\"/?c=+100\">full</a>\
          </p>\
          <script type=\"text/javascript\">\
            window.onload = function() {\
              let links = document.getElementsByTagName(\"a\");\
              for(let index = 0; index < links.length; index++) {\
                links[index].addEventListener(\"click\", (event) => {\
                  if(event.target != null) {\
                    let command = event.target.dataset.command;\
                    let request = new XMLHttpRequest();\
                    request.addEventListener('load', function(event) {\
                      if (request.status >= 200 && request.status < 300) {\
                        if(command == \"s\") {\
                          document.getElementById(\"speed\").innerHTML = request.responseText;\
                        } else if(command == \"d\") {\
                          document.getElementById(\"direction\").innerHTML = request.responseText;\
                        }\
                      } else {\
                         console.warn(request.statusText, request.responseText);\
                      }\
                    });\
                    request.open(\"GET\", event.target.dataset.target);\
                    request.send();\
                  }\
                });\
              }\
              "+ String(timeout==0?"":("\
                setInterval(function(){\
                  let request = new XMLHttpRequest();\
                  request.open(\"GET\", \"/?t=1\");\
                  request.send();\
                }," + String(timeout/2.5))) +")\
            }\
          </script>\
        </body>\
      </html>");

  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("trying to connect to wifi");
  wifiManager.autoConnect("train");
  Serial.print("connected to wifi");
  if (!MDNS.begin("train")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
  MDNS.addService("esp", "tcp", 80); //add mdms to be found by controller
  last_update = millis();
}

void loop() {
  server.handleClient();
  if(timeout!=0 && millis() - last_update > timeout){ //check if we hit the timeout and should emrgency break
    Serial.println("timeout");
    speed = 0;
    M1.setmotor(_STOP);
    last_update = millis();
  }
}
