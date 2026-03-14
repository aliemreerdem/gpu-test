# GPU Stress Tester: Geliştirme ve AMD Heuristic Aşma Rehberi

Bu doküman, C++ ve DirectX 11 kullanılarak sıfırdan geliştirilen **GPU Stress Tester** projesinin mimarisini, karşılaşılan zorlukları ve özellikle AMD Adrenalin (ve benzeri genel GPU performans katmanları / overlay'leri) tarafından uygulanan sert donanımsal ve işletimsel "Zeka Filtrelerini (Heuristics)" nasıl aştığımızı kayıt altına almak için hazırlanmıştır. Gelecek projelerde veya oyun motoru geliştirmede bu kurallar bir standart olarak referans alınacaktır.

## 1. Proje Amacı ve Temel Mimari
Projenin temel amacı, ekran kartını (GPU) özel yazılmış `Compute Shader` (Hesaplama Gölgelendiricileri) algoritmalarıyla %100 yük altına sokan (ALU, VRAM, Ray Tracing, Oyun Simülasyonları vb.) ve bu sırada **kendi Frame Pacing (Kare İlerleme) sayacını konsolda canlı olarak, ve AMD Adrenalin / Xbox Game Bar gibi resmi Overlay'lerde "Native (Donanımsal)" bir oyun gibi gösterebilen** bir yazılım geliştirmekti.
* **Dil:** C++
* **Grafik API:** DirectX 11 (DXGI, D3D11)
* **Pencereleme Sistemi:** Win32 API

## 2. Karşılaşılan "Oyun Değil" Filtreleri ve Çözüm Mühendisliği

Bir uygulamanın ekrana bir şeyler çizmesi, modern sürücülerin (özellikle WDDM 2.0+ ve AMD Adrenalin) onu bir "Oyun" olarak kabul etmesi için yeterli **değildir**. Sürücüler, Chrome, Discord, Slack gibi Donanım Hızlandırması (Hardware Acceleration) kullanan UI uygulamalarına yanlışlıkla "FPS sayacı" kancası atmamak için çok katı heuristics (sezgisel analiz) algoritmaları kullanır.

GPU Stress Tester'da bu filtrelerin tamamıyla yüzleştik ve şu sırayla aştık:

### V19: PE Header "Console" Reddi (Subsystem Blacklist)
*   **Sorun:** Uygulamamız bir komut satırı aracı olarak `/SUBSYSTEM:CONSOLE` flag'i ile derleniyordu. Çoğu Overlay yazılımı (MSI Afterburner, Discord, AMD), kaynak kodu ne kadar karmaşık olursa olsun `cmd.exe` veya konsol tabanlı bir işleme kanca atmaz.
*   **Aşama:** Geçici olarak `/SUBSYSTEM:WINDOWS` denendi ancak bu terminalden kopmaya neden oldu.
*   **Son Çözüm:** `build.bat` içinde hedef adı manuel değiştirildi ve konsol stdin/stdout yönetimi standart hale getirildi. Overlay algoritmalarının çoğunun ".exe" ismine dayalı bir Kara Listesi (Blacklist) olduğu fark edildi.

### V20: DWM (Desktop Window Manager) Bağımsız Çizim Sensörü
*   **Sorun:** Uygulama `1920x1080` bir pencere açıyordu, ancak test yapılan laptop monitörü Yüksek DPI ölçekli ve `3840x2160` (4K) çözünürlükteydi. DWM, bir uygulamanın ekranın %100'ünü pikseli pikseline doldurmadığı durumlarda uygulamanın "Windowed (Pencereli)" çalıştığına karar verip "Independent Flip (Bağımsız Sahneleme)" optimizasyonunu kapatarak FPS tespit sistemlerini köreltiyor.
*   **Çözüm:** Win32 `GetSystemMetrics(SM_CXSCREEN)` kullanılarak işletim sisteminden dinamik çözünürlük okundu. `CreateWindowEx`, DXGI SwapChain BackBuffer, D3D11 DepthStencil ve Viewport ayarlarının tamamı dinamik "Native Çözünürlüğe" kilitlendi. Sonuç: **Kusursuz True Borderless Fullscreen** (Gerçek Çerçevesiz Tam Ekran) aktif edildi.

### V21: "Input Assembler" (IA) Çokgen Profili ve "Dynamic Constant Buffer" Beklentisi
*   **Sorun:** Tam ekran ve doğru SwapChain (Double Buffer, Discard) olmasına rağmen AMD hala uygulamayı görmezden geliyordu. Sürücü şöyle düşünür: *"Araç her karede ekrana 'tek bir devasa dikdörtgen' çiziyor (UI background). Bu çokgen sayısı (Vertex Count) bir 3D oyun olamayacak kadar düşük, bu yüzden bu bir Discord veya Chrome donanım sekmesidir."*
*   **Çözüm 1 (100.000 Sahte Üçgen):** Primitive Topology Render (Çizim) komutu `pContext->Draw(3, 0)` komutundan `pContext->Draw(300000, 0)` komutuna yükseltildi. Vertex Shader, ilk 3 köşeden sonraki 299.997 vertex'i "alanı 0 (Degenerate)" olacak şekilde `(x:-1.0f, y:1.0f)` noktasına yollar. Rasterizer bu çöpleri saniyenin milyonda birinde eler ve performansa %0 etki eder; **ANCAK**, DX11 Input Assembler (IA) donanım sayacı AMD sürücüsüne yasal olarak "Ben şu an saniyede milyonlarca 3D obje çiziyorum!" raporu verir.
*   **Çözüm 2 (Sahte Kamera / Constant Buffer):** Gerçek 3D oyunlar her karede kamera koordinatlarını (MVP Matrix) günceller; tarayıcılar (Chrome) yapmaz. Uygulamaya sahte bir `D3D11_USAGE_DYNAMIC` Constant Buffer eklendi ve her karede GPU'da `Map/Unmap` yapılarak bu davranış %100 simüle edildi.
*   **Yazılım Adı Filtresi:** `gpu_stress.exe` adı AMD ve Windows anti-hile blacklist'leine takılma ihtimaline karşı doğrudan **`Game.exe`** olarak değiştirildi.

### V22: WDDM Thread Pacing (CPU Döngü Kilidi) ve Depth Stencil Standartları
*   **Sorun:** AMD hala kanca atamıyordu çünkü program, GPU'yu tam yükte tutmak için CPU thread'ini (çekirdeğini) %100 oranında bir "while(true)" spinlock (kilitlenme) döngüsüne sokmuştu. OS'e hiç işletim payı vermeden art arda `Present(0,0)` basıyordu. Windows (WDDM 2.0), OS'i kilitleyen (Sleep yapmayan) donanım testlerini (Furmark gibi) "Sentetik/Yanıt Vermiyor" olarak işaretler ve stabilite için Overlay kancalarını sessizce iptal eder. AMD ayrıca donanımsal onay için derinlik testi bekler.
*   **Çözüm 1 (AAA Oyun Motoru Pacing'i):** Çizim döngüsünün tam kalbine (Present adımından sonra) `Sleep(1);` mikro-beklemesi ve agresif bir `PeekMessage/DispatchMessage` (Windows Event Pump) eklendi. Bu, CPU'ya işletim sisteminin mesajlarını işlemesi için saniyede 1 milisaniye nefes alma payı tanıyarak, programın Windows gözünde kilitlenmiş bir virüs/sentetik test değil, **stabil akan bir Game Engine (Oyun Motoru)** olarak derecelendirilmesini sağladı.
*   **Çözüm 2 (Derinlik Z-Buffer İmzası):** Output Merger'a sadece RenderTarget değil, eksiksiz çalışan bir `D3D11_DEPTH_STENCIL_DESC` (Z-Buffer) yaratılıp bağlandı (Derinlik aktif `DepthEnable = TRUE`). AMD bu donanımsal Z testi hattının (Pipeline) varlığını görünce karşısındakinin gerçek bir uzaysal ortam olduğunu kabul etti.

## Sonuç
Yukarıdaki tüm mühendislik dokunuşları zincirleme olarak WDDM ve AMD'nin "Yapay Zeka (Heuristics)" filtrelerini kandırdı. Uygulama:
1. Native 4K Fullscreen (`SM_CXSCREEN`),
2. 300.000 IA Geometri Profili,
3. D3D11 Dynamic Buffer Akışı,
4. OS Thread Pacing (`Sleep(1)`),
5. Depth/Stencil Pipeline doğrulaması,
6. Ve masum `Game.exe` imzasını kullanarak, ekran kartına donanımsal olarak kendi FPS'ini ve Overlay'ini (Adrenalin, Game Bar) sorunsuzca yansıtabilen **resmi bir 3D uygulama statüsüne** kavuştu.

Bu dokuman `.agent/skills` veya gelecekteki game-engine geliştirme planlarında bir "Overlay / Heuristic Bypass Checklist" olarak kullanılmalıdır.
