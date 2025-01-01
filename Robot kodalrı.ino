#include <WiFi.h>
#include <WebServer.h>
// Adafruit TCS34725 kütüphanesi dahil edilir (RGB renk sensörü için)
#include <Adafruit_TCS34725.h>
#include <QTRSensors.h>
#include <LiquidCrystal_I2C.h>
// Entegrasyon zamanı ve kazanç değerleri (Renk sensörü için tanımlanır)
#define TCS34725_INTEGRATIONTIME_700MS 0xF6 // Entegrasyon zamanı (verilerin toplanma süresi)
#define TCS34725_GAIN_1X 0x00 // Kazanç ayarı (düşük ışık için hassasiyet)

// LCD ayarları
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C adresi: 0x27

// Wi-Fi ayarları
const char* ssid = "WiFi_ADI";       // Wi-Fi adı
const char* password = "WiFi_SIFRESI"; // Wi-Fi şifresi

// Web sunucusu
WebServer server(80);

using System;
using System.Net.Http;
using System.Threading.Tasks;
using System.Windows.Forms;

public partial class MainForm : Form
{
    private static readonly HttpClient client = new HttpClient();

    public MainForm()
    {
        InitializeComponent();
    }

    private async void SendCommand(int hedef)
    {
        string url = $"http://ESP32_IP/istasyon";
        var values = new Dictionary<string, string> { { "hedef", hedef.ToString() } };
        var content = new FormUrlEncodedContent(values);

        try
        {
            var response = await client.PostAsync(url, content);
            if (response.IsSuccessStatusCode)
            {
                MessageBox.Show($"Hedef istasyon {hedef} ayarlandı!");
            }
            else
            {
                MessageBox.Show("İstek başarısız oldu!");
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Hata: {ex.Message}");
        }
    }

    private void buttonIstasyon1_Click(object sender, EventArgs e)
    {
        SendCommand(1);
    }

    private void buttonIstasyon2_Click(object sender, EventArgs e)
    {
        SendCommand(2);
    }

    private void buttonIstasyon3_Click(object sender, EventArgs e)
    {
        SendCommand(3);
    }

    private void buttonIstasyon4_Click(object sender, EventArgs e)
    {
        SendCommand(4);
    }

    private void buttonGiris_Click(object sender, EventArgs e)
    {
        SendCommand(0); // Giriş istasyonu
    }
}


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
int qtrPin[] = {9, 10, 11, 12, 13}; // Sensör pinleri
int sensorSayisi = 5;               // Sensör sayısı
int sensorDegerleri[5];             // Sensör değerlerini saklamak için dizi

// Motor ve sensör ayarları
int motorHizi = 150; // Motor hızı (PWM değeri)
int engelAlgila = 15; // Engel algılama mesafesi (cm)

// Adafruit TCS34725 nesnesi (Renk sensörü için)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

// Robotun başlangıç pozisyonu ve hedef istasyonu
int mevcutKonum = 0; // Robotun başlangıç konumunu 0 olarak tanımlıyoruz.
int hedef = 1; // Varsayılan olarak mevcut istasyon

void setup() {
  Serial.begin(115200);

  // Wi-Fi bağlantısını başlat
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Wi-Fi bağlantısı bekleniyor...");
  }
  Serial.println("Wi-Fi'ye bağlandı!");

  // Sunucuya gelen istekleri işle
  server.on("/istasyon", HTTP_POST, []() {
    if (server.hasArg("hedef")) {
      hedefIstasyon = server.arg("hedef").toInt();
      Serial.print("Hedef istasyon: ");
      Serial.println(hedefIstasyon);

      server.send(200, "text/plain", "Hedef istasyon ayarlandı!");
    } else {
      server.send(400, "text/plain", "Hedef parametresi eksik!");
    }
  });

  // Sunucuyu başlat
  server.begin();

  Serial.begin(9600);

  // LCD başlat
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Renk Algilaniyor");

  // Renk sensörünü başlat
  if (tcs.begin()) {
    Serial.println("TCS34725 baglandi.");
  } else {
    Serial.println("TCS34725 baglanamadi!");
    while (1); // Hata varsa dur
  }
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

 
  Serial.begin(9600);

}

void loop() {
  uint16_t r, g, b, c;
  float redNorm, greenNorm, blueNorm;

  // Renk sensöründen değerleri oku
  tcs.getRawData(&r, &g, &b, &c);

  // Normalize edilmiş RGB değerlerini hesapla
  redNorm = (float)r / c * 255.0;
  greenNorm = (float)g / c * 255.0;
  blueNorm = (float)b / c * 255.0;

  // Algılanan rengi belirle
  String detectedColor = detectColor(redNorm, greenNorm, blueNorm);

  // LCD üzerinde göster
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Renk: ");
  lcd.setCursor(6, 0);
  lcd.print(detectedColor);

  delay(1000); // 1 saniye bekle
  
  server.handleClient();

  // Robot hareket mantığı burada yazılacak
  if (hedefIstasyon != 0) {
    Serial.print("Robot, istasyon ");
    Serial.print(hedefIstasyon);
    Serial.println(" yönünde hareket ediyor...");
    // Hedef istasyona gitme kodlarını buraya ekleyin
    delay(5000); // Simülasyon için bekleme
    hedefIstasyon = 0; // Hedefe ulaştıktan sonra sıfırla
  }
  
  // Hedef istasyon bilgisi kontrol edilir
  if (hedef != mevcutKonum) {
  istasyonaGit(mevcutKonum, hedef);  // Hedef istasyona gitmek için fonksiyon çağrılır
  }

  delay(100); // Döngüyü stabilize etmek için gecikme
}

// Robotun istasyona gitmesini sağlayan fonksiyon
void istasyonaGit(int mevcutKonum, int hedef) {
  // Robot başlangıç pozisyonundaysa (istasyon dışı)
  if (mevcutKonum == 0) {
    if (hedef == 1) {
      cizgiTakip("mavi");
    } else if (hedef == 2) {
      cizgiTakip("mavi");
      cizgiTakip("sari");
    } else if (hedef == 3) {
      cizgiTakip("mavi");
      cizgiTakip("sari");
      cizgiTakip("yesil");
    } else if (hedef == 4) {
      cizgiTakip("mavi");
      cizgiTakip("sari");
      cizgiTakip("yesil");
      cizgiTakip("turuncu");
    }
  } else {
    // Robot bir istasyondaysa
    if (mevcutKonum == 1 && hedef == 2) {
      cizgiTakip("sari");
    } else if (mevcutKonum == 2 && hedef == 3) {
      cizgiTakip("yesil");
    } else if (mevcutKonum == 3 && hedef == 4) {
      cizgiTakip("turuncu");
    } else if (mevcutKonum == 4 && hedef == 1) {
      cizgiTakip("mavi");
    } else if (mevcutKonum == 1 && hedef == 3) {
      cizgiTakip("sari");
      cizgiTakip("yesil");
    } else if (mevcutKonum == 2 && hedef == 4) {
      cizgiTakip("yesil");
      cizgiTakip("turuncu");
    } else if (mevcutKonum == 3 && hedef == 1) {
      cizgiTakip("turuncu");
      cizgiTakip("mavi");
    } else if (mevcutKonum == 4 && hedef == 2) {
      cizgiTakip("mavi");
      cizgiTakip("sari");
    }
  }

  // Hedefe ulaşıldığında mevcut konumu güncelle
  ::mevcutKonum = hedef;
}

void cizgiTakip(String hedefRenk) {
    while (true) { // Çizgi üzerinde kalmak için sürekli kontrol
        String algilananRenk = renkAlgila(); // Mevcut algılanan rengi oku
        
        if (algilananRenk == "Hata") {
            Serial.println("Hata: Renk algılanamadı, robot durduruluyor.");
            motorControl(0, 0); // Motorları durdur
            return; // Hatalı durumda fonksiyondan çık
        }

        if (algilananRenk == hedefRenk) {
            // Hedef renkteyiz, düz ilerle
            motorControl(motorHizi, motorHizi);
        } else if (algilananRenk == "Bilinmeyen") {
            // Hedef renkten saptı, motorları durdur ve yeniden algıla
            Serial.println("Renk bilinmiyor, tekrar algılama yapılıyor.");
            motorControl(0, 0);
        } else {
            // Robot hedef renkten uzaklaşmış, düzeltici hareket yap
            if (hedefRenk == "Mavi" || hedefRenk == "Yeşil") {
                // Daha düşük renk tonları için sağa dönüş yap
                sagDonus();
            } else {
                // Daha yüksek renk tonları için sola dönüş yap
                solDonus();
            }
        }

        // Algılama süreci devam ederken bir miktar bekle
        delay(100); // Robotun tepki süresine göre bu değer ayarlanabilir
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

String renkAlgila() {
    uint16_t r, g, b, c; // Renk sensöründen gelen ham veriler için değişkenler
    tcs.getRawData(&r, &g, &b, &c); // Renk sensöründen ham değerleri oku

    // Toplam aydınlık kontrolü
    if (c == 0 || c < 100) { // Toplam renk değeri sıfır ya da düşükse hata mesajı döndür
        Serial.println("Hata: Toplam renk değeri düşük. Algılama yapılamadı.");
        return "Hata"; // Hata durumunda fonksiyondan çık
    }

    // Renk oranlarını hesapla (normalizasyon)
    float rNorm = (float)r / c; // Kırmızı renk oranı
    float gNorm = (float)g / c; // Yeşil renk oranı
    float bNorm = (float)b / c; // Mavi renk oranı

    // Seri monitöre renk oranlarını yazdır
    Serial.print("R: "); Serial.print(rNorm, 2); 
    Serial.print(" G: "); Serial.print(gNorm, 2); 
    Serial.print(" B: "); Serial.println(bNorm, 2);

    // Algılanan rengi belirle
    if (rNorm > 0.4 && gNorm > 0.4 && bNorm < 0.2) {
        Serial.println("Sarı renk algılandı!"); // Sarı renk
        return "Sarı";
    } else if (rNorm > 0.5 && gNorm < 0.3 && bNorm < 0.2) {
        Serial.println("Turuncu renk algılandı!"); // Turuncu renk
        return "Turuncu";
    } else if (bNorm > 0.5 && rNorm < 0.3 && gNorm < 0.3) {
        Serial.println("Mavi renk algılandı!"); // Mavi renk
        return "Mavi";
    } else if (gNorm > 0.5 && rNorm < 0.3 && bNorm < 0.3) {
        Serial.println("Yeşil renk algılandı!"); // Yeşil renk
        return "Yeşil";
    } else {
        Serial.println("Renk algılanamadı veya bilinmeyen renk."); // Tanımsız renk
        return "Bilinmeyen";
    }
}



void engelKontrol() {
  digitalWrite(uTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(uTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(uTrig, LOW);

  long sure = pulseIn(uEcho, HIGH, 30000); // Maksimum 30ms bekle
  if (sure == 0) return; // Yanıt yoksa fonksiyondan çık

  int mesafe = sure / 58.2;

  if (mesafe < engelAlgila) {
    Serial.println("Engel algılandı! Duruluyor...");
    motorControl(0, 0);
    while (true) {
      delay(500); // Engel durumu sürekli kontrol edilir
      digitalWrite(uTrig, LOW);
      delayMicroseconds(2);
      digitalWrite(uTrig, HIGH);
      delayMicroseconds(10);
      digitalWrite(uTrig, LOW);

      sure = pulseIn(uEcho, HIGH, 30000);
      if (sure == 0) continue;

      mesafe = sure / 58.2;
      if (mesafe > engelAlgila) {
        Serial.println("Engel kaldırıldı. Devam ediliyor...");
        break; // Engel kalktığında döngüden çık
      }
    }
  }
}

/ Renk algılama fonksiyonu
String detectColor(float r, float g, float b) {
  if (r > g && r > b) {
    return "Kirmizi";
  } else if (g > r && g > b) {
    return "Yesil";
  } else if (b > r && b > g) {
    return "Mavi";
  } else if (r > 200 && g > 200 && b > 200) {
    return "Beyaz";
  } else if (r < 50 && g < 50 && b < 50) {
    return "Siyah";
  } else {
    return "Bilinmeyen";
  }
}

