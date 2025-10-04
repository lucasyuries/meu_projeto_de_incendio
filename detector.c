// =================================================================
//          PROJETO DE VISÃO COMPUTACIONAL - DETECTOR DE FOGO
// =================================================================
// Este programa detecta a presença de fogo em imagens com base
// na análise de cor pixel a pixel nos espaços RGB e YCbCr.
//
// Para compilar (no terminal):
// gcc detector_fogo.c -o detector -lm
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

// Define as macros para implementar as bibliotecas stb_image.
// Os arquivos .h devem estar na mesma pasta do seu código.
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
 * @brief Segmenta pixels de fogo com base em regras de cor no espaço RGB.
 * O fogo em RGB geralmente tem R > G > B e um valor alto de R.
 */
Image segmentar_fogo_rgb(Image *img) {
    unsigned char *output_data = (unsigned char *)malloc(img->width * img->height);
    Image mascara_fogo = {output_data, img->width, img->height, 1};
    const int R_THRESHOLD = 210; // Limiar de brilho para o canal vermelho

    for (int i = 0; i < img->width * img->height; ++i) {
        unsigned char r = img->data[i * img->channels];
        unsigned char g = img->data[i * img->channels + 1];
        unsigned char b = img->data[i * img->channels + 2];

        if (r > R_THRESHOLD && r > g && g > b) {
            mascara_fogo.data[i] = 255; // Branco (fogo)
        } else {
            mascara_fogo.data[i] = 0;   // Preto (não fogo)
        }
    }
    return mascara_fogo;
}

/**
 * @brief Converte uma imagem do espaço de cor RGB para YCbCr.
 * Y = Luma (brilho), Cb = Croma Azul, Cr = Croma Vermelho.
 */
Image rgb_para_ycbcr(Image *img) {
    unsigned char *ycbcr_data = (unsigned char *)malloc(img->width * img->height * 3);
    Image img_ycbcr = {ycbcr_data, img->width, img->height, 3};

    for (int i = 0; i < img->width * img->height; ++i) {
        float r = img->data[i * img->channels];
        float g = img->data[i * img->channels + 1];
        float b = img->data[i * img->channels + 2];

        img_ycbcr.data[i * 3]     = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
        img_ycbcr.data[i * 3 + 1] = (unsigned char)(128 - 0.168736 * r - 0.331264 * g + 0.5 * b);
        img_ycbcr.data[i * 3 + 2] = (unsigned char)(128 + 0.5 * r - 0.418688 * g - 0.081312 * b);
    }
    return img_ycbcr;
}

/**
 * @brief Segmenta pixels de fogo com base em regras no espaço YCbCr.
 * O fogo em YCbCr tem Y alto, Cb baixo e Cr alto.
 */
Image segmentar_fogo_ycbcr(Image *img_ycbcr) {
    unsigned char *output_data = (unsigned char *)malloc(img_ycbcr->width * img_ycbcr->height);
    Image mascara_fogo = {output_data, img_ycbcr->width, img_ycbcr->height, 1};

    for (int i = 0; i < img_ycbcr->width * img_ycbcr->height; ++i) {
        unsigned char y  = img_ycbcr->data[i * 3];
        unsigned char cb = img_ycbcr->data[i * 3 + 1];
        unsigned char cr = img_ycbcr->data[i * 3 + 2];

        if (y > 130 && cb < 120 && cr > 150) {
            mascara_fogo.data[i] = 255; // Branco (fogo)
        } else {
            mascara_fogo.data[i] = 0;   // Preto (não fogo)
        }
    }
    return mascara_fogo;
}

/**
 * @brief Combina duas máscaras usando uma operação lógica E (AND).
 * Um pixel só é branco no final se for branco em AMBAS as máscaras de entrada.
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
 * @brief Analisa a máscara final para decidir se há fogo.
 * Retorna 'true' se a porcentagem de pixels de fogo for maior que o limiar.
 */
bool verificar_presenca_fogo(Image *mascara, float threshold_percent) {
    long fire_pixel_count = 0;
    long total_pixels = mascara->width * mascara->height;

    for (int i = 0; i < total_pixels; ++i) {
        if (mascara->data[i] == 255) {
            fire_pixel_count++;
        }
    }

    float fire_percentage = 100.0f * fire_pixel_count / total_pixels;
    printf("Análise: %.4f%% da imagem foi classificada como fogo.\n", fire_percentage);

    return fire_percentage > threshold_percent;
}


// -----------------------------------------------------------------
// 4. FUNÇÃO PRINCIPAL (MAIN)
// -----------------------------------------------------------------
int main() {
    // Carrega a imagem de entrada (certifique-se que ela existe na pasta)
    int width, height, channels;
    unsigned char *data = stbi_load("imagem_teste.jpg", &width, &height, &channels, 0);
    if (data == NULL) {
        printf("ERRO: Não foi possível carregar a imagem.\n");
        printf("Verifique se o arquivo 'imagem_teste.jpg' está na mesma pasta do executável.\n");
        return 1;
    }
    Image img = {data, width, height, channels};
    printf("Imagem '%s' carregada: %d x %d, Canais: %d\n\n", "imagem_teste.jpg", img.width, img.height, img.channels);

    // --- ETAPA 1: Segmentação com RGB ---
    Image mascara_fogo_rgb = segmentar_fogo_rgb(&img);
    stbi_write_png("resultado_fogo_rgb.png", mascara_fogo_rgb.width, mascara_fogo_rgb.height, 1, mascara_fogo_rgb.data, mascara_fogo_rgb.width);
    printf("Passo 1: Máscara RGB salva como 'resultado_fogo_rgb.png'\n");

    // --- ETAPA 2: Conversão para YCbCr e Segmentação ---
    Image img_ycbcr = rgb_para_ycbcr(&img);
    Image mascara_fogo_ycbcr = segmentar_fogo_ycbcr(&img_ycbcr);
    stbi_write_png("resultado_fogo_ycbcr.png", mascara_fogo_ycbcr.width, mascara_fogo_ycbcr.height, 1, mascara_fogo_ycbcr.data, mascara_fogo_ycbcr.width);
    printf("Passo 2: Máscara YCbCr salva como 'resultado_fogo_ycbcr.png'\n");

    // --- ETAPA 3: Combinar as máscaras para mais precisão ---
    Image mascara_final_fogo = combinar_mascaras(&mascara_fogo_rgb, &mascara_fogo_ycbcr);
    stbi_write_png("resultado_fogo_final.png", mascara_final_fogo.width, mascara_final_fogo.height, 1, mascara_final_fogo.data, mascara_final_fogo.width);
    printf("Passo 3: Máscara combinada salva como 'resultado_fogo_final.png'\n\n");

    // --- ETAPA 4: Tomar a decisão final ---
    float deteccao_threshold = 0.1; // Limiar: alerta se mais de 0.1% da imagem for fogo.
    bool fogo_detectado = verificar_presenca_fogo(&mascara_final_fogo, deteccao_threshold);

    if (fogo_detectado) {
        printf("\n===================================================\n");
        printf(">>> ALERTA: Possível foco de incêndio detectado! <<<\n");
        printf("===================================================\n");
    } else {
        printf("\n====================================================\n");
        printf(">>> Nenhum sinal claro de incêndio detectado. <<<\n");
        printf("====================================================\n");
    }

    // --- ETAPA 5: Liberar toda a memória alocada ---
    stbi_image_free(img.data);
    free(mascara_fogo_rgb.data);
    free(img_ycbcr.data);
    free(mascara_fogo_ycbcr.data);
    free(mascara_final_fogo.data);
    
    printf("\nProcesso concluído.\n");
    return 0;
}