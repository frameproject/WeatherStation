/////////////////////////////////////////////////////////////////
///////////////////////////// INCLUDE ///////////////////////////
//RTC
#include <DS3231.h>

//DHT
#include <DHT.h>
#define DHTPIN 5        // Digital pin
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#include <Wire.h>       // arduino-1.8.8/hardware/arduino/avr/libraries/Wire/src

// LCD
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

//SD library:
#include <SPI.h>
#include <SD.h>

// DEBUG
//#define DEBUG

/////////////////////////////////////////////////////////////////
///////////////////////////// OBJETS ////////////////////////////
// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);
// Init a Time-data structure
Time  t;
// LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address
// Temp and humidity
DHT dht(DHTPIN, DHTTYPE);
// Serial port
//SoftwareSerial ESP8266(2, 3);
/////////////////////////////////////////////////////////////////
//////////////////////////// PRIMITIVE //////////////////////////
void checkLCD();
void checkDHT();
void checkSD();
void checkRTC();
void checkESP();

int getSensor(float*, float*);
void saveData();
void sendData();

void sendData();
void initESP8266();
void envoieAuESP8266(String);
void recoitDuESP8266(const int);
/////////////////////////////////////////////////////////////////
//////////////////////////// VAR ////////////////////////////////

float temp, humidity;

/////////////////////////////////////////////////////////////////

/****************************************************************/
/*                          setup                               */
/****************************************************************/
void setup()
{
Serial.begin(9600);

// T and H sensor
dht.begin();

// LCD
lcd.begin (16, 2);                 
lcd.clear();
lcd.setCursor(0, 0);  
lcd.setBacklight(LOW);
}

/****************************************************************/
/*                          loop                                */
/****************************************************************/
void loop()
{  
  getSensor();
  
  delay(3000);

} 

/****************************************************************/
/*                       check component                        */
/****************************************************************/
void checkSystem()
{
checkLCD();
checkSD();
checkRTC();
checkESP();
checkDHT();
}
/****************************************************************/
/*                           Print                              */
/****************************************************************/
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}
/****************************************************************/
/*                         Record data                          */
/****************************************************************/
void saveData()
{
Serial.println("Save data");
  
   File myFile = SD.open("log.txt", FILE_WRITE);
  
  if (myFile) {    
   // Serial.print("Writing temp...");
    myFile.print(temp);
    myFile.print(" ");
    myFile.print(humidity);
    myFile.print(" ");
    myFile.print(t.hour);
    myFile.print(":");
    myFile.print(t.min);
    myFile.print(" ");
    myFile.print(t.date);
    myFile.print("/");
    myFile.print(rtc.getMonthStr());
    myFile.print("/");
    myFile.println(t.year);
    myFile.close();   
  }
}
/****************************************************************/
/*                         get Sensor T / H                     */
/****************************************************************/
int getSensor()
{
Serial.println("getSensor");

 humidity=dht.readHumidity();
// Read temperature as Celsius (the default)
 temp = dht.readTemperature();

if (isnan(humidity) || isnan(temp) ) {
     lcd.clear();
     lcd.setCursor(0, 0);
     lcd.print("Failed sensor!");
    return -1;
}
else
{  
  lcd.setCursor(0, 0);
  lcd.print("T:     ");
  lcd.setCursor(2, 0);
  lcd.print(temp);
  lcd.setCursor(8, 0);
  lcd.print("H:     ");
  lcd.setCursor(10, 0);
  lcd.print(humidity); 
}
  return 0;
}

/****************************************************************/
/*                       Data to cloud                          */
/****************************************************************/
void sendData()
{
 
Serial.println("sendData");
  
// TCP connection
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "api.thingspeak.com"; // api.thingspeak.com connection IP
  cmd += "\",80";       // api.thingspeak.com connection port, 80
  Serial1.println(cmd);

  recoitDuESP8266(2000);

  if(Serial1.find("Error")){
    Serial.println("AT+CIPSTART error");
    return;
  }
  
  // Set String, Data to send by GET method
  String getStr = "GET /update?api_key=";
  getStr += "Q9015QGHZZCML7Qm";
  getStr +="&field1=1";  
  getStr += "\r\n\r\n";
 
  // Send Data
  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  Serial1.println(cmd);
  
  if(Serial1.find(">")){
    Serial1.print(getStr);
    Serial.println(getStr);
  }
  else{
    Serial1.println("AT+CIPCLOSE");
    // alert user
    Serial.println("AT+CIPCLOSE");
  }
  recoitDuESP8266(2000);
}
/****************************************************************/
/*                  initialize ESP8266                          */
/****************************************************************/
void initESP8266()
{  
  String NomduReseauWifi = "HUAWEI"; 
  String MotDePasse      = "test1234"; 

  Serial.println("**********************************************************");  
  Serial.println("**************** DEBUT DE L'INITIALISATION ***************");
  Serial.println("**********************************************************");  
  envoieAuESP8266("AT+RST");
  recoitDuESP8266(2000);
  Serial.println("**********************************************************");
  envoieAuESP8266("AT+CWMODE=3");
  recoitDuESP8266(5000);
  Serial.println("**********************************************************");
  envoieAuESP8266("AT+CWJAP=\""+ NomduReseauWifi + "\",\"" + MotDePasse +"\"");
  recoitDuESP8266(10000);
  Serial.println("**********************************************************");
  envoieAuESP8266("AT+CIFSR");
  recoitDuESP8266(1000);
  Serial.println("**********************************************************");
  envoieAuESP8266("AT+CIPMUX=0");   
  recoitDuESP8266(1000);
//  Serial.println("**********************************************************");
//  envoieAuESP8266("AT+CIPSERVER=1,80");
//  recoitDuESP8266(1000);
  Serial.println("**********************************************************");
  Serial.println("***************** INITIALISATION TERMINEE ****************");
  Serial.println("**********************************************************");
  Serial.println("");  
}

/****************************************************************/
/*                        Data to ESP8266                       */
/****************************************************************/
void envoieAuESP8266(String commande)
{  
  Serial1.println(commande);
}
/****************************************************************/
/*                       Data from ESP8266                      */
/****************************************************************/
void recoitDuESP8266(const int timeout)
{
  String reponse = "";
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(Serial1.available())
    {
      char c = Serial1.read();
      reponse+=c;
    }
  }
  Serial.print(reponse);   
}

/****************************************************************/
/*                        check DS3231                          */
/****************************************************************/
// Arduino Mega:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 20 (SDA) or the dedicated SDA pin
//          SCL pin   -> Arduino Digital 21 (SCL) or the dedicated SCL pin
void checkRTC()
{
  // Initialize the rtc object
  rtc.begin();
  
  // The following lines can be uncommented to set the date and time
  //rtc.setDOW(SUNDAY);     // Set Day-of-Week to SUNDAY
  //rtc.setTime(17, 26, 0);     // Set the time to 12:00:00 (24hr format)
  //rtc.setDate(18, 02, 2019);   // Set the date to January 1st, 2014

  for(int i=0;i<10;i++)
  {
   // Get data from the DS3231
  t = rtc.getTime();
  
  // Send Day-of-Week and time
  Serial.print(rtc.getDateStr());
  Serial.print(" ");
  Serial.print(t.hour, DEC);
  Serial.print(":");
  Serial.print(t.min, DEC);
  Serial.print(":");
  Serial.print(t.sec, DEC);
  Serial.print(" ");
  Serial.println();

  delay(1000);
  }
}

/****************************************************************/
/*                        check LCD                             */
/****************************************************************/
void checkLCD()
{
  lcd.begin (16, 2);                 
  lcd.clear();
  lcd.setCursor(0, 0);  
  lcd.setBacklight(LOW);

 lcd.setCursor(0, 0);
 lcd.print("ok !");
}

/****************************************************************/
/*                        check SD                              */
/****************************************************************/
void checkSD()
{
///////////////////////////////////////////////////////
// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

Serial.print("\nInitializing SD card...");

// we'll use the initialization code from the utility libraries
// since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, 10)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    while (1);
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

// print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

// Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1);
  }

  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();

// print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Gb):  ");
  Serial.println((float)volumesize / 1024.0);

  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);

// list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
}

/****************************************************************/
/*                        check ESP                             */
/****************************************************************/
void checkESP()
{
Serial1.begin(9600); 
initESP8266();
sendData();
}

/****************************************************************/
/*                        check DHT                             */
/****************************************************************/
void checkDHT()
{
  dht.begin();

  float h=dht.readHumidity();
  Serial.print("Humidity = ");
  Serial.println(h);
 
  float t = dht.readTemperature();
  Serial.print("Temp = ");
  Serial.println(t);
}
