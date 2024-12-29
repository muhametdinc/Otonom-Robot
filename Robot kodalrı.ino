#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD ekran nesnesi (I2C adres: 0x27, satır: 2, sütun: 16)
LiquidCrystal_I2C lcd(0x27, 16, 2);


// Adafruit TCS34725 kütüphanesi dahil edilir (RGB renk sensörü için)
#include <Adafruit_TCS34725.h>

// Entegrasyon zamanı ve kazanç değerleri (Renk sensörü için tanımlanır)
#define TCS34725_INTEGRATIONTIME_700MS 0xF6 // Entegrasyon zamanı (verilerin toplanma süresi)
#define TCS34725_GAIN_1X 0x00 // Kazanç ayarı (düşük ışık için hassasiyet)

// Motor pinlerinin tanımlanması
int motorR1 = 3; // Sağ ön motor pini
int motorR2 = 4; // Sağ arka motor pini
int motorL1 = 5; // Sol ön motor pini
int motorL2 = 6; // Sol arka motor pini
int motorSurucu = 2; // Motor sürücü pini (aktif/deaktif kontrol)

// Ultrasonik sensör pinlerinin tanımlanması
const int uTrig = 7; // Ultrasonik sensör sinyal gönderme pini
const int uEcho = 8; // Ultrasonik sensör echo pini (yansıyan sinyali algılar)

// Çizgi takip sensörlerinin bağlı olduğu pinler
int qtrPin[] = {9, 10, 11, 12, 13}; // Çizgi sensör pinleri dizi olarak tanımlanır

// Motor ve sensör ayarları
int motorHizi = 150; // Motor hızı (PWM değeri)
int engelAlgila = 15; // Engel algılama mesafesi (cm)

// PID kontrol değişkenleri
float kp = 1.0;  // Oransal kazanç
float ki = 0.0;  // İntegral kazanç
float kd = 0.5;  // Türev kazanç
float integral = 0.0; // İntegral bileşeni için başlangıç değeri
float lastError = 0.0; // Önceki hata değeri (PID için)

// Adafruit TCS34725 nesnesi (Renk sensörü için)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

// Robotun başlangıç pozisyonu ve hedef istasyonu
int mevcutIstasyon = 1; // Robotun bulunduğu istasyon
int hedefIstasyon = 1; // Varsayılan olarak mevcut istasyon

void setup() {
  // Motor pinlerini çıkış olarak ayarla
  pinMode(motorR1, OUTPUT);
  pinMode(motorR2, OUTPUT);
  pinMode(motorL1, OUTPUT);
  pinMode(motorL2, OUTPUT);

  // Ultrasonik sensör pinlerini ayarla
  pinMode(uTrig, OUTPUT);
  pinMode(uEcho, INPUT);

  // Motor sürücü pinini aktif hale getir
  pinMode(motorSurucu, OUTPUT);
  digitalWrite(motorSurucu, HIGH); // Motor sürücüyü aktif eder

  // Çizgi sensör pinlerini giriş olarak ayarla
  for (int i = 0; i < 5; i++) {
    pinMode(qtrPin[i], INPUT);
  }

  // Renk sensörünün başlatılmasını kontrol et
  if (tcs.begin()) {
    Serial.println("TCS34725 Renk Sensörü algılandı."); // Sensör algılandıysa mesaj yazdır
  } else {
    Serial.println("TCS34725 algılanamadı. Bağlantıları kontrol edin."); // Sensör algılanamadıysa hata mesajı yazdır
    while (1); // Sonsuz döngüye girerek sistemi durdur
  }

  // LCD ekranı başlat
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Renk Sensoru...");
  delay(2000);
  lcd.clear();

  Serial.begin(9600);

}

void loop() {
  // Hedef istasyon bilgisi kontrol edilir
  if (hedefIstasyon != mevcutIstasyon) {
    istasyonaGit(hedefIstasyon); // Hedef istasyona gitmek için fonksiyon çağrılır
  }

  delay(100); // Döngüyü stabilize etmek için gecikme
}

void istasyonaGit(int hedef) {
  // İstasyonlar arasında hareket için izlenecek algoritma
  if (mevcutIstasyon == 1 && hedef == 2) {
    cizgiTakip("mavi");
    cizgiTakip("sari");
  } else if (mevcutIstasyon == 2 && hedef == 3) {
    cizgiTakip("sari");
    cizgiTakip("yesil");
  } else if (mevcutIstasyon == 3 && hedef == 4) {
    cizgiTakip("yesil");
    cizgiTakip("turuncu");
  } else if (mevcutIstasyon == 4 && hedef == 1) {
    cizgiTakip("turuncu");
    cizgiTakip("mavi");
  } else if (mevcutIstasyon == 1 && hedef == 3) {
    cizgiTakip("mavi");
    cizgiTakip("sari");
    cizgiTakip("yesil");
  } else if (mevcutIstasyon == 2 && hedef == 4) {
    cizgiTakip("sari");
    cizgiTakip("yesil");
    cizgiTakip("turuncu");
  } else if (mevcutIstasyon == 3 && hedef == 1) {
    cizgiTakip("yesil");
    cizgiTakip("turuncu");
    cizgiTakip("mavi");
  } else if (mevcutIstasyon == 4 && hedef == 2) {
    cizgiTakip("turuncu");
    cizgiTakip("mavi");
    cizgiTakip("sari");
  }

  mevcutIstasyon = hedef; // Yeni istasyonu mevcut olarak ayarla
}  

void cizgiTakip(String renk) {
  Serial.print(renk); // Takip edilen çizginin rengini yazdır
  Serial.println(" çizgisi takip ediliyor...");

  while (true) {
    int sensorValues[5]; // Çizgi sensör değerlerini saklamak için dizi
    for (int i = 0; i < 5; i++) {
      sensorValues[i] = digitalRead(qtrPin[i]); // Sensör değerlerini oku
    }

    // Hata hesaplama (PID için)
    float error = (sensorValues[0] * -2) + (sensorValues[1] * -1) + (sensorValues[3] * 1) + (sensorValues[4] * 2); // Sensörlerden gelen hatayı hesapla
    integral += error; // İntegral değerine hatayı ekle
    float derivative = error - lastError; // Türev bileşenini hesapla
    float pid = (kp * error) + (ki * integral) + (kd * derivative); // PID denklemini uygula
    lastError = error; // Son hatayı güncelle

    // Motor hızlarını PID ile ayarla
    int solHiz = motorHizi + pid; // Sol motor için hız
    int sagHiz = motorHizi - pid; // Sağ motor için hız

    motorControl(constrain(solHiz, 0, 255), constrain(sagHiz, 0, 255)); // Hızları sınırlayıp motor kontrol fonksiyonuna gönder

    // Çizgi kaybolursa dur
    if (sensorValues[0] == LOW && sensorValues[1] == LOW && sensorValues[2] == LOW && sensorValues[3] == LOW && sensorValues[4] == LOW) {
      motorControl(0, 0); // Motorları durdur
      break; // Döngüden çık
    }
  }
}

void motorControl(int solHiz, int sagHiz) {
  analogWrite(motorL1, solHiz); // Sol motor ileri hız
  analogWrite(motorL2, 0); // Sol motor geri kapalı
  analogWrite(motorR1, sagHiz); // Sağ motor ileri hız
  analogWrite(motorR2, 0); // Sağ motor geri kapalı
}

void sagDonus() {
  motorControl(motorHizi, 0); // Sol tekerleği hareket ettir, sağ tekerleği durdur
  delay(500); // Dönüş süresi, ihtiyaca göre ayarlanabilir
  motorControl(0, 0); // Dönüş sonrası dur
}

void solDonus() {
  motorControl(0, motorHizi); // Sağ tekerleği hareket ettir, sol tekerleği durdur
  delay(500); // Dönüş süresi, ihtiyaca göre ayarlanabilir
  motorControl(0, 0); // Dönüş sonrası dur
}

void renkAlgila() {
  uint16_t r, g, b, c; // Renk sensörü ham verileri için değişkenler
  tcs.getRawData(&r, &g, &b, &c); // Renk sensöründen ham değerleri oku

  float rNorm = (float)r / c; // Kırmızı renk oranı
  float gNorm = (float)g / c; // Yeşil renk oranı
  float bNorm = (float)b / c; // Mavi renk oranı

  Serial.print("R: "); Serial.print(rNorm, 2); // Kırmızı oranını yazdır
  Serial.print(" G: "); Serial.print(gNorm, 2); // Yeşil oranını yazdır
  Serial.print(" B: "); Serial.println(bNorm, 2); // Mavi oranını yazdır

  if (rNorm > 0.4 && gNorm > 0.4 && bNorm < 0.2) {
    Serial.println("Sarı renk algılandı!"); // Sarı renk algılandı
  } else if (rNorm > 0.5 && gNorm < 0.3 && bNorm < 0.2) {
    Serial.println("Turuncu renk algılandı!"); // Turuncu renk algılandı
  } else if (bNorm > 0.5 && rNorm < 0.3 && gNorm < 0.3) {
    Serial.println("Mavi renk algılandı!"); // Mavi renk algılandı
  } else if (gNorm > 0.5 && rNorm < 0.3 && bNorm < 0.3) {
    Serial.println("Yeşil renk algılandı!"); // Yeşil renk algılandı
  } else {
    Serial.println("Renk algılanamadı veya bilinmeyen renk."); // Renk tanımlanamadı
  }
}

void engelKontrol() {
  digitalWrite(uTrig, LOW); // Ultrasonik sensör trig pinini kapat
  delayMicroseconds(2); // 2 mikro saniye bekle
  digitalWrite(uTrig, HIGH); // Ultrasonik sensör trig pinini aç
  delayMicroseconds(10); // 10 mikro saniye sinyal gönder
  digitalWrite(uTrig, LOW); // Trig pinini kapat

  long sure = pulseIn(uEcho, HIGH); // Echo pini yüksek olduğunda geçen süreyi ölç
  int mesafe = sure / 58.2; // Ölçülen süreyi mesafeye çevir

  if (mesafe < engelAlgila) {
    Serial.println("Engel algılandı! Duruluyor..."); // Engel algılandığında mesaj yazdır
    motorControl(0, 0); // Motorları durdur
  }
}

void renkAlgila(float rNorm, float gNorm, float bNorm) {
  lcd.clear();
  lcd.setCursor(0, 0); // İlk satıra yaz

  // Algılanan renk bilgisi
  if (rNorm > 0.4 && gNorm < 0.3 && bNorm < 0.3) {
    lcd.print("KIRMIZI Algilandi");
  } else if (rNorm > 0.4 && gNorm > 0.4 && bNorm < 0.2) {
    lcd.print("SARI Algilandi ");
  } else if (bNorm > 0.5 && rNorm < 0.3 && gNorm < 0.3) {
    lcd.print("MAVI Algilandi ");
  } else if (gNorm > 0.5 && rNorm < 0.3 && bNorm < 0.3) {
    lcd.print("YESIL Algilandi");
  } else {
    lcd.print("Renk Algilanamadi");
  }

  // İkinci satıra RGB oranlarını yaz
  lcd.setCursor(0, 1);
  lcd.print("R: ");
  lcd.print(rNorm, 2);
  lcd.print(" G: ");
  lcd.print(gNorm, 2);
  lcd.print(" B: ");
  lcd.print(bNorm, 2);

  delay(1000); // Ekranda gösterim süresi
}
