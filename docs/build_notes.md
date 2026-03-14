# GPU Stress Tester - Derleme Notları (Build Notes)

Bu belge, C++ ve DirectX 11 tabanlı stres testinin (gpu_stress.exe) Windows ortamında başarıyla derlenebilmesi için keşfedilen yöntemleri, kullanılan araçları ve kritik dizin yollarını içerir. Gelecekteki çalışmalarda derleme sorunu yaşanmaması adına bir referans olarak hazırlanmıştır.

## 1. Kullanılan Araçlar (Build Tools)
*   **Derleyici (Compiler):** Microsoft C/C++ Optimizing Compiler (`cl.exe`)
*   **Bağlayıcı (Linker Parametreleri):** `/link /OUT:gpu_stress.exe d3d11.lib d3dcompiler.lib` (Direct3D 11 Core ve Shader derleme kütüphaneleri)
*   **C++ Ortam Değişkeni Aracı:** `vcvars64.bat` (Komut satırında C++ kütüphanelerinin bulunabilmesi için gerekli PATH ayarlarını yapar).

## 2. Kurulum Yolunu Tespit Etme (vswhere.exe)
Visual Studio varsayılan standart kurulum dizinlerinde (örneğin `2022\Community`) bulunmadığında, gerçek kurulum yolunu dinamik olarak tespit etmek için Microsoft'un araç bulucusu kullanılmıştır.
*   **Kullanılan Program:** `"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"`
*   **Kullanılan Komut:** `vswhere.exe -latest -property installationPath`
*   **Tespit Edilen Gerçek VS Yolu:** `C:\Program Files\Microsoft Visual Studio\18\Community`

## 3. Kritik Dizinler ve Dosyalar
*   **vcvars64.bat Tam Dizin Yolu:** 
    `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat`
*   **Proje Çalışma Dizini:** 
    `c:\#Calismalar\gpu_test`
*   **Sorun ve Çözüm (Tırnak İşareti Sorunu):** Komut satırı (cmd.exe) üzerinden içinde boşluk barındıran yolları `&&` operatörü ile birleştirmeye çalışırken oluşan syntax hatalarını aşmak için, aracı bir script olan **`run.bat`** oluşturulmuştur.

## 4. Derleme Prosedürü
Gelecek sefer projeyi derlemek istediğinizde sadece terminalden şu komutu çağırmanız yeterlidir:
`run.bat`

**Arka planda gerçekleşen adımlar:**
1. `run.bat` dosyası öncelikle doğru yoldaki `vcvars64.bat` dosyasını `call` komutu ile çağırır.
2. C++ ortamı terminale yüklendiği an `build.bat` dosyasını `call` ile tetikler.
3. `build.bat` içerisinde bulunan C++ derleme emri (`cl.exe main.cpp ...`) çalışır ve `gpu_stress.exe` çıktısı oluşturulur.
