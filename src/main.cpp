#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>    
#include <LittleFS.h>
#include <ESP8266WiFiMulti.h>
#define ESP_getChipId()   (ESP.getChipId())
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <ArduinoOTA.h>


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"ua.pool.ntp.org",7200);

ESP8266WiFiMulti wifiMulti;

String Router_SSID= "Crio";
String Router_Pass= "Crio6678Crio";
DNSServer dns;
AsyncWebServer server(80);

uint8_t Powerpin = 12;
uint8_t Buttonpin = 0;
uint8_t Ledpin = 13;
bool timer_mode=true;
String timer_mode_str="Enabled";
String power_mode_str="Enabled";
bool power_mode=false;

unsigned long now=millis(); 
unsigned long last=millis(); 
boolean connectioWasAlive = true;

String webPage_current="";
String webPageHead =R"V( 
  <html><head>
  <link rel="stylesheet" href="picnic.min.css">
  <script src="/axios.min.js" defer></script>
  <script>
    function power(arg){
      if (arg)
        axios.get("/onBtn");
      else  
        axios.get("/offBtn");

      updatehtml()        
    };

    function settimer(arg){
      if (arg)
        axios.get("/timerOn");
      else  
        axios.get("/timerOff");

      updatehtml()        
    };

    setInterval(() => {
        updatehtml();
    }, "10000");

    function updatehtml(){

      axios.get("/status")  
      .then(function (response) {
        document.getElementById('footer').innerHTML =response.data;
      })
      .catch(function (error) {
        if (error.response) {
           document.getElementById('footer').innerHTML =error.response.data
          console.log(error.response.data);
          console.log(error.response.status);
          console.log(error.response.headers);
        } else if (error.request) {
          console.log(error.request);
        } else {
          document.getElementById('footer').innerHTML ='Error !';
          console.log('Error', error.message);
        }
          console.log(error.config);
        });
      }
  </script>
  </head>
  <body style="margin: 1em;">
  <a href="#" onclick="power(true);">
  <button style="width:100%;background-color: green;height: 10vh;font-size:5vh">ON</button>
  </a>&nbsp;
  <a href="#" onclick="power(false);">
  <button style="width:100%;background-color: red;height: 10vh;font-size:5vh">OFF</button>
  </a>&nbsp;
   <h3>Sonoff relay with timer 02.10.2022</h3>
)V";

String webPage =webPageHead+R"V( 
<p> <a href="#" onclick="settimer(true);"><button>Timer ON</button></a>&nbsp;<a href="#"  onclick="settimer(false);"><button>Timer OFF</button></a> <BR>
<!-- <p> <a href="#" onclick="power(true);"><button>ON</button></a>&nbsp;<a href="#" onclick="power(false);"><button>OFF</button></a></p> -->
<div id='footer'>
)V";

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
};

// void (){
// 	if (timer_mode) return "Enabled";
// 	else return "Disabled";
// };
String pageFooter(){
  return "<div id='mode'>"+power_mode_str+"<br>"+timer_mode_str+"</div><div id='curr_time'>"+timeClient.getFormattedTime()+"</div></div>";
}

void update_str()
{
  if (power_mode)
    power_mode_str="Relay on";
  else  
    power_mode_str="Relay off";

  if (timer_mode)
    timer_mode_str="Timer mode on";
  else  
  {
      timer_mode_str="Timer mode off";
  }
    webPage_current=webPage+pageFooter();
}


void relay(bool state){
	if (state){
	    digitalWrite(Powerpin, HIGH);
      digitalWrite(Ledpin, LOW);
      Serial.println("Relay on");
      power_mode=true;
	}
	else{
      digitalWrite(Ledpin, HIGH);
      digitalWrite(Powerpin, LOW);
      //Serial.println("Relay off");
      power_mode=false;
	}
  update_str();
}

void timer_mode_change(bool state){
	if (state){
      Serial.println("Timer on");
      timer_mode=true;
	}
	else{
      Serial.println("Timer off");
      timer_mode=false;
	}
  update_str();
}

void setup() {
  pinMode(Buttonpin, INPUT);
  pinMode(Powerpin, OUTPUT);
  pinMode(Ledpin, OUTPUT);
  digitalWrite(Powerpin, LOW);

  WiFi.mode(WIFI_STA);
  AsyncWiFiManager wifiManager(&server,&dns); 
  delay(1000);
  Serial.begin(115200);
  Serial.println("START2");
  
   if (LittleFS.begin()) {
     Serial.println("Filesystem mounted");
   }
   else {
     Serial.println("ERROR!!! Filesystem not mounted...");
   }

  wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());


  uint8_t status;
  int i = 0;
  status = wifiMulti.run();
  #define WIFI_MULTI_1ST_CONNECT_WAITING_MS             2200L
  #define WIFI_MULTI_CONNECT_WAITING_MS                   500L
  delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);
  while ( ( i++ < 20 ) && ( status != WL_CONNECTED ) )
  {
    status = WiFi.status();
    if ( status == WL_CONNECTED )
      break;
    else{
        Serial.print("...");
      delay(WIFI_MULTI_CONNECT_WAITING_MS);
    }
  }

  if ( status == WL_CONNECTED )
  {
    Serial.println("WiFi connected after time: "+i);
    Serial.println("SSID:"+WiFi.SSID()+",RSSI="+WiFi.RSSI());
    Serial.println("Channel:"+WiFi.channel());
    Serial.println("IP address:"+WiFi.localIP().toString());
    Serial.println("MAC address:"+WiFi.macAddress());
  }
  else
  {
    Serial.println(F("WiFi not connected"));
    ESP.reset();
  }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", webPage_current);
  });
      
  server.on("/timerOn", [](AsyncWebServerRequest *request){
    Serial.println("Timer ON command "+millis());   
    timer_mode_change(true);
    request->send(200, "text/html","Last command: Timer ON");
  });
    
  server.on("/timerOff", [](AsyncWebServerRequest *request){
    Serial.println("Timer OFF command "+millis());   
    timer_mode_change(false);
    request->send(200, "text/html","Last command: Timer OFF");
  });
    
  server.on("/onBtn", [](AsyncWebServerRequest *request){
	  relay(true);
    Serial.println("Timer ON command "+millis());   
    request->send(200, "text/html","Last command: Button ON");
  });
    
  server.on("/offBtn", [](AsyncWebServerRequest *request){
	  relay(false);
    Serial.println("Timer OFF command "+millis());   
    request->send(200, "text/html","Last command: Button OFF");
  });

  server.on("/rssi", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", ("SSID:"+WiFi.SSID()+",RSSI="+WiFi.RSSI()));
  });

  server.on("/chip", HTTP_GET, [](AsyncWebServerRequest *request){
         String chip = String( ESP_getChipId());
        request->send(200, "text/html",chip);
  });

  server.on("/mac", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", WiFi.macAddress());
    });  
	
  server.on("/axios.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/axios.min.js", "text/css");
    response->addHeader("Cache-Control","immutable");
    request->send(response);
  });		
  server.on("/picnic.min.css", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/picnic.min.css", "text/css");
    response->addHeader("Cache-Control","immutable");
    request->send(response);
  });	
 server.on("/free", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", String(ESP.getFreeSketchSpace()));
    });  

     server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", pageFooter());
    });  
  server.onNotFound(notFound);
  server.begin();
  Serial.println("HTTP server started");   
  timeClient.begin();
  
}

void loop() {
  update_str();
wifiMulti.run();
  now=millis();
  if (digitalRead(Buttonpin)==0){
    Serial.println("Manual");
    timer_mode_change(false);
    relay(!power_mode);
    delay(1000);
  }

  if ((now-last)>10000){
	  last=millis();
	  timeClient.update();
	  //Serial.println(timeClient.getFormattedTime());
	  if (timer_mode){
      if (timeClient.getMinutes()==0){
			  relay(false);
      }

      // 		   if (timeClient.getHours()>7 and timeClient.getHours()<21)
			//   relay(true);
		  //  else 
      // 			  relay(false);

	  }
  }
}