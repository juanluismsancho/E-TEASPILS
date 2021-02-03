#include <OLED_I2C.h>//libreria necesaria para el control de la pantala OLED
#include <Wire.h>
#include "configuration.h"
#include "SparkFun_SCD30_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD30
#include "ThingsBoard.h"
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"

//thingsboard
// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;
#define TOKEN               "CQwX8e4u7tCveJAPvz5b"
#define THINGSBOARD_SERVER  "iot.etsisi.upm.es"

#define WIFI_AP             "Jl"
#define WIFI_PASSWORD       "lolwifigratis"
int pinInternet=4;
boolean internetActivo= false;
boolean thingsboardActivo=false;
int pinThingsboard=16;

RTC_DS3231 rtc;

int pinSD=17;
boolean sdActiva=false;
File logData;
int hora=0;

SCD30 airSensor;

int LIGH_SENSOR=32;
int lightValue=0;

OLED  pantalla(SDA, SCL, 8);// inicializamos la pantalla OLED

int momentoActual=0;

void setup()
{ 
  Serial.begin(115200);
  pinMode(pinInternet,OUTPUT);
  pinMode(pinThingsboard,OUTPUT);
  pinMode(pinSD,OUTPUT);
  
  startDisplay();
  if (!SD.begin(5))
  {
    sdActiva=false;  
    digitalWrite(pinSD,HIGH);    
    pantalla.clrScr(); // borra la pantalla    pantalla.print(" Hay un ", CENTER, 0);//imprime la frase en el centro de la zona superior
    pantalla.print("Problema", CENTER, 10);//imprime la frase en el centro de la zona superior
    pantalla.print("en la SD", CENTER, 20);//imprime la frase en el centro de la zona superior
    pantalla.update();
    while(!SD.begin(5));
    
  }
  
    sdActiva=true;
    digitalWrite(pinSD,LOW);    
    logData = SD.open("/log.txt", FILE_WRITE);
 

  //delay(10000);
  //SD.end();
  //while(SD.begin(5))Serial.println("WIIII");
  
  conectIOT(); 
  
  Wire.begin();
  if (airSensor.begin() == false)
  {
    pantalla.clrScr(); // borra la pantalla
    pantalla.print(" No se ha", CENTER, 0);//imprime la frase en el centro de la zona superior
    pantalla.print("detectado", CENTER, 10);//imprime la frase en el centro de la zona superior
    pantalla.print("  SCD30  ", CENTER, 20);//imprime la frase en el centro de la zona superior
    pantalla.update();// actualiza la pantalla    
    while (1);
  }

  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }
  Serial.println("TODO OK");
  // Si se ha perdido la corriente, fijar fecha y hora
  if (rtc.lostPower()) {
    // Fijar a fecha y hora de compilacion
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
    // rtc.adjust(DateTime(2016, 1, 21, 3, 0, 0));
  }
  
}

void loop () {
  SD.end();
  if (SD.begin(5))
  {
    sdActiva=true;
    digitalWrite(pinSD,LOW);    
    logData = SD.open("/log.txt", FILE_WRITE);
  }else{
    sdActiva=false;  
    digitalWrite(pinSD,HIGH);    
    pantalla.clrScr(); // borra la pantalla    pantalla.print(" Hay un ", CENTER, 0);//imprime la frase en el centro de la zona superior
    pantalla.print("Problema", CENTER, 10);//imprime la frase en el centro de la zona superior
    pantalla.print("en la SD", CENTER, 20);//imprime la frase en el centro de la zona superior
    pantalla.update();
    while(!SD.begin(5));
  }
  
  if (WiFi.status() != WL_CONNECTED || !tb.connected()) {
    digitalWrite(pinInternet,HIGH);
    conectIOT();
  }else{
    digitalWrite(pinInternet,LOW);
    internetActivo=true;
    thingsboardActivo=true;
  }
  
  int newLight=analogRead(LIGH_SENSOR);
  if (airSensor.dataAvailable() || newLight!=lightValue)
  {
    logData.println(completeDate());
    pantalla.clrScr(); // borra la pantalla
    pantalla.print("Light: ", LEFT,30);
    pantalla.printNumI(newLight,RIGHT,30);//el numero anterior ocupa 24 pixels de alto por lo que este debe empezar a partir del 25
    logData.print("Luz: ");
    logData.println(newLight);
    
    pantalla.print("CO2: ",LEFT,0);
    pantalla.printNumI(airSensor.getCO2(),RIGHT,0);
    logData.print("CO2: ");
    logData.println(airSensor.getCO2());
    
    pantalla.print("Temperature: ",LEFT,10);
    pantalla.printNumI(airSensor.getTemperature(),RIGHT,10);//el numero anterior ocupa 24 pixels de alto por lo que este debe empezar a partir del 25
    logData.print("Temperature: ");
    logData.println(airSensor.getTemperature());
    
    pantalla.print("Humidity: ", LEFT,20);
    pantalla.printNumI(airSensor.getHumidity(),RIGHT,20);//el numero anterior ocupa 24 pixels de alto por lo que este debe empezar a partir del 25
    logData.print("Humidity: ");
    logData.println(airSensor.getHumidity());

    logData.flush();
    
    pantalla.update();// actualiza la pantalla
  }

  if(thingsboardActivo && momentoActual==envio)
  {
    Serial.println("ENVIO");
    pantalla.print("Enviando datos", LEFT,40);
    pantalla.update();// actualiza la pantalla
    tb.sendTelemetryInt("co2", airSensor.getCO2());
    tb.sendTelemetryInt("temperature", airSensor.getTemperature());
    tb.sendTelemetryInt("humidity", airSensor.getHumidity());
    tb.sendTelemetryInt("light", newLight);
    delay(1000);
    momentoActual=0;
  }
  hora++;
  delay(500);
}

String completeDate()
{
  DateTime date = rtc.now();
  String date2=String(date.year())+'/'+String(date.month())+'/'+String(date.day())+' '+String(date.hour())+':'+String(date.minute())+':'+String(date.second());
  return date2;
}

void conectIOT()
{  
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  if(WiFi.status() != WL_CONNECTED) {
    digitalWrite(pinInternet,HIGH);
    int i=0;
    while(i<5 && WiFi.status() != WL_CONNECTED)
    {
      i++;
      delay(100);
    }
  }else{
    digitalWrite(pinInternet,LOW);
    if (!tb.connected()){
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
        thingsboardActivo=false;
        digitalWrite(pinThingsboard,HIGH);
      }else{
        thingsboardActivo=true;
        digitalWrite(pinThingsboard,LOW);
      }
    }
  }
}

void startDisplay()
{
  pantalla.begin();//inicializa el display OLED
  pantalla.setFont(SmallFont);//seteo el tamaño de la fuente
  pantalla.print("BIENVENIDO", CENTER, 28);//imprime la frase en el centro de la zona superior
  pantalla.update();// actualiza la pantalla
  delay(1000);
  pantalla.drawBitmap(0,0,LOGO,128,64);
  pantalla.update();// actualiza la pantalla
  delay(3000);
  pantalla.clrScr(); // borra la pantalla
}

/*String date(){
 
  char fecha[19];
  DateTime now = RTC.now(); //Obtener fecha y hora actual.

  int dia = now.day();
  int mes = now.month();
  int anio = now.year();
  int hora = now.hour();
  int minuto = now.minute();
  int segundo = now.second();

  sprintf( fecha, "%.2d.%.2d.%.4d %.2d:%.2d:%.2d", dia, mes, anio, hora, minuto, segundo);
  return String( fecha );
}*/
