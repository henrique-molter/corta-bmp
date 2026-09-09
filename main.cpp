#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

#pragma pack(push, 1)

// Cabeçalho do arquivo BMP (54 bytes)
struct CabecalhoArquivo {
    char tipo[2];             //dois primeiros bytes do arquivo (devem ser BM - 40 4D) - 2 bytes
    uint32_t   tamArquivo;    //do byte 3 a 6 (diz o tamanho)  - 4 bytes
    uint32_t reservado;       // 4 bytes reservados por padrão (devem ser 0)
    uint32_t   offset;       // 4 bytes (como é em truecolor, deve ser "54" por padrão)
    uint32_t tamCab;         // 4 bytes - (é por padrao 40)
    int32_t  largura;       // 4 bytes - altura da largura
    int32_t  altura;        // 4 bytes - altura da imagem
    uint16_t planos;         //por padrao é "1" -  2 bytes
    uint16_t bpp;           // numero de bits por pixel (por padrao é 24) 2 bytes
    uint32_t compressao;     // 4 bytes (deve ser 0)
    uint32_t tamImagem;      // 4 bytes - guarda o tamanho total da imagem
    int32_t resX;             //resoluçao horizontal da imagem em pixels por metro 4 bytes
    int32_t resY;             //resoluçao vertical da imagem em pixels por metro 4 bytes
    uint32_t clrUsadas;      // 4 bytes - (em true color deve ser 0)
    uint32_t clrImportantes; // 4 bytes - (em true color deve ser 0)
};


#pragma pack(pop)

// Estrutura da Imagem Interna para as futuras alterações
struct ImagemInterna {
    int largura;
    int altura;
    vector<unsigned char> pixels;
};

// 1. Exibe apenas o tamanho e as dimensões do arquivo BMP
bool exibirInformacoes(const string& caminho, CabecalhoArquivo& cab) {
    ifstream arq(caminho, ios::binary);
    if (!arq) {
        cerr << "Erro: Nao foi possivel abrir o arquivo." << endl;
        return false;
    }

    arq.read((char*)&cab, sizeof(cab));
    arq.close();

    // Validações básicas do formato bmp
    if (cab.tipo[0] != 'B' || cab.tipo[1] != 'M' || cab.bpp != 24 || cab.compressao != 0 || cab.largura <= 0 || cab.altura <= 0) {
        cerr << "Erro: Arquivo invalido ou nao suportado (deve ser BMP 24-bits nao comprimido)." << endl;
        return false;
    }

    // Exibição das informações básicas do arquivo de imagem
    cout << "--- Informacoes do Arquivo ---" << endl;
    cout << "Tamanho  : " << cab.tamArquivo << " bytes" << endl;
    cout << "Dimensoes: " << cab.largura << "x" << cab.altura << " pixels" << endl;

    return true;
}

// 2. Carrega os pixels para a ImagemInterna sem exibir nada no console
bool carregarImagemInterna(const string& caminho, const CabecalhoArquivo& cab, ImagemInterna& img) {
    ifstream arq(caminho, ios::binary);
    if (!arq) return false;

    img.largura = cab.largura;
    img.altura = cab.altura;

    int bytesPorLinha = img.largura * 3; // bpp = 24 bits = 3 bytes -> bytes por linha = bpp*numero de pixels da linha
    int padding = (4 - (bytesPorLinha % 4)) % 4; //caso os bytes por linha não forem multiplos de 4, o arquivo insere bytes nulos no final da linha.
    //a funçao padding descobre quantos bytes nulos tem no final da linha.

    img.pixels.resize(img.largura * img.altura * 3); //tamanho total do vetor na RAM, alocado em um bloco contínuo da memória.
    arq.seekg(cab.offset, ios::beg); //move os ponteiros de leitura do arquivo para logo depois dos 54bytes padrão de arquivos bmp, para que o programa não leia nenhum byte do cabeçalho como pixel.

    // Lê linha por linha, armazenando apenas os dados RGB e descartando o padding do arquivo
    for (int y = 0; y < img.altura; y++) {
        int inicioLinha = y * img.largura * 3;
        arq.read((char*)&img.pixels[inicioLinha], bytesPorLinha);

        if (padding > 0) {
            arq.seekg(padding, ios::cur); //descarta o padding do arquivo
        }
    }

    arq.close();
    return true;
}

int main() {
    string caminhoArquivo = "batman.bmp";
    CabecalhoArquivo cab;
    ImagemInterna img;

    if (exibirInformacoes(caminhoArquivo, cab)) {
        carregarImagemInterna(caminhoArquivo, cab, img);
    }

    return 0;
}