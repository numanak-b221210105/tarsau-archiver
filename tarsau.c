#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define MAX_FILES 32
#define MAX_SIZE (200 * 1024 * 1024) // 200 MB (Bayt cinsinden)

// ASCII dosya kontrol fonksiyonu
int is_ascii_file(const char *filename) {
    FILE *file = fopen(filename, "rb"); // Binary modda okuyoruz ki müdahale olmasın
    if (!file) return 0; // Dosya açılamazsa başarısız say

    int ch;
    while ((ch = fgetc(file)) != EOF) {
        // ASCII standardına göre karakterler 0-127 arasında olmalıdır.
        // NULL (0) baytı genelde metin dosyalarında olmaz, binary'ye işarettir.
        if (ch > 127 || ch == 0) {
            fclose(file);
            return 0; // Metin dosyası değil
        }
    }
    fclose(file);
    return 1; // Başarılı, standart metin dosyası
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Kullanim: ./tarsau -b [dosyalar] -o [arsiv_adi.sau]\n");
        printf("Veya    : ./tarsau -a [arsiv_adi.sau] [hedef_dizin]\n");
        return 1;
    }

    // --- ARŞİVLEME MODU (-b) ---
    if (strcmp(argv[1], "-b") == 0) {
        char *input_files[MAX_FILES + 1];
        int input_count = 0;
        char *output_file = "a.sau"; // Varsayılan arşiv adı

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) {
                    output_file = argv[i + 1];
                    i++; 
                } else {
                    printf("Hata: -o parametresinden sonra dosya adi belirtilmedi.\n");
                    return 1;
                }
            } else {
                if (input_count >= MAX_FILES) {
                    printf("Hata: En fazla 32 giris dosyasi belirtilebilir.\n");
                    return 1;
                }
                input_files[input_count++] = argv[i];
            }
        }

        if (input_count == 0) {
            printf("Hata: Arsivlenecek giris dosyasi belirtilmedi.\n");
            return 1;
        }

        long total_size = 0;
        struct stat file_stat;
        
        for (int i = 0; i < input_count; i++) {
            // 1. Dosya okuma ve boyut kontrolü
            if (stat(input_files[i], &file_stat) == 0) {
                total_size += file_stat.st_size;
            } else {
                printf("Hata: %s dosyasi okunamadi veya bulunamadi.\n", input_files[i]);
                return 1;
            }

            // 2. Metin (ASCII) dosyası kontrolü
            if (!is_ascii_file(input_files[i])) {
                printf("%s giris dosyasinin formati uyumsuzdur!\n", input_files[i]);
                return 0; // Sorunsuz bir şekilde programdan çıkılmalıdır
            }
        }

        if (total_size > MAX_SIZE) {
            printf("Hata: Giris dosyalarinin toplam boyutu 200 MB'i gecemez.\n");
            return 1;
        }

        printf("Kontrol Basarili: %d dosya isleme alindi. Toplam boyut: %ld bayt.\n", input_count, total_size);
        printf("Olusturulacak Arsiv Dosyasi: %s\n", output_file);
        
        // TODO: Dosyaları .sau formatında birleştirme (İzinler ve içerik)

    } 
    // --- ARŞİVDEN ÇIKARMA MODU (-a) ---
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