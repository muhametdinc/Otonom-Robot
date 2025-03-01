#include <SPI.h>
#include <MFRC522.h>

// RFID pin tanımlamaları (Arduino Mega)
#define SS_PIN 53    // SDA pini
#define RST_PIN 49   // RST pini
// MOSI -> 51, MISO -> 50, SCK -> 52

// Motor pin tanımlamaları (Arduino Mega PWM pinleri)
// Sağ Motor Sürücü (BTS7960)
const int motorSagRPWM = 44;  // Sağ motor ileri PWM
const int motorSagLPWM = 45;  // Sağ motor geri PWM
const int motorSagREN = 22;   // Sağ motor ileri enable
const int motorSagLEN = 23;   // Sağ motor geri enable

// Sol Motor Sürücü (BTS7960)
const int motorSolRPWM = 46;  // Sol motor ileri PWM
const int motorSolLPWM = 4;   // Sol motor geri PWM
const int motorSolREN = 24;   // Sol motor ileri enable
const int motorSolLEN = 25;   // Sol motor geri enable

// QTR Sensör pin tanımlamaları (Arduino Mega analog pinleri)
const int sensorSayisi = 8;
int qtrPin[8] = {A8, A9, A10, A11, A12, A13, A14, A15};  // Analog pinler

// Ultrasonik sensör pinleri (Arduino Mega digital pinleri)
const int uTrig = 30;  // Digital
const int uEcho = 31;  // Digital

// PID parametreleri - İyileştirilmiş
float Kp = 0.3;
float Ki = 0.01;  // Küçük bir integral etkisi eklendi
float Kd = 0.25;
float Kp_yavas = 0.4;
float Ki_yavas = 0.015;  // Yavaş hareket için integral
float Kd_yavas = 0.3;
float Kp_keskin = 0.5;
float Ki_keskin = 0.02;  // Keskin dönüş için integral
float Kd_keskin = 0.35;
float oncekiHata = 0;
float integral = 0;
unsigned long sonPIDZamani = 0;

// Motor hız parametreleri
const int DUZGIDIS_HIZ = 150;    // Düz gidiş hızı
const int YAVAS_DONUS = 120;     // Yavaş dönüş hızı
const int KESKIN_DONUS = 100;    // Keskin dönüş hızı
const int ARAMA_HIZI = 80;       // Çizgi arama hızı

// QTR Sensör kalibrasyon değişkenleri
int minimumDeger[8] = {1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023};
int maksimumDeger[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int esikDeger[8] = {500, 500, 500, 500, 500, 500, 500, 500};

// Başlangıç istasyonu ve konum değişkenleri
int mevcutKonum = 0;  // Robot giriş istasyonunda (0) başlayacak
bool robotIleriYonde = true;

// RFID nesnesi oluştur
MFRC522 rfid(SS_PIN, RST_PIN);

// RFID kart ID'leri
String girisIstasyonu_ID = "";  // Giriş istasyonu (0) için RFID kart ID'si
String istasyon1_ID = "";
String istasyon2_ID = "";
String istasyon3_ID = "";
String istasyon4_ID = "";

// Global değişkenler için son motor hızları
int sonSolHiz = 0;
int sonSagHiz = 0;

// RFID okuma için zaman kontrolü
unsigned long sonRFIDOkuma = 0;
const unsigned long RFID_OKUMA_ARALIGI = 100; // 100ms

// Motor kontrol fonksiyonu - BTS7960 için güncellendi
void motorControl(int solHiz, int sagHiz) {
    // Hızları kaydet
    sonSolHiz = solHiz;
    sonSagHiz = sagHiz;
    
    // Sol motor
    if (solHiz >= 0) {
        digitalWrite(motorSolREN, HIGH);
        digitalWrite(motorSolLEN, HIGH);
        analogWrite(motorSolRPWM, solHiz);
        analogWrite(motorSolLPWM, 0);
    } else {
        digitalWrite(motorSolREN, HIGH);
        digitalWrite(motorSolLEN, HIGH);
        analogWrite(motorSolRPWM, 0);
        analogWrite(motorSolLPWM, -solHiz);
    }
    
    // Sağ motor
    if (sagHiz >= 0) {
        digitalWrite(motorSagREN, HIGH);
        digitalWrite(motorSagLEN, HIGH);
        analogWrite(motorSagRPWM, sagHiz);
        analogWrite(motorSagLPWM, 0);
    } else {
        digitalWrite(motorSagREN, HIGH);
        digitalWrite(motorSagLEN, HIGH);
        analogWrite(motorSagRPWM, 0);
        analogWrite(motorSagLPWM, -sagHiz);
    }
}

// RFID kartlarını tanımlama fonksiyonu - Timeout eklendi
bool kartTanimla(int istasyon) {
    if(istasyon == 0) {
        String okunan = rfidOku();
        if(okunan != "") {
            girisIstasyonu_ID = okunan;
            Serial.println("Giriş istasyonu kartı tanımlandı: " + girisIstasyonu_ID);
            return true;
        }
    }
    Serial.println("İstasyon " + String(istasyon) + " kartını okutun");
    
    unsigned long baslangicZamani;
    const unsigned long TIMEOUT = 30000; // 30 saniye timeout
    
    baslangicZamani = millis();
    while(istasyon1_ID == "") {
        if(millis() - baslangicZamani > TIMEOUT) {
            Serial.println("HATA: İstasyon " + String(istasyon) + " kartı okunamadı - Timeout!");
            return false;
        }
        String okunan = rfidOku();
        if(okunan != "") {
            istasyon1_ID = okunan;
            Serial.println("İstasyon " + String(istasyon) + " kartı tanımlandı: " + istasyon1_ID);
            return true;
        }
        delay(100);
    }
    
    return true;
}

void setup() {
    Serial.begin(115200);
    SPI.begin();
    rfid.PCD_Init();
    
    // Motor pinleri
    for(int i = 0; i < 4; i++) {
        pinMode(motorPin[i], OUTPUT);
    }
    
    Serial.println("RFID okuyucu hazır.");
    Serial.println("RFID Kart Tanımlama Modu");
    Serial.println("Lütfen kartları sırayla tanımlayın.");
    
    // Giriş istasyonu kartı
    Serial.println("\nGİRİŞ İSTASYONU için kartı okutun...");
    while(!kartTanimla(0));
    Serial.println("Giriş istasyonu kartı tanımlandı!");
    
    // İstasyon 1 kartı
    Serial.println("\nİSTASYON 1 için kartı okutun...");
    while(!kartTanimla(1));
    Serial.println("İstasyon 1 kartı tanımlandı!");
    
    // İstasyon 2 kartı
    Serial.println("\nİSTASYON 2 için kartı okutun...");
    while(!kartTanimla(2));
    Serial.println("İstasyon 2 kartı tanımlandı!");
    
    // İstasyon 3 kartı
    Serial.println("\nİSTASYON 3 için kartı okutun...");
    while(!kartTanimla(3));
    Serial.println("İstasyon 3 kartı tanımlandı!");
    
    // İstasyon 4 kartı
    Serial.println("\nİSTASYON 4 için kartı okutun...");
    while(!kartTanimla(4));
    Serial.println("İstasyon 4 kartı tanımlandı!");
    
    Serial.println("\nTüm kartlar başarıyla tanımlandı!");
    
    // QTR sensör kalibrasyonu
    Serial.println("\nQTR Sensör Kalibrasyonu başlıyor...");
    qtrKalibrasyon();
    
    Serial.println("Robot başlatıldı!");
    Serial.println("Mevcut konum: Giriş İstasyonu (0)");
}

void loop() {
    if (Serial2.available()) {
        String komut = Serial2.readStringUntil('\n');
        
        if (komut == "GIRIS") {
            istasyonaGit(0);
        }
        else if (komut == "ISTASYON1") {
            istasyonaGit(1);
        }
        else if (komut == "ISTASYON2") {
            istasyonaGit(2);
        }
        else if (komut == "ISTASYON3") {
            istasyonaGit(3);
        }
        else if (komut == "ISTASYON4") {
            istasyonaGit(4);
        }
    }
    
    engelKontrol();
}

// RFID kart okuma fonksiyonu
String rfidOku() {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return "";
    }
    
    String kartID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        kartID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        kartID += String(rfid.uid.uidByte[i], HEX);
    }
    
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    
    return kartID;
}

// QTR sensör kalibrasyonu - Geliştirilmiş versiyon
void qtrKalibrasyon() {
    Serial.println("QTR Sensör Kalibrasyonu Başlıyor...");
    
    // Kalibrasyon süresi (7 saniye)
    for(int i = 0; i < 7000; i++) {
        // İleri-geri ve sağ-sol hareket
        if(i < 1750) {
            motorControl(YAVAS_DONUS, -YAVAS_DONUS);  // Sola dön
        } else if(i < 3500) {
            motorControl(-YAVAS_DONUS, YAVAS_DONUS);  // Sağa dön
        } else if(i < 5250) {
            motorControl(YAVAS_DONUS, YAVAS_DONUS);   // İleri git
        } else {
            motorControl(-YAVAS_DONUS, -YAVAS_DONUS); // Geri git
        }
        
        for(int j = 0; j < sensorSayisi; j++) {
            int deger = analogRead(qtrPin[j]);
            minimumDeger[j] = min(minimumDeger[j], deger);
            maksimumDeger[j] = max(maksimumDeger[j], deger);
        }
        delay(1);
    }
    
    // Kalibrasyon değerlerini doğrula
    bool kalibrasyonGecerli = true;
    for(int i = 0; i < sensorSayisi; i++) {
        esikDeger[i] = (minimumDeger[i] + maksimumDeger[i]) / 2;
        
        // Minimum ve maksimum değerler arasında yeterli fark olmalı
        if(maksimumDeger[i] - minimumDeger[i] < 200) {
            Serial.print("UYARI: Sensör ");
            Serial.print(i);
            Serial.println(" için kalibrasyon aralığı çok dar!");
            kalibrasyonGecerli = false;
        }
    }
    
    motorControl(0, 0);
    
    if(!kalibrasyonGecerli) {
        Serial.println("Kalibrasyon değerleri optimal değil! Tekrar kalibrasyon önerilir.");
    } else {
        Serial.println("Kalibrasyon başarıyla tamamlandı!");
    }
    
    // Kalibrasyon değerlerini yazdır
    for(int i = 0; i < sensorSayisi; i++) {
        Serial.print("Sensör ");
        Serial.print(i);
        Serial.print(" -> Min: ");
        Serial.print(minimumDeger[i]);
        Serial.print(" Max: ");
        Serial.print(maksimumDeger[i]);
        Serial.print(" Eşik: ");
        Serial.println(esikDeger[i]);
    }
}

// Gelişmiş pozisyon okuma
int getPozisyon() {
    int pozisyon = 0;
    int sensorDegerleriToplami = 0;
    int sensorSayaci = 0;
    bool siyahCizgiVar = false;
    
    // Her sensörü oku
    for (int i = 0; i < sensorSayisi; i++) {
        int deger = analogRead(qtrPin[i]);
        
        // Kalibrasyon değerlerine göre siyah/beyaz kontrolü
        if (deger > esikDeger[i]) {  // Siyah çizgi
            pozisyon += i * 1000;
            sensorSayaci++;
            siyahCizgiVar = true;
        }
    }
    
    // Hiç siyah çizgi görmüyorsa
    if (!siyahCizgiVar) {
        // En son görülen pozisyona göre maksimum sapma
        if (oncekiHata > 0) return 7000;  // Sağda kayboldu
        else return 0;                     // Solda kayboldu
    }
    
    // Ağırlıklı ortalama pozisyon
    if (sensorSayaci > 0) {
        return pozisyon / sensorSayaci;
    }
    
    return 3500;  // Orta pozisyon
}

// Geliştirilmiş çizgi takip fonksiyonu - Dinamik PID
void cizgiTakipPID() {
    int pozisyon = getPozisyon();
    int hata = pozisyon - 3500;
    
    unsigned long simdikiZaman = millis();
    float deltaTime = (simdikiZaman - sonPIDZamani) / 1000.0;
    
    if(deltaTime < 0.001) deltaTime = 0.001;
    sonPIDZamani = simdikiZaman;
    
    // Dinamik PID parametreleri
    float aktif_Kp = Kp;
    float aktif_Ki = Ki;
    float aktif_Kd = Kd;
    
    if(abs(hata) > 2000) {
        aktif_Kp = Kp_keskin;
        aktif_Ki = Ki_keskin;
        aktif_Kd = Kd_keskin;
    } else if(abs(hata) > 1000) {
        aktif_Kp = Kp_yavas;
        aktif_Ki = Ki_yavas;
        aktif_Kd = Kd_yavas;
    }
    
    integral = constrain(integral + hata * deltaTime, -300, 300);
    float turev = (hata - oncekiHata) / deltaTime;
    oncekiHata = hata;
    
    float pidCikis = (aktif_Kp * hata) + (aktif_Ki * integral) + (aktif_Kd * turev);
    
    int temelHiz = DUZGIDIS_HIZ;
    if(abs(hata) > 2000) {
        temelHiz = KESKIN_DONUS;
    } else if(abs(hata) > 1000) {
        temelHiz = YAVAS_DONUS;
    }
    
    int solHiz = temelHiz + pidCikis;
    int sagHiz = temelHiz - pidCikis;
    
    solHiz = constrain(solHiz, -ARAMA_HIZI, ARAMA_HIZI);
    sagHiz = constrain(sagHiz, -ARAMA_HIZI, ARAMA_HIZI);
    
    motorControl(solHiz, sagHiz);
}

// Çizgi kontrol yardımcı fonksiyonu
bool cizgiKontrol() {
    for(int j = 0; j < sensorSayisi; j++) {
        if(analogRead(qtrPin[j]) > esikDeger[j]) {
            Serial.println("Çizgi bulundu!");
            motorControl(0, 0);
            delay(100);
            return true;
        }
    }
    return false;
}

// Engel kontrol fonksiyonu - Yumuşak geçişli
bool engelKontrol() {
    digitalWrite(uTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(uTrig, HIGH);
    delayMicroseconds(10);
    digitalWrite(uTrig, LOW);
    
    long sure = pulseIn(uEcho, HIGH);
    int mesafe = sure * 0.034 / 2;
    
    static bool engelVar = false;
    static int yumusatmaAdimi = 0;
    const int YUMUSATMA_SURESI = 5;  // 5 adımda tam hıza çık
    
    if (mesafe < 15) {  // 15cm'den yakın engel
        if (!engelVar) {
            motorControl(0, 0);
            engelVar = true;
            yumusatmaAdimi = 0;
            Serial.println("Engel algılandı - Robot durdu");
        }
        return true;
    } else if (engelVar) {  // Engel kalktı
        yumusatmaAdimi++;
        if(yumusatmaAdimi >= YUMUSATMA_SURESI) {
            // Tam hıza ulaş
            motorControl(sonSolHiz, sonSagHiz);
            engelVar = false;
            Serial.println("Engel kalktı - Robot tam hızda");
        } else {
            // Kademeli hız artışı
            float hizOrani = (float)yumusatmaAdimi / YUMUSATMA_SURESI;
            motorControl(sonSolHiz * hizOrani, sonSagHiz * hizOrani);
            Serial.println("Engel kalktı - Hız artırılıyor");
        }
    }
    
    return false;
}

// İstasyona gitme fonksiyonu - Çizgi arama ve RFID kontrol iyileştirmesi
void istasyonaGit(int hedefIstasyon) {
    if(mevcutKonum == hedefIstasyon) {
        Serial.println("Zaten istasyon " + String(hedefIstasyon) + "'desiniz!");
        return;
    }
    
    String hedefKartID;
    switch(hedefIstasyon) {
        case 0: hedefKartID = girisIstasyonu_ID; break;  // Giriş istasyonu eklendi
        case 1: hedefKartID = istasyon1_ID; break;
        case 2: hedefKartID = istasyon2_ID; break;
        case 3: hedefKartID = istasyon3_ID; break;
        case 4: hedefKartID = istasyon4_ID; break;
        default: 
            Serial.println("Geçersiz istasyon!");
            return;
    }
    
    if(hedefKartID == "") {
        Serial.println("HATA: Hedef istasyon ID'si tanımlanmamış!");
        return;
    }
    
    Serial.print("İstasyon ");
    Serial.print(mevcutKonum);
    Serial.print("'den İstasyon ");
    Serial.print(hedefIstasyon);
    Serial.println("'e gidiliyor...");
    
    motorControl(0, 0);
    
    // Yön kontrolü - Yeni mantık
    if(hedefIstasyon > mevcutKonum) {  // Artan yönde hareket (0->1, 1->2, 2->3, 3->4)
        if(!robotIleriYonde) {  // Robot ters yöndeyse
            Serial.println("Artan yöne dönülüyor...");
            tamDonus();
            robotIleriYonde = true;
        }
    } else {  // Azalan yönde hareket (4->3, 3->2, 2->1, 1->0)
        if(robotIleriYonde) {  // Robot ileri yöndeyse
            Serial.println("Azalan yöne dönülüyor...");
            tamDonus();
            robotIleriYonde = false;
        }
    }
    
    unsigned long baslangicZamani = millis();
    int hataliOkumaSayisi = 0;
    const unsigned long TIMEOUT = 60000; // 60 saniye
    
    while(true) {
        if(millis() - baslangicZamani > TIMEOUT) {
            Serial.println("Zaman aşımı! Hedefe ulaşılamadı.");
            motorControl(0, 0);
            return;
        }
        
        cizgiTakipPID();
        
        // Çizgi kaybedildi mi kontrol et
        if(abs(oncekiHata) > 3000) {
            Serial.println("Çizgi kaybedildi - Arama başlatılıyor");
            cizgiAra();
            if(abs(oncekiHata) > 3000) {  // Hala bulunamadıysa
                Serial.println("Çizgi bulunamadı - Görev iptal ediliyor");
                motorControl(0, 0);
                return;
            }
        }
        
        // RFID kontrol - Belirli aralıklarla
        if(millis() - sonRFIDOkuma >= RFID_OKUMA_ARALIGI) {
            String okunanKartID = rfidOku();
            sonRFIDOkuma = millis();
            
            if(okunanKartID != "") {
                if(okunanKartID == hedefKartID) {
                    Serial.print("İstasyon ");
                    Serial.print(hedefIstasyon);
                    Serial.println("'e ulaşıldı!");
                    motorControl(0, 0);
                    mevcutKonum = hedefIstasyon;
                    break;
                } else {
                    hataliOkumaSayisi++;
                    Serial.println("Yanlış istasyon kartı okundu!");
                    if(hataliOkumaSayisi >= 3) {
                        Serial.println("Çok fazla yanlış kart okuma! Rota kontrol edilmeli.");
                        motorControl(0, 0);
                        return;
                    }
                }
            }
        }
        
        if(engelKontrol()) {
            baslangicZamani = millis();
        }
    }
}

// Tam dönüş fonksiyonu - Hıza bağlı süre hesaplama
void tamDonus() {
    Serial.println("180 derece dönüş yapılıyor...");
    int donusHizi = YAVAS_DONUS;
    // Dönüş süresi = (180 derece * pi) * tekerlek mesafesi / (2 * dönüş hızı)
    int donusSuresi = (int)(180 * 3.14159 * 20) / (2 * donusHizi);  // 20cm tekerlek mesafesi varsayımı
    
    motorControl(donusHizi, -donusHizi);
    delay(donusSuresi);
    motorControl(0, 0);
    delay(100);
    Serial.println("Dönüş tamamlandı");
}

