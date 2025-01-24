// ========================= KÜTÜPHANE TANIMLARI =========================
// Arduino'nun temel fonksiyonlarını içeren ana kütüphane - Dijital/Analog pin kontrolü, seri haberleşme vb. için gerekli
#include <Arduino.h>                

// Adafruit TCS34725 renk sensörü kütüphanesi - RGB renk değerlerini okumak ve renk tespiti yapmak için kullanılır
#include <Adafruit_TCS34725.h>     

// QTR sensör kütüphanesi - Çizgi takibi için kullanılan kızılötesi sensörlerin kontrolünü sağlar
#include <QTRSensors.h>            
#include <Wire.h>                  // I2C haberleşme için gerekli
// #include <SoftwareSerial.h>

// SoftwareSerial ardiunoSerial(10, 11); // RX=10, TX=11

// ========================= MOTOR PIN TANIMLARI =========================
// Sağ motor sürücü pinleri (BTS7960B)
const int motorSagRPWM = 2;     // Sağ motor ileri PWM
const int motorSagLPWM = 3;     // Sağ motor geri PWM
const int motorSagREN = 4;      // Sağ motor ileri enable
const int motorSagLEN = 8;      // Sağ motor geri enable

// Sol motor sürücü pinleri (BTS7960B)
const int motorSolRPWM = 5;     // Sol motor ileri PWM
const int motorSolLPWM = 6;     // Sol motor geri PWM
const int motorSolREN = 7;      // Sol motor ileri enable
const int motorSolLEN = 9;      // Sol motor geri enable

// ========================= ULTRASONİK SENSÖR PIN TANIMLARI =========================
const int uTrig = 10;     // Ultrasonik trigger pin
const int uEcho = 11;     // Ultrasonik echo pin

// ========================= ÇİZGİ SENSÖR AYARLARI =========================
// Çizgi sensörleri için dijital pinler (soldan sağa)
int qtrPin[] = {22, 23, 24, 25, 26, 27, 28, 29};  // Dijital pinler

// Kullanılan çizgi sensörü sayısı
const int sensorSayisi = 8;     // Toplam sensör sayısı

// ========================= SENSÖR AYARLARI =========================
// Renk sensörü nesnesi oluştur
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_600MS, TCS34725_GAIN_1X);

// ========================= HAREKET PARAMETRELERİ =========================
// Motorların varsayılan çalışma hızı - 0-255 arası bir değer (PWM)
const int VARSAYILAN_HIZ = 150;  

// Anlık olarak kullanılan motor hızı - Hareket sırasında değişebilir
int motorHizi = VARSAYILAN_HIZ;  

// Robotun duracağı minimum engel mesafesi (cm) - Bu mesafeden yakın engeller için robot durur
const int engelAlgila = 16;      

// ========================= RENK SENSÖRÜ TANIMLAMASI =========================
// Bu satırı siliyoruz çünkü yukarıda zaten tanımlanmış
// Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

// ========================= KONUM BİLGİLERİ =========================
// Robotun şu an bulunduğu istasyon numarası - 0 başlangıç noktasını temsil eder
int mevcutKonum = 0;      

// Robotun gitmesi gereken hedef istasyon - Hareket komutları ile güncellenir
int hedef = 1;           

// ========================= RENK ALGILAMA PARAMETRELERİ =========================
// Her rengin HSV uzayındaki ton (hue) aralıklarını tanımlayan yapı
const struct {
    float hueMin;        // Rengin HSV uzayında başlangıç tonu (0-360 derece)
    float hueMax;        // Rengin HSV uzayında bitiş tonu (0-360 derece)
    const char* name;    // Rengin tanımlayıcı ismi
} RENK_ESIKLERI[] = {
    {200, 250, "blue"},   // Mavi renk için ton aralığı
    {100, 140, "green"},  // Yeşil renk için ton aralığı
    {40, 70, "yellow"},   // Sarı renk için ton aralığı
    {15, 40, "orange"},   // Turuncu renk için ton aralığı
    {340, 360, "red"},    // Kırmızı renk için üst ton aralığı
    {0, 20, "red"}        // Kırmızı renk için alt ton aralığı (kırmızı 360/0 derecede kesişir)
};

// ========================= MOTOR GÜVENLİK PARAMETRELERİ =========================
// Bir döngüde motorların hızındaki maksimum değişim miktarı - Ani hız değişimlerini önler
const int MAX_HIZ_DEGISIMI = 20;     

// Motorların minimum çalışma hızı - Bu değerin altındaki hızlarda motor düzgün çalışmayabilir
const int MIN_MOTOR_HIZ = 50;        

// Yumuşak durma sırasında her adımda azaltılacak hız miktarı
const int YUMUSAK_DURMA_ADIM = 10;   

// ========================= PID KONTROL PARAMETRELERİ =========================
// İntegral teriminin maksimum değeri - Aşırı integral birikimini önler
const float MAX_INTEGRAL = 1000.0;    

// PID hesaplaması için minimum hata değeri - Küçük hataları yok sayar
const float MIN_PID_HATA = 100.0;     

// İstasyon algılama için gereken ardışık başarılı okuma sayısı
const int ISTASYON_DOGRULAMA = 3;     

// ========================= PID KONTROL DEĞİŞKENLERİ =========================
// Bir önceki döngüdeki hata değeri - Türev hesaplaması için kullanılır
float oncekiHata = 0;     

// Toplam hata değeri - İntegral hesaplaması için kullanılır
float hataIntegral = 0;   

// Son PID hesaplama zamanı - Zaman bazlı hesaplamalar için kullanılır
unsigned long sonPIDZaman = 0;  

// ========================= PID KONTROL KATSAYILARI =========================
// Oransal kontrol katsayısı - Anlık hataya verilen tepkiyi belirler
const float KP = 0.35;    

// İntegral kontrol katsayısı - Birikmiş hataya verilen tepkiyi belirler
const float KI = 0.0008;  

// Türevsel kontrol katsayısı - Hatanın değişim hızına verilen tepkiyi belirler
const float KD = 0.25;    

// İstasyon yapısı - Her istasyonun özelliklerini tanımlar
struct Istasyon {
    int numara;             // İstasyon numarası
    String durmaRengi;      // İstasyonda durulacak renk
    int qtrEsik;           // QTR sensör eşik değeri
    bool metalVarMi;       // Metal algılama durumu
};

// İstasyon dizisi tanımı
Istasyon istasyonlar[] = {
    {1, "yellow", 800, true},
    {2, "green", 800, true},
    {3, "orange", 800, true},
    {4, "red", 800, true}
};

// Kalibrasyon değerleri için yapı - Her renk için min/max değerleri tutar
struct RenkKalibrasyonu {
    float minR, maxR;      // Kırmızı renk için min/max değerler
    float minG, maxG;      // Yeşil renk için min/max değerler
    float minB, maxB;      // Mavi renk için min/max değerler
    float minH, maxH;      // Ton için min/max değerler
    float minS, maxS;      // Doygunluk için min/max değerler
    float minV, maxV;      // Parlaklık için min/max değerler
};

RenkKalibrasyonu kalibrasyon[5]; // red, blue, green, orange, black için kalibrasyon değerleri

// ========================= GENEL AÇIKLAMALAR =========================
/*
Bu kod, çizgi izleyen ve renk algılayan bir robotun kontrol yazılımıdır.
Temel özellikleri:
- İstasyonlar arası hareket
- Renk sensörü ile çizgi takibi
- PID kontrol ile hassas hareket
- Engel algılama
- Kalibrasyon sistemi
*/

// Setup fonksiyonu - Program başladığında bir kez çalışır
void setup() {
    // Seri haberleşmeyi başlat
    Serial.begin(115200);  
    Serial2.begin(115200); 
    // I2C başlatma
    Wire.begin();  
    Wire.setClock(100000);  

    // Motor pinlerini ayarla
    pinMode(motorSagRPWM, OUTPUT);
    pinMode(motorSagLPWM, OUTPUT);
    pinMode(motorSagREN, OUTPUT);
    pinMode(motorSagLEN, OUTPUT);
    pinMode(motorSolRPWM, OUTPUT);
    pinMode(motorSolLPWM, OUTPUT);
    pinMode(motorSolREN, OUTPUT);
    pinMode(motorSolLEN, OUTPUT);
    
    // Motor sürücüleri aktif et
    digitalWrite(motorSagREN, HIGH);
    digitalWrite(motorSagLEN, HIGH);
    digitalWrite(motorSolREN, HIGH);
    digitalWrite(motorSolLEN, HIGH);
    
    // Motorları durdur
    analogWrite(motorSagRPWM, 0);
    analogWrite(motorSagLPWM, 0);
    analogWrite(motorSolRPWM, 0);
    analogWrite(motorSolLPWM, 0);

    // Ultrasonik sensör pinlerini ayarla
    pinMode(uTrig, OUTPUT);
    pinMode(uEcho, INPUT);

    // Çizgi sensör pinlerini ayarla
    for (int i = 0; i < sensorSayisi; i++) {
        pinMode(qtrPin[i], INPUT_PULLUP);
    }
    
    // Renk sensörünü başlat
    /*if (!tcs.begin()) {
        Serial.println("HATA: TCS34725 sensörü bulunamadı!");
        while (1) {
            delay(1000);
            Serial.println("Lütfen bağlantıları kontrol edin");
        }
    }*/
    
    Serial.println("TCS34725 Renk Sensörü başarıyla başlatıldı");
    Serial.println("Robot hazır.");

   
}

// Ana döngü - Program çalıştığı sürece tekrar eder
void loop() {
    String veri = "";

    if (Serial2.available()) {
        veri = Serial2.readStringUntil('\n');
        if(veri.length() > 0) {
            Serial.println("Alınan komut: " + veri);
            
            // Giriş (İstasyon 0'a dönüş)
            if(veri == "giris") {
                Serial.println("Başlangıç noktasına dönülüyor...");
                motorControl(0, 0); // Önce dur
                // Bulunduğu konuma göre başlangıç noktasına dönüş
                if(mevcutKonum == 1) {
                    cizgiTakipRenk("red", "black");
                } else if(mevcutKonum == 2) {
                    cizgiTakipRenk("black", "black");
                } else if(mevcutKonum == 3) {
                    cizgiTakipRenk("green", "black");
                } else if(mevcutKonum == 4) {
                    cizgiTakipRenk("orange", "black");
                }
                mevcutKonum = 0;
            }
            
            // İstasyon 1
            else if(veri == "istasyon1") {
                Serial.println("1. İstasyona gidiliyor...");
                motorControl(0, 0); // Önce dur
                if(mevcutKonum == 0) {
                    cizgiTakipRenk("red", "black");
                } else if(mevcutKonum == 2) {
                    cizgiTakipRenk("black", "black");
                } else if(mevcutKonum == 3) {
                    cizgiTakipRenk("green", "black");
                } else if(mevcutKonum == 4) {
                    cizgiTakipRenk("orange", "black");
                }
                mevcutKonum = 1;
            }
            
            // İstasyon 2
            else if(veri == "istasyon2") {
                Serial.println("2. İstasyona gidiliyor...");
                motorControl(0, 0); // Önce dur
                if(mevcutKonum == 0) {
                    cizgiTakipRenk("red", "green");
                } else if(mevcutKonum == 1) {
                    cizgiTakipRenk("black", "green");
                } else if(mevcutKonum == 3) {
                    cizgiTakipRenk("green", "green");
                } else if(mevcutKonum == 4) {
                    cizgiTakipRenk("orange", "green");
                }
                mevcutKonum = 2;
            }
            
            // İstasyon 3
            else if(veri == "istasyon3") {
                Serial.println("3. İstasyona gidiliyor...");
                motorControl(0, 0); // Önce dur
                if(mevcutKonum == 0) {
                    cizgiTakipRenk("red", "orange");
                } else if(mevcutKonum == 1) {
                    cizgiTakipRenk("black", "orange");
                } else if(mevcutKonum == 2) {
                    cizgiTakipRenk("green", "orange");
                } else if(mevcutKonum == 4) {
                    cizgiTakipRenk("orange", "orange");
                }
                mevcutKonum = 3;
            }
            
            // İstasyon 4
            else if(veri == "istasyon4") {
                Serial.println("4. İstasyona gidiliyor...");
                motorControl(0, 0); // Önce dur
                if(mevcutKonum == 0) {
                    // 0'dan 4'e özel rota: önce kırmızı sonra mavi
                    cizgiTakipRenk("red", "blue");
                    delay(100); // Kısa bekleme
                    cizgiTakipRenk("blue", "blue");
                } else if(mevcutKonum == 1) {
                    cizgiTakipRenk("black", "blue");
                } else if(mevcutKonum == 2) {
                    cizgiTakipRenk("green", "blue");
                } else if(mevcutKonum == 3) {
                    cizgiTakipRenk("orange", "blue");
                }
                mevcutKonum = 4;
            }
        }
    }
    
    // Her döngüde engel kontrolü yap
    engelKontrol();
    delay(100);
}

// Kalibrasyon fonksiyonu - Her renk için sensör değerlerini kalibre eder
void renkKalibrasyonuYap() {
    Serial.println("Renk kalibrasyonu başlıyor...");
    const int OLCUM_SAYISI = 10;
    
    // Her renk için kalibrasyon
    String renkler[] = {"red", "blue", "green", "orange", "black"};
    for(int i = 0; i < 5; i++) {
        Serial.print(renkler[i]);
        Serial.println(" rengini sensörün altına getirin ve 'k' gönderin");
        
        while(!Serial.available() || Serial.read() != 'k') {
            delay(100);
        }
        
        // Çoklu ölçüm al
        float sumR = 0, sumG = 0, sumB = 0;
        float minR = 255, maxR = 0;
        float minG = 255, maxG = 0;
        float minB = 255, maxB = 0;
        float minH = 360, maxH = 0;
        float minS = 1, maxS = 0;
        float minV = 1, maxV = 0;
        
        for(int j = 0; j < OLCUM_SAYISI; j++) {
            uint16_t r, g, b, c;
            tcs.getRawData(&r, &g, &b, &c);
            
            float rNorm = (float)r / c * 255.0;
            float gNorm = (float)g / c * 255.0;
            float bNorm = (float)b / c * 255.0;
            
            float h, s, v;
            rgbToHsv(rNorm, gNorm, bNorm, h, s, v);
            
            // Min-max değerleri güncelle
            minR = min(minR, rNorm);
            maxR = max(maxR, rNorm);
            minG = min(minG, gNorm);
            maxG = max(maxG, gNorm);
            minB = min(minB, bNorm);
            maxB = max(maxB, bNorm);
            minH = min(minH, h);
            maxH = max(maxH, h);
            minS = min(minS, s);
            maxS = max(maxS, s);
            minV = min(minV, v);
            maxV = max(maxV, v);
            
            delay(100);
        }
        
        // Kalibrasyon değerlerini kaydet
        kalibrasyon[i].minR = minR;
        kalibrasyon[i].maxR = maxR;
        kalibrasyon[i].minG = minG;
        kalibrasyon[i].maxG = maxG;
        kalibrasyon[i].minB = minB;
        kalibrasyon[i].maxB = maxB;
        kalibrasyon[i].minH = minH;
        kalibrasyon[i].maxH = maxH;
        kalibrasyon[i].minS = minS;
        kalibrasyon[i].maxS = maxS;
        kalibrasyon[i].minV = minV;
        kalibrasyon[i].maxV = maxV;
        
        Serial.println("Kalibrasyon tamamlandı!");
    }
}

// İstasyonlar arası hareket fonksiyonu - Tam tur dönüşlü
// baslangic: Başlangıç istasyonu numarası
// bitis: Hedef istasyon numarası
void istasyonaGit(int baslangic, int bitis) {
    if (baslangic < 0 || bitis < 0 || baslangic > 4 || bitis > 4) {
        Serial.println("HATA: Geçersiz istasyon numarası!");
        return;
    }

    Serial.print("Hareket: İstasyon ");
    Serial.print(baslangic);
    Serial.print(" -> İstasyon ");
    Serial.println(bitis);

    motorControl(0, 0);

    // Geri dönüş gerektiren durumlar için tam tur
    bool geriDonus = false;
    if((baslangic == 1 && bitis == 0) ||
       (baslangic == 2 && bitis == 1) ||
       (baslangic == 3 && bitis == 2) ||
       (baslangic == 4 && bitis == 3)) {
        geriDonus = true;
        tamTurDon(true);  // Saat yönünde dön
    }

    // İstasyonlar arası hareket stratejileri
    switch(baslangic) {
        case 0:
            switch(bitis) {
                case 1:
                    cizgiTakipRenk("red", "black");
                    break;
                case 2:
                    cizgiTakipRenk("red", "black");
                    delay(100);
                    cizgiTakipRenk("black", "green");
                    break;
                case 3:
                    cizgiTakipRenk("red", "black");
                    delay(100);
                    cizgiTakipRenk("black", "green");
                    delay(100);
                    cizgiTakipRenk("green", "orange");
                    break;
                case 4:
                    cizgiTakipRenk("red", "blue");
                    break;
            }
            break;
            
        // Diğer case'ler benzer şekilde devam eder...
    }

    if(geriDonus) {
        tamTurDon(false);  // Tekrar düz pozisyona dön
    }

    mevcutKonum = bitis;
    Serial.print("Hedef istasyona ulaşıldı: ");
    Serial.println(bitis);
}

// Renk sensörü okumaları için filtreleme
// yeniDeger: Yeni ölçüm değeri
// eskiDeger: Önceki filtrelenmiş değer
// alpha: Filtreleme katsayısı (0-1 arası)
float renkFiltreleme(float yeniDeger, float eskiDeger, float alpha = 0.2) {
    return alpha * yeniDeger + (1 - alpha) * eskiDeger;
}

// Renk takip fonksiyonu - PID kontrollü, filtreli ve hız optimizasyonlu
// takipRenk: Takip edilecek çizginin rengi
// durmaRenk: Durulacak renk
void cizgiTakipRenk(String takipRenk, String durmaRenk) {
    const unsigned long ZAMAN_ASIMI = 30000;  // 30 saniye zaman aşımı süresi
    unsigned long baslangicZamani = millis();
    
    // Durma rengi doğrulama sayacı
    int durmaRengiSayaci = 0;
    const int DURMA_DOGRULAMA = 3;  // Durma için gereken ardışık okuma sayısı
    
    // Filtrelenmiş renk değerleri için değişkenler
    float filtreliRed = 0, filtreliGreen = 0, filtreliBlue = 0;
    
    // PID değişkenleri
    float hata = 0;
    float sonHata = 0;
    float hataIntegral = 0;
    unsigned long sonZaman = millis();
    
    // Hız optimizasyonu için değişkenler
    const int MAKS_HIZ = motorHizi;
    const int MIN_HIZ = motorHizi / 2;
    const float DONUS_HIZ_FAKTORU = 0.7;
    
    while (millis() - baslangicZamani < ZAMAN_ASIMI) {
        // Zaman hesaplama
        unsigned long simdikiZaman = millis();
        float deltaTime = (simdikiZaman - sonZaman) / 1000.0;
        sonZaman = simdikiZaman;
        
        // Renk okuması (çoklu okuma ile güvenilirlik artırma)
        uint16_t r = 0, g = 0, b = 0, c = 0;
        for(int i = 0; i < 3; i++) {  // 3 okuma ortalaması
            uint16_t tempR, tempG, tempB, tempC;
            tcs.getRawData(&tempR, &tempG, &tempB, &tempC);
            if(tempC > 0) {
                r += tempR;
                g += tempG;
                b += tempB;
                c += tempC;
            }
            delay(5);
        }
        
        // Ortalama alma
        r /= 3; g /= 3; b /= 3; c /= 3;
        
        if (c == 0) continue;  // Geçersiz okuma kontrolü

        // Renk değerlerini normalize et ve filtrele
        float redNorm = (float)r / c * 255.0;
        float greenNorm = (float)g / c * 255.0;
        float blueNorm = (float)b / c * 255.0;
        
        filtreliRed = renkFiltreleme(redNorm, filtreliRed);
        filtreliGreen = renkFiltreleme(greenNorm, filtreliGreen);
        filtreliBlue = renkFiltreleme(blueNorm, filtreliBlue);
        
        // Filtrelenmiş değerlerle renk tespiti
        String algilananRenk = detectColor(filtreliRed, filtreliGreen, filtreliBlue);
        
        // Renk pozisyon hatası hesaplama (-100 ile 100 arası)
        if(algilananRenk == takipRenk) {
            // Rengin merkeze göre pozisyonunu hesapla
            float renkPozisyon = (filtreliRed + filtreliGreen + filtreliBlue) / 3.0;
            hata = renkPozisyon - 127.5;  // Merkez değerden sapma
        }
        
        // PID hesaplamaları
        float hataTurev = (hata - sonHata) / deltaTime;
        hataIntegral += hata * deltaTime;
        hataIntegral = constrain(hataIntegral, -MAX_INTEGRAL, MAX_INTEGRAL);
        
        float pidCikis = (KP * hata) + (KI * hataIntegral) + (KD * hataTurev);
        sonHata = hata;
        
        // Durma rengi kontrolü ve doğrulama
        if (algilananRenk == durmaRenk) {
            durmaRengiSayaci++;
            
            // Yavaşlama - Durma rengini gördükçe hızı azalt
            int yavaslamaHizi = motorHizi * (1.0 - (float)durmaRengiSayaci / DURMA_DOGRULAMA);
            motorControl(yavaslamaHizi, yavaslamaHizi);
            
            if (durmaRengiSayaci >= DURMA_DOGRULAMA) {
                yumusakDur();  // Yumuşak duruş
                
                // Son kontrol
                delay(100);
                tcs.getRawData(&r, &g, &b, &c);
                if (c > 0) {
                    redNorm = (float)r / c * 255.0;
                    greenNorm = (float)g / c * 255.0;
                    blueNorm = (float)b / c * 255.0;
                    if (detectColor(redNorm, greenNorm, blueNorm) == durmaRenk) {
                        Serial.println("Durma rengi doğrulandı. Robot durduruldu.");
                        return;
                    }
                }
            }
        } else {
            durmaRengiSayaci = 0;
            
            if (algilananRenk == takipRenk) {
                // PID çıkışına göre motor hızlarını ayarla
                float solHiz = MAKS_HIZ - pidCikis;
                float sagHiz = MAKS_HIZ + pidCikis;
                
                // Hızları sınırla
                solHiz = constrain(solHiz, MIN_HIZ, MAKS_HIZ);
                sagHiz = constrain(sagHiz, MIN_HIZ, MAKS_HIZ);
                
                // Dönüşlerde hız optimizasyonu
                if (abs(pidCikis) > 30) {  // Keskin dönüş
                    solHiz *= DONUS_HIZ_FAKTORU;
                    sagHiz *= DONUS_HIZ_FAKTORU;
                }
                
                motorControl(solHiz, sagHiz);
            } else {
                // Renk kaybedildiğinde arama stratejisi
                int aramaHizi = MIN_HIZ;
                motorControl(aramaHizi, -aramaHizi);  // Yerinde dön
                delay(50);
                
                if (algilananRenk != takipRenk) {
                    motorControl(-aramaHizi, aramaHizi);  // Diğer yöne dön
                    delay(100);
                }
            }
        }
        
        delay(10);
    }
    
    // Zaman aşımı
    yumusakDur();
    Serial.println("UYARI: Çizgi takibinde zaman aşımı!");
}

// Motor kontrol fonksiyonu - Motorların hız ve yönünü ayarlar
// solHiz: Sol motor hızı (-255 ile 255 arası)
// sagHiz: Sağ motor hızı (-255 ile 255 arası)
void motorControl(int solHiz, int sagHiz) {
    // Hızları sınırla
    solHiz = constrain(solHiz, -255, 255);
    sagHiz = constrain(sagHiz, -255, 255);
    
    // Sağ motor kontrolü
    if (sagHiz >= 0) {
        digitalWrite(motorSagREN, HIGH);  // İleri yönü aktif
        digitalWrite(motorSagLEN, LOW);   // Geri yönü pasif
        analogWrite(motorSagRPWM, sagHiz);
        analogWrite(motorSagLPWM, 0);
    } else {
        digitalWrite(motorSagREN, LOW);   // İleri yönü pasif
        digitalWrite(motorSagLEN, HIGH);  // Geri yönü aktif
        analogWrite(motorSagRPWM, 0);
        analogWrite(motorSagLPWM, -sagHiz);
    }
    
    // Sol motor kontrolü
    if (solHiz >= 0) {
        digitalWrite(motorSolREN, HIGH);  // İleri yönü aktif
        digitalWrite(motorSolLEN, LOW);   // Geri yönü pasif
        analogWrite(motorSolRPWM, solHiz);
        analogWrite(motorSolLPWM, 0);
    } else {
        digitalWrite(motorSolREN, LOW);   // İleri yönü pasif
        digitalWrite(motorSolLEN, HIGH);  // Geri yönü aktif
        analogWrite(motorSolRPWM, 0);
        analogWrite(motorSolLPWM, -solHiz);
    }
}

// Yumuşak durma fonksiyonu - Robotun kademeli olarak durmasını sağlar
void yumusakDur() {
    int solHiz = motorHizi;
    int sagHiz = motorHizi;
    
    while (solHiz > 0 || sagHiz > 0) {
        solHiz = max(0, solHiz - YUMUSAK_DURMA_ADIM);
        sagHiz = max(0, sagHiz - YUMUSAK_DURMA_ADIM);
        motorControl(solHiz, sagHiz);
        delay(20);
    }
}

// Engel algılama ve kontrol fonksiyonu - Ultrasonik sensör ile engel tespiti
void engelKontrol() {
    const unsigned long TIMEOUT_US = 30000;  // 30ms timeout süresi
    const int OLCUM_SAYISI = 3;  // Yapılacak ölçüm sayısı
    int mesafeler[OLCUM_SAYISI];  // Ölçüm sonuçlarını tutacak dizi
    
    // Birden fazla ölçüm yap
    for (int i = 0; i < OLCUM_SAYISI; i++) {
        // Ultrasonik sensör sinyali gönder
        digitalWrite(uTrig, LOW);
        delayMicroseconds(2);
        digitalWrite(uTrig, HIGH);
        delayMicroseconds(10);
        digitalWrite(uTrig, LOW);

        // Yanıt süresini ölç ve mesafeye çevir
        long sure = pulseIn(uEcho, HIGH, TIMEOUT_US);
        mesafeler[i] = sure == 0 ? 999 : sure / 58.2;
        delay(10);
    }

    // Medyan filtreleme ile gürültüyü azalt
    int medyanMesafe = mesafeler[1];
    
    // Engel kontrolü
    if (medyanMesafe < engelAlgila) {
        Serial.print("Engel algılandı! Mesafe: ");
        Serial.print(medyanMesafe);
        Serial.println(" cm");
        motorControl(0, 0);
        
        while (true) {
            delay(500);
            int yeniMesafe = ultrasonikOlcum();
            if (yeniMesafe > engelAlgila) {
                Serial.println("Engel kalktı. Devam ediliyor...");
                break;
            }
        }
    }
}

// Tek ultrasonik ölçüm fonksiyonu - Mesafe ölçümü yapar
// Dönüş: Mesafe (cm cinsinden)
int ultrasonikOlcum() {
    // Ultrasonik sensör sinyali gönder
    digitalWrite(uTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(uTrig, HIGH);
    delayMicroseconds(10);
    digitalWrite(uTrig, LOW);

    // Yanıt süresini ölç ve mesafeye çevir
    long sure = pulseIn(uEcho, HIGH, 30000);
    return sure == 0 ? 999 : sure / 58.2;
}

// RGB renk değerlerini HSV (Ton, Doygunluk, Parlaklık) değerlerine dönüştürür
// r, g, b: RGB değerleri (0-255 arası)
// h, s, v: HSV değerleri (h:0-360, s:0-1, v:0-1)
void rgbToHsv(float r, float g, float b, float& h, float& s, float& v) {
    r /= 255.0f;
    g /= 255.0f;
    b /= 255.0f;
    
    float minVal = min(r, min(g, b));
    float maxVal = max(r, max(g, b));
    float delta = maxVal - minVal;
    
    v = maxVal;
    s = maxVal == 0 ? 0 : delta / maxVal;
    
    if (delta == 0) {
        h = 0;
    } else {
        if (maxVal == r) {
            h = 60.0f * fmod(((g - b) / delta), 6.0f);
        } else if (maxVal == g) {
            h = 60.0f * (((b - r) / delta) + 2.0f);
        } else {
            h = 60.0f * (((r - g) / delta) + 4.0f);
        }
        
        if (h < 0) h += 360.0f;
    }
}

// Renk tespit fonksiyonu - RGB değerlerinden renk ismini belirler
// r, g, b: RGB değerleri (0-255 arası)
// Dönüş: Algılanan rengin ismi (String)
String detectColor(float r, float g, float b) {
    float h, s, v;
    rgbToHsv(r, g, b, h, s, v);

    // Düşük parlaklık kontrolü
    if (v < 0.10) return "unknown";  // Çok karanlık

    // Gri/Siyah kontrolü
    if (s < 0.15) {
        if (v < 0.25) return "black";  // Siyah için parlaklık eşiği
        return "unknown";
    }

    // Minimum renk yoğunluğu kontrolü
    const float MIN_INTENSITY = 40.0;
    if (r < MIN_INTENSITY && g < MIN_INTENSITY && b < MIN_INTENSITY) return "unknown";

    // KIRMIZI renk kontrolü - Daha hassas algılama
    if ((h >= 340 || h <= 15) && r > g * 1.4 && r > b * 1.4) {
        if (s > 0.4 && v > 0.2) {  // Doygunluk ve parlaklık kontrolü
            if (r > 100) {  // Minimum kırmızı yoğunluğu
                return "red";
            }
        }
    }

    // MAVİ renk kontrolü - Daha hassas algılama
    if (h >= 180 && h <= 250) {
        if (b > r * 1.3 && b > g * 1.2) {
            if (s > 0.3 && v > 0.2) {  // Doygunluk ve parlaklık kontrolü
                if (b > 80) {  // Minimum mavi yoğunluğu
                    return "blue";
                }
            }
        }
    }

    // YEŞİL renk kontrolü - Daha hassas algılama
    if (h >= 90 && h <= 160) {
        if (g > r * 1.2 && g > b * 1.2) {
            if (s > 0.3 && v > 0.2) {  // Doygunluk ve parlaklık kontrolü
                if (g > 80) {  // Minimum yeşil yoğunluğu
                    return "green";
                }
            }
        }
    }

    // TURUNCU renk kontrolü - Daha hassas algılama
    if (h >= 15 && h <= 45) {
        if (r > g * 1.2 && r > b * 2.0 && g > b * 1.2) {
            if (s > 0.4 && v > 0.3) {  // Doygunluk ve parlaklık kontrolü
                if (r > 100 && g > 50) {  // Minimum kırmızı ve yeşil yoğunluğu
                    return "orange";
                }
            }
        }
    }

    // Renk tanımlanamadıysa
    return "unknown";
}

// PID kontrol ile çizgi takibi yapan fonksiyon
void cizgiTakipPID() {
    unsigned long simdikiZaman = millis();
    float deltaTime = (simdikiZaman - sonPIDZaman) / 1000.0;
    sonPIDZaman = simdikiZaman;
    
    float pozisyon = 0;
    int sensorToplam = 0;
    
    for(int i = 0; i < sensorSayisi; i++) {
        int deger = analogRead(qtrPin[i]);
        pozisyon += deger * (i * 1000);
        sensorToplam += deger;
    }
    
    if(sensorToplam > 0) {
        pozisyon = pozisyon / sensorToplam;
    }
    
    float hata = pozisyon - 3500;
    
    // Küçük hataları yoksay
    if (abs(hata) < MIN_PID_HATA) {
        hata = 0;
        hataIntegral = 0;  // Integral terimini sıfırla
    }
    
    // Integral hesaplama ve sınırlama
    hataIntegral += hata * deltaTime;
    hataIntegral = constrain(hataIntegral, -MAX_INTEGRAL, MAX_INTEGRAL);
    
    float hataTurev = (hata - oncekiHata) / deltaTime;
    oncekiHata = hata;
    
    float pidCikis = (KP * hata) + (KI * hataIntegral) + (KD * hataTurev);
    
    int solHiz = VARSAYILAN_HIZ + pidCikis;
    int sagHiz = VARSAYILAN_HIZ - pidCikis;
    
    motorControl(solHiz, sagHiz);
    
    // Debug bilgisi
    Serial.print("Poz:");
    Serial.print(pozisyon);
    Serial.print(" Hata:");
    Serial.print(hata);
    Serial.print(" I:");
    Serial.print(hataIntegral);
    Serial.print(" D:");
    Serial.print(hataTurev);
    Serial.print(" PID:");
    Serial.println(pidCikis);
}

// İstasyon kontrolü - Doğru istasyonda olup olmadığını kontrol eder
// hedefIstasyon: Kontrol edilecek istasyon numarası
// Dönüş: İstasyonda olup olmadığı (bool)
bool istasyonKontrol(int hedefIstasyon) {
    int dogrulamaCount = 0;
    
    // Birden fazla okuma yap
    for (int i = 0; i < ISTASYON_DOGRULAMA; i++) {
        String algilananRenk = getCurrentColor();
        int qtrDeger = getQTRValue();
        
        if(hedefIstasyon < 1 || hedefIstasyon > 4) {
            Serial.println("HATA: Geçersiz istasyon numarası!");
            return false;
        }
        
        Istasyon hedef = istasyonlar[hedefIstasyon - 1];
        
        if (algilananRenk == hedef.durmaRengi && qtrDeger > hedef.qtrEsik) {
            dogrulamaCount++;
        }
        
        delay(50);  // Okumalar arası kısa bekleme
    }
    
    // Tüm okumaların çoğunluğu doğruysa
    if (dogrulamaCount >= (ISTASYON_DOGRULAMA / 2 + 1)) {
        Serial.print("İstasyon ");
        Serial.print(hedefIstasyon);
        Serial.println(" doğrulandı!");
        return true;
    }
    
    return false;
}

// QTR sensörlerinin ortalamasını alan yardımcı fonksiyon
// Dönüş: Sensörlerin ortalama değeri
int getQTRValue() {
    int toplam = 0;
    for(int i = 0; i < sensorSayisi; i++) {
        toplam += analogRead(qtrPin[i]);
    }
    return toplam / sensorSayisi;
}

// Sensör bağlantılarını kontrol eden fonksiyon
// Dönüş: Tüm sensörler bağlı mı (bool)
bool checkSensorConnections() {
    Wire.beginTransmission(0x29);  // TCS34725 I2C adresi
    if (Wire.endTransmission() != 0) {
        Serial.println("TCS34725 bağlantı hatası!");
        return false;
    }
    
    // QTR sensör kontrolü
    bool qtrOK = false;
    for (int i = 0; i < sensorSayisi; i++) {
        if (analogRead(qtrPin[i]) > 0) {
            qtrOK = true;
            break;
        }
    }
    
    if (!qtrOK) {
        Serial.println("QTR sensör bağlantı hatası!");
        return false;
    }
    
    return true;
} 

// Sensör okumalarında gürültü önleme
const int SAMPLE_COUNT = 5;

int getFilteredQTRValue(int pin) {
    int sum = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        sum += analogRead(pin);
        delayMicroseconds(100);
    }
    return sum / SAMPLE_COUNT;
} 

// Güncel renk algılama fonksiyonu
// Dönüş: Algılanan rengin ismi (String)
String getCurrentColor() {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);
    
    if (c > 0) {
        float redNorm = (float)r / c * 255.0;
        float greenNorm = (float)g / c * 255.0;
        float blueNorm = (float)b / c * 255.0;
        return detectColor(redNorm, greenNorm, blueNorm);
    }
    return "unknown";
} 

// Tam tur dönüş fonksiyonu
// saatYonu: Dönüş yönü (true: saat yönü, false: saat yönünün tersi)
void tamTurDon(bool saatYonu) {
    const int donusHizi = motorHizi / 2;  // Dönüş için daha düşük hız
    const int donusSuresi = 2000;  // 2 saniye (ayarlanabilir)
    
    Serial.println(saatYonu ? "Saat yönünde dönülüyor..." : "Saat yönünün tersine dönülüyor...");
    
    if(saatYonu) {
        motorControl(donusHizi, -donusHizi);
    } else {
        motorControl(-donusHizi, donusHizi);
    }
    
    delay(donusSuresi);
    motorControl(0, 0);
}

// Debug bilgisi yazdırma fonksiyonu - Sensör değerlerini seri porta yazdırır
void debugBilgisiYazdir(float r, float g, float b, float h, float s, float v, String algilananRenk) {
    Serial.println("=== DEBUG BİLGİSİ ===");
    Serial.print("RGB: (");
    Serial.print(r); Serial.print(", ");
    Serial.print(g); Serial.print(", ");
    Serial.print(b); Serial.println(")");
    
    Serial.print("HSV: (");
    Serial.print(h); Serial.print("°, ");
    Serial.print(s*100); Serial.print("%, ");
    Serial.print(v*100); Serial.println("%)");
    
    Serial.print("Algılanan Renk: ");
    Serial.println(algilananRenk);
    Serial.println("==================");
} 

