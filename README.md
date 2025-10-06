# Detector de Incêndio em C

Um programa simples em C para detectar focos de incêndio em imagens, utilizando técnicas de segmentação de cor pixel a pixel.

## Pré-requisitos

Para compilar e executar este projeto, você precisará de:

1.  **Visual Studio Code**: [https://code.visualstudio.com/](https://code.visualstudio.com/)
2.  **Um Compilador C/C++ (GCC)**:
    * **Windows**: Instale o **MinGW-w64** através do [MSYS2](https://www.msys2.org/). No terminal do MSYS2, rode `pacman -S mingw-w64-ucrt-x86_64-toolchain` e adicione a pasta `C:\msys64\ucrt64\bin` ao PATH do sistema.
    * **Linux (Debian/Ubuntu)**: `sudo apt update && sudo apt install build-essential`
    * **macOS**: `xcode-select --install`
3.  **Extensão do VS Code**: Instale o **"C/C++ Extension Pack"** da Microsoft a partir da loja de extensões do VS Code.

## Estrutura do Projeto

Certifique-se de que sua pasta de projeto contenha os seguintes arquivos:
seu-projeto/
|- detector_fogo.c
|- stb_image.h
|- stb_image_write.h
|- imagem_teste.jpg

## Como Compilar e Executar

1.  Abra a pasta do projeto no **Visual Studio Code**.

2.  Abra o **terminal integrado** do VS Code (`Ctrl` + ` ` ` ` ou pelo menu `Terminal > Novo Terminal`).

3.  **Compile** o programa usando o seguinte comando:
    ```bash
    gcc detector_fogo.c -o detector -lm
    ```
    * `gcc`: O comando para chamar o compilador.
    * `-o detector`: Define o nome do arquivo executável de saída como `detector`.
    * `-lm`: Faz o link com a biblioteca matemática, necessária para o projeto.

4.  **Execute** o programa com o comando:
    ```bash
    ./detector
    ```

## Saída

Após a execução, o programa irá analisar a `imagem_teste.jpg`, exibir uma mensagem no terminal indicando se um incêndio foi detectado e salvar três arquivos de imagem com os resultados do processamento (`resultado_fogo_rgb.png`, `resultado_fogo_ycbcr.png` e `resultado_fogo_final.png`).
