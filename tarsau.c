#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_FILES 32
#define MAX_SIZE (200 * 1024 * 1024) // 200 MB
#define EXTRACTED_BUF_SIZE (MAX_FILES * 256 + MAX_FILES * 2) // max dosya adı * dosya sayısı

// ASCII dosya kontrol fonksiyonu
int is_ascii_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;

    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch > 127 || ch == 0) {
            fclose(file);
            return 0;
        }
    }
    fclose(file);
    return 1;
}

// .sau arşiv oluşturma fonksiyonu (-b)
void create_archive(char *output_filename, char *input_files[], int input_count) {
    FILE *out = fopen(output_filename, "w");
    if (!out) {
        printf("Hata: %s dosyasi olusturulamadi.\n", output_filename);
        return;
    }

    int org_size = 0;
    struct stat st;
    char buffer[512];

    // Organizasyon bölümü boyutunu hesapla
    for (int i = 0; i < input_count; i++) {
        if (stat(input_files[i], &st) != 0) {
            printf("Hata: %s dosyasi okunamadi.\n", input_files[i]);
            fclose(out);
            return;
        }
        int mode = st.st_mode & 0777;
        snprintf(buffer, sizeof(buffer), "|%s,%04o,%ld|", input_files[i], mode, (long)st.st_size);
        org_size += strlen(buffer);
    }

    fprintf(out, "%010d", org_size);

    // Organizasyon bölümünü yaz
    for (int i = 0; i < input_count; i++) {
        stat(input_files[i], &st);
        int mode = st.st_mode & 0777;
        fprintf(out, "|%s,%04o,%ld|", input_files[i], mode, (long)st.st_size);
    }

    // Dosya içeriklerini yaz
    for (int i = 0; i < input_count; i++) {
        FILE *in = fopen(input_files[i], "r");
        if (in) {
            int ch;
            while ((ch = fgetc(in)) != EOF) {
                fputc(ch, out);
            }
            fclose(in);
        }
    }

    fclose(out);
    printf("Dosyalar birlestirildi.\n");
}

// .sau arşivden çıkarma fonksiyonu (-a)
void extract_archive(char *archive_name, char *target_dir) {
    // .sau uzantısı kontrolü
    int len = strlen(archive_name);
    if (len < 4 || strcmp(archive_name + len - 4, ".sau") != 0) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        return;
    }

    FILE *in = fopen(archive_name, "r");
    if (!in) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        return;
    }

    // Hedef dizin kontrolü ve oluşturma
    if (target_dir != NULL) {
        struct stat st = {0};
        if (stat(target_dir, &st) == -1) {
            mkdir(target_dir, 0777); // Dizin yoksa oluştur
        }
    } else {
        target_dir = "."; // Dizin verilmezse mevcut dizin
    }

    // İlk 10 baytı oku (Organizasyon bölümü boyutu)
    char header[11];
    if (fread(header, 1, 10, in) != 10) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        fclose(in);
        return;
    }
    header[10] = '\0';
    int org_size = atoi(header);

    if (org_size <= 0) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        fclose(in);
        return;
    }

    // Organizasyon verisini oku
    char *org_data = malloc(org_size + 1);
    if (!org_data) {
        printf("Hata: Bellek ayrilamadi.\n");
        fclose(in);
        return;
    }
    if (fread(org_data, 1, org_size, in) != (size_t)org_size) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        free(org_data);
        fclose(in);
        return;
    }
    org_data[org_size] = '\0';

    // Kayıtları ayrıştır ve dosyaları oluştur
    char *ptr = org_data;
    char *extracted_files = malloc(EXTRACTED_BUF_SIZE);
    if (!extracted_files) {
        printf("Hata: Bellek ayrilamadi.\n");
        free(org_data);
        fclose(in);
        return;
    }
    extracted_files[0] = '\0';

    while (*ptr == '|') {
        ptr++; // baştaki '|' işaretini atla
        char filename[256];
        int permissions;
        long size;

        // formatı oku: Dosya adı, izinler, boyut
        if (sscanf(ptr, "%255[^,],%o,%ld|", filename, &permissions, &size) != 3) {
            break;
        }

        // mesaj için dosya ismini kaydet (taşma korumalı)
        if (strlen(extracted_files) > 0) {
            strncat(extracted_files, ", ", EXTRACTED_BUF_SIZE - strlen(extracted_files) - 1);
        }
        strncat(extracted_files, filename, EXTRACTED_BUF_SIZE - strlen(extracted_files) - 1);

        // Bir sonraki kayda atla
        ptr = strchr(ptr, '|');
        if (ptr) ptr++;

        // Hedef dizine dosyayı yaz
        char filepath[1024];
        if (strcmp(target_dir, ".") == 0) {
            snprintf(filepath, sizeof(filepath), "%s", filename);
        } else {
            snprintf(filepath, sizeof(filepath), "%s/%s", target_dir, filename);
        }

        FILE *out = fopen(filepath, "w");
        if (out) {
            for (long i = 0; i < size; i++) {
                int ch = fgetc(in);
                if (ch != EOF) fputc(ch, out);
            }
            fclose(out);
            chmod(filepath, permissions); // Dosya izinlerini geri yükle
        }
    }

    free(org_data);
    free(extracted_files);
    fclose(in);

    // Başarı mesajı — extracted_files zaten free edildi, önceden yazdır
    // Bu nedenle mesajı döngü sonrasında ayrı tutuyoruz
    if (strcmp(target_dir, ".") == 0) {
        printf("Mevcut dizinde dosyalar acildi.\n");
    } else {
        printf("%s dizininde dosyalar acildi.\n", target_dir);
    }
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
        char *output_file = "a.sau";

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
            if (stat(input_files[i], &file_stat) == 0) {
                total_size += file_stat.st_size;
            } else {
                printf("Hata: %s dosyasi okunamadi veya bulunamadi.\n", input_files[i]);
                return 1;
            }

            if (!is_ascii_file(input_files[i])) {
                printf("%s giris dosyasinin formati uyumsuzdur!\n", input_files[i]);
                return 0;
            }
        }

        if (total_size > MAX_SIZE) {
            printf("Hata: Giris dosyalarinin toplam boyutu 200 MB'i gecemez.\n");
            return 1;
        }

        create_archive(output_file, input_files, input_count);

    }
    // --- ARŞİVDEN ÇIKARMA MODU (-a) ---
    else if (strcmp(argv[1], "-a") == 0) {
        if (argc < 3 || argc > 4) {
            printf("Hata: -a parametresi en fazla 2 parametre alabilir.\n");
            return 1;
        }

        char *archive_name = argv[2];
        char *target_dir = (argc == 4) ? argv[3] : NULL;

        extract_archive(archive_name, target_dir);
    }
    else {
        printf("Hata: Gecersiz parametre. Lutfen -b veya -a kullanin.\n");
        return 1;
    }

    return 0;
}