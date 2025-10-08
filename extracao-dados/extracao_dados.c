#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Estrutura para armazenar estatísticas dos pixels
typedef struct {
    double min, max, mean, std_dev;
} ChannelStats;

// Estrutura para acumular valores do algoritmo de Welford
typedef struct {
    long long count;
    double mean, m2;
} Welford;

void update_welford(Welford *w, double value) {
    w->count++;
    double delta = value - w->mean;
    w->mean += delta / w->count;
    double delta2 = value - w->mean;
    w->m2 += delta * delta2;
}

double finalize_std_dev(Welford *w) {
    return sqrt(w->m2 / w->count);
}

void rgb_to_hsi(double r, double g, double b, double *h, double *s, double *i) {
    r /= 255.0; g /= 255.0; b /= 255.0;
    
    double max = fmax(r, fmax(g, b));
    double min = fmin(r, fmin(g, b));
    double delta = max - min;
    
    *i = (r + g + b) / 3.0;
    
    if (delta == 0) {
        *h = 0;
        *s = 0;
    } else {
        *s = 1 - min / *i;
        
        if (max == r) {
            *h = fmod(60 * ((g - b) / delta) + 360, 360);
        } else if (max == g) {
            *h = 60 * ((b - r) / delta + 2);
        } else {
            *h = 60 * ((r - g) / delta + 4);
        }
    }
}

void process_image(const char *filename, Welford rgb_stats[3], Welford hsi_stats[3]) {
    int width, height, channels;
    unsigned char *image = stbi_load(filename, &width, &height, &channels, 3);
    
    if (!image) {
        printf("Erro ao carregar imagem: %s\n", filename);
        return;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            unsigned char r = image[idx];
            unsigned char g = image[idx + 1];
            unsigned char b = image[idx + 2];

            // Atualiza estatísticas RGB
            update_welford(&rgb_stats[0], r);
            update_welford(&rgb_stats[1], g);
            update_welford(&rgb_stats[2], b);

            // Converte para HSI
            double h, s, i_val;
            rgb_to_hsi(r, g, b, &h, &s, &i_val);

            // Atualiza estatísticas HSI
            update_welford(&hsi_stats[0], h);
            update_welford(&hsi_stats[1], s);
            update_welford(&hsi_stats[2], i_val * 255);
        }
    }

    stbi_image_free(image);
}

void calculate_thresholds(Welford stats[3], ChannelStats thresholds[3]) {
    for (int i = 0; i < 3; i++) {
        thresholds[i].mean = stats[i].mean;
        thresholds[i].std_dev = finalize_std_dev(&stats[i]);
        thresholds[i].min = stats[i].mean - 2 * thresholds[i].std_dev;
        thresholds[i].max = stats[i].mean + 2 * thresholds[i].std_dev;
    }
}

void save_thresholds_to_csv(ChannelStats rgb_thresholds[3], ChannelStats hsi_thresholds[3], const char *output_dir) {
    char filename[1024];
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    
    strftime(filename, sizeof(filename), "thresholds_%Y%m%d_%H%M%S.csv", tm_info);
    
    // Se output_dir foi especificado, usa ele
    char full_path[2048];
    if (output_dir != NULL && strlen(output_dir) > 0) {
        snprintf(full_path, sizeof(full_path), "%s/%s", output_dir, filename);
    } else {
        strcpy(full_path, filename);
    }
    
    FILE *csv_file = fopen(full_path, "w");
    if (!csv_file) {
        printf("Erro ao criar arquivo CSV: %s\n", full_path);
        return;
    }
    
    // Cabeçalho
    fprintf(csv_file, "Channel,Min,Max,Mean,Std_Dev\n");
    
    // Dados RGB
    char *rgb_channels[] = {"Red", "Green", "Blue"};
    for (int i = 0; i < 3; i++) {
        fprintf(csv_file, "RGB_%s,%.2f,%.2f,%.2f,%.2f\n",
                rgb_channels[i],
                rgb_thresholds[i].min,
                rgb_thresholds[i].max,
                rgb_thresholds[i].mean,
                rgb_thresholds[i].std_dev);
    }
    
    // Dados HSI
    char *hsi_channels[] = {"Hue", "Saturation", "Intensity"};
    for (int i = 0; i < 3; i++) {
        fprintf(csv_file, "HSI_%s,%.2f,%.2f,%.2f,%.2f\n",
                hsi_channels[i],
                hsi_thresholds[i].min,
                hsi_thresholds[i].max,
                hsi_thresholds[i].mean,
                hsi_thresholds[i].std_dev);
    }
    
    fclose(csv_file);
    printf("Arquivo CSV salvo: %s\n", full_path);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Uso: %s <diretorio_imagens> [diretorio_saida]\n", argv[0]);
        printf("Exemplo: %s ./imagens_fumaca ./resultados\n", argv[0]);
        return 1;
    }

    const char *output_dir = (argc == 3) ? argv[2] : ".";

    if (argc != 2) {
        printf("Uso: %s <diretorio_imagens>\n", argv[0]);
        return 1;
    }

    DIR *dir;
    struct dirent *entry;
    char path[1024];
    
    Welford rgb_stats[3] = {0};
    Welford hsi_stats[3] = {0};
    
    ChannelStats rgb_thresholds[3], hsi_thresholds[3];

    if ((dir = opendir(argv[1])) == NULL) {
        perror("Erro ao abrir diretório");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            snprintf(path, sizeof(path), "%s/%s", argv[1], entry->d_name);
            printf("Processando: %s\n", path);
            process_image(path, rgb_stats, hsi_stats);
        }
    }
    closedir(dir);

    // Calcula thresholds
    calculate_thresholds(rgb_stats, rgb_thresholds);
    calculate_thresholds(hsi_stats, hsi_thresholds);

    // Exibe resultados
    printf("\n=== THRESHOLDS RGB ===\n");
    char *rgb_channels[] = {"Vermelho", "Verde", "Azul"};
    for (int i = 0; i < 3; i++) {
        printf("%s: Min=%.2f Max=%.2f Mean=%.2f Std=%.2f\n",
               rgb_channels[i],
               rgb_thresholds[i].min,
               rgb_thresholds[i].max,
               rgb_thresholds[i].mean,
               rgb_thresholds[i].std_dev);
    }

    printf("\n=== THRESHOLDS HSI ===\n");
    char *hsi_channels[] = {"Matiz", "Saturação", "Intensidade"};
    for (int i = 0; i < 3; i++) {
        printf("%s: Min=%.2f Max=%.2f Mean=%.2f Std=%.2f\n",
               hsi_channels[i],
               hsi_thresholds[i].min,
               hsi_thresholds[i].max,
               hsi_thresholds[i].mean,
               hsi_thresholds[i].std_dev);
    }

        // Salva os thresholds em arquivo CSV
    save_thresholds_to_csv(rgb_thresholds, hsi_thresholds, output_dir);

    return 0;
}