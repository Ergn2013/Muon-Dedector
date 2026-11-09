# Dosya doğrulama kaydı

**Yeni devre Windows LTspice ile bu ortamda çalıştırılmadı. Elektriksel test geçiş sonucu yoktur.**

Aşağıdaki denetimler dosya, bağlantı, kaynak modeli ve sonuç okuma düzeyindedir:

- **ASC/CIR:** 13 dosya; 129 elemanin her pini ve her test yonergesi ayni; tek .tran, tekrarlanan .step yok
- **Schmitt -> AND:** AND A=CH1_PS, B=CH2_PS; kullanilmayan Schmitt girisi GND
- **TI modelleri:** 3 vendor dosyasi degistirilmedi; SUBCKT/pin siralari dogru
- **Asimetrik kaynak:** 10/15/20/25/30ns FWHM; maksimum matematiksel hata 7.95e-12ns; birim tepe=1
- **PWL gurultusu:** 20 farkli kayit; monoton zaman; sifir baslangic/bitis; birim RMS hata 2.84e-10
- **Tarama parametreleri:** 163 kosulun ifadeleri/cozumleme sinirlari kontrol edildi; ana 50 kosul
- **RAW okuyucu:** Onceki native Windows sonucunda 1 coincidence / 1 singles1 / 1 singles2 ve 1025.406ns dogrulandi; yeni devre simule edilmedi

Referans gürültüsü ana testte DC filtresinden sonra eklenir. Filtre öncesi enjeksiyon 07/MODE5 ayrı kontroldür.
TI modelleri tipik davranış modelleridir; BAS70/BAT54 ve besleme/ferrit katları yaklaşık kalmıştır.
RAW okuyucu için eski trace kullanımı, yeni Schmitt→AND devresinin simüle edildiği anlamına gelmez.
