#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // En az 2 argüman olmalı: programın adı ve -b veya -a parametresi
    if (argc < 2) {
        printf("Kullanim: ./tarsau -b [dosyalar] -o [arsiv_adi.sau]\n");
        printf("Veya    : ./tarsau -a [arsiv_adi.sau] [hedef_dizin]\n");
        return 1;
    }

    // Arşivleme Modu
    if (strcmp(argv[1], "-b") == 0) {
        printf("Arsivleme (-b) modu baslatildi.\n");
        // Gelecek adımlar: 32 dosya limiti, 200 MB sınırı ve ASCII kontrolü
    } 
    // Arşivden Çıkarma Modu
    else if (strcmp(argv[1], "-a") == 0) {
        printf("Arsivden cikarma (-a) modu baslatildi.\n");
        // Gelecek adımlar: .sau kontrolü ve dizin/dosya çıkarma
    } 
    else {
        printf("Hata: Gecersiz parametre. Lutfen -b veya -a kullanin.\n");
        return 1;
    }

    return 0;
}