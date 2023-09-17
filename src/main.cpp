#include <SPI.h>
#include <Arduino.h>
#include <MFRC522.h>
#include <freertos/FreeRTOS.h>
#include <freertos/list.h>
#include <WiFiManager.h> 
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
//RFID
#define RST_PIN         15          
#define SS_PIN          5    
MFRC522 mfrc522(SS_PIN, RST_PIN); 
MFRC522::MIFARE_Key key;  
char uid[10];

//LCD
#define I2C_SDA 21
#define I2C_SCL 22
LiquidCrystal_I2C lcd(0x27, 16, 2);

//wifi
#define WIFI_SSID "Chiko"
#define WIFI_PASSWORD "chikojuga"

SemaphoreHandle_t xSemaphore = NULL;

void printHex(byte *buffer, byte bufferSize);
void printLocalTime();

bool stale = true;
int ulang=0;

void baca_kartu(void * parameters){
  for(;;){
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
    if (stale)
    { 
      if (ulang==0)
      {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Tempelkan kartu!");
        ulang++;
      }
      else if (ulang>20)
      {
        ulang=0;
      } else
      {
        ulang++;
      }
      vTaskDelay(500/portTICK_PERIOD_MS);
    }
	}

	// Select one of the cards
	  if ( ! mfrc522.PICC_ReadCardSerial()) {
	
	} else
  {
    stale=false;
    sprintf(uid, "%d%d%d%d", mfrc522.uid.uidByte[0],mfrc522.uid.uidByte[1],mfrc522.uid.uidByte[2],mfrc522.uid.uidByte[3]);
    
    Serial.print("Uid is:");
    printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    //lcd section
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Uid is :");
    lcd.setCursor(0,1);
    lcd.print(uid);
    Serial.println();
    Serial.print("Waktu absen:");
    Serial.println();
    xSemaphoreGive(xSemaphore);
    digitalWrite(21,LOW);
    vTaskDelay(500/portTICK_PERIOD_MS);
    digitalWrite(21,HIGH);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
  }
  
  }
}



void kirim_data(void * parameters){
  for(;;){
    if(xSemaphoreTake(xSemaphore,portMAX_DELAY)){
    
  char payload[100];
  sprintf(payload,"http://192.168.184.224/api/rfid/%s",uid);  
  Serial.println(payload); 
  // String payload= "http://192.168.184.224/api/rfid/120399";
  //send_data
  HTTPClient http;
  String serverpath = payload;
  http.begin(serverpath);
  int httpResponseCode = http.GET();
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  Serial.println(http.getString());
  http.end();
    
    //   json.set("UID",uid );
    //   json.set("time",waktu);
    // if (Firebase.ready() && signupOK ){
    
    // if (Firebase.RTDB.pushJSON(&fbdo,"uid/history_log",&json)){
    //   Serial.println("PASSED");
    // }
    // else {
    //   Serial.println("FAILED");
    //   Serial.println("REASON: " + fbdo.errorReason());
    // }
    // char find[50];
    // sprintf(find,"uid/access/%s",uid);
    // Serial.println(find);
    
    // if (Firebase.RTDB.getJSON(&fbda, find))
    // {
    //   Serial.println(fbda.stringData());
    //   lcd.clear();
    //   lcd.setCursor(0,0);
    //   lcd.print("Berhasil absen!");
    //   lcd.setCursor(0,1);
    //   lcd.print(fbda.stringData());
    //   digitalWrite(21,LOW);
    //   vTaskDelay(500/portTICK_PERIOD_MS);
    //   digitalWrite(21,HIGH);
    //   stale=true;
    // }
    // else
    // {
    //   // Failed to get JSON data at defined database path, print out the error reason
    // Serial.println(fbda.errorReason());
    // lcd.clear();
    // lcd.setCursor(0,0);
    // lcd.print("Kartu tidak");
    // lcd.setCursor(0,1);
    // lcd.print("terdaftar");
    // digitalWrite(21,LOW);
    //   vTaskDelay(500/portTICK_PERIOD_MS);
    //   digitalWrite(21,HIGH);
    //   vTaskDelay(200/portTICK_PERIOD_MS);
    //   digitalWrite(21,LOW);
    //   vTaskDelay(500/portTICK_PERIOD_MS);
    //   digitalWrite(21,HIGH);
    // stale=true;
    // }
    // }
    
  }
  }
}




void setup() {
	Serial.begin(9600);
  	
  pinMode(21,OUTPUT);
  digitalWrite(21,HIGH);
	while (!Serial);		
	SPI.begin();			
	mfrc522.PCD_Init();		
	delay(4);				
	mfrc522.PCD_DumpVersionToSerial();	
	for (byte i = 0; i < 6; i++) {
    	key.keyByte[i] = 0xFF;
  }
  
  //lcdsetup
  lcd.init(I2C_SDA, I2C_SCL); 
	lcd.backlight();
  

  //wifi
  WiFiManager wm;
  bool done;
  lcd.setCursor(0,0);
  lcd.print("Please Connect :");
  lcd.setCursor(0,1);
  lcd.print("to Smartdoorlock");
  done=wm.autoConnect("Smartdoorlock","password");
  
  
  Serial.println();
  lcd.clear();
  lcd.setCursor(0,0);
  Serial.print("Connected to : ");
  lcd.print("Connected to : ");
  Serial.println(wm.getWiFiSSID());
  lcd.setCursor(0,1);
  lcd.print(wm.getWiFiSSID());
  delay(5000);
  Serial.println();

  

  xSemaphore = xSemaphoreCreateBinary();
  
  xTaskCreate(
    baca_kartu, "baca_kartu", 4000,NULL,0,NULL
  );

  xTaskCreate(
    kirim_data, "kirim_data", 8000,NULL,0, NULL
    );

}

void loop() {
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}



