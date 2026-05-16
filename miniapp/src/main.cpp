// Display-Treiberbibliothek
#include <TFT_eSPI.h>
// SPI-Treiberbibliothek
#include <SPI.h>
// JPEG-Dekoder fuer Bilddaten
#include <TJpg_Decoder.h>
// JSON-Bibliothek fuer Web-API-Antworten
#include "ArduinoJson.h"
// Zeitbibliothek fuer die Uhranzeige
#include <TimeLib.h>
// WLAN-Bibliothek
#include <WiFi.h>
// Deep-Sleep-Unterstuetzung
#include <esp_sleep.h>
#include <driver/gpio.h>
// HTTP-Client fuer Web-Anfragen
#include <HTTPClient.h>
// UDP-Unterstuetzung fuer NTP ueber WLAN
#include <WiFiUdp.h>
// EEPROM-Speicher fuer feste Parameter
 #include <EEPROM.h>

#include <Arduino.h>
#include "lvgl.h"
// Wettersymbole
#include "weathernum.h"

// Temperatur- und Feuchtigkeitssymbole
#include "img/temperature.h"
#include "img/humidity.h"

// Schriftart fuer die Uhranzeige
#include "font/FxLED_32.h"
// Schriftart fuer Wochentag, Datum und Zahlenanzeigen
#include "font/zkyyt12.h"
// Schriftart fuer den Stadtnamen
#include "font/city10.h"
// Schriftart fuer Wettertexte
#include "font/tq10.h"
//#include "font/AAA.h"
#include "font/ALBB10.h"


// WLAN-Zugangsdaten fuer das Testnetz.
const char *ssid     = "Agathas-Netz-16";  // WLAN-Name des Testnetzes.
const char *password = "1234567890123050363"; // WLAN-Passwort des Testnetzes.

constexpr uint8_t deepSleepButtonPin = 8;
constexpr unsigned long deepSleepButtonPressDuration = 2500;
unsigned long deepSleepButtonPressedSince = 0;
bool deepSleepButtonPressHandled = false;

WeatherNum  wrat; // Wetteranzeige-Objekt
int prevTime = 0;
int AprevTime = 0;
int Anim = 0;
uint32_t targetTime = 0;


// Hintergrundfarbe
uint16_t bgColor =0x0000 ;

// Schriftfarben

// Schriftfarbe fuer Stunden und Minuten
uint16_t timehmfontColor =0xFFFF ;
// Schriftfarbe fuer Sekunden
uint16_t timesfontColor =0xFFFF ;
// Schriftfarbe fuer den Wochentag
uint16_t weekfontColor =0xFFFF ;
// Schriftfarbe fuer Datum
uint16_t monthfontColor =0xFFFF ;
// Schriftfarbe fuer Temperatur und Feuchtigkeit
uint16_t thfontColor =0xFFFF ;
// Schriftfarbe fuer die Wetter-Laufschrift
uint16_t tipfontColor =0xFFFF ;
// Schriftfarbe fuer den Stadtnamen
uint16_t cityfontColor =0xFFFF ;
// Schriftfarbe fuer die Wetterkennzahl
uint16_t airfontColor =0xFFFF ;
// Schriftfarbe fuer die Bilibili-Fanzahl
uint16_t bilifontColor =0xF81F ;




// Rahmenfarbe

uint16_t  xkColor= 0xFFFF;

//lvgl
static const uint16_t screenWidth = 128;
static const uint16_t screenHeight = 128;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenHeight * screenWidth / 10];





// Uhrparameter---------------------------------
// NTP-Server
static const char ntpServerName[] = "time.nist.gov"; // NTP-Server
float timeZone;     // Zeitzone



static String cityCode = "";  // Fuer eine feste Stadt hier den Stadtnamen eintragen und den automatischen Stadtabruf deaktivieren.
const char* api_key = "fbf5a0e942e6fea3ff18103b9fd46ed9"; // API-Schluessel fuer OpenWeatherMap.


WiFiUDP Udp;
unsigned int localPort = 8000;
WiFiClient wificlient;


time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
String num2str(int digits);
void sendNTPpacket(IPAddress &address);
void scrollTxt(int pos);
String week();
String monthDay();
String hourMinute();
void configureDeepSleepButton();
void handleDeepSleepButton();
bool isDeepSleepButtonPressed();
void enterDeepSleep();
void waitForDeepSleepButtonRelease();
void prepareDisplayForDeepSleep();
void releaseDisplayResetHold();
void holdDisplayResetForDeepSleep();

byte setNTPSyncTime = 20; // NTP-Synchronisationsintervall in Minuten.

int backLight_hour=0;

time_t prevDisplay = 0; // Letzte angezeigte Zeit


//-----------------------------------------

//---------------------- Wetter-Messwerte ------------------
unsigned long wdsdTime = 0;
byte wdsdValue = 0;
String wendu1 = "",wendu2 = "",shidu = "",yaqiang = "",tianqi = "",kjd = "";

//----------------------------------------------------

LV_IMG_DECLARE(TKR_A);
static lv_obj_t *logo_imga = NULL;


TFT_eSPI tft = TFT_eSPI(128,128);  // Display-Pins werden in platformio.ini fuer TFT_eSPI gesetzt.
TFT_eSprite clk = TFT_eSprite(&tft);



bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{

  if ( y >= tft.height() ) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  // Return 1 to decode next block
  return 1;

}


void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}


void tkr(void)
{
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenHeight * screenWidth / 10);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = 128;
  disp_drv.ver_res = 128;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;  
  lv_disp_drv_register(&disp_drv);

  static lv_style_t style;  
  lv_style_init(&style);
  lv_style_set_bg_color(&style, lv_color_black());
  lv_obj_add_style(lv_scr_act(), &style, 0);

  // Die importierte GIF-Startanimation loest auf dem ESP32-C3 einen LVGL-Decoder-Crash aus.
  // Die Hauptanzeige nutzt TFT_eSPI direkt und funktioniert ohne diese Startanimation.
  logo_imga = NULL;
}


//----------------------------------- Stadt- und Wetterdaten
// Stadt ueber die aktuelle IP-Adresse ermitteln.
void getCityCode() {
 
String URL = "http://ip-api.com/json/?fields=city,lat,lon";

HTTPClient httpClient;
httpClient.begin(wificlient,URL);


// HTTP-Verbindung starten und GET-Anfrage senden.
  int httpCode = httpClient.GET();
  Serial.print("Sende GET-Anfrage an URL: ");
  Serial.println(URL);

  if (httpCode == HTTP_CODE_OK) {
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc,httpClient.getString());
    String city = doc["city"];

    cityCode=city; // Lokalen Stadtnamen uebernehmen.
  }

  httpClient.end();

}



void getCityTime(){
  getCityCode();
  String URL = "http://api.openweathermap.org/data/2.5/weather?q=" + cityCode + "&appid=" + String(api_key) + "&units=metric"; //524901
  HTTPClient httpClient;
  httpClient.begin(URL);

  int httpCode = httpClient.GET();
   if (httpCode == HTTP_CODE_OK) {
      const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + JSON_OBJECT_SIZE(40) + 470;
      DynamicJsonDocument doc(capacity);
      deserializeJson(doc,httpClient.getString());

      long timezone1 = doc["timezone"]; // Zeitzonenversatz aus der Wetter-API uebernehmen.
      timeZone=(float)(timezone1/3600);

      Serial.println("Stadtinformationen erfolgreich geladen");
   }else {
    Serial.println("Stadtinformationen konnten nicht geladen werden");
    Serial.print(httpCode);
  }
  httpClient.end();
}

int temp_i1,temp_i2;
 String scrollText[5];
// Aktuelle Wetterdaten fuer die erkannte Stadt laden.
void getCityWeater(){

  float temp_f,temp_min_f,temp_max_f;
  getCityCode();
  String URL = "http://api.openweathermap.org/data/2.5/weather?q=" + cityCode + "&appid=" + String(api_key) + "&units=metric"; //524901
 
  // HTTPClient-Objekt fuer die API-Anfrage.
 HTTPClient httpClient;
 httpClient.begin(URL);


  int httpCode = httpClient.GET();
  Serial.println("Wetterdaten werden geladen");
  Serial.println(URL);

  // Bei erfolgreicher Antwort JSON-Daten auswerten.
  if (httpCode == HTTP_CODE_OK) {

    const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + JSON_OBJECT_SIZE(40) + 470;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc,httpClient.getString());
    // JsonObject sk = doc.as<JsonObject>();
    // String str1 = httpClient.getString();
 
    float temp = doc["main"]["temp"]; // Temperatur in Grad Celsius.
    int humidity = doc["main"]["humidity"]; // Luftfeuchtigkeit in Prozent.
    int pressure = doc["main"]["pressure"];
    String description = doc["weather"][0]["description"]; // Wetterbeschreibung
    String icon = doc["weather"][0]["icon"];
    float temp_min = doc["main"]["temp_min"];
    float temp_max = doc["main"]["temp_max"];
    int visibility = doc["visibility"];
    
    temp_f = 32 + temp*1.8;
    temp_min_f = 32 + temp_min*1.8;
    temp_max_f = 32 + temp_max*1.8;

    temp_i1 = int(temp_f);
    temp_i2 = int(temp);

    wendu1 = String(temp_i1);// Temperatur in Fahrenheit
    wendu2 = String(temp_i2); // Temperatur in Grad Celsius
    shidu = String(humidity); // Luftfeuchtigkeit
    yaqiang = String(pressure); // Luftdruck
    tianqi = String(description);// Wetterbeschreibung
    kjd = String(visibility/1000); // Sichtweite
    
    clk.setColorDepth(8);
    clk.loadFont(ALBB10);
  
    // shidu = sk["SD"].as<String>();

  // Stadtname
  clk.createSprite(77, 16);
  clk.fillSprite(bgColor);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(cityfontColor, bgColor);
  clk.drawString(cityCode,1,8); //
  clk.pushSprite(1,89);
  clk.deleteSprite();
  clk.unloadFont();
  // temp=26;
  uint16_t pm25BgColor = tft.color565(156, 202, 127);
  if(temp<10)
    pm25BgColor = tft.color565(0,0,255);// Blau
  else if(temp<20 && temp>=10)
    pm25BgColor = tft.color565(46,185,201);// Hellblau
  else if(temp>=20 && temp<=25)
    pm25BgColor = tft.color565(156, 202, 127); // Gruen
  else if(temp>25 && temp<30)
    pm25BgColor = tft.color565(222, 202, 24); // Gelb
  else if(temp>=30)
    pm25BgColor = tft.color565(136, 11, 32);// Rot
  
  clk.setColorDepth(8);
  clk.loadFont(ALBB10);
  clk.createSprite(36,15);
  clk.fillSprite(bgColor);
  clk.fillRoundRect(0, 0, 32, 15, 4, pm25BgColor);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(airfontColor);
  clk.drawString("Temp.", 3, 7);

  clk.pushSprite(93, 69);
  clk.deleteSprite();
  clk.unloadFont();

  scrollText[0] = "Min. T "+String(temp_min_f)+ "℉ / "+String(temp_min)+"℃";
  scrollText[1] = "Max. T "+String(temp_max_f)+ "℉ / "+String(temp_max)+"℃";
  scrollText[2] = "Wetter "+String(tianqi);
  scrollText[3] = "Luftdruck "+String(yaqiang)+" hPa";
  scrollText[4] = "Sichtweite "+String(kjd)+" km";
  wrat.printfweather1(1,47,icon);

  Serial.println("Wetterdaten erfolgreich geladen");

  } else {
    Serial.println("Fehler beim Laden der Wetterdaten:");
    Serial.print(httpCode);
  }

  // HTTP-Verbindung schliessen.
  httpClient.end();
}

//---------------- Temperatur- und Feuchtigkeitsanzeige ----------------

void weatherWarning() { // Wechselt alle 5 Sekunden zwischen Temperatur und Luftfeuchtigkeit aus der Wetter-API.
  if(millis() - wdsdTime > 5000) {
    wdsdValue = wdsdValue + 1;
    //Serial.println("wdsdValue0" + String(wdsdValue));
    clk.setColorDepth(8);
    clk.loadFont(ALBB10);
    switch(wdsdValue) {
      case 1:
      //Serial.println("wdsdValue1" + String(wdsdValue));
        TJpgDec.drawJpg(82,89,temperature, sizeof(temperature));  // Temperatursymbol
        for(int i=20;i>0;i--) {
          clk.createSprite(30,16);
          clk.fillSprite(bgColor);
          clk.setTextDatum(ML_DATUM);
          clk.setTextColor(thfontColor, bgColor);
          clk.drawString(wendu1+"℉",1,i+8); //AW wendu+
          clk.pushSprite(98,89);
          clk.deleteSprite();
          delay(10);
        }
        break;
      case 2:
      //Serial.println("wdsdValue1" + String(wdsdValue));
        TJpgDec.drawJpg(82,89,temperature, sizeof(temperature));  // Temperatursymbol
        for(int i=20;i>0;i--) {
          clk.createSprite(30,16);
          clk.fillSprite(bgColor);
          clk.setTextDatum(ML_DATUM);
          clk.setTextColor(thfontColor, bgColor);
          clk.drawString(wendu2+"℃",1,i+8); //AW wendu+
          clk.pushSprite(98,89);
          clk.deleteSprite();
          delay(10);
        }
        break;
      case 3:
      //Serial.println("wdsdValue2" + String(wdsdValue));
        TJpgDec.drawJpg(82,89,humidity, sizeof(humidity));  // Feuchtigkeitssymbol
        for(int i=20;i>0;i--) {
          clk.createSprite(30, 16);
          clk.fillSprite(bgColor);
          clk.setTextDatum(ML_DATUM);
          clk.setTextColor(thfontColor, bgColor);
          clk.drawString(shidu+"%",1,i+8);
          clk.pushSprite(98,89);
          clk.deleteSprite();
          delay(10);
        }
        wdsdValue = 0;
        break;
    }
    wdsdTime = millis();
    clk.unloadFont();
  }
}


//---------------------------- Wetter-Laufschrift oben links -------------------------
int currentIndex = 0;

TFT_eSprite clkb = TFT_eSprite(&tft);

void scrollBanner(){
  if(millis() - prevTime > 3500){ // Alle 3,5 Sekunden zum naechsten Eintrag wechseln.

    if(scrollText[currentIndex]){

      clkb.setColorDepth(8);
      clkb.loadFont(ALBB10);

      for(int pos = 20; pos>0 ; pos--){
        scrollTxt(pos);
      }

      clkb.deleteSprite();
      clkb.unloadFont();

      if(currentIndex>=4){
        currentIndex = 0;  // Zurueck zum ersten Eintrag.
      }else{
        currentIndex += 1;  // Zum naechsten Eintrag wechseln.
      }

    }
    prevTime = millis();

  }
}

void scrollTxt(int pos){
    clkb.createSprite(128, 16);
    clkb.fillSprite(bgColor);
    clkb.setTextWrap(false);
    clkb.setTextDatum(ML_DATUM);
    clkb.setTextColor(tipfontColor, bgColor);
    clkb.drawString(scrollText[currentIndex],1,pos+8);
    clkb.pushSprite(1,1);
}

//----------------------------------------------




byte loadNum = 6;
void loading(byte delayTime)// Fortschrittsbalken zeichnen.
{
  clk.setColorDepth(8);
  clk.createSprite(100, 100);// Sprite-Flaeche anlegen.
  clk.fillSprite(0x0000);   // Sprite-Hintergrund fuellen.

  clk.drawRoundRect(0,0,100,16,8,0xFFFF);       // Umriss des Fortschrittsbalkens.
  clk.fillRoundRect(3,3,loadNum,10,5,0xFFFF);   // Gefuellter Fortschrittsbalken.
  clk.setTextDatum(CC_DATUM);   // Textausrichtung setzen.
  clk.setTextColor(TFT_GREEN, 0x0000);
  clk.drawString("Verbinde WLAN...",50,40,2);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawRightString("MiNiApp V1.0",100,60,2);
  clk.pushSprite(14,40);  // Sprite-Position auf dem Display.

  //clk.setTextDatum(CC_DATUM);
  //clk.setTextColor(TFT_WHITE, 0x0000);
  //clk.pushSprite(130,180);

  clk.deleteSprite();
  loadNum += 1;
  delay(delayTime);
}


void get_wifi()
{
  Serial.println("WLAN-Verbindung wird gestartet.");
  // WLAN-Verbindung starten.
  WiFi.begin(ssid, password);
  const unsigned long wifiStartTime = millis();
  // Auf WLAN-Verbindung warten.
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 30000)
  {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("");
    Serial.println("WLAN-Verbindung nach 30 Sekunden abgebrochen.");
    return;
  }
  Serial.println("");
  Serial.println("WLAN verbunden");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());
}

void configureDeepSleepButton()
{
  pinMode(deepSleepButtonPin, INPUT_PULLUP);
}

bool isDeepSleepButtonPressed()
{
  return digitalRead(deepSleepButtonPin) == LOW;
}

void handleDeepSleepButton()
{
  if (!isDeepSleepButtonPressed()) {
    deepSleepButtonPressedSince = 0;
    deepSleepButtonPressHandled = false;
    return;
  }

  if (deepSleepButtonPressedSince == 0) {
    deepSleepButtonPressedSince = millis();
    return;
  }

  if (!deepSleepButtonPressHandled && millis() - deepSleepButtonPressedSince >= deepSleepButtonPressDuration) {
    deepSleepButtonPressHandled = true;
    enterDeepSleep();
  }
}

void waitForDeepSleepButtonRelease()
{
  while (isDeepSleepButtonPressed()) {
    delay(20);
  }
  delay(150);
}

void prepareDisplayForDeepSleep()
{
  tft.fillScreen(TFT_BLACK);
  delay(80);
  tft.writecommand(ST7735_DISPOFF);
  delay(120);
  tft.writecommand(ST7735_SLPIN);
  delay(120);
  holdDisplayResetForDeepSleep();
}

void releaseDisplayResetHold()
{
#if defined(TFT_RST) && (TFT_RST >= 0)
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis((gpio_num_t)TFT_RST);
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  delay(5);
#endif
}

void holdDisplayResetForDeepSleep()
{
#if defined(TFT_RST) && (TFT_RST >= 0)
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  gpio_hold_en((gpio_num_t)TFT_RST);
  gpio_deep_sleep_hold_en();
#endif
}

void enterDeepSleep()
{
  Serial.println("DeepSleep wird vorbereitet.");

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(CC_DATUM);
  tft.drawString("Schlafmodus", 64, 42, 2);
  tft.drawString("Taste loslassen", 64, 64, 2);
  tft.drawString("IO8 weckt auf", 64, 86, 2);

  delay(700);
  waitForDeepSleepButtonRelease();

  prepareDisplayForDeepSleep();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.flush();

  esp_deep_sleep_enable_gpio_wakeup(1ULL << deepSleepButtonPin, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}


//---------------------------------------------------------------------------
void setup() {

         Serial.begin(115200);
         delay(200);
         Serial.println("Start: Setup beginnt.");
         configureDeepSleepButton();
         releaseDisplayResetHold();
         EEPROM.begin(1024);
         Serial.println("Start: EEPROM initialisiert.");



         tft.begin(); /* TFT init */
         Serial.println("Start: TFT initialisiert.");
         tft.invertDisplay(0);// Display-Invertierung: 1 invertiert, 0 normal.
         tft.fillScreen(0x0000);
         tft.setTextColor(TFT_WHITE, 0x0000);
         // Display-Rotation setzen. Gueltige Werte: 0, 1, 2, 3.
        // Werte entsprechen 0, 90, 180 und 270 Grad.
        // Rotation passend zur Montage auswaehlen.
          tft.setRotation(2);
          Serial.println("Start: Display konfiguriert.");




        tft.setCursor(0,8,1);
        // Textfarbe Weiss auf schwarzem Hintergrund.
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        // Startinformationen auf dem Display ausgeben.
        tft.println("---------------------");
        tft.println("WLAN wird verbunden:");
        tft.print("SSID: ");
        tft.println(ssid);
        tft.println("Passwort ist im Code hinterlegt.");
        tft.println("Bei falscher Stadt bitte WLAN und Standortprüfung kontrollieren.");
        tft.println("---------------------");
          

          TJpgDec.setJpgScale(1);
          TJpgDec.setSwapBytes(true);
          TJpgDec.setCallback(tft_output);

          targetTime = millis() + 1000;
          get_wifi();
          Serial.print("Verbinde WLAN ");
          // Serial.println(ssid);
          //WiFi.begin(ssid,password);

          TJpgDec.setJpgScale(1);
          TJpgDec.setSwapBytes(true);
          TJpgDec.setCallback(tft_output);

          while (WiFi.status() != WL_CONNECTED)
          {
            loading(70);

            if(loadNum>=94)
            {
              // SmartConfig();
              break;
            }
          }
          tft.fillScreen(TFT_BLACK);
          while(loadNum < 94) // Ladeanimation vollstaendig anzeigen.
          {
            loading(1);
          }
          loading(1);
        
          Udp.begin(localPort);

          setSyncProvider(getNtpTime);
          setSyncInterval(setNTPSyncTime*60); // NTP-Synchronisationsintervall in Sekunden.


          // Hauptansicht zeichnen.

          tft.fillScreen(0x0000);
          tft.fillRoundRect(0,0,128,128,0,bgColor);// Gefuelltes Rechteck.


          // Rahmenlinien zeichnen.
          tft.drawFastHLine(0,0,128,xkColor);


          tft.drawFastHLine(0,18,128,xkColor); // Horizontale Trennlinie ueber die volle Breite.
          tft.drawFastHLine(0,106,128,xkColor);

          // tft.drawFastVLine(80,0,18,xkColor);

          tft.drawFastHLine(0,88,128,xkColor);

          // tft.drawFastVLine(32,88,18,xkColor);
          tft.drawFastVLine(78,88,18,xkColor);

          tft.drawFastVLine(40,106,20,xkColor);
          tft.drawFastHLine(0,127,128,xkColor);
          
          getCityWeater();
          
}

unsigned long weaterTime = 0;
void loop() {
                handleDeepSleepButton();
  
                if (now() != prevDisplay) {
                prevDisplay = now();
                digitalClockDisplay();
              }

             if(millis() - weaterTime > 300000){ // Wetterdaten alle 5 Minuten aktualisieren.
                weaterTime = millis();
                getCityWeater();
                // get_Bstation_follow();
                // fanspush();
              }
    digitalClockDisplay();
     scrollBanner();
     weatherWarning();
    delay(1);
    //  Serial_set();
  }


// Uhranzeige --------------------------------------------------------------------------


void digitalClockDisplay()
{

  clk.setColorDepth(8);

  /*** Mittlerer Uhrbereich ***/
  // Stunden und Minuten
  clk.createSprite(75, 28);
  clk.fillSprite(bgColor);
  clk.loadFont(FxLED_32);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(timehmfontColor, bgColor);
  clk.drawString(hourMinute(),1,14,7); // Stunden und Minuten zeichnen.
  clk.unloadFont();
  clk.pushSprite(10,19);
  clk.deleteSprite();

  // Sekunden
  clk.createSprite(50, 28);
  clk.fillSprite(bgColor);

  clk.loadFont(FxLED_32);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(timesfontColor, bgColor);
  clk.drawString(":"+num2str(second()),1,14);

  clk.unloadFont();
  clk.pushSprite(86,19);
  clk.deleteSprite();
  /*** Mittlerer Uhrbereich ***/

  /*** Unterer Anzeigebereich ***/
  clk.loadFont(ALBB10);
  clk.createSprite(70, 16);
  clk.fillSprite(bgColor);

  // Wochentag
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(weekfontColor, bgColor);
  clk.drawString(week(),1,8);
  clk.pushSprite(45,108); // Position der Schrift.
  clk.deleteSprite();

  // Monat und Tag
  clk.createSprite(30,16);// Groesse des Sprite-Bereichs.
  clk.fillSprite(bgColor);
  clk.setTextDatum(ML_DATUM);
  clk.setTextColor(monthfontColor, bgColor);
  clk.drawString(monthDay(),1,8);
  clk.pushSprite(1,108);
  clk.deleteSprite();

  clk.unloadFont();
  /*** Unterer Anzeigebereich ***/

}

// Wochentag
String week(){
  String wk[7] = {"Sonntag","Montag","Dienstag","Mittwoch","Donnerstag","Freitag","Samstag"};
  String s = wk[weekday()-1];
  return s;
}

// Monat und Tag
String monthDay(){
  String s = String(month());
  s = s + " - " + day();
  return s;
}
// Stunden und Minuten
String hourMinute(){
  String s = num2str(hour());
  backLight_hour = s.toInt();
  s = s + ":" + num2str(minute());
  return s;
}

String num2str(int digits)
{
  String s = "";
  delay(9); // Kleine Verzoegerung fuer die Display-Aktualisierung.
  if (digits < 10)
    s = s + "0";
  s = s + digits;
  return s;
}

void printDigits(int digits)
{
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
//------------------------------------------------------------------------------------





// NTP-Funktionen ------------------------------------------------------------

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP-Zeitinformationen liegen in den ersten 48 Bytes.
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  getCityTime();
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  //Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  //Serial.print(ntpServerName);
  //Serial.print(": ");
  //Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("NTP-Synchronisation erfolgreich");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      //Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  //ESP.restart(); // Optionaler Neustart bei fehlgeschlagener Zeitsynchronisation.
  Serial.println("NTP-Synchronisation fehlgeschlagen");
  return 0; // Bei fehlgeschlagener Zeitsynchronisation 0 zurueckgeben.
}

// Anfrage an den NTP-Server senden.
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision: Zeitgenauigkeit.
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
//--------------------------------------------------------------------------

