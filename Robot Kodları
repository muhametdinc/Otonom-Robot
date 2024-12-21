// Motor Sürücü Pinleri
int pwm1 = 5; // RPWM pinine bağlı, motorun sağa dönmesini sağlar
int pwm2 = 6; // LPWM pinine bağlı, motorun sola dönmesini sağlar
int enablePin = 7; // Motor sürücüyü aktif hale getiren pin

// Ultrasonik Sensör Pinleri
int trigPin = 2; // Ultrasonik sensörün trig (tetikleme) pini
int echoPin = 3; // Ultrasonik sensörün echo (yansıma) pini

// Çizgi Takip Sensörü (QTR-8RC) Pinleri
int qtrPins[] = {8, 9, 10, 11, 12,}; // Çizgi sensörlerinin bağlı olduğu pinler

// RGB Renk Sensörü Pinleri
int sdaPin = A4; // I2C veri (data) pini
int sclPin = A5; // I2C saat (clock) pini

// Wi-Fi Modülü Pinleri
int espRx = 18; // Wi-Fi modülünün RX (alıcı) pini
int espTx = 19; // Wi-Fi modülünün TX (verici) pini

// Genel Değişkenler
int motorSpeed = 150; // Motor hızı (0-255 arasında değer alır)
int obstacleDistance = 15; // Engel algılama mesafesi (cm)

void setup() {
  // Motor Sürücü Ayarları
  pinMode(pwm1, OUTPUT); // pwm1 pini çıkış olarak ayarlandı
  pinMode(pwm2, OUTPUT); // pwm2 pini çıkış olarak ayarlandı
  pinMode(enablePin, OUTPUT); // enablePin pini çıkış olarak ayarlandı
  digitalWrite(enablePin, HIGH); // Motor sürücü aktif hale getirildi

  // Ultrasonik Sensör Ayarları
  pinMode(trigPin, OUTPUT); // trigPin pini çıkış olarak ayarlandı
  pinMode(echoPin, INPUT); // echoPin pini giriş olarak ayarlandı

  // Çizgi Takip Sensörü Ayarları
  for (int i = 0; i < 5; i++) { 
    pinMode(qtrPins[i], INPUT); // Çizgi sensörü pinleri giriş olarak ayarlandı
  }

  // Serial Port Ayarları
  Serial.begin(9600); // Seri haberleşme başlatıldı
}

void loop() {
  // Ultrasonik Sensör Mesafe Ölçümü
  int distance = measureDistance(); // measureDistance fonksiyonu çağrılarak mesafe ölçülür
  Serial.print("Engel Mesafesi: "); // Mesafeyi terminalde yazdırmak için
  Serial.println(distance); // Ölçülen mesafe terminale yazdırılır

  // Engel Kontrolü
  if (distance <= obstacleDistance) { // Eğer engel mesafesi belirlenen sınırdan küçükse
    stopMotors(); // Motorları durdur
    Serial.println("Engel Algılandı. Motorlar durduruldu."); // Terminale bilgi yazdır
  } else { 
    moveForward(); // Engel yoksa motorları ileri hareket ettir
    Serial.println("Motorlar Çalışıyor."); // Terminale bilgi yazdır
  }

  delay(100); // 100 ms bekleme
}

void moveForward() {
  analogWrite(pwm1, motorSpeed); // pwm1 pinine motor hızı kadar PWM sinyali gönder
  analogWrite(pwm2, motorSpeed); // pwm2 pinine motor hızı kadar PWM sinyali gönder
}

void stopMotors() {
  analogWrite(pwm1, 0); // pwm1 pinine 0 gönder (motor durur)
  analogWrite(pwm2, 0); // pwm2 pinine 0 gönder (motor durur)
}

int measureDistance() {
  digitalWrite(trigPin, LOW); // Trig pinini LOW yap (sensörü sıfırlamak için)
  delayMicroseconds(2); // 2 mikro saniye bekleme
  digitalWrite(trigPin, HIGH); // Trig pinini HIGH yap (sensörü tetiklemek için)
  delayMicroseconds(10); // 10 mikro saniye bekleme
  digitalWrite(trigPin, LOW); // Trig pinini tekrar LOW yap

  long duration = pulseIn(echoPin, HIGH); // Echo pininden gelen sinyal süresini ölç
  int distance = duration * 0.034 / 2; // Süreyi mesafeye dönüştür (cm cinsinden)
  return distance; // Ölçülen mesafeyi geri döndür
}
