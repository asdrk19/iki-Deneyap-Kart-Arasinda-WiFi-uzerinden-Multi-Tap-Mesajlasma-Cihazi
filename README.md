# İki Deneyap Kart Arasında WiFi Üzerinden Multi-Tap Mesajlaşma Cihazı

İki adet **Deneyap Kart**'ın aynı yerel ağ (WiFi) üzerinden **UDP protokolü** ile karşılıklı haberleştiği, eski tip tuşlu telefonlardaki **multi-tap** mantığıyla metin mesajı yazılabilen, OLED ekran ve LED'lerle geri bildirim sağlayan gömülü bir haberleşme sistemi.

---

## 📷 Görseller

> Aşağıdaki bölümlere ilgili görselleri ekleyin (resimleri repo içinde `images/` klasörüne koyup yollarını buna göre güncelleyebilirsiniz).

### Devre Şeması (Fritzing)
![Fritzing Şeması](images/fritzing_semasi.png)

### Gerçek Devre Görüntüsü
![Devre Fotoğrafı](images/devre_fotografi.jpg)

### Mesaj Gönderirken
![Mesaj Gönderme Ekranı](images/mesaj_gonderirken.jpg)

### Mesaj Alırken
![Mesaj Alma Ekranı](images/mesaj_alirken.jpg)

---

## 🎯 Projenin Amacı

Bu projenin amacı, iki Deneyap Kart'ı kablosuz ağ üzerinden birbiriyle haberleştirerek basit ama işlevsel, eski tip tuşlu telefon mantığıyla çalışan bir mesajlaşma sistemi kurmaktır.

## ⚙️ Nasıl Çalışır?

### 1. IP Adreslerinin Öğrenilmesi
Kartların birbirini ağ üzerinde bulabilmesi için önce IP adreslerinin öğrenilmesi gerekir. Bunun için ayrı bir yardımcı kod (`ip_ogren.ino`) yazılmıştır; bu kod kartın WiFi ağına bağlanmasını sağlar ve kendi IP adresini OLED ekranda gösterir. Elde edilen IP adresleri karşılıklı olarak birbirinin koduna (`hedefIP` değişkenine) tanımlanır:

- **Kart 1**'in kodunda `hedefIP` = Kart 2'nin IP adresi
- **Kart 2**'nin kodunda `hedefIP` = Kart 1'in IP adresi

### 2. Mesaj Yazma (Multi-Tap Klavye Mantığı)
Her karta bağlı 4x3 tuş takımı (keypad) ile kullanıcı, eski tip tuşlu telefonlardaki gibi rakam tuşlarına art arda basarak harf girebilir (örnek: `2` tuşuna bir kez basınca "a", iki kez basınca "b", üç kez basınca "c", dört kez basınca "2" gelir).

- Ekranın üst satırında hangi tuşa kaçıncı kez basıldığını gösteren bir **sayaç** (örn. `2->1/4`) belirir.
- Bir tuşa bastıktan sonra **2 saniye** içinde başka bir tuşa basılırsa aynı tuşun sıradaki harfine geçilir.
- Hiçbir tuşa basılmadan **3 saniye** geçerse, seçili harf otomatik olarak mesaja eklenir ve ekranda geri sayım gösterilir.
- Yazılan mesaj her zaman ekranın ikinci satırında sabit olarak görüntülenir.

### 3. Karakter Silme
`*` tuşuna basıldığında son karakter silinir; bu işlem yapıldığında ekranın üst satırında **"Silindi"** yazısı belirip kullanıcıya bilgi verilir.

### 4. Mesaj Gönderme
Mesaj tamamlandığında `#` tuşuna basılarak mesaj **UDP paketi** olarak karşı karta gönderilir. Gönderim sırasında:
- Ekranda **"Mesaj Gönderildi"** bilgisi kısa süreliğine gösterilir.
- Yeşil LED yanar.
- Mesaj değişkeni temizlenir ve ekran belirli bir süre sonra varsayılan görünüme ("Mesajlaşmak için / Tuşa basın") döner.

### 5. Mesaj Alma
Karşı kart sürekli olarak gelen UDP paketlerini dinler (`udp.parsePacket()`). Bir mesaj geldiğinde:
- Ekranda **"Gelen Mesaj"** bilgisi ve mesajın içeriği gösterilir.
- Kırmızı LED yanar.
- Belirli bir süre sonra ekran otomatik olarak varsayılan görünüme döner.

### 6. Sabit Bilgi Ekranı
OLED ekranın alt satırlarında her zaman şu bilgiler sabit olarak gösterilir:
- **Benim adresim:** kartın kendi IP adresi
- **Hedef adres:** karşı tarafın IP adresi

---

## 🧰 Kullanılan Malzemeler

| Malzeme | Açıklama |
|---|---|
| Deneyap Kart (x2) | Ana işlemci kartı, WiFi özellikli |
| Deneyap OLED Ekran (0.96", 128x64, SSD1306) | Durum ve mesaj gösterimi için |
| 4x3 Membran Tuş Takımı (Keypad) (x2) | Mesaj girişi için |
| LED (Yeşil) (x2) | Mesaj gönderildiğinde yanar |
| LED (Kırmızı) (x2) | Mesaj alındığında yanar |
| Breadboard ve jumper kablolar | Bağlantılar için |
| Direnç (LED'ler için uygun değerde) | Akım sınırlama |

---

## 🔌 Bağlantılar

| Bileşen | Deneyap Kart Pini |
|---|---|
| Keypad Satır Pinleri | D0, D1, D4, D8 |
| Keypad Sütun Pinleri | D9, D12, D13 |
| Yeşil LED | D14 |
| Kırmızı LED | D15 |
| OLED (I2C) | JST SH 4 pinli konnektör (I2C - varsayılan adres 0x7A) |

> Detaylı bağlantı şeması için `images/fritzing_semasi.png` dosyasına bakınız.

---

## 💻 Kod Yapısı

Proje iki ana koddan oluşur:

1. **`ip_ogren.ino`** — Kartın WiFi ağına bağlanıp kendi IP adresini OLED ekranda göstermesini sağlayan yardımcı kod. Her iki kartın IP adresini öğrenmek için kullanılır.
2. **`mesajlasma.ino`** — Asıl mesajlaşma mantığının bulunduğu ana kod. Bu kodun her iki karta da yüklenmesi gerekir; tek fark `hedefIP` değişkenine karşı tarafın IP adresinin girilmesidir.

### Önemli Değişkenler

```cpp
const char* ssid = "";      // Wi-Fi adınız
const char* password = "";  // Wi-Fi şifreniz

// hedefIP: Karşı tarafın (alıcının) IP adresi
// - Kart_1'e yükleyeceğiniz kodda buraya Kart_2'nin IP adresini yazın
// - Kart_2'ye yükleyeceğiniz kodda buraya Kart_1'in IP adresini yazın
const char* hedefIP = "";
```

---

## 🚀 Kurulum ve Kullanım

1. Arduino IDE'de Deneyap Kart kartı desteğini ve gerekli kütüphaneleri (`WiFi`, `WiFiUdp`, `Deneyap_OLED`, `Keypad`) kurun.
2. `ip_ogren.ino` kodunu her iki karta ayrı ayrı yükleyip WiFi ağınıza bağlayın; OLED ekranda görünen IP adreslerini not edin.
3. Ana kodda (`mesajlasma.ino`) her kart için:
   - `ssid` ve `password` değişkenlerine WiFi bilgilerinizi girin.
   - `hedefIP` değişkenine **karşı kartın** IP adresini girin.
4. Güncellenmiş kodu ilgili kartlara yükleyin.
5. Her iki kart da açıldığında, tuş takımı ile mesaj yazıp `#` tuşuyla gönderebilir, `*` tuşuyla karakter silebilirsiniz.

---

## 🧩 Ekran Düzeni (OLED - 8 Satır)

| Satır | İçerik |
|---|---|
| 0 | Durum bilgisi (sayaç / Kaydedildi / Gönderildi / Silindi / Gelen Mesaj) |
| 1 | Yazılan / gelen mesaj |
| 2-3 | Boşluk (ayraç) |
| 4 | "Benim adresim:" |
| 5 | Kartın kendi IP adresi |
| 6 | "Hedef adres:" |
| 7 | Karşı tarafın IP adresi |

---

## 🛠️ Kullanılan Teknolojiler ve Kavramlar

- Kablosuz ağ (WiFi) programlama
- UDP protokolü ile veri iletişimi
- Multi-tap (eski usul tuşlu telefon) klavye mantığı
- OLED ekran (SSD1306) ile kullanıcı arayüzü tasarımı
- LED ile görsel geri bildirim
- Donanım-yazılım entegrasyonu

---

## 📌 Notlar

- `EKRAN_GENISLIK` sabiti, kullandığınız OLED fontuna göre ayarlanabilir (varsayılan: 16 karakter).
- Zaman aşımı süreleri (`DONGU_ZAMAN_ASIMI`, `KAYIT_ZAMAN_ASIMI`, durum mesajı süreleri) koddaki sabitlerden özelleştirilebilir.
- Proje, gerçek bir telefon değildir; sesli arama içermez, yalnızca metin tabanlı mesajlaşma sağlar.

---

## 📄 Lisans

Bu proje eğitim amaçlı hazırlanmıştır. Dilediğiniz gibi kullanabilir, değiştirebilir ve paylaşabilirsiniz.
