#include <cstdint>
#include <iostream>

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

int main() {

    return 0;
}