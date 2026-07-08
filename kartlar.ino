#include <WiFi.h>
#include <WiFiUdp.h>
#include <Deneyap_OLED.h>
#include <Keypad.h>

OLED OLED;

const char* ssid = ""; //Wi-Fi adiniz
const char* password = ""; //Wi-Fi sifreniz
const char* hedefIP = ""; /* hedefIP: Karsi tarafin (alicinin) IP adresi
                                           - Kart_1'e yukleyeceginiz kodda buraya Kart_2'nin IP adresini yazin
                                           - Kart_2'ye yukleyeceginiz kodda buraya Kart_1'in IP adresini yazin
                                           (Ornegin: 192.168.1.101) */
const int udpPort = 4210;
WiFiUDP udp;
char buffer[255];

String mesaj = "";  // basilan tuslarin biriktigi string
String benimIP = "";

// Eski tus telefon mantigi (multi-tap) icin harf gruplari
const char* harfGruplari[10] = {
  " ",      // 0 -> bosluk
  "1",      // 1 -> sadece rakam
  "abc2",   // 2 -> a b c 2
  "def3",   // 3 -> d e f 3
  "ghi4",   // 4 -> g h i 4
  "jkl5",   // 5 -> j k l 5
  "mno6",   // 6 -> m n o 6
  "pqrs7",  // 7 -> p q r s 7
  "tuv8",   // 8 -> t u v 8
  "wxyz9"   // 9 -> w x y z 9
};

char sonTus = 0;              // en son basilan rakam tusu
int tusSayaci = 0;            // ayni tusa kac kez ust uste basildigi
unsigned long sonBasmaZamani = 0;
const unsigned long DONGU_ZAMAN_ASIMI = 2000;  // ms - ayni tusa tekrar basip harf degistirme penceresi
const unsigned long KAYIT_ZAMAN_ASIMI = 3000;  // ms - hicbir tusa basilmazsa onizlemedeki harf otomatik dizeye eklenir
char onizleme = 0;            // henuz mesaja eklenmemis, onizlemedeki harf
int sonGosterilenSaniye = -1; // geri sayimda en son ekrana yazilan saniye (gereksiz yeniden yazmayi onler)

// Durum yazilarinin (Kaydedildi/Gonderildi/Silindi) ekranda kalma suresi ve takibi
unsigned long durumMesajiZamani = 0;
bool durumMesajiAktif = false;
int durumTuru = 1;  // 1: Gonderildi/Kaydedildi (normal sure), 2: Silindi (uzun sure)
const unsigned long DURUM_MESAJI_SURESI = 1500;      // ms - Kaydedildi / Gonderildi
const unsigned long SILINDI_MESAJI_SURESI = 2500;    // ms - Silindi biraz daha uzun gorunsun

// Durum turune gore bekleme suresini dondurur
unsigned long durumMesajiSuresiAl() {
  if (durumTuru == 2) return SILINDI_MESAJI_SURESI;
  return DURUM_MESAJI_SURESI;
}

// LED pinleri
const int LED_YESIL = D14;   // mesaj gonderilince yanar
const int LED_KIRMIZI = D15; // mesaj gelince yanar

// Keypad ayarlari
const byte satir = 4;
const byte sutun = 3;

char keys[satir][sutun] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte satirPin[satir] = { D0, D1, D4, D8 };
byte sutunPin[sutun] = { D9, D12, D13 };

Keypad keypad = Keypad(makeKeymap(keys), satirPin, sutunPin, satir, sutun);

// --- Satir duzeni (8 satir: 0-7) ---
// Satir 0: durum yazisi / sayac / Kaydedildi / Gonderildi / Silindi / Yanit
// Satir 1: yazilan mesaj 
// Satir 2: bos - ayrac
// Satir 3: bos - ayrac
// Satir 4: "Benim adresim:"
// Satir 5: <benimIP>
// Satir 6: "Hedef adres:"
// Satir 7: <hedefIP>

const int EKRAN_GENISLIK = 16;  // 128px genislikte bir satira sigan karakter sayisi (fontunuza gore degisebilir)

void satirYaz(int satirNo, String metin) {
  while (metin.length() < EKRAN_GENISLIK) metin += ' ';  // eski karakterlerin uzerini bosluklarla kapat
  if (metin.length() > EKRAN_GENISLIK) metin = metin.substring(0, EKRAN_GENISLIK);
  OLED.setTextXY(satirNo, 0);
  OLED.putString(metin.c_str());
}

// Tek bir metni ilgili satira yazar (uzunsa kirpar, kisaysa bosluklarla tamamlar)
void ipSatiriYaz(int satirNo, String metin) {
  satirYaz(satirNo, metin);
}

// Alt kisimda benim IP ve hedef IP'yi kalici olarak, ayri satirlarda gosterir
void ipSatirlariniYaz() {
  ipSatiriYaz(4, "Benim adresim:");
  ipSatiriYaz(5, benimIP);
  ipSatiriYaz(6, "Hedef adres:");
  ipSatiriYaz(7, String(hedefIP));
}

// Durum/bilgi yazisi
void durumYaz(String metin) {
  satirYaz(0, metin);
}

// Yazilan mesaj 
void mesajYaz(String metin) {
  satirYaz(1, metin);
}

void varsayilanEkraniGoster() {
  durumYaz("Mesajlasmak icin");
  mesajYaz("Tusa basin");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_YESIL, OUTPUT);
  pinMode(LED_KIRMIZI, OUTPUT);
  digitalWrite(LED_YESIL, LOW);
  digitalWrite(LED_KIRMIZI, LOW);

  OLED.begin(0x7A);
  OLED.clearDisplay();
  OLED.setTextXY(0, 0);
  OLED.putString("Baslatiliyor...");

  WiFi.begin(ssid, password);
  Serial.print("WiFi baglaniyor");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Baglandi! Kart A IP adresi: ");
  Serial.println(WiFi.localIP());

  benimIP = WiFi.localIP().toString();

  OLED.clearDisplay();
  OLED.setTextXY(0, 0);
  OLED.putString("WiFi Baglandi!");
  OLED.setTextXY(2, 0);
  OLED.putString(benimIP.c_str());
  delay(1000);

  udp.begin(udpPort);

  OLED.clearDisplay();
  delay(50);              // ekranin temizlenmesi icin kisa bir bekleme
  satirYaz(1, "");
  satirYaz(3, "");
  ipSatirlariniYaz();
  varsayilanEkraniGoster();  
}

void ekraniGuncelle() {
  if (onizleme != 0) {
    // Sayac bilgisi: hangi tusa, kacinci kez basildigi (ornek: 3->1/4)
    const char* grup = harfGruplari[sonTus - '0'];
    int uzunluk = strlen(grup);
    int mevcutSira = (tusSayaci % uzunluk) + 1;
    String sayac = String(sonTus) + "->" + String(mevcutSira) + "/" + String(uzunluk);
    durumYaz(sayac);
  } else {
    durumYaz("");
  }

  String gosterilecek = mesaj;
  if (onizleme != 0) gosterilecek += onizleme;  // henuz onaylanmamis harfi de goster
  mesajYaz(gosterilecek);
}

void onizlemeyiOnayla() {
  if (onizleme != 0) {
    mesaj += onizleme;
    onizleme = 0;
    sonTus = 0;
    tusSayaci = 0;
  }
}

void geriSil() {
  if (onizleme != 0) {
    // Onaylanmamis onizlemedeki harf varsa, kaydetmeden iptal et
    onizleme = 0;
    sonTus = 0;
    tusSayaci = 0;
  } else if (mesaj.length() > 0) {
    // Onizleme yoksa, mesajin son karakterini sil
    mesaj.remove(mesaj.length() - 1);
  }
  durumTuru = 2;          
  durumYaz("Silindi");    // silme islemi yapildiginda kullaniciya satir 0'da bilgi verilir
  sonGosterilenSaniye = -1;
  durumMesajiAktif = true;
  durumMesajiZamani = millis();
  ekraniGuncelle();
}

void loop() {
  unsigned long simdi = millis();

  // Zaman asimi: hicbir tusa basilmazsa geri sayim gosterilir, sure dolunca otomatik kaydedilir
  if (onizleme != 0) {
    unsigned long gecenSure = simdi - sonBasmaZamani;

    if (gecenSure >= KAYIT_ZAMAN_ASIMI) {
      onizlemeyiOnayla();
      durumTuru = 1;
      durumYaz("Kaydedildi");
      sonGosterilenSaniye = -1;
      durumMesajiAktif = true;
      durumMesajiZamani = simdi;
      ekraniGuncelle();
    } else {
      int kalanSaniye = 3 - (gecenSure / 1000);  // 3, 2, 1
      if (kalanSaniye != sonGosterilenSaniye) {
        const char* grup = harfGruplari[sonTus - '0'];
        int uzunluk = strlen(grup);
        int mevcutSira = (tusSayaci % uzunluk) + 1;
        String sayacYazisi = String(sonTus) + "->" + String(mevcutSira) + "/" + String(uzunluk) + " " + String(kalanSaniye);
        durumYaz(sayacYazisi);
        sonGosterilenSaniye = kalanSaniye;
      }
    }
  }


  if (durumMesajiAktif && (simdi - durumMesajiZamani >= durumMesajiSuresiAl())) {
    varsayilanEkraniGoster();
    durumMesajiAktif = false;
  }

  // --- 1) Tus okuma ve gonderme kismi ---
  char key = keypad.getKey();

  if (key) {
    Serial.print("Tusa basildi: ");
    Serial.println(key);

    if (key == '#') {
      onizlemeyiOnayla();  // gonderilmeden once bekleyen harfi onayla

      if (mesaj.length() > 0) {
        udp.beginPacket(hedefIP, udpPort);
        udp.print(mesaj);
        udp.endPacket();

        Serial.print("Gonderildi -> ");
        Serial.println(mesaj);

        // Mesaj gonderilince yesil LED yak
        digitalWrite(LED_YESIL, HIGH);

        durumTuru = 1;
        durumYaz("Mesaj Gonderildi");
        mesajYaz(mesaj);   // gonderilen mesaj kisa sure gosterilir

        mesaj = "";        // string icerigi temizleniyor

        delay(2500);
        digitalWrite(LED_YESIL, LOW);

        // Bir sure sonra varsayilan ekrana ("Mesajlasmak icin" / "Tusa basin") donmesi icin durum zamani baslatiliyor
        durumMesajiAktif = true;
        durumMesajiZamani = millis();
      }
    }
    else if (key == '*') {
      // Yildiz: son karakteri siler
      geriSil();
    }
    else if (key >= '0' && key <= '9') {
      bool ayniTusZamanIcinde = (key == sonTus) && (simdi - sonBasmaZamani <= DONGU_ZAMAN_ASIMI);

      if (ayniTusZamanIcinde) {
        // Ayni tusa ust uste basildi -> siradaki harfe gec
        tusSayaci++;
      } else {
        // Farkli bir tusa basildi -> onceki onizlemedeki harfi onayla, yeni tusla basla
        onizlemeyiOnayla();
        sonTus = key;
        tusSayaci = 0;
      }

      const char* grup = harfGruplari[key - '0'];
      int uzunluk = strlen(grup);
      onizleme = grup[tusSayaci % uzunluk];
      sonBasmaZamani = simdi;

      sonGosterilenSaniye = -1;
      durumMesajiAktif = false;

      ekraniGuncelle();
    }
  }

  // --- 2) Karsi taraftan gelen mesaji dinleme kismi ---
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(buffer, 255);
    if (len > 0) {
      buffer[len] = 0;
    }

    Serial.print("Yanit Geldi: ");
    Serial.println(buffer);
    Serial.print("Gonderen IP: ");
    Serial.println(udp.remoteIP());

    // Mesaj gelince kirmizi LED yak
    digitalWrite(LED_KIRMIZI, HIGH);

    durumYaz("Gelen Mesaj: ");
    mesajYaz(String(buffer));

    delay(2500);
    digitalWrite(LED_KIRMIZI, LOW);

    varsayilanEkraniGoster();
  }
}