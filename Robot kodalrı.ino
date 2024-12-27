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
    // 1'den 2'ye gitmek için mavi ve sarı çizgiyi takip et
    cizgiTakip("mavi");
    cizgiTakip("sari");
  } else if (mevcutIstasyon == 2 && hedef == 3) {
    // 2'den 3'e gitmek için sarı ve yeşil çizgiyi takip et
    cizgiTakip("sari");
    cizgiTakip("yesil");
  } else if (mevcutIstasyon == 3 && hedef == 4) {
    // 3'ten 4'e gitmek için yeşil ve turuncu çizgiyi takip et
    cizgiTakip("yesil");
    cizgiTakip("turuncu");
  } else if (mevcutIstasyon == 4 && hedef == 1) {
    // 4'ten 1'e gitmek için turuncu ve mavi çizgiyi takip et
    cizgiTakip("turuncu");
    cizgiTakip("mavi");
  }

  mevcutIstasyon = hedef; // Yeni istasyonu mevcut olarak ayarla
}  

void cizgiTakip(String renk) {
  // Belirtilen rengi takip eder
  Serial.print(renk);
  Serial.println(" çizgisi takip ediliyor...");

  // Çizgi takip algoritması burada tanımlanabilir
  // Çizgi sensörlerinden okuma yap ve motorları kontrol et
  // Detaylı kontrol için çizgi algılama algoritması entegre edilir

  delay(2000); // Geçici simülasyon gecikmesi (çizgi takibi simülasyonu)
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

void renkAlgila() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c); // Renk sensöründen ham değerleri oku

  // Normalizasyon
  float rNorm = (float)r / c;
  float gNorm = (float)g / c;
  float bNorm = (float)b / c;

  // Normalleştirilmiş RGB değerlerini seri monitöre yazdır
  Serial.print("R: "); Serial.print(rNorm, 2);
  Serial.print(" G: "); Serial.print(gNorm, 2);
  Serial.print(" B: "); Serial.println(bNorm, 2);

  // Renk algılama mantığı
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






