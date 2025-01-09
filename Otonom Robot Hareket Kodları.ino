// ========================= KÜTÜPHANE TANIMLARI =========================
// Arduino'nun temel fonksiyonlarını içeren ana kütüphane - Dijital/Analog pin kontrolü, seri haberleşme vb. için gerekli
#include <Arduino.h>                

// Adafruit TCS34725 renk sensörü kütüphanesi - RGB renk değerlerini okumak ve renk tespiti yapmak için kullanılır
#include <Adafruit_TCS34725.h>     

// QTR sensör kütüphanesi - Çizgi takibi için kullanılan kızılötesi sensörlerin kontrolünü sağlar
#include <QTRSensors.h>            
#include <Wire.h>                  // I2C haberleşme için gerekli

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

// İstasyon yapısı tanımı - En üste eklenecek
struct Istasyon {
    int numara;
    String durmaRengi;
    int qtrEsik;
    bool metalVarMi;
};

// İstasyon dizisi tanımı
Istasyon istasyonlar[] = {
    {1, "yellow", 800, true},
    {2, "green", 800, true},
    {3, "orange", 800, true},
    {4, "red", 800, true}
};

// Setup fonksiyonu - Program başladığında bir kez çalışır
void setup() {
    // Seri haberleşmeyi başlat
    Serial.begin(115200);  
    
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
    if (!tcs.begin()) {
        Serial.println("HATA: TCS34725 sensörü bulunamadı!");
        while (1) {
            delay(1000);
            Serial.println("Lütfen bağlantıları kontrol edin");
        }
    }
    
    Serial.println("TCS34725 Renk Sensörü başarıyla başlatıldı");
    Serial.println("Robot hazır.");
}

// Ana döngü - Program çalıştığı sürece tekrar eder
void loop() {
    // Renk okuma zamanlaması için değişkenler
    static unsigned long sonRenkOkuma = 0;
    const unsigned long RENK_OKUMA_ARALIGI = 100;  // Her 100ms'de bir renk oku
    
    // Belirli aralıklarla renk okuması yap
    if (millis() - sonRenkOkuma >= RENK_OKUMA_ARALIGI) {
        // Renk değerlerini tutacak değişkenler
        uint16_t r, g, b, c;
        float redNorm, greenNorm, blueNorm;
        
        // Renk sensöründen ham verileri oku
        tcs.getRawData(&r, &g, &b, &c);
        
        // Renk değerlerini normalize et (0-255 aralığına)
        if (c > 0) {  // Sıfıra bölme kontrolü
            redNorm = (float)r / c * 255.0;
            greenNorm = (float)g / c * 255.0;
            blueNorm = (float)b / c * 255.0;
            
            // Rengi tespit et ve bilgiyi yazdır
            String detectedColor = detectColor(redNorm, greenNorm, blueNorm);
            Serial.print("Algılanan Renk: ");
            Serial.print(detectedColor);
            Serial.print(" (R:");
            Serial.print(redNorm);
            Serial.print(" G:");
            Serial.print(greenNorm);
            Serial.print(" B:");
            Serial.print(blueNorm);
            Serial.println(")");
        }
        
        // Son okuma zamanını güncelle
        sonRenkOkuma = millis();
    }

    // Engel kontrolü yap
    engelKontrol();

    // Hedef kontrolü ve hareket
    if (hedef != mevcutKonum) {
        istasyonaGit(mevcutKonum, hedef);
    }

    // Ana döngü gecikmesi - İşlemciyi rahatlatmak için
    delay(10);
}

// İstasyonlar arası hareket fonksiyonu
void istasyonaGit(int baslangic, int bitis) {
    // Geçerli istasyon numarası kontrolü
    if (baslangic < 0 || bitis < 0 || baslangic > 4 || bitis > 4) {
        Serial.println("HATA: Geçersiz istasyon numarası!");
        return;
    }

    // Rota planlaması - Her istasyon çifti için takip ve durma renkleri
    struct {
        String takip;  // Takip edilecek renk
        String durma;  // Durulacak renk
    } rotalar[5][5] = {
        // [başlangıç][hedef] = {takip rengi, durma rengi}
        {{}, {"blue", "yellow"}, {"blue", "green"}, {"blue", "orange"}, {"blue", "red"}},
        {{"yellow", "blue"}, {}, {"yellow", "green"}, {"yellow", "orange"}, {"yellow", "red"}},
        {{"green", "blue"}, {"green", "yellow"}, {}, {"green", "orange"}, {"green", "red"}},
        {{"orange", "blue"}, {"orange", "yellow"}, {"orange", "green"}, {}, {"orange", "red"}},
        {{"red", "blue"}, {"red", "yellow"}, {"red", "green"}, {"red", "orange"}, {}}
    };

    // Rota bilgilerini al
    String takipRenk = rotalar[baslangic][bitis].takip;
    String durmaRenk = rotalar[baslangic][bitis].durma;
    
    // Geçerli rota kontrolü
    if (takipRenk.length() > 0 && durmaRenk.length() > 0) {
        Serial.print("Rota başlatılıyor: ");
        Serial.print(takipRenk);
        Serial.print(" rengini takip et, ");
        Serial.print(durmaRenk);
        Serial.println(" renginde dur");
        cizgiTakipRenk(takipRenk, durmaRenk);
    } else {
        Serial.println("HATA: Geçersiz rota!");
        return;
    }
    
    // Hedef istasyona varıldığını kaydet
    mevcutKonum = bitis;
    Serial.print("Hedef istasyona ulaşıldı: ");
    Serial.println(bitis);
}

// Renk takip fonksiyonu - Belirli bir rengi takip ederek hedefe ulaşır
void cizgiTakipRenk(String takipRenk, String durmaRenk) {
    const unsigned long ZAMAN_ASIMI = 30000;  // 30 saniye zaman aşımı süresi
    unsigned long baslangicZamani = millis();
    
    // Zaman aşımına ulaşana kadar çalış
    while (millis() - baslangicZamani < ZAMAN_ASIMI) {
        // Renk değerlerini oku
        uint16_t r, g, b, c;
        tcs.getRawData(&r, &g, &b, &c);

        // Geçersiz okuma kontrolü
        if (c == 0) continue;

        // Renk değerlerini normalize et
        float redNorm = (float)r / c * 255.0;
        float greenNorm = (float)g / c * 255.0;
        float blueNorm = (float)b / c * 255.0;

        // Algılanan rengi tespit et
        String algilananRenk = detectColor(redNorm, greenNorm, blueNorm);

        // Durma rengi kontrolü
        if (algilananRenk == durmaRenk) {
            motorControl(0, 0);  // Motorları durdur
            Serial.println("Durma rengi algılandı. Robot durduruluyor.");
            return;
        }

        // Çizgi takibi
        if (algilananRenk == takipRenk) {
            motorControl(motorHizi, motorHizi);  // Düz git
        } else {
            // Renk kaybedildiğinde arama stratejisi
            motorControl(motorHizi/2, motorHizi);  // Sağa dön
            delay(100);
            if (algilananRenk != takipRenk) {
                motorControl(motorHizi, motorHizi/2);  // Sola dön
                delay(200);
            }
        }
        
        delay(10);  // Kısa bekleme
    }
    
    // Zaman aşımı durumunda
    motorControl(0, 0);  // Motorları durdur
    Serial.println("UYARI: Çizgi takibinde zaman aşımı!");
}

// Motor kontrol fonksiyonu - Motorların hız ve yönünü ayarlar
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

// Yumuşak durma fonksiyonu
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

// Engel algılama ve kontrol fonksiyonu
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

// Tek ultrasonik ölçüm fonksiyonu
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
void rgbToHsv(float r, float g, float b, float& h, float& s, float& v) {
    // RGB değerlerini 0-1 aralığına normalize et
    r /= 255.0f;
    g /= 255.0f;
    b /= 255.0f;
    
    // Minimum ve maksimum değerleri bul
    float minVal = min(r, min(g, b));
    float maxVal = max(r, max(g, b));
    float delta = maxVal - minVal;
    
    // Parlaklık değeri
    v = maxVal;
    // Doygunluk değeri
    s = maxVal == 0 ? 0 : delta / maxVal;
    
    // Ton değeri hesaplama
    if (delta == 0) {
        h = 0;  // Gri tonları için ton değeri 0
    } else {
        // Renk tonunu hesapla
        if (maxVal == r) {
            h = 60.0f * fmod(((g - b) / delta), 6.0f);
        } else if (maxVal == g) {
            h = 60.0f * (((b - r) / delta) + 2.0f);
        } else {
            h = 60.0f * (((r - g) / delta) + 4.0f);
        }
        
        // Negatif değerleri düzelt
        if (h < 0) h += 360.0f;
    }
}

// Renk tespit fonksiyonu - RGB değerlerinden renk ismini belirler
String detectColor(float r, float g, float b) {
    float h, s, v;
    rgbToHsv(r, g, b, h, s, v);  // RGB'den HSV'ye dönüşüm

    // Düşük parlaklık ve doygunluk kontrolü
    if (v < 0.15) return "unknown";  // Çok karanlık
    if (s < 0.20) return "unknown";  // Çok az doygunluk (beyaz/gri/siyah)

    // Renk eşiklerini kontrol et
    for (const auto& renk : RENK_ESIKLERI) {
        if (h >= renk.hueMin && h <= renk.hueMax) {
            return renk.name;  // Eşleşen rengi döndür
        }
    }

    return "unknown";  // Eşleşme bulunamazsa bilinmeyen renk
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

// İstasyon kontrolü - Doğru istasyonda mıyız?
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
int getQTRValue() {
    int toplam = 0;
    for(int i = 0; i < sensorSayisi; i++) {
        toplam += analogRead(qtrPin[i]);
    }
    return toplam / sensorSayisi;
}

// Renk sensörü bağlantı kontrolü geliştirmesi
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

// Renk algılama fonksiyonu
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
