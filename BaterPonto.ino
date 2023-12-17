#include <Arduino.h>
#include <time.h>
#include <SPI.h>
#include <MFRC522.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

const char* ssid = "NOME_REDE";
const char* password = "SENHA_REDE";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;

#define SS_PIN D8
#define RST_PIN D0

MFRC522 rfid(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;

String GOOGLE_SCRIPT_ID = "ID_PLANILHA_GOOGLE";

String ident(byte *buffer, byte bufferSize) {
  String content= "";
  for (byte i = 0; i < rfid.uid.size; i++) {
     content.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  String output = content.substring(1);
  output.toUpperCase();
  return output;
}

void post_info(String user_id){
  if ((WiFi.status() == WL_CONNECTED)) {
    time_t now = time(nullptr);
    time_t rawtime;
    struct tm *timeinfo;
    timeinfo = gmtime(&now);

    timeinfo->tm_hour -= 3;

    rawtime = mktime(timeinfo);
    
    timeinfo = localtime(&rawtime);
    
    char datetime[40];
    const char* sp = "%20";
    const char* sl = "%2F";
    const char* cl = "%3A";

    int dia = timeinfo->tm_mday;
    int mes = timeinfo->tm_mon + 1;
    int ano = timeinfo->tm_year + 1900;
    int hora = timeinfo->tm_hour;
    int minuto = timeinfo->tm_min;
    int segundo = timeinfo->tm_sec;

    sprintf(datetime, "%02d%s%02d%s%d%s%d%s%d%s%d", dia, sl, mes, sl, ano, sp, hora, cl, minuto, cl, segundo);

    user_id.replace(" ", "%20");
    
    String urlFinal = "https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?"+"date=" + datetime + "&user=" + user_id;
    Serial.println("Enviando dados para planilha:");
    
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    // Ignore SSL certificate validation
    client->setInsecure();
    
    //create an HTTPClient instance
    HTTPClient https;
    
    Serial.print("[HTTP] iniciando...\n");
    
    if (https.begin(*client, urlFinal)) {  // HTTP
      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          //Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] GET... falhou, erro: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.println("[HTTP] Conexão mal-sucedida");
    }
  }
  else{
   Serial.println("Erro de rede");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  Serial.println("Conectando Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  
  Serial.println("");
  Serial.println("WiFi conectada");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());

  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Sincronizando servidor NTP: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  time_t rawtime;
  struct tm *timeinfo;
  timeinfo = gmtime(&now);

  timeinfo->tm_hour -= 3;

  rawtime = mktime(timeinfo);
   
  timeinfo = localtime(&rawtime);
  char buffer[50];
  strftime(buffer, 50, "%d/%m/%Y %H:%M:S", timeinfo);
  
  Serial.print("Data e hora: ");
  Serial.println(buffer);
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  SPI.begin();
  rfid.PCD_Init();
  Serial.println();
  Serial.print(F("Leitor RFID :"));
  rfid.PCD_DumpVersionToSerial();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println();
}

void loop() {
    String key = "CARTEIRINHA_1";
    
    if (!rfid.PICC_IsNewCardPresent())
      return;
  
    if (!rfid.PICC_ReadCardSerial())
      return;
    
    String content = ident(rfid.uid.uidByte, rfid.uid.size);
    
    Serial.print(F("Identificação: "));
    Serial.println(content);

    if (content == key){
      Serial.println("Acesso autorizado!");
      post_info(content);
    }
    else {
      Serial.println("Acesso negado.");
    }
    Serial.println();
    
    delay(1500);

    rfid.PICC_HaltA();

    rfid.PCD_StopCrypto1();
}
