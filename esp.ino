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

const char* ssid = "NetMASTER Uydunet-59B9";
const char* password = "46282ce4fa8d380f";

String yon="";

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

// MySQL'e bağlan
  if (conn.connect(server, 3306, user, passwordSQL)) {
    Serial.println("MySQL bağlantısı başarılı!");

    // Veri ekleme sorgusu
    MySQL_Cursor* cursor = new MySQL_Cursor(&conn);
    //cursor->execute("INSERT INTO kazmgp_arduino.TBL_TEST (TEST) VALUES ('Arduino')");
    
     cursor->execute("SELECT TEST FROM kazmgp_arduino.TBL_TEST LIMIT 1");

    // Sonuçları çekmek için döngü
   // Kolonları okuma
    column_names *columns = cursor->get_columns();
    
    // Sonuçları çekmek için döngü
    row_values *row = cursor->get_next_row();
    if (row != NULL) {
      // Çekilen veriyi yazdırma
      String testValue = row->values[0];  // İlk kolonun değeri
      Serial.print("Cekilen veri: ");
      Serial.println(testValue);
      yon=testValue;
    } else {
      Serial.println("Veri çekilemedi.");
    }
    
    delete cursor;

  } else {
    Serial.println("MySQL bağlantısı başarısız!");
  }

  
  

}

void loop() {
  // put your main code here, to run repeatedly:
  // Arduino'ya veri gönder
  espSerial.println(yon);
  Serial.println("Gonderilen veri:");
  Serial.println(yon);
  delay(1000);

   

}
