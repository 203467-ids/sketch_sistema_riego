//LIBRERIAS 
#include <LiquidCrystal_I2C.h> //Libreria para la pantalla led con interfac I2C
#include "ThingSpeak.h"        //Libreria de ThingSpeak para monitorizar nuestros sensores 
#include "WiFi.h"              //Libreria para concectarnos a wifi
#include <DHT11.h>             //Libreria DHT11
#include <HTTPClient.h>        //Libreria httpclient
#include <ArduinoJson.h>      //Libreria para el envio de JSon

//CONSTANTES PARA PINES
const int sensorHs = 34;     //Pin para el sensor de humedad del suelo
const int sensorAg = 35;     //Pin para el sensor nivel de agua 
const int sensorDh= 14;      //Pin para el sensor de temperatura y humedad ambiental 
const int relay = 4;         //Pin para el relay, motor agua.

//VARIABLES
int humedadS;                //Variable para obtener la humedad del suelo
int nivelAg;                 //Variable para obtener nivel del agua 
int lvl;                     //Variable para mapear los valores del sensor del agua 
float temp, hum;             //Variables para obtener los valores de temperatura y humedad ambiental (aire)
bool is_riego=false;

LiquidCrystal_I2C lcd(0x27, 16, 2);  //Declarar el protocolo y que tipo de pantalla se trata 
DHT11 dht11(sensorDh);               //Se agrega el pin para declarar el sensor DHT!!
unsigned long channelID = 1806697;                //ID de vuestro canal.
const char* WriteAPIKey = "33GBUHO2NA1Y65KB";     //Write API Key de vuestro canal.
WiFiClient cliente;                              //Cliente wifi

void setup(){
  Serial.begin(115200);                     //Inicializamos el puerto serial
  pinMode(relay, OUTPUT);                   //Declaramos el pin del relay como salida
  conectarWifi();                           //Funcion para conectarse a WIFI
  ThingSpeak.begin(cliente);                //Inicializar servicio
  lcd.init();                               //Inicializar lcd                    
  lcd.backlight();                          //Encender pantalla
}

void loop(){

  delay(1000);
  nivelAgua();                              //Funcion para sensor de agua 
  delay(1000);
  leerdht11();                              //Funcion para sensor de humedad y temperatura ambiental
  delay(1000);
  humedadSuelo();                           //Funcion para sensor de humedad del suelo
  delay(1000);
  ThingSpeak.writeFields(channelID,WriteAPIKey); // Mantener conexion de Thingspeak
  Serial.println("Datos enviados a ThingSpeak!");
  postDataToServer();
  delay(1000);
}

//FUNCIONES 

//WIFI
void conectarWifi(){
  const char* ssid = "DiNet-AC15";                     //SSID de vuestro router.
  const char* password = "Qe6RT37gb1";                //Contraseña de vuestro router.
  
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi conectado!");

  
}

//DHT11
void leerdht11() {
int err;
if((err=dht11.read(hum, temp))==0)
  {
    Serial.print("Temperatura: ");
    Serial.print(temp);                //Valor de la temperatura
    Serial.print(" Humedad: "); 
    Serial.print(hum);                 //Valor de humedad 
    Serial.println();
   lcd.clear();
  lcd.setCursor(0, 0);
                                        //Enviamos los datos al LCD                 
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print(" C");
  lcd.setCursor(0,1);
  lcd.print("Humedad: ");
  lcd.print(hum);
    
  }
       else
          {
             Serial.println();              //Nos envia mensaje de error que no detecta el sensor de DHT11
             Serial.print("Error Num :");
             Serial.print(err);
             Serial.println();
          }

  ThingSpeak.setField (1,temp);
  ThingSpeak.setField (2,hum);             //Enviamos datos 
}

//NIVEL DE AGUA 
void nivelAgua(){
  nivelAg = analogRead(sensorAg);      //Lectura del sensor de agua 
  lvl = map(nivelAg, 0, 680, 0, 100);  //Mapeo de los valores que puede tomar el sensor  
  Serial.print("Nivel Agua: ");
  Serial.print(lvl);
  Serial.println();
  lcd.clear();
  lcd.setCursor(0, 0);
  // print message
  lcd.print("Nivel Agua:");          //Envio de datos para lcd y thingspeak
  lcd.setCursor(0,1);
  if(lvl<=100)
  {
    lcd.print("Vacio");
    ThingSpeak.setField (4,lvl);
  
  }
  else if(lvl>100 && lvl<=300)
  {
    lcd.print("Bajo");
    ThingSpeak.setField (4,lvl);
  }
  else if(lvl>300 && lvl<=400)
  {
    lcd.print("Medio");
    ThingSpeak.setField (4,lvl);
  }
  else if(lvl>400)
  {
    lcd.print("Lleno");
    ThingSpeak.setField (4,lvl);
  }
}

void humedadSuelo(){
   humedadS = analogRead(sensorHs);               //Lectura del sensor de humedad del suelo
   lcd.clear();
  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print message
  lcd.print("Humedad Suelo:");                  // Enviar datos al lcd  y thingspeak
  lcd.setCursor(0,1);
  if(humedadS==4095)
  {
    lcd.print("Seco, ");
    lcd.print("Regando!!");
    ThingSpeak.setField (3,humedadS);
   
  digitalWrite(relay, LOW);                      //Si esta seco mandamos señal para que se encienda la bomba mediante el relay 
  Serial.println("prender bomba");
  delay(2000);
   digitalWrite(relay, HIGH);
  Serial.println("apagar bomba");
  is_riego=true;
  delay(1000); 
  }
  else if(humedadS>3000 && humedadS<=4095)
  {
    lcd.print("Semiseco");
    ThingSpeak.setField (3,humedadS);
    is_riego=false;
  
  
  
  }
  else if(humedadS>2000 && humedadS<=3000)
  {
    lcd.print("Semihumedo");
    ThingSpeak.setField (3,humedadS);
   is_riego=false;
  
  
  }
  else if(humedadS<2000)
  {
    lcd.print("Humedo");
    ThingSpeak.setField (3,humedadS);
    is_riego=false;
 
  }
  Serial.print("humedad del suelo: ");
  Serial.print(humedadS);
  Serial.println();
  delay(1000);
}
void postDataToServer() {
 
  Serial.println("Posting JSON data to server...");
 
     
    HTTPClient http;   
     
    http.begin("http://192.168.228.6:3000/riego");           //Datos de nuestro servidor 
    http.addHeader("Content-Type", "application/json");         
     
    StaticJsonDocument<200> doc;
    // Add values in the document          //agregamos los datos a enviar al Api
    //
    doc["temperatura_tanque"] = temp-5.0;
    doc["nivel_tanque"] = lvl;
    doc["nivel_humedad"] = humedadS;
    doc["temperatura_ambiente"] = temp;
    doc["fue_riego"] = is_riego;
   
   
    String requestBody;
    serializeJson(doc, requestBody);
     
    int httpResponseCode = http.POST(requestBody);   //enviamos el json 
 
    if(httpResponseCode>0){
       
      String response = http.getString();                       
       
      Serial.println(httpResponseCode);   
      Serial.println(response);
     
    }
    else {
     
      Serial.printf("Error occurred while sending HTTP POST: %s\n");
       
    }
     
  
 }
