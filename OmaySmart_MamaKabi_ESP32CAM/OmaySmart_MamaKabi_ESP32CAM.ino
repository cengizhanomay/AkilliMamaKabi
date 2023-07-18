#include <WiFi.h>
#include <esp_camera.h>
#include <Arduino.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include <driver/rtc_io.h>
#include <FS.h>
#include <LITTLEFS.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <ArduinoJson.h>
#include <NTPtimeESP.h>
#include <virtuabotixRTC.h>
#include <List.hpp>
#include <ESP32Servo.h>
#include <HTTPClient.h>
#include <UUID.h>
#include <WebServer.h>



#define FORMAT_LITTLEFS_IF_FAILED true
#define API_KEY "*"
#define FILE_PHOTO "/images.jpg"
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define CLK_PIN 14
#define DATA_PIN 13
#define RST_PIN 15
#define SERVO_PIN 12
#define LED_PIN 4
#define BUTTON_PIN 2





StaticJsonDocument<1536> SJDoc;
List<int> ListSaat;
List<int> ListDakika;
List<int> ListPorsiyon;
Servo ServoMotor;
UUID uuid;
WebServer server(80);




NTPtime NTPch("pool.ntp.org");
strDateTime DATETime;
virtuabotixRTC SaatModule(CLK_PIN, DATA_PIN, RST_PIN);
FirebaseAuth authF;
FirebaseConfig configF;
FirebaseData fbdoF;
FirebaseJson jValF;
HTTPClient httpClient;



byte ActualHour;
byte ActualMinute;
byte ActualSecond;
int ActualYear;
byte ActualMonth;
byte ActualDay;
byte ActualDayOfWeek;
byte DayOfWeek;
String gunler[7] = {"PAZARTESI", "SALI", "CARSAMBA", "PERSEMBE", "CUMA", "CUMARTESI", "PAZAR"};
boolean FirebaseFotoCek = false;
String WIFI_SSID = "Default";
String WIFI_PASSWORD = "Default";
const char* USER_EMAIL = "*";
const char* USER_PASSWORD = "*";
const char* STORAGE_BUCKET_ID = "*";
const char* DATABASE_URL = "*";
String DEVICE_ID = "Default";
String IP_URL = "";
boolean YeniFotografCek = true;
String FotografUrl;
boolean FotografGonderme = false;
boolean ButtonDurum = false;
int ButtonSayac = 0;
int BaglantiDeneme = 0;
boolean BaglantiDurum = false;
boolean EslestirmeModKontrol = false;
boolean MamaVerKontrol = false;
unsigned long SimdikiZaman = 0;
unsigned long OncekiZaman = 0;
int Aralik = 600000;
unsigned long LedOncekiZaman = 0;
int LedAralik = 1000;
unsigned long MamaOncekiZaman = 0;
int MamaAralik = 90000;
unsigned long FirebaseOncekiZaman = 0;
int FirebaseAralik = 3000;
boolean LedDurumReset = false;
boolean FotografCekKontrol = false;
boolean AyarYapildiKontrol = false;
String UrlKontrol = "";
boolean IlkCalisma = false;
String st;
String content;
int statusCode;
boolean ServerSsid = false;
boolean ServerPass = false;
boolean ServerCihazid = false;
boolean CihazIdKontrol = false;
boolean SaatVarKontrol = false;
boolean DakikaVarKontrol = false;
boolean PorsiyonVarKontrol = false;
boolean MamaVerildiKontrol = false;
int Porsiyon = 0;
int PorsiyonDelay = 1000;




void WifiBaglan()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  const char* SSID_WIFI = WIFI_SSID.c_str();
  const char* PASSWORD_WIFI = WIFI_PASSWORD.c_str();
  WiFi.begin(SSID_WIFI, PASSWORD_WIFI);
  Serial.print("Ağ Bağlantısı Oluşturuluyor");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
    BaglantiDeneme += 1;
    if (BaglantiDeneme == 15)
    {
      BaglantiDeneme = 0;
      break;
    }
  }
  Serial.println();
  Serial.print("IP Adresine Bağlanıldı: ");
  Serial.println(WiFi.localIP());
  Serial.print("Sinyal Seviyesi: ");
  Serial.println(WiFi.RSSI());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void FirebaseBaglan()
{
  configF.api_key = API_KEY;
  configF.database_url = DATABASE_URL;
  configF.token_status_callback = tokenStatusCallback;
  configF.fcs.upload_buffer_size = 512;
  configF.max_token_generation_retry = 5;
  configF.timeout.serverResponse = 10 * 1000;
  authF.user.email = USER_EMAIL;
  authF.user.password = USER_PASSWORD;
  fbdoF.setResponseSize(8192);
  Firebase.begin(&configF, &authF);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase Bağlandı");
  Serial.print("Firebase Client: ");
  Serial.println(FIREBASE_CLIENT_VERSION);
  String uid = authF.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
}

void InternetKontrol()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    httpClient.begin(IP_URL);
    int HTTPCode = httpClient.GET();
    if (HTTPCode > 0)
    {
      BaglantiDurum = true;
      Serial.println("İnternet Bağlantısı Var");
    }
    else
    {
      BaglantiDurum = false;
      Serial.println("İnternet Bağlantısı Yok");
    }
    httpClient.end();
  }
  else
  {
    BaglantiDurum = false;
    Serial.println("Ağ Bağlantısı Yok");
  }
}

void FirebaseHataKontrol()
{
  Serial.println(fbdoF.errorReason());
  BaglantiDurum = false;
  InternetKontrol();
}

void DosyaSistemiBaglan()
{
  if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    Serial.println("LITTLEFS dosya sistemi açılırken hata oluştu.");
    return;
  }
  else
  {
    delay(500);
    Serial.println("LITTLEFS dosya sistemi açıldı.");
  }
}

static void IRAM_ATTR ButtonBasma(void * arg)
{
  if (!ButtonDurum)
  {
    ButtonSayac = SimdikiZaman;
    ButtonDurum = true;
  }
}

void KameraBaglan()
{
  camera_config_t KameraConfig;
  KameraConfig.ledc_channel = LEDC_CHANNEL_0;
  KameraConfig.ledc_timer = LEDC_TIMER_0;
  KameraConfig.pin_d0 = Y2_GPIO_NUM;
  KameraConfig.pin_d1 = Y3_GPIO_NUM;
  KameraConfig.pin_d2 = Y4_GPIO_NUM;
  KameraConfig.pin_d3 = Y5_GPIO_NUM;
  KameraConfig.pin_d4 = Y6_GPIO_NUM;
  KameraConfig.pin_d5 = Y7_GPIO_NUM;
  KameraConfig.pin_d6 = Y8_GPIO_NUM;
  KameraConfig.pin_d7 = Y9_GPIO_NUM;
  KameraConfig.pin_xclk = XCLK_GPIO_NUM;
  KameraConfig.pin_pclk = PCLK_GPIO_NUM;
  KameraConfig.pin_vsync = VSYNC_GPIO_NUM;
  KameraConfig.pin_href = HREF_GPIO_NUM;
  KameraConfig.pin_sscb_sda = SIOD_GPIO_NUM;
  KameraConfig.pin_sscb_scl = SIOC_GPIO_NUM;
  KameraConfig.pin_pwdn = PWDN_GPIO_NUM;
  KameraConfig.pin_reset = RESET_GPIO_NUM;
  KameraConfig.xclk_freq_hz = 20000000;
  KameraConfig.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    KameraConfig.frame_size = FRAMESIZE_UXGA;
    KameraConfig.jpeg_quality = 10;
    KameraConfig.fb_count = 2;
  }
  else
  {
    KameraConfig.frame_size = FRAMESIZE_SVGA;
    KameraConfig.jpeg_quality = 12;
    KameraConfig.fb_count = 1;
  }
  
  esp_err_t err = esp_camera_init(&KameraConfig);
  if (err != ESP_OK)
  {
    Serial.print("esp_camera_init error 0x");
    Serial.println(err);
    ESP.restart();
  }
  
  err = gpio_isr_handler_add(GPIO_NUM_2, &ButtonBasma, (void *) 2);
  if (err != ESP_OK)
  {
    Serial.print("gpio_isr_handler_add error 0x");
    Serial.println(err);
    ESP.restart();
  }
  
  err = gpio_set_intr_type(GPIO_NUM_2, GPIO_INTR_POSEDGE);
  if (err != ESP_OK)
  {
    Serial.print("gpio_set_intr_type error 0x");
    Serial.println(err);
    ESP.restart();
  }
  
  if (err == ESP_OK) {
  Serial.println("Kamera başlatıldı.");
  }
}

void MamaVerildi()
{
  if(SimdikiZaman - MamaOncekiZaman >= MamaAralik)
  {
    MamaOncekiZaman = SimdikiZaman;
    MamaVerildiKontrol = false;
  }
}

void MamaVer()
{
  Serial.println("MAMA VER VOID");
  MamaOncekiZaman = SimdikiZaman;
  MamaVerildiKontrol = true;

  PorsiyonDelay = 1000;
  if (Porsiyon > 0)
  {
    Serial.print("Porsiyon: ");
    Serial.println(Porsiyon);
    switch (Porsiyon)
    {
      case 1:
      {
        PorsiyonDelay = 1000;
        break;
      }
      case 2:
      {
        PorsiyonDelay = 2000;
        break;
      }
      case 3:
      {
        PorsiyonDelay = 3000;
        break;
      }
      case 4:
      {
        PorsiyonDelay = 4000;
        break;
      }
      case 5:
      {
        PorsiyonDelay = 5000;
        break;
      }
      case 6:
      {
        PorsiyonDelay = 6000;
        break;
      }
      case 7:
      {
        PorsiyonDelay = 7000;
        break;
      }
      case 8:
      {
        PorsiyonDelay = 8000;
        break;
      }
      case 9:
      {
        PorsiyonDelay = 9000;
        break;
      }
      default:
      {
        PorsiyonDelay = 1000;
        break;
      }
    }
    Porsiyon = 0;
  }

  ServoMotor.write(60);
  delay(PorsiyonDelay);

  if(ServoMotor.read() == 60)
  {
    ServoMotor.write(0);
  }
}

void EslestirmeLed()
{
  if(SimdikiZaman - LedOncekiZaman >= LedAralik)
  {
    LedOncekiZaman = SimdikiZaman;
    if(LedDurumReset == true)
    {
      digitalWrite(LED_PIN, LOW);
      LedDurumReset = false;
    }
    else
    {
      digitalWrite(LED_PIN, HIGH);
      LedDurumReset = true;
    }
  }
}

void EslestirmeMod()
{
  Serial.println("ESLESTIRME MOD VOID");
  setupAP();
}

void HardResetMod()
{
  Serial.println("HARD RESET MOD VOID");
  if(LITTLEFS.format())
  {
    Serial.println("Dosya sistemi formatlandı.");
    ESP.restart();
  }
  else
  {
    Serial.println("Dosya sistemi formatlanırken hata oluştu.");
  }
}

void SaatSenkron()
{
  if (BaglantiDurum == true)
  {
    do
    {
      DATETime = NTPch.getNTPtime(3.0, 0);
    }
    while (!DATETime.valid);
    
    if (DATETime.valid)
    {
      ActualHour = DATETime.hour;
      ActualMinute = DATETime.minute;
      ActualSecond = DATETime.second;
      ActualYear = DATETime.year;
      ActualMonth = DATETime.month;
      ActualDay = DATETime.day;
      ActualDayOfWeek = DATETime.dayofWeek;
    }
    
    switch(ActualDayOfWeek)
    {
      case 1:
        DayOfWeek = 7;
        break;
      case 2:
        DayOfWeek = 1;
        break;
      case 3:
        DayOfWeek = 2;
        break;
      case 4:
        DayOfWeek = 3;
        break;
      case 5:
        DayOfWeek = 4;
        break;
      case 6:
        DayOfWeek = 5;
        break;
      case 7:
        DayOfWeek = 6;
        break;
      default:
        DayOfWeek = 1;
    }
    
    Serial.print("Saat: ");
    Serial.print(ActualHour);
    Serial.print(":");
    Serial.print(ActualMinute);
    Serial.print(":");
    Serial.print(ActualSecond);
    Serial.println();
    Serial.print("Tarih: ");
    Serial.print(ActualDay);
    Serial.print("/");
    Serial.print(ActualMonth);
    Serial.print("/");
    Serial.print(ActualYear);
    Serial.println();
    Serial.print("Haftanın Günü (online): ");
    Serial.print(ActualDayOfWeek);
    Serial.println();
    Serial.print("Haftanın Günü: ");
    Serial.print(DayOfWeek);
    Serial.println();

    SaatModule.setDS1302Time(ActualSecond, ActualMinute, ActualHour, DayOfWeek, ActualDay, ActualMonth, ActualYear);
    
    Serial.println("Saat senkron edildi.");
  }
  else
  {
    SaatModule.updateTime();
    Serial.print("Sistem Saat ve Tarih: ");
    Serial.print(SaatModule.hours);
    Serial.print(":");
    Serial.print(SaatModule.minutes);
    Serial.print(":");
    Serial.print(SaatModule.seconds);
    Serial.print(" ");
    Serial.print(SaatModule.dayofmonth);
    Serial.print("/");
    Serial.print(SaatModule.month);
    Serial.print("/");
    Serial.print(SaatModule.year);
    Serial.print(" ");
    Serial.println(gunler[SaatModule.dayofweek - 1]);
  }
}

void MamaSaatiCek()
{
  if (BaglantiDurum == true)
  {
    if (Firebase.ready())
    {
      String MamaSureleriYolu = DEVICE_ID + "/mamasureleri";
      if (Firebase.RTDB.getJSON(&fbdoF, MamaSureleriYolu, &jValF))
      {
        DeserializationError jValError = deserializeJson(SJDoc, jValF.raw());
        
        if (jValError)
        {
          Serial.print("deserializeJson() failed (Mama süreleri çekme): ");
          Serial.println(jValError.c_str());
          return;
        }
        else
        {
          File FileSaatYaz = LITTLEFS.open("/saat.txt", "w+");
          File FileDakikaYaz = LITTLEFS.open("/dakika.txt", "w+");
          File FilePorsiyonYaz = LITTLEFS.open("/porsiyon.txt", "w+");

          for (JsonPair item : SJDoc.as<JsonObject>())
          {
            const char* item_key = item.key().c_str();
            int value_dakika = item.value()["dakika"];
            int value_porsiyon = item.value()["porsiyon"];
            int value_saat = item.value()["saat"];
            
            if(FileSaatYaz && FileDakikaYaz && FilePorsiyonYaz)
            {
              FileSaatYaz.println(value_saat);
              FileDakikaYaz.println(value_dakika);
              FilePorsiyonYaz.println(value_porsiyon);
            }
            else
            {
              Serial.println("Saat, Dakika ve Porsiyon yazma için açılamadı.");
            }
          }
          FileSaatYaz.close();
          FileDakikaYaz.close();
          FilePorsiyonYaz.close();
        }
      }
      else
      {
        Serial.print("Mama süreleri çekilemedi. Hata: ");
        FirebaseHataKontrol();
        if (fbdoF.errorReason() == "path not exist")
        {
          Serial.println("path_not_exist girdi");
          SaatYaz();
          DakikaYaz();
          PorsiyonYaz();
        }
      }
    }
  }

  ListSaat.clear();
  ListDakika.clear();
  ListPorsiyon.clear();
  File FileSaatOku = LITTLEFS.open("/saat.txt", "r");
  File FileDakikaOku = LITTLEFS.open("/dakika.txt", "r");
  File FilePorsiyonOku = LITTLEFS.open("/porsiyon.txt", "r");
    
  if(FileSaatOku && FileDakikaOku && FilePorsiyonOku)
  {
    while (FileSaatOku.available())
    {
      int gecicisaat = FileSaatOku.readStringUntil('\n').toInt();
      ListSaat.add(gecicisaat);
      SaatVarKontrol = true;
    }

    while (FileDakikaOku.available())
    {
      int gecicidakika = FileDakikaOku.readStringUntil('\n').toInt();
      ListDakika.add(gecicidakika);
      DakikaVarKontrol = true;
    }

    while (FilePorsiyonOku.available())
    {
      int geciciporsiyon = FilePorsiyonOku.readStringUntil('\n').toInt();
      ListPorsiyon.add(geciciporsiyon);
      PorsiyonVarKontrol = true;
    }
  }
  else
  {
    Serial.println("Saat, Dakika ve Porsiyon okuma için açılamadı.");
  }
  FileSaatOku.close();
  FileDakikaOku.close();
  FilePorsiyonOku.close();

  Serial.println("Mama saatleri çekildi.");
}

bool FotografKontrol() 
{
  File f_pic = LITTLEFS.open(FILE_PHOTO);
  int pic_sz = f_pic.size();
  f_pic.close();
  if (pic_sz > 100)
    return true;
  else
    return false;
}

void FotografCekKaydet(void)
{
  camera_fb_t * fb = NULL;
  bool FCKok = false;
  do
  {
    Serial.println("Fotoğraf çekiliyor...");

    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Fotoğraf çekimi başarısız oldu.");
      return;
    }
    
    Serial.print("Resim dosyası adı: ");
    Serial.println(FILE_PHOTO);
    File FileFotograf = LITTLEFS.open(FILE_PHOTO, FILE_WRITE);
    
    if (!FileFotograf)
    {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      FileFotograf.write(fb->buf, fb->len);
      Serial.print("Resim şuraya kaydedildi ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Boyut: ");
      Serial.print(FileFotograf.size());
      Serial.println(" bytes");
    }
    FileFotograf.close();
    esp_camera_fb_return(fb);
    FCKok = FotografKontrol();
  }
  while (!FCKok);
}

void FotografCek()
{
  FotografCekKontrol = false;
  if (YeniFotografCek)
  {
    FotografCekKaydet();
    YeniFotografCek = false;
  }
  delay(100);
  
  if (Firebase.ready() && !FotografGonderme)
  {
    FotografGonderme = true;
    Serial.println("Resim yükleniyor...");
    String StoragePath = "/" + DEVICE_ID + FILE_PHOTO;
    if (Firebase.Storage.upload(&fbdoF, STORAGE_BUCKET_ID, FILE_PHOTO, mem_storage_type_flash, StoragePath, "image/jpeg"))
    {
      FotografUrl = fbdoF.downloadURL().c_str();
      Serial.print("Fotograf yüklendi. URL: ");
      Serial.println(FotografUrl);
    }
    else
    {
      FotografUrl = "false";
      Serial.print("Fotograf yüklenemedi. Hata: ");
      FirebaseHataKontrol();
    }

    String FotografUrlYolu = DEVICE_ID + "/fotografurl";
    if (Firebase.RTDB.setString(&fbdoF, FotografUrlYolu, FotografUrl))
    {   
      Serial.println("Fotograf linki gönderildi.");
    }
    else
    {
      Serial.print("Fotograf linki gönderilemedi. Hata: ");
      FirebaseHataKontrol();
    }
  }
}

void AyarYapildi()
{
  AyarYapildiKontrol = false;
  Serial.println("Ayar yapıldı VOID");
  MamaSaatiCek();
}

void SaatYaz()
{
  File FileSaatYaz = LITTLEFS.open("/saat.txt", "w+");
  if(FileSaatYaz)
  {
    Serial.println("Saat metin belgesi oluşturuldu.");
  }
  else
  {
    Serial.println("Saat metin belgesi oluşturulamadı.");
  }
  FileSaatYaz.close();
}

void DakikaYaz()
{
  File FileDakikaYaz = LITTLEFS.open("/dakika.txt", "w+");
  if(FileDakikaYaz)
  {
    Serial.println("Dakika metin belgesi oluşturuldu.");
  }
  else
  {
    Serial.println("Dakika metin belgesi oluşturulamadı.");
  }
  FileDakikaYaz.close();
}

void PorsiyonYaz()
{
  File FilePorsiyonYaz = LITTLEFS.open("/porsiyon.txt", "w+");
  if(FilePorsiyonYaz)
  {
    Serial.println("Porsiyon metin belgesi oluşturuldu.");
  }
  else
  {
    Serial.println("Porsiyon metin belgesi oluşturulamadı.");
  }
  FilePorsiyonYaz.close();
}

void SSIDYaz()
{
  File FileSsidYaz = LITTLEFS.open("/ssid.txt", "w+");
  if(FileSsidYaz)
  {
    Serial.println("ssid metin belgesi oluşturuldu.");
  }
  else
  {
    Serial.println("ssid metin belgesi oluşturulamadı.");
  }
  FileSsidYaz.close();
}

void PasswordYaz()
{
  File FilePassYaz = LITTLEFS.open("/password.txt", "w+");
  if(FilePassYaz)
  {
    Serial.println("password metin belgesi oluşturuldu.");
  }
  else
  {
    Serial.println("password metin belgesi oluşturulamadı.");
  }
  FilePassYaz.close();
}

void CihazIDYaz()
{
  File FileCihazIdYaz = LITTLEFS.open("/cihazid.txt", "w+");
  if(FileCihazIdYaz)
  {
    Serial.println("cihazid metin belgesi oluşturuldu.");
  }
  else
  {
    Serial.println("cihazid metin belgesi oluşturulamadı.");
  }
  FileCihazIdYaz.close();
}

void UrlYaz()
{
  File FileUrlYaz = LITTLEFS.open("/urlkontrol.txt", "w+");
  if(FileUrlYaz)
  {
    FileUrlYaz.print("http://example.com/");
    Serial.println("Url yazıldı.");
  }
  else
  {
    Serial.println("Url kontrol yazma için açılamadı.");
    IP_URL = "http://example.com/";
  }
  FileUrlYaz.close();
}

void UrlCekYaz()
{
  if (BaglantiDurum == true)
  {
    if (Firebase.ready())
    {
      String UrlKontrolYolu = DEVICE_ID + "/urlkontrol";
      if(Firebase.RTDB.getString(&fbdoF, UrlKontrolYolu, &UrlKontrol))
      {
        if (UrlKontrol != "" && UrlKontrol != NULL)
        {
          File FileUrlYaz = LITTLEFS.open("/urlkontrol.txt", "w+");
          if(FileUrlYaz)
          {
            FileUrlYaz.println(UrlKontrol);
            IP_URL = UrlKontrol;
          }
          else
          {
            Serial.println("Url kontrol yazma için açılamadı.");
            IP_URL = "http://example.com/";
          }
          FileUrlYaz.close();
        }
      }
      else
      {
        Serial.print("URL kontrol bilgisi çekilemedi. Hata: ");
        IP_URL = "http://example.com/";
        FirebaseHataKontrol();
      }
    }
  }
}

void UrlOku()
{  
  File FileUrlOku = LITTLEFS.open("/urlkontrol.txt", "r");
    
  if(FileUrlOku)
  {
    while (FileUrlOku.available())
    {
      String geciciurl = FileUrlOku.readStringUntil('\n');
      if (geciciurl != "" && geciciurl != NULL)
      {
        IP_URL = geciciurl;
      }
      else
      {
        IP_URL = "http://example.com/";
      }
    }
  }
  else
  {
    Serial.println("Url kontrol okuma için açılamadı.");
    IP_URL = "http://example.com/";
  }
  FileUrlOku.close();
}

void SSIDOku()
{  
  File FileSsidOku = LITTLEFS.open("/ssid.txt", "r");
    
  if(FileSsidOku)
  {
    while (FileSsidOku.available())
    {
      String gecicissid = FileSsidOku.readStringUntil('\n');
      if (gecicissid != "" && gecicissid != NULL)
      {
        WIFI_SSID = gecicissid;
      }
      else
      {
        Serial.println("gecicissid boş");
      }
    }
  }
  else
  {
    Serial.println("ssid okuma için açılamadı.");
  }
  FileSsidOku.close();
}

void PasswordOku()
{  
  File FilePassOku = LITTLEFS.open("/password.txt", "r");
    
  if(FilePassOku)
  {
    while (FilePassOku.available())
    {
      String gecicipass = FilePassOku.readStringUntil('\n');
      if (gecicipass != "" && gecicipass != NULL)
      {
        WIFI_PASSWORD = gecicipass;
      }
      else
      {
        Serial.println("gecicipass boş");
      }
    }
  }
  else
  {
    Serial.println("password okuma için açılamadı.");
  }
  FilePassOku.close();
}

void CihazIDOku()
{  
  File FileCihazIdOku = LITTLEFS.open("/cihazid.txt", "r");
    
  if(FileCihazIdOku)
  {
    while (FileCihazIdOku.available())
    {
      String gecicicihazid = FileCihazIdOku.readStringUntil('\n');
      if (gecicicihazid != "" && gecicicihazid != NULL)
      {
        DEVICE_ID = gecicicihazid;
        CihazIdKontrol = true;
      }
      else
      {
        Serial.println("gecicicihazid boş");
      }
    }
  }
  else
  {
    Serial.println("cihazid okuma için açılamadı.");
  }
  FileCihazIdOku.close();
}

void IlkCalismaKontrol()
{
  File IlkCalismaOpen = LITTLEFS.open("/ilkcalisma");
  if (IlkCalismaOpen)
  {
    Serial.println("Daha önce çalışmış.");
    IlkCalisma = false;
  }
  else
  {
    Serial.println("İlk defa çalışıyor.");
    IlkCalisma = true;
  }
  IlkCalismaOpen.close();
}

void IlkCalismaOlustur()
{
  if(LITTLEFS.mkdir("/ilkcalisma"))
  {
    Serial.println("ilkcalisma klasörü oluşturuldu.");
  }
  else
  {
    Serial.println("ilkcalisma klasörü oluşturulamadı.");
  }
}

void IlkCalismaAyar()
{
  SaatYaz();
  DakikaYaz();
  PorsiyonYaz();
  SSIDYaz();
  PasswordYaz();
  CihazIDYaz();
  UrlYaz();
  IlkCalismaOlustur();
  EslestirmeOlustur();
}

void EslestirmeKontrol()
{
  File EslestirmeOpen = LITTLEFS.open("/eslestirme");
  if (EslestirmeOpen)
  {
    Serial.println("eslestirme klasör bulundu.");
    EslestirmeModKontrol = true;
  }
  else
  {
    Serial.println("eslestirme klasör bulunamadı.");
    EslestirmeModKontrol = false;
  }
  EslestirmeOpen.close();
}

void EslestirmeOlustur()
{
  if(LITTLEFS.mkdir("/eslestirme"))
  {
    Serial.println("eslestirme klasörü oluşturuldu.");
  }
  else
  {
    Serial.println("eslestirme klasörü oluşturulamadı.");
  }
}

void EslestirmeSil()
{
  if(LITTLEFS.rmdir("/eslestirme"))
  {
    Serial.println("eslestirme klasörü silindi.");
    ESP.restart();
  }
  else
  {
    Serial.println("eslestirme klasörü silinemedi.");
  }
}

void setupAP()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  uuid.generate();
  String key = uuid.toCharArray();
  String APSSID = "OmaySmart_Mama_" + key.substring(0, 8);
  const char* softAPSSID = APSSID.c_str();
  String APPASS = "OmaySmart";
  const char* softAPPASS = APPASS.c_str();
  delay(100);
  WiFi.softAP(softAPSSID, softAPPASS);
  Serial.print("Acces Point Success. SSID: ");
  Serial.print(softAPSSID);
  Serial.print(" PASSWORD: ");
  Serial.println(softAPPASS);
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  server.begin();
  Serial.println("Server started.");
}

void createWebServer()
{
  server.on("/", []()
  {
    content = "<!DOCTYPE HTML>\r\n<html>";
    content += "<head><title>OMAY SMART</title></head>";
    content += "<body></p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32></p><label>PASSWORD: </label><input name='pass' length=64></p><label>DEVICE ID: </label><input name='deviceid' length=64></p><input type='submit'></form></body>";
    content += "</html>";
    server.send(200, "text/html", content);
  });
    
  server.on("/setting", []()
  {
    String GeciciSsid = server.arg("ssid");
    String GeciciPass = server.arg("pass");
    String GeciciDeviceid = server.arg("deviceid");
    if (GeciciSsid.length() > 0 && GeciciPass.length() > 0 && GeciciDeviceid.length() > 0)
    {
      Serial.print("Server SSID: ");
      Serial.println(GeciciSsid);
      Serial.print("Server PASSWORD: ");
      Serial.println(GeciciPass);
      Serial.print("Server DEVICE ID: ");
      Serial.println(GeciciDeviceid);

      File FileServerSsidYaz = LITTLEFS.open("/ssid.txt", "w+");
      if(FileServerSsidYaz)
      {
        FileServerSsidYaz.print(GeciciSsid);
        Serial.println("ssid metin belgesi oluşturuldu.");
        ServerSsid = true;
      }
      else
      {
        Serial.println("ssid metin belgesi oluşturulamadı.");
      }
      FileServerSsidYaz.close();

      File FileServerPassYaz = LITTLEFS.open("/password.txt", "w+");
      if(FileServerPassYaz)
      {
        FileServerPassYaz.print(GeciciPass);
        Serial.println("password metin belgesi oluşturuldu.");
        ServerPass = true;
      }
      else
      {
        Serial.println("password metin belgesi oluşturulamadı.");
      }
      FileServerPassYaz.close();

      File FileServerCihazIdYaz = LITTLEFS.open("/cihazid.txt", "w+");
      if(FileServerCihazIdYaz)
      {
        FileServerCihazIdYaz.print(GeciciDeviceid);
        Serial.println("cihazid metin belgesi oluşturuldu.");
        ServerCihazid = true;
      }
      else
      {
        Serial.println("cihazid metin belgesi oluşturulamadı.");
      }
      FileServerCihazIdYaz.close();
          
      content = "{\"Success\":\"information is being saved...\"}";
      statusCode = 200;
    }
    else
    {
      content = "{\"Error\":\"404 not found\"}";
      statusCode = 404;
      Serial.println("Sending 404");
    }
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(statusCode, "application/json", content);
    });
}

void setup()
{
  Serial.begin(115200);
  DosyaSistemiBaglan();
  IlkCalismaKontrol();
  EslestirmeKontrol();
  
  if (IlkCalisma == true)
  {
    IlkCalismaAyar();
    ESP.restart();
  }
  else if (EslestirmeModKontrol == true)
  {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    EslestirmeMod();
  }
  else
  {
    SSIDOku();
    PasswordOku();
    CihazIDOku();
    WifiBaglan();
    UrlOku();
    InternetKontrol();
    pinMode(BUTTON_PIN, INPUT);
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    KameraBaglan();
    if (BaglantiDurum == true && CihazIdKontrol == true)
    {
      FirebaseBaglan();
    }
    SaatSenkron();
    MamaSaatiCek();
    UrlCekYaz();
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    ServoMotor.attach(SERVO_PIN);
    ServoMotor.write(0);
  }
}

void loop()
{
  SimdikiZaman = millis();
  if (IlkCalisma != true && EslestirmeModKontrol != true)
  {
    HataDurumu:
    if(SimdikiZaman - OncekiZaman >= Aralik)
    {
      OncekiZaman = SimdikiZaman;
      InternetKontrol();
      SaatSenkron();
      MamaSaatiCek();
      UrlCekYaz();
    }
  
    if (ButtonDurum)
    {
      if (digitalRead(BUTTON_PIN) == LOW)
      {
        ButtonDurum = false;
        if ((SimdikiZaman - ButtonSayac) >= 50)
        {
          if ((SimdikiZaman - ButtonSayac) <= 3000)
          {
            Serial.println("3 saniyeden az bir basma oldu ve mama verilecek.");
            MamaVer();
          }
          else if ((SimdikiZaman - ButtonSayac) >= 5000 && (SimdikiZaman - ButtonSayac) <= 10000)
          {
            Serial.println("8 ve 15 saniye aralığında basıldı ve eşleştirme modu açıldı.");
            EslestirmeOlustur();
            ESP.restart();
          }
          else if ((SimdikiZaman - ButtonSayac) >= 15000 && (SimdikiZaman - ButtonSayac) <= 25000)
          {
            Serial.println("15 ve 25 saniye aralığında basıldı ve hard reset modu açıldı.");
            HardResetMod();
          }
        }
      }
    }
    
    if (BaglantiDurum == true)
    {
      digitalWrite(LED_PIN, HIGH);
  
      if(SimdikiZaman - FirebaseOncekiZaman >= FirebaseAralik)
      {
        FirebaseOncekiZaman = SimdikiZaman;
        if (Firebase.ready())
        {
          String MamaVerYolu = DEVICE_ID + "/mamaver";
          if(Firebase.RTDB.getBool(&fbdoF, MamaVerYolu, &MamaVerKontrol))
          {
            if(MamaVerKontrol == true)
            {
              Serial.print("Mama ver bilgisi geldi: ");
              Serial.println(MamaVerKontrol);
              MamaVer();
              
              if (Firebase.RTDB.setBool(&fbdoF, MamaVerYolu, false))
              {
                Serial.println("Mama ver 'false' olarak gönderildi.");
              }
              else
              {
                Serial.print("Mama ver 'false' gönderilemedi. Hata: ");
                FirebaseHataKontrol();
                goto HataDurumu;
              }
            }
          }
          else
          {
            Serial.print("Mama ver bilgisi çekilemedi. Hata: ");
            FirebaseHataKontrol();
            goto HataDurumu;
          }

          String FotografCekYolu = DEVICE_ID + "/fotografcek";
          if(Firebase.RTDB.getBool(&fbdoF, FotografCekYolu, &FotografCekKontrol))
          {
            if(FotografCekKontrol == true)
            {
              Serial.print("Fotoğraf çekme bilgisi geldi: ");
              Serial.println(FotografCekKontrol);
              FotografGonderme = false;
              YeniFotografCek = true;
              FotografCek();

              if (Firebase.RTDB.setBool(&fbdoF, FotografCekYolu, false))
              {
                Serial.println("Fotoğraf çekme 'false' olarak gönderildi.");
              }
              else
              {
                Serial.print("Fotograf çekme 'false' gönderilemedi. Hata: ");
                FirebaseHataKontrol();
                goto HataDurumu;
              }
            }
          }
          else
          {
            Serial.print("Fotoğraf çekme bilgisi çekilemedi. Hata: ");
            FirebaseHataKontrol();
            goto HataDurumu;
          }

          String AyarYapildiYolu = DEVICE_ID + "/ayaryapildi";
          if(Firebase.RTDB.getBool(&fbdoF, AyarYapildiYolu, &AyarYapildiKontrol))
          {
            if(AyarYapildiKontrol == true)
            {
              Serial.print("Ayar yapıldı bilgisi geldi: ");
              Serial.println(AyarYapildiKontrol);
              AyarYapildi();
    
              if (Firebase.RTDB.setBool(&fbdoF, AyarYapildiYolu, false))
              {
                Serial.println("Ayar yapıldı 'false' olarak gönderildi.");
              }
              else
              {
                Serial.print("Ayar yapıldı 'false' gönderilemedi. Hata: ");
                FirebaseHataKontrol();
                goto HataDurumu;
              }
            }
          }
          else
          {
            Serial.print("Ayar yapıldı bilgisi çekilemedi. Hata: ");
            FirebaseHataKontrol();
            goto HataDurumu;
          }
          
        }
      }
    }
    else
    {
      digitalWrite(LED_PIN, LOW);
    }

    if (SaatVarKontrol == true && DakikaVarKontrol == true && PorsiyonVarKontrol == true && ListSaat.getSize() > 0 && ListDakika.getSize() > 0 && ListPorsiyon.getSize() > 0)
    {
      SaatModule.updateTime();
      for(int i = 0; i < ListSaat.getSize(); i++)
      {
        if (ListSaat.getValue(i) == SaatModule.hours)
        {
          if (ListDakika.getValue(i) == SaatModule.minutes)
          {
            MamaVerildi();
            if (MamaVerildiKontrol == false)
            {
              Serial.println("Saat ve Dakika uyuştu.");
              Porsiyon = ListPorsiyon.getValue(i);
              MamaVer();
            }
          }
        }
      }
    }
  }
  else if (EslestirmeModKontrol == true)
  {
    EslestirmeLed();
    Serial.println("Server dinleniyor...");
    server.handleClient();
    if (ServerSsid == true && ServerPass == true && ServerCihazid == true)
    {
      Serial.println("Server dinleme başarılı.");
      EslestirmeSil();
      ESP.restart();
    }
  }

  delay(1000);
}
