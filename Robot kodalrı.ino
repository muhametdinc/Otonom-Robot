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
float integral = 0.0;
float lastError = 0.0;

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

  // Seri haberleşmeyi başlat
  Serial.begin(9600);

  // Renk sensörünün başlatılmasını kontrol et
  if (tcs.begin()) {
    Serial.println("TCS34725 Renk Sensörü algılandı.");
  } else {
    Serial.println("TCS34725 algılanamadı. Bağlantıları kontrol edin.");
    while (1);
  }
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
  Serial.print(renk);
  Serial.println(" çizgisi takip ediliyor...");

  while (true) {
    int sensorValues[5];
    for (int i = 0; i < 5; i++) {
      sensorValues[i] = digitalRead(qtrPin[i]);
    }

    // Hata hesaplama (PID için)
    float error = (sensorValues[0] * -2) + (sensorValues[1] * -1) + (sensorValues[3] * 1) + (sensorValues[4] * 2);
    integral += error;
    float derivative = error - lastError;
    float pid = (kp * error) + (ki * integral) + (kd * derivative);
    lastError = error;

    // Motor hızlarını PID ile ayarla
    int solHiz = motorHizi + pid;
    int sagHiz = motorHizi - pid;

    motorControl(constrain(solHiz, 0, 255), constrain(sagHiz, 0, 255));

    // Çizgi kaybolursa dur
    if (sensorValues[0] == LOW && sensorValues[1] == LOW && sensorValues[2] == LOW && sensorValues[3] == LOW && sensorValues[4] == LOW) {
      motorControl(0, 0);
      break;
    }
  }
}

void motorControl(int solHiz, int sagHiz) {
  analogWrite(motorL1, solHiz);
  analogWrite(motorL2, 0);
  analogWrite(motorR1, sagHiz);
  analogWrite(motorR2, 0);
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
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c); // Renk sensöründen ham değerleri oku

  float rNorm = (float)r / c;
  float gNorm = (float)g / c;
  float bNorm = (float)b / c;

  Serial.print("R: "); Serial.print(rNorm, 2);
  Serial.print(" G: "); Serial.print(gNorm, 2);
  Serial.print(" B: "); Serial.println(bNorm, 2);

  if (rNorm > 0.4 && gNorm > 0.4 && bNorm < 0.2) {
    Serial.println("Sarı renk algılandı!");
  } else if (rNorm > 0.5 && gNorm < 0.3 && bNorm < 0.2) {
    Serial.println("Turuncu renk algılandı!");
  } else if (bNorm > 0.5 && rNorm < 0.3 && gNorm < 0.3) {
    Serial.println("Mavi renk algılandı!");
  } else if (gNorm > 0.5 && rNorm < 0.3 && bNorm < 0.3) {
    Serial.println("Yeşil renk algılandı!");
  } else {
    Serial.println("Renk algılanamadı veya bilinmeyen renk.");
  }
}

void engelKontrol() {
  digitalWrite(uTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(uTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(uTrig, LOW);

  long sure = pulseIn(uEcho, HIGH);
  int mesafe = sure / 58.2;

  if (mesafe < engelAlgila) {
    Serial.println("Engel algılandı! Duruluyor...");
    motorControl(0, 0);
  }
}

