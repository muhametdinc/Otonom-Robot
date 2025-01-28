#include "WiFi.h"
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <HardwareSerial.h>

HardwareSerial espSerial(2);


// MySQL Bilgileri

// MySQL sunucusu bilgileri
char server[] = "31.192.214.3"; // MySQL sunucu IP'si
char user[] = "kazmgp_arduino";   // MySQL kullanıcı adı
char passwordSQL[] = "kazmgp_arduino";    // MySQL şifresi

WiFiClient client;
MySQL_Connection conn((Client *)&client);

const char* ssid = "OppoA74";
const char* password = "12345678mda";


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  espSerial.begin(115200, SERIAL_8N1, 16, 17); // TX = 17, RX = 16

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("WiFi'ya baglaniliyor");
  }

  Serial.println("WiFi'ya baglandi");
  Serial.print("ESP IP'si: ");
  Serial.println(WiFi.localIP());



  
  

}

unsigned long previousMillis1 = 0;  // İlk döngü için zamanlayıcı
unsigned long previousMillis2 = 0;  // İkinci döngü için zamanlayıcı

const long interval1 = 1000;  // İlk döngü aralığı (1 saniye)
const long interval2 = 5000; 

String yon="";

void loop() {
  // put your main code here, to run repeatedly:
  // Arduino'ya veri gönder

 // İlk döngü için zaman kontrolü
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis;
    firstLoopTask();  // İlk döngüde yapılacak iş
  }

  // İkinci döngü için zaman kontrolü
  if (currentMillis - previousMillis2 >= interval2) {
    previousMillis2 = currentMillis;
    secondLoopTask();  // İkinci döngüde yapılacak iş
  }




  
 // delay(500);

   

}

// İlk döngüde yapılacak iş
void firstLoopTask() {
  Serial.println(yon);
  espSerial.println(yon);
}

// İkinci döngüde yapılacak iş
void secondLoopTask() {
  Serial.println("Veritabanına bağlanılıyor...");
  
  IPAddress server_addr;
  if(!server_addr.fromString(server)) {
    Serial.println("IP adresi dönüştürme hatası!");
    return;
  }
  
  if (conn.connect(server_addr, 3306, user, passwordSQL, "kazmgp_arduino")) {
    Serial.println("Veritabanına bağlandı!");
    
    MySQL_Cursor* cursor = new MySQL_Cursor(&conn);
    
    // Önce sorguyu çalıştır
    cursor->execute("SELECT TEST FROM kazmgp_arduino.TBL_TEST LIMIT 1");
    
    // Sütunları oku
    column_names *columns = cursor->get_columns();
    
    // Sonra satırları oku
    row_values *row = cursor->get_next_row();
    if (row != NULL) {
      String testValue = row->values[0];
      if(testValue.length() == 9) {
        yon = testValue;
        Serial.print("Çekilen veri: ");
        Serial.println(yon);
      }
    } else {
      Serial.println("Veri çekilemedi.");
    }
    
    delete cursor;
    conn.close();
  } else {
    Serial.println("MySQL bağlantı hatası!");
  }
}
