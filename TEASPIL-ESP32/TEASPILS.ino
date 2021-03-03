#include <OLED_I2C.h>//libreria necesaria para el control de la pantala OLED
#include <Wire.h>
#include "configuration.h"
#include "credentials.h"
#include "SparkFun_SCD30_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD30
#include "ThingsBoard.h"
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>


//wifi data
#define WIFI_AP  wifi
#define WIFI_PASSWORD  wifipass
int pinInternet=4;
boolean internetActivo= false;

//thingsboard
#define TOKEN   tokenDevice
#define THINGSBOARD_SERVER  thingsboardServer
// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;
boolean thingsboardActivo=false;
int pinThingsboard=16;

//telegram

#define BOT_TOKEN tokenBot
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // last time messages' scan has been done

//real time clock
RTC_DS3231 rtc;

//SD
int pinSD=17;
boolean sdActiva=false;
File logData;
int hora=0;

//SCD30 Sensor
SCD30 airSensor;

//OLED
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);
OLED  pantalla(SDA, SCL);// inicializamos la pantalla OLED

//contador para lectura de datos
int lightCont = 0;
int airCont = 0;
int soilHumidityCont = 0;
int loudnessCont = 0;

int lightValue = 0;
int CO2Value = 0;
int tempValue = 0;
int humidityValue = 0;
int soilHumidityValue = 0;
int loudnessValue = 0;

int presentMoment = 0;

void setup()
{ 
  pinMode(pinInternet,OUTPUT);    //Se inicializan pin de los leds de aviso
  pinMode(pinThingsboard,OUTPUT);
  pinMode(pinSD,OUTPUT);
  
  startDisplay(); //Se inicia la pantalla OLED presentando un mensaje de bienvenida y el logo
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Otra direccion es la 0x3D
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  delay(2000); // Pause for 2 seconds
  //Se comprueba si hay microSD evitando continuar hasta que no se inserte
  
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
      
  writeFile(SD, "/log.txt", LogInitialMessage.c_str());
  //logData = SD.open("/log.txt", FILE_WRITE);

  //Se conecta a WIFI y la plataforma IoT
  conectIOT(); 

  //Se inicializa el sensor de CO2, Temperatura y humedad ambiental
  Wire.begin();
  while (airSensor.begin() == false)
  {
    pantalla.clrScr(); // borra la pantalla
    pantalla.print(" No se ha", CENTER, 0);//imprime la frase en el centro de la zona superior
    pantalla.print("detectado", CENTER, 10);//imprime la frase en el centro de la zona superior
    pantalla.print("  SCD30  ", CENTER, 20);//imprime la frase en el centro de la zona superior
    pantalla.update();// actualiza la pantalla    
  }

  //Se inicializa el reloj 
  while (!rtc.begin()) {
    pantalla.clrScr(); // borra la pantalla
    pantalla.print(" No se ha", CENTER, 0);//imprime la frase en el centro de la zona superior
    pantalla.print("detectado", CENTER, 10);//imprime la frase en el centro de la zona superior
    pantalla.print("  reloj  ", CENTER, 20);//imprime la frase en el centro de la zona superior
    pantalla.update();// actualiza la pantalla     
  }
  
  // Si se ha perdido la corriente, fijar fecha y hora
  if (rtc.lostPower()) {
    // Fijar a fecha y hora de compilacion
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
    // rtc.adjust(DateTime(2016, 1, 21, 3, 0, 0));
  }
  
}

void loop () {
  //Se comprueba que hay una SD insertada
  comprobarSD();
  
  if (WiFi.status() != WL_CONNECTED || !tb.connected()) {
    digitalWrite(pinInternet,HIGH);
    conectIOT();
  }else{
    digitalWrite(pinInternet,LOW);
    internetActivo=true;
    thingsboardActivo=true;
  }

  comprobarBot();
  
  //lectura de sensores
  lecturaSensores();

  //escritura de los sensores en pantalla y en la microSD
  if(carruselON)
    mostrarCarrusel();
  else
    mostrarTodo();

  escrituraSensores();

  if(thingsboardActivo && presentMoment==envio)
  {
    tb.sendTelemetryInt("co2", CO2Value);
    tb.sendTelemetryInt("temperature", tempValue);
    tb.sendTelemetryInt("humidity", humidityValue);
    tb.sendTelemetryInt("light", lightValue);
    tb.sendTelemetryInt("loudness", loudnessValue);
    tb.sendTelemetryInt("soilHumidity", soilHumidityValue);
    delay(1000);
    presentMoment=0;
  }
  
  lightCont++;
  airCont++;
  soilHumidityCont++;
  loudnessCont++;
  presentMoment++;
  delay(500);
}

void comprobarSD()
{
  SD.end();
  if (SD.begin(5))
  {
    sdActiva=true;
    digitalWrite(pinSD,LOW);    
    //logData = SD.open("/log.txt", FILE_WRITE);
    
  }else{
    sdActiva=false;  
    digitalWrite(pinSD,HIGH);    
    pantalla.clrScr(); // borra la pantalla    pantalla.print(" Hay un ", CENTER, 0);//imprime la frase en el centro de la zona superior
    pantalla.print("Problema", CENTER, 10);//imprime la frase en el centro de la zona superior
    pantalla.print("en la SD", CENTER, 20);//imprime la frase en el centro de la zona superior
    pantalla.update();
    while(!SD.begin(5));
    sdActiva=true;
    digitalWrite(pinSD,LOW);    
    //logData = SD.open("/log.txt", FILE_WRITE);
  }  
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
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
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
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

void lecturaSensores()
{
  if(lightCont == light)
  {
    lightValue = analogRead(LIGHT_SENSOR_PIN);
    lightCont=0;
  } 
  if(airCont == air)
  {     
    CO2Value = airSensor.getCO2();
    tempValue = airSensor.getTemperature();
    humidityValue = airSensor.getHumidity(); 
    airCont=0;
  }
  if(soilHumidityCont == soilHumidity) 
  {
    soilHumidityValue = analogRead(SOILHUMIDITY_SENSOR_PIN);
    soilHumidityCont = 0;
  }
    
  if(loudnessCont == loudness) 
  {
    loudnessValue = analogRead(LOUDNESS_SENSOR_PIN);
    loudnessValue=(loudnessValue*1023)/4096;
    loudnessCont=0;
  }
}

//modo carrusel
void mostrarCarrusel()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(50,0);
  display.print("CO2:");
  display.setCursor(0,50);
  display.print(CO2Value);
  display.display(); 
  delay(cambio);  
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Temperature:");
  display.setCursor(0,50);
  display.print(tempValue);
  display.display(); 
  delay(cambio); 
   
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(5,0);
  display.print("Humidity:");
  display.setCursor(0,50);
  display.print(humidityValue);
  display.display(); 
  delay(cambio);  
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(30,0);
  display.print("Light:");
  display.setCursor(0,50);
  display.print(lightValue);
  display.display(); 
  delay(cambio);
    
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(5,0);
  display.print("Loudness:");
  display.setCursor(0,50);
  display.print(loudnessValue);
  display.display(); 
  delay(cambio);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Soil Humidity:");
  display.setCursor(0,50);
  display.print(soilHumidityValue);
  display.display(); 
  delay(cambio);
}

void mostrarTodo()
{
  pantalla.clrScr(); // borra la pantalla
  
  pantalla.print("CO2: ",LEFT,0);
  pantalla.printNumI(CO2Value,RIGHT,0);
  
  pantalla.print("Temperature: ",LEFT,10);
  pantalla.printNumI(tempValue,RIGHT,10);
 
  pantalla.print("Humidity: ", LEFT,20);
  pantalla.printNumI(humidityValue,RIGHT,20);
  
  pantalla.print("Light: ", LEFT,30);
  pantalla.printNumI(lightValue,RIGHT,30);

  pantalla.print("Loudness: ", LEFT,40);
  pantalla.printNumI(loudnessValue,RIGHT,40);

  pantalla.print("Soil Humidity: ", LEFT,50);
  pantalla.printNumI(soilHumidityValue,RIGHT,50);
  
  pantalla.update();// actualiza la pantalla
}

void escrituraSensores()
{
  String CO2Message = completeDate()+";"+CO2_ID+";"+CO2Value;
  String TempMessage = completeDate()+";"+Temp_ID+";"+tempValue;
  String HumidityMessage = completeDate()+";"+Humidity_ID+";"+humidityValue;  
  String LightMessage = completeDate()+";"+Light_ID+";"+lightValue;
  String LoudnessMessage = completeDate()+";"+Loudness_ID+";"+humidityValue;
  String soilHumidityMessage = completeDate()+";"+SoilHumidity_ID+";"+soilHumidityValue;
  
  String writeLog = "\n"+ CO2Message +"\n"+ TempMessage +"\n"+ HumidityMessage +"\n"+ LightMessage +"\n"+ LoudnessMessage +"\n"+ soilHumidityMessage;
  
  appendFile(SD, "/log.txt", writeLog.c_str());
  
}

void comprobarBot()
{
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      //Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}

void handleNewMessages(int numNewMessages)
{

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/carrusel")
    {
      carruselON = true;
      bot.sendMessage(chat_id, "Se ha configurado en modo carrusel", "");
    }

    if (text == "/fijo")
    {      
      carruselON = false;
      bot.sendMessage(chat_id, "Se ha configurado en modo fijo", "");
    }


    if (text == "/start")
    {
      String welcome = "Bienvenido a la configuracion del sistema TEASPILS, " + from_name + ".\n";
      welcome += "/carrusel : modo carrusel\n";
      welcome += "/fijo : modo carrusel\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}
