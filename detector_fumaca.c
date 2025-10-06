// =================================================================
//      PROJETO DE VISÃO COMPUTACIONAL - DETECTOR DE FUMAÇA
// =================================================================
// Este programa detecta a presença de fumaça em imagens com base
// na análise de cor pixel a pixel nos espaços RGB e HSI.
//
// Para compilar (no terminal):
// gcc detector_fumaca.c -o detector -lm
//
// Para executar:
// ./detector
// =================================================================

// -----------------------------------------------------------------
// 1. CABEÇALHOS E CONFIGURAÇÃO DA BIBLIOTECA STB
// -----------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h> // Para usar o tipo 'bool' (true/false)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// -----------------------------------------------------------------
// 2. DEFINIÇÃO DA ESTRUTURA DA IMAGEM
// -----------------------------------------------------------------
typedef struct {
    unsigned char *data;
    int width;
    int height;
    int channels;
} Image;

// -----------------------------------------------------------------
// 3. FUNÇÕES DE PROCESSAMENTO
// -----------------------------------------------------------------

/**
 * @brief Segmenta pixels de fumaça com base em regras de cor no espaço RGB.
 * A fumaça em RGB geralmente é clara (R,G,B altos) e acinzentada (R,G,B próximos).
 */
Image segmentar_fumaca_rgb(Image *img) {
    unsigned char *output_data = (unsigned char *)malloc(img->width * img->height);
    Image mascara = {output_data, img->width, img->height, 1};
    const int BRILHO_MINIMO = 190;
    const int TOLERANCIA_CINZA = 25;

    for (int i = 0; i < img->width * img->height; ++i) {
        unsigned char r = img->data[i * img->channels];
        unsigned char g = img->data[i * img->channels + 1];
        unsigned char b = img->data[i * img->channels + 2];

        if (r > BRILHO_MINIMO && g > BRILHO_MINIMO && b > BRILHO_MINIMO &&
            abs(r - g) < TOLERANCIA_CINZA && 
            abs(r - b) < TOLERANCIA_CINZA && 
            abs(g - b) < TOLERANCIA_CINZA) {
            mascara.data[i] = 255; // Branco (fumaça)
        } else {
            mascara.data[i] = 0;   // Preto (não fumaça)
        }
    }
    return mascara;
}

/**
 * @brief Converte uma imagem do espaço de cor RGB para HSI.
 */
Image rgb_para_hsi(Image *img) {
    unsigned char *hsi_data = (unsigned char *)malloc(img->width * img->height * 3);
    Image img_hsi = {hsi_data, img->width, img->height, 3};

    for (int i = 0; i < img->width * img->height; ++i) {
        float r = img->data[i * img->channels] / 255.0f;
        float g = img->data[i * img->channels + 1] / 255.0f;
        float b = img->data[i * img->channels + 2] / 255.0f;
        float h = 0.0, s = 0.0, in = (r + g + b) / 3.0f;

        float min_val = fmin(r, fmin(g, b));
        if (in > 0.001) s = 1.0f - min_val / in;
        
        if (s > 0.001) {
            float num = 0.5f * ((r - g) + (r - b));
            float den = sqrtf((r - g) * (r - g) + (r - b) * (g - b));
            if (den > 0.001) {
                float theta = acosf(num / den) * (180.0 / M_PI);
                if (b > g) h = 360.0f - theta; else h = theta;
            }
        }
        
        img_hsi.data[i * 3]     = (unsigned char)(h / 360.0f * 255.0f); // H
        img_hsi.data[i * 3 + 1] = (unsigned char)(s * 255.0f);         // S
        img_hsi.data[i * 3 + 2] = (unsigned char)(in * 255.0f);       // I
    }
    return img_hsi;
}

/**
 * @brief Segmenta pixels de fumaça com base em regras no espaço HSI.
 * A fumaça em HSI tem baixa Saturação (S) e média a alta Intensidade (I).
 */
Image segmentar_fumaca_hsi(Image *img_hsi) {
    unsigned char *output_data = (unsigned char *)malloc(img_hsi->width * img_hsi->height);
    Image mascara = {output_data, img_hsi->width, img_hsi->height, 1};
    const int SATURACAO_MAXIMA = 50; // Quão "cinza" o pixel deve ser (quanto menor, mais cinza)
    const int INTENSIDADE_MINIMA = 150; // Quão "claro" o pixel deve ser

    for (int i = 0; i < img_hsi->width * img_hsi->height; ++i) {
        unsigned char s = img_hsi->data[i * 3 + 1];
        unsigned char in = img_hsi->data[i * 3 + 2];

        if (s < SATURACAO_MAXIMA && in > INTENSIDADE_MINIMA) {
            mascara.data[i] = 255; // Branco (fumaça)
        } else {
            mascara.data[i] = 0;   // Preto (não fumaça)
        }
    }
    return mascara;
}

/**
 * @brief Combina duas máscaras usando uma operação lógica E (AND).
 */
Image combinar_mascaras(Image *mascara_a, Image *mascara_b) {
    unsigned char *output_data = (unsigned char *)malloc(mascara_a->width * mascara_a->height);
    Image mascara_final = {output_data, mascara_a->width, mascara_a->height, 1};
    for (int i = 0; i < mascara_a->width * mascara_a->height; ++i) {
        if (mascara_a->data[i] == 255 && mascara_b->data[i] == 255) {
            mascara_final.data[i] = 255;
        } else {
            mascara_final.data[i] = 0;
        }
    }
    return mascara_final;
}

/**
 * @brief Analisa a máscara final para decidir se há fumaça.
 */
bool verificar_presenca_fumaca(Image *mascara, float threshold_percent) {
    long smoke_pixel_count = 0;
    long total_pixels = mascara->width * mascara->height;
    for (int i = 0; i < total_pixels; ++i) {
        if (mascara->data[i] == 255) {
            smoke_pixel_count++;
        }
    }

    float smoke_percentage = 100.0f * smoke_pixel_count / total_pixels;
    printf("Análise: %.4f%% da imagem foi classificada como fumaça.\n", smoke_percentage);
    return smoke_percentage > threshold_percent;
}

// -----------------------------------------------------------------
// 4. FUNÇÃO PRINCIPAL (MAIN)
// -----------------------------------------------------------------
int main() {
    int width, height, channels;
    unsigned char *data = stbi_load("imagem_teste.jpg", &width, &height, &channels, 0);
    if (data == NULL) {
        printf("ERRO: Não foi possível carregar a imagem.\n");
        printf("Verifique se 'imagem_teste.jpg' está na mesma pasta do executável.\n");
        return 1;
    }
    Image img = {data, width, height, channels};
    printf("Imagem '%s' carregada: %d x %d, Canais: %d\n\n", "imagem_teste.jpg", img.width, img.height, img.channels);

    // ETAPA 1: Segmentação com RGB
    Image mascara_rgb = segmentar_fumaca_rgb(&img);
    stbi_write_png("resultado_fumaca_rgb.png", mascara_rgb.width, mascara_rgb.height, 1, mascara_rgb.data, mascara_rgb.width);
    printf("Passo 1: Máscara RGB salva como 'resultado_fumaca_rgb.png'\n");

    // ETAPA 2: Conversão para HSI e Segmentação
    Image img_hsi = rgb_para_hsi(&img);
    Image mascara_hsi = segmentar_fumaca_hsi(&img_hsi);
    stbi_write_png("resultado_fumaca_hsi.png", mascara_hsi.width, mascara_hsi.height, 1, mascara_hsi.data, mascara_hsi.width);
    printf("Passo 2: Máscara HSI salva como 'resultado_fumaca_hsi.png'\n");

    // ETAPA 3: Combinar as máscaras
    Image mascara_final = combinar_mascaras(&mascara_rgb, &mascara_hsi);
    stbi_write_png("resultado_fumaca_final.png", mascara_final.width, mascara_final.height, 1, mascara_final.data, mascara_final.width);
    printf("Passo 3: Máscara combinada salva como 'resultado_fumaca_final.png'\n\n");

    // ETAPA 4: Tomar a decisão final
    float deteccao_threshold = 0.2; // Limiar: alerta se mais de 0.2% da imagem for fumaça.
    bool fumaca_detectada = verificar_presenca_fumaca(&mascara_final, deteccao_threshold);

    if (fumaca_detectada) {
        printf("\n=======================================================\n");
        printf(">>> ALERTA: Possível foco de fumaça detectado! <<<\n");
        printf("=======================================================\n");
    } else {
        printf("\n========================================================\n");
        printf(">>> Nenhum sinal significativo de fumaça detectado. <<<\n");
        printf("========================================================\n");
    }

    // ETAPA 5: Liberar toda a memória alocada
    stbi_image_free(img.data);
    free(mascara_rgb.data);
    free(img_hsi.data);
    free(mascara_hsi.data);
    free(mascara_final.data);
    
    printf("\nProcesso concluído.\n");
    return 0;
}