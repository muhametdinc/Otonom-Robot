#include <Adafruit_TCS34725.h> // Adafruit TCS34725 kütüphanesi dahil edilir (RGB renk sensörü için)

// Entegrasyon zamanı ve kazanç değerleri (Renk sensörü için)
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

// Adafruit TCS34725 nesnesi (Renk sensörü için)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X); 

void setup() {
  // Motor pinlerini çıkış olarak ayarlama
  pinMode(motorR1, OUTPUT); 
  pinMode(motorR2, OUTPUT); 
  pinMode(motorL1, OUTPUT); 
  pinMode(motorL2, OUTPUT); 

  // Ultrasonik sensör pinlerini ayarlama
  pinMode(uTrig, OUTPUT); 
  pinMode(uEcho, INPUT); 

  // Motor sürücü pinini aktif hale getirme
  pinMode(motorSurucu, OUTPUT); 
  digitalWrite(motorSurucu, HIGH); // Motor sürücüyü aktif eder

  // Çizgi sensör pinlerini giriş olarak ayarlama
  for (int i = 0; i < 5; i++) { 
    pinMode(qtrPin[i], INPUT); 
  }

  
  Serial.begin(9600); 

  // Renk sensörünün başlatılmasını kontrol etme
  if (tcs.begin()) {
    Serial.println("TCS34725 Renk Sensörü algılandı."); // Sensör algılandı
  } else {
    Serial.println("TCS34725 algılanamadı. Bağlantıları kontrol edin."); // Sensör algılanmadı
    while (1); // Sensör algılanmazsa kod burada durur
  }
}

void loop() {
  // Mesafe ölçüm fonksiyonunu çağırarak mesafeyi hesapla
  int mesafe = mesafeOlc(); 
  Serial.print("Engel Mesafesi: "); 
  Serial.println(mesafe); 

  // Engel algılamayı kontrol et
  if (mesafe <= engelAlgila) { 
    motorStop(); // Engel algılandıysa motorları durdur
    Serial.println("Motorlar Durdu."); 
  } else { 
    motorMove(); // Engel yoksa motorları çalıştır
    Serial.println("Motorlar Çalışıyor."); 
  }

  // Renk algılama fonksiyonunu çağır
  renkAlgila();
}

void motorMove() {
  // Motorları ileri sürmek için gerekli pinlere HIGH sinyali gönder
  digitalWrite(motorR1, HIGH); 
  digitalWrite(motorR2, HIGH); 
  digitalWrite(motorL1, HIGH); 
  digitalWrite(motorL2, HIGH); 
}

void motorStop() {
  // Motorları durdurmak için gerekli pinlere LOW sinyali gönder
  digitalWrite(motorR1, LOW); 
  digitalWrite(motorR2, LOW); 
  digitalWrite(motorL1, LOW); 
  digitalWrite(motorL2, LOW); 
}

void sagDonus() {
  // Sağ dönüş için sağ motorları durdur, sol motorları çalıştır
  digitalWrite(motorR1, LOW); 
  digitalWrite(motorR2, LOW); 
  digitalWrite(motorL1, HIGH); 
  digitalWrite(motorL2, HIGH); 
}

void solDonus() {
  // Sol dönüş için sol motorları durdur, sağ motorları çalıştır
  digitalWrite(motorR1, HIGH); 
  digitalWrite(motorR2, HIGH); 
  digitalWrite(motorL1, LOW); 
  digitalWrite(motorL2, LOW); 
}

int mesafeOlc() {
  // Ultrasonik sensör ile mesafe ölçümü
  digitalWrite(uTrig, LOW); // Düşük sinyal gönder (başlatma öncesi)
  delayMicroseconds(2); 
  digitalWrite(uTrig, HIGH); // Sinyal gönder
  delayMicroseconds(10); 
  digitalWrite(uTrig, LOW); // Sinyali kapat

  // Echo pininden yansıyan sinyal süresini ölç
  long sure = pulseIn(uEcho, HIGH); 
  int mesafe = sure * 0.034 / 2; // Süreyi cm'ye dönüştür
  return mesafe; 
}

void renkAlgila() {
  // Renk sensöründen RGB değerlerini okuma
  uint16_t r, g, b, c; 
  tcs.getRawData(&r, &g, &b, &c); // Sensörden RGB ve toplam değerleri al

  // RGB değerlerini seri monitöre yazdır
  Serial.print("R: ");
  Serial.print(r);
  Serial.print(" G: ");
  Serial.print(g);
  Serial.print(" B: ");
  Serial.print(b);
  Serial.print(" C: ");
  Serial.println(c);

  // Renk algılama mantığı (örneğin, kırmızı algılama)
  if (r > g && r > b && r > 1000) { // Kırmızı renk eşiği
    Serial.println("Kırmızı renk algılandı!");
    motorStop(); // Kırmızı renk algılandıysa motorları durdur
  }
}

