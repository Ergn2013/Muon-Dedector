# Müon sayım devresi — asimetrik darbeler ve Schmitt → AND

Bu paket, gönderdiğin **Rev AG** devresinden düzenlendi. Ana bağlantı:

**PMT → 1 nF → 100 Ω → TLV3601 → 33 Ω → SN74LVC2G17 → SN74LVC1G08 → diyot/RC → Schmitt → STM_COINC.**

İki bağımsız tek kanal çıkışı `STM_SING1` ve `STM_SING2` de korunuyor.

## İlk açılacak dosya

1. ZIP'in tamamını çıkar. `NOISE` klasörünü, `.asy`, `.lib`, `.cir` ve `.inc` dosyalarını birlikte tut.
2. **`00_ONCE_BUNU_AC.asc`** dosyasını LTspice'ta aç ve Run'a bas.
3. Bu ilk dosya 30 mV, 20 ns FWHM ve hem sinyal hem eşik referansında 10 mV RMS gürültüyle **bir koşul** çalıştırır.
4. Ana tarama için **`01_GENLIK_GENISLIK_10mV.asc`** dosyasını çalıştır.
5. `Ctrl+L` ile ölçüm sonuçlarını aç. Ölçüm penceresinde yalnız tepe gerilimine bakma: uzun çıkışın önceki gürültü nedeniyle zaten HIGH olup olmadığını da kontrol et.

Her testin `.asc` ve `.cir` karşılığı aynı devreye sahiptir. `.asc` düzenlenebilir çizimdir. Sağdaki `.param` ve `.step` satırlarını çizim üzerinden değiştirebilirsin. `.cir` kullanırsan ayarları karşılık gelen `TEST_....inc` dosyasından değiştir. Bir dosyayı çalıştırmak yeterli; iki karşılığı art arda çalıştırmak gerekmez.

## Ana tarama

| Ayar | Değerler |
|---|---|
| Negatif darbenin nominal tepe büyüklüğü | 30, 50, 75, 100, 150, 200, 300, 500, 750, 1000 mV |
| Yarı yükseklik genişliği, FWHM | 10, 15, 20, 25, 30 ns |
| Sinyal yolu gürültüsü | Kanal başına nominal 10 mV RMS |
| Eşik referansı gürültüsü | Kanal başına 10 mV RMS, DC filtresinden sonra |
| Ana koşul sayısı | 10 × 5 = **50** |
| İki kanal arasındaki gecikme | Ana taramada 0 ns; ayrı gecikme testinde taranıyor |
| En büyük zaman adımı | **10 ps** |
| Solver | `.options solver=alt` ile Alternate |

`AMP` pozitif büyüklüktür: `AMP=30m`, negatif yaklaşık −30 mV'luk darbe hedefler. 50 Ω kaynak ve 49,9 Ω sonlandırma korunmuştur. Giriş devresinin ek yükü nedeniyle **gerçek SMA tepe değeri nominal değerden farklı olabilir**; `SMA1_EVENT_MIN` ve `SMA2_EVENT_MIN` bunu ölçer. Kaynak hiçbir noktada yapay olarak kırpılmaz.

## Darbe modeli

Negatif darbe, hızlı ön kenar ve daha yavaş kuyruk içeren normalize edilmiş iki üstel terimin farkıdır:

`p(u) = [exp(-u/tau_fall) - exp(-u/tau_rise)] / normalizasyon`, `u >= 0`.

- `tau_rise = 0,30 ns` sabit tutuldu.
- `tau_fall`, istenen **gerçek FWHM** değerini vermesi için sayısal olarak hesaplandı.
- Bu değerlerle kaynağın %10–%90 ön kenarı yaklaşık 0,5–0,6 ns'dir.
- `PW_NS=10`, 10 ns **FWHM** demektir; toplam darbe süresi değildir. Üstel kuyruk devam eder.
- İlk darbenin başlangıcı `T0=1u`; tepe yaklaşık 1 ns daha sonra oluşur.
- Hesaplanan zaman sabitleri `PULSE_CALIBRATION.csv`, çizimleri `ATMALAR_ONIZLEME.png` içindedir.

Bu, ölçülmüş PMT dalgasına uydurulmuş bir model değildir. Sintilatör, optik yol, kablo, PMT ve ölçüm sisteminin gerçek cevabına göre daha sonra ayarlanmalıdır. `TR_ASYM` değiştirilirse mevcut FWHM kalibrasyonu da yeniden yapılmalıdır.

## Gürültünün tam olarak eklendiği yerler

**Sinyal:** `BNOISE1` ve `BNOISE2`, `PMT1_SMA` ve `PMT2_SMA` düğümlerine Norton akımı ekler. Akım, 50 Ω kaynak ve 49,9 Ω sonlandırmanın paralel direncine göre nominal 10 mV RMS için ayarlanır. Gerçek giriş empedansı frekansa bağlı olduğundan, devrede oluşan RMS ayrıca ölçülür. `SMA1_IDLE_RMS` / `SMA2_IDLE_RMS`, 400–700 ns arasındaki gerçek port gürültüsüdür.

**Referans:** `BREF1` ve `BREF2`, sakin `TH1_QUIET` / `TH2_QUIET` noktalarından sonra seri gerilim ekler. Dolayısıyla ana testte **DC referans filtresinden sonra girişe ulaşmış 10 mV RMS parazit** sınanır. Bu, referans filtresinin önüne 10 mV verip filtrede bastırılmasıyla aynı test değildir. `REF1_PIN_NOISE_RMS` / `REF2_PIN_NOISE_RMS` bunu doğrudan ölçer.

`07_GURULTU_KAYNAGINI_AYIR.asc` içindeki MODE=5, gürültüyü referans bölücüsünün dönüşüne, **DC filtresinden önce** ekleyen ayrı kontroldür. Ana taramada `N_UP=0`; MODE=5'te `N_REF=0` ve `N_UP=10m` olur. Sinyal gürültüsü bu modda da aktiftir.

Buradaki “referans”, **TLV eşik referansıdır**. PMT modülünün 1,2 V `VREF` ve `VCONT` kolu ayrı tutulmuştur; sentetik darbe genliği bu kontrol geriliminden hesaplanmaz.

Gürültü kayıtları:

- Sabit tohumlu Gauss örneklerinden üretildi; yaklaşık 200 MHz bantla sınırlandı.
- Örnek aralığı 0,5 ns; LTspice arada doğrusal interpolasyon yapar.
- RMS, darbe bulunmayan 400–700 ns aralığında normalize edildi.
- Her kanal ve referans için bağımsız kayıtlar, beş farklı tohum vardır.
- Gürültü 100–300 ns arasında yumuşakça açılır. Bu başlangıç bölümü olay sayımına dahil edilmez.
- **10 mV RMS, 10 mV tepe veya tepeden tepeye değer değildir.** Bazı gürültü tepeleri bunun birkaç katına ulaşır.

## Diğer testler

| Dosya başlangıcı | Koşul | Ne incelenir? |
|---|---:|---|
| 00 | 1 | İlk açılacak tek örnek |
| 01 | 50 | Ana genlik × FWHM taraması |
| 02 | 9 | Gürültüsüz kontrol, seçilmiş genlik ve genişlikler |
| 03 | 30 | −200…+200 ns iki PMT gecikmesi, 10 ve 30 ns FWHM |
| 04 | 15 | İkinci PMT yarı genlikte; en küçük PMT2 darbesi 30 mV |
| 05 | 3 | Darbe yok / yalnız PMT1 / yalnız PMT2; gürültü aktif |
| 06 | 16 | 20…2500 ns aralıkla iki atma; aynı PMT ve iki kanallı olay |
| 07 | 6 | Sinyal/ref gürültüsünü ve referans filtresinin yerini ayırma |
| 08 | 5 | Aynı olayın beş farklı gürültü kaydıyla tekrarı |
| 09 | 9 | İki TLV için bağımsız −5 / 0 / +5 mV offset |
| 10 | 6 | Eşik trimini birlikte değiştirme |
| 11 | 12 | AND çıkış yükü ve darbe uzatma kolundaki parazit kapasitesi |
| 12 | 1 | İsteğe bağlı, yavaş besleme/bias açılışı |

Toplam **163 koşul** hazırlanmıştır. Bunların tamamı ana tarama değildir. Gürültüsüz ve kaynak ayırma kontrolleri, ana 10 mV RMS koşulunun istisnalarıdır; amaçları sorun kaynağını ayırmaktır.

`06` dosyasında `PAIR_MODE=0`, yalnız PMT1'e iki darbe gönderir. `PAIR_MODE=1`, iki kanalda iki eşzamanlı olay gönderir. İki fiziksel darbenin STM'de tek uzun darbeye birleşip birleşmediği araştırılır; yalnız çıkış tepesinin 3,3 V olması yeterli değildir.

`12` dosyasında ns darbeleri ve gürültü kapalıdır. DC yerleşmenin görülebilmesi için 80 ms ve **2 µs maksimum adım** kullanılır. Diğer bütün hızlı testlerin 10 ps ayarı korunmuştur. Başlangıç testi, nanosaniye darbe yakalama testi olarak yorumlanamaz.

Sıcaklık taraması eklenmedi: TI lojik modellerinin başlıkları tipik 25 °C davranışı belirtir. Bu modellerle sıcaklık değiştirmek tek başına sıcaklık doğrulaması sayılmaz. Önceki 8× aşırı-genlik testleri de istenen 1 V üst sınırına uymadığı için bu pakette ana genlik aralığının yerine kullanılmadı.

## Hangi dalgalara bakılacak?

1. Giriş: `PMT1_SMA`, `PMT2_SMA`.
2. Eşik karşılaştırması: `INM1`, `TH1_OFF`; ikinci kanal için `INM2`, `TH2_OFF`.
3. TLV: `U3A_OUT`, `U3B_OUT`.
4. Schmitt: `CH1_PS`, `CH2_PS` — AND artık bu iki düğümü alır.
5. Coincidence: `COINC_FAST`, `S_COINC`, `STM_COINC`.
6. Tek kanal: `STM_SING1`, `STM_SING2`.

## Kenar sayısını otomatik çıkarma

LTspice'ın ürettiği `.raw` ve `.log` dosyaları proje klasöründeyken `SAYIM_OZETINI_OLUSTUR.cmd` dosyasını çalıştır. Python 3 gerekir; ek Python paketi gerekmez. Alternatif komut:

`py -3 TOOLS/analyze_outputs.py`

Araç `SAYIM_SONUCLARI.csv` ve `.json` üretir. Şunları raporlar:

- Her aşamada 1,65 V'u yukarı geçen kenar sayısı.
- Darbe öncesi 300–700 ns boş aralıkta oluşan kenarlar.
- Sayım aralığının başında/sonunda zaten HIGH olan çıkışlar ve HIGH kalma oranı.
- Tamamlanmış darbelerin süreleri; bitmeden HIGH kalanlar ayrıca belirtilir.
- Gerçek SMA ve referans pin gürültüsü RMS değerleri.

`--threshold 2.0` gibi bir değerle farklı sayısal eşik de seçilebilir. **1,65 V burada analiz eşiğidir; STM32'nin garantili giriş eşiği veya gerçek timer/firmware simülasyonu değildir.** MCU bu devrede elektriksel yüklerle temsil edilir.

## Sonuçları doğru yorumlama

Ana eşik farkı yaklaşık 10 mV, istenen gürültü ise her yolda 10 mV RMS düzeyindedir. Dolayısıyla yanlış tetiklenmeler, kaçan olaylar veya sürekli yeniden şarj olup HIGH kalan darbe uzatma çıkışları görülebilir. Test dosyaları bunları saklamaz.

FWHM veya kenar ölçümünde `FAIL'ed`, ilgili geçiş oluşmadığında beklenen bir ölçüm sonucudur; tek başına simülatörün çöktüğü anlamına gelmez. Darbe yokken veya çıkış sonuna kadar HIGH kalırken bu durum oluşabilir. Darbe süreleri RAW okuyucuda gerçek yükseliş–düşüş çiftlerinden hesaplanır. Peak, RMS, dalga şekli ve kenar sayısı birlikte değerlendirilmelidir.

Uzun üstel kuyrukta eşik üstünde kalma süresi genlikle de artar. Bu nedenle iki PMT için kabul edilen gecikme aralığı sabit bir sayı değildir. Gecikme taraması bu etkiyi de içerir.

## Modeller ve doğrulama durumu

- `TLV3601`, `SN74LVC2G17` ve `SN74LVC1G08` için Drive paketindeki **TI dosyaları byte byte korunmuştur**. Dosya kimlikleri `MODEL_KONTROLU.csv` içindedir.
- İki TLV3601, üç çift kanallı SN74LVC2G17 ve bir SN74LVC1G08 karşılığı vardır. Kullanılmayan son Schmitt girişi GND'ye bağlıdır.
- BAS70/BAT54 diyotları ve besleme/ferrit katları Rev AG'deki yaklaşık modellerdir. Bu paket onları üretici modeliyle doğrulanmış hale getirmez.
- Esas STM çıkışı gerçek diyot/RC/Schmitt yolundan gelir. Önceki ideal `STM_MONO` tanısal bloğu bu yeni sayım yolunda bulunmaz.
- Şema pinleri 16 birim ızgaradadır; gerçek tel bağlantıları ve ASC/CIR eşleşmesi denetlenmiştir.
- **Bu ortamda Windows LTspice çalıştırılmadı. Yeni elektriksel testlerin geçtiği iddia edilmiyor.** `DOSYA_DOGRULAMA.md`, yapılan kaynak/bağlantı/model denetimlerini belirtir.

Ana 50 koşullu tarama 10 ps adımla büyük RAW dosyası oluşturabilir. Önce tek koşullu `00` dosyasını çalıştırmak, bağlantı/model/yakınsama sorununu uzun taramadan önce görmeyi sağlar.
