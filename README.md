# Final Project Algoritma Pemrograman - 06
Kelompok iman: 

Anggota:

1. Natano Juditya - 2306206742
2. Kevin Imanuel- 2306156763
3. Rafi Ikhsan A. - 2306156555
4. Rafi Naryama D. - 2306156864

[**Youtube video**](https://youtu.be/GSMcCsTPP7U?feature=shared)

## Build Command:

### Using CMAKE:

- Config
  ```shell
  cmake -S . -B .\cmake-build
  ```
- Build server
  ```shell
  cmake --build .\cmake-build --target server
  ```
- Run server
  ```shell
  .\cmake-build\server.exe 
  ```
  
- Build client
  ```shell
  cmake --build .\cmake-build --target client
  ```
- Run client
  ```shell
  .\cmake-build\client.exe 
  ```
  
Cara kerja: 

- Jalankan server, terdapat CLI server dimana ada opsi help yang akan membantu
- Jalankan client dengan konfigurasi host serta port yang sesuai dengan server
