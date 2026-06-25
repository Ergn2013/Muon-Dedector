%% 1. VERİ YÜKLEME 
filename = 'live_muon_data3.csv';
data = readtable(filename);

% Sütun isimleri
event_id = data.event_no;
zaman = data.time_s;
voltaj = data.voltage_V;

%% 2. MÜON AYIKLAMA (THRESHOLDING)
% Gürültü (noise) -1V civarında görünüyor. 
threshold = -1.0; 

% Her bir event içindeki en düşük voltaj değerini bulalım
stats = groupsummary(data, 'event_no', 'min', 'voltage_V');
muon_candidates = stats.event_no(stats.min_voltage_V < threshold);

fprintf('\n=== GENEL BİLGİLER ===\n');
fprintf('Toplam Olay Sayısı: %d\n', length(unique(event_id)));
fprintf('Müon Adayı Sayısı (Eşik: %.2fV): %d\n', threshold, length(muon_candidates));

%% 3. GRAFİKSEL ANALİZ
figure('Color', 'w', 'Name', 'Müon Veri Analizi');

% --- A. Ham Veri ve Eşik Çizgisi ---
subplot(2,2,1);
plot(voltaj(1:min(10000, length(voltaj)))); % İlk 10 bin örneği görelim
hold on;
yline(threshold, 'r--', 'Eşik Değeri');
title('Ham Voltaj Verisi (Kısmi)');
xlabel('Örnek Index'); ylabel('Voltaj (V)');
grid on;

% --- B. Gerçek Müon Sinyal Formları (Pulse Shapes) ---
subplot(2,2,2);
hold on;
num_to_plot = min(5, length(muon_candidates)); % İlk 5 müonu çizdir
for i = 1:num_to_plot
    idx = (event_id == muon_candidates(i));
    plot(zaman(idx) - min(zaman(idx)), voltaj(idx)); % Zamanı 0'dan başlatalım
end
title(['İlk ' num2str(num_to_plot) ' Müon Sinyal Formu']);
xlabel('Zaman (s)'); ylabel('Voltaj (V)');
legend(string(muon_candidates(1:num_to_plot)), 'Location', 'best');
grid on;

% --- C. Enerji Dağılımı (Histogram) ---
subplot(2,1,2);
histogram(stats.min_voltage_V, 50, 'FaceColor', [0.2 0.5 0.8]);
set(gca, 'XDir','reverse'); % Negatif voltaj olduğu için ekseni ters çevirelim
title('Müon Enerji Spektrumu (Pik Voltaj Dağılımı)');
xlabel('Minimum Voltaj (V) -> Daha fazla enerji');
ylabel('Gözlem Sayısı');
grid on;

%% 4. İSTATİSTİKSEL ÖZET
fprintf('\n=== ENERJİ (VOLTAJ) İSTATİSTİKLERİ ===\n');
fprintf('Ortalama Pik Voltajı: %.3f V\n', mean(stats.min_voltage_V(stats.min_voltage_V < threshold)));
fprintf('En Şiddetli Müon: %.3f V\n', min(stats.min_voltage_V));

if ismember('pc_time', data.Properties.VariableNames)
    gercek_sure_saniye = max(data.pc_time) - min(data.pc_time);
    fprintf('Deneyin toplam süresi: %.2f saniye\n', gercek_sure_saniye);
end

%% 5. SİNYAL SÜRESİ (PULSE WIDTH) ANALİZİ
% Sinyalin süresini bulmak için voltajın gürültü seviyesini (-0.2V) 
% kestiği başlangıç ve bitiş anları arasındaki zamanı ölçeceğiz.
width_threshold = -0.2; 
pulse_durations = zeros(length(muon_candidates), 1);

for i = 1:length(muon_candidates)
    % Sadece bu müona ait verileri çek
    idx = (event_id == muon_candidates(i));
    t_event = zaman(idx);
    v_event = voltaj(idx);
    
    % Voltajın -0.2V'un altına indiği tüm noktaları (indexleri) bul
    below_idx = find(v_event < width_threshold);
    
    if ~isempty(below_idx)
        % İlk indiği an ile son çıktığı an arasındaki zaman farkı
        t_start = t_event(below_idx(1));
        t_end = t_event(below_idx(end));
        pulse_durations(i) = t_end - t_start;
    end
end

% Sıfır olan değerleri filtrele
valid_durations = pulse_durations(pulse_durations > 0);

if ~isempty(valid_durations)
    mean_duration = mean(valid_durations);
    min_duration = min(valid_durations);
    max_duration = max(valid_durations);

    fprintf('\n=== SİNYAL (ATIM) SÜRESİ İSTATİSTİKLERİ ===\n');
    % Saniyeyi Nanosaniyeye (ns) çevirerek ekrana yazdırıyoruz (* 1e9)
    fprintf('Ortalama Atım Süresi : %.2f ns\n', mean_duration * 1e9);
    fprintf('En Kısa Atım Süresi  : %.2f ns\n', min_duration * 1e9);
    fprintf('En Uzun Atım Süresi  : %.2f ns\n', max_duration * 1e9);
else
    fprintf('\nUYARI: Süre hesaplanamadı. Sinyaller çok zayıf veya eşik hatalı olabilir.\n');
end
