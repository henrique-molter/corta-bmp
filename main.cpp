#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

#pragma pack(push, 1)

// Cabeçalho do arquivo BMP (54 bytes)
struct CabecalhoArquivo {
    char     tipo[2];        // dois primeiros bytes do arquivo (devem ser BM - 40 4D) - 2 bytes
    uint32_t tamArquivo;     // do byte 3 a 6 (diz o tamanho) - 4 bytes
    uint32_t reservado;      // 4 bytes reservados por padrão (devem ser 0)
    uint32_t offset;         // 4 bytes (como é em truecolor, deve ser "54" por padrão)
    uint32_t tamCab;         // 4 bytes - (é por padrao 40)
    int32_t  largura;        // 4 bytes - altura da largura
    int32_t  altura;         // 4 bytes - altura da imagem
    uint16_t planos;         // por padrao é "1" - 2 bytes
    uint16_t bpp;            // numero de bits por pixel (por padrao é 24) 2 bytes
    uint32_t compressao;     // 4 bytes (deve ser 0)
    uint32_t tamImagem;      // 4 bytes - guarda o tamanho total da imagem
    int32_t  resX;           // resoluçao horizontal da imagem em pixels por metro 4 bytes
    int32_t  resY;           // resoluçao vertical da imagem em pixels por metro 4 bytes
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
bool AbreArq(const string& caminho, CabecalhoArquivo& cab) {
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
} //funcoes json: abrir arquivo, conversao cinza, recorta, grava bmp

// 2. Carrega os pixels para a ImagemInterna sem exibir nada no console
bool carregarImagemInterna(const string& caminho, const CabecalhoArquivo& cab, ImagemInterna& img) {
    ifstream arq(caminho, ios::binary);
    if (!arq) return false;

    img.largura = cab.largura;
    img.altura = cab.altura;

    int bytesPorLinha = img.largura * 3; // bpp = 24 bits = 3 bytes -> bytes por linha = bpp*numero de pixels da linha
    int padding = (4 - (bytesPorLinha % 4)) % 4; // caso os bytes por linha não forem multiplos de 4, o arquivo insere bytes nulos no final da linha.
    // a funçao padding descobre quantos bytes nulos tem no final da linha.

    img.pixels.resize(img.largura * img.altura * 3); // tamanho total do vetor na RAM, alocado em um bloco contínuo da memória.
    arq.seekg(cab.offset, ios::beg); // move os ponteiros de leitura do arquivo para logo depois dos 54bytes padrão de arquivos bmp, para que o programa não leia nenhum byte do cabeçalho como pixel.

    // Lê linha por linha, armazenando apenas os dados RGB e descartando o padding do arquivo
    for (int y = 0; y < img.altura; y++) {
        int inicioLinha = y * img.largura * 3;
        arq.read((char*)&img.pixels[inicioLinha], bytesPorLinha);

        if (padding > 0) {
            arq.seekg(padding, ios::cur); // descarta o padding do arquivo
        }
    }

    arq.close();
    return true;
}

// 3. Converte a imagem na RAM para escala de cinza usando a fórmula de luminância
void ConvGray(ImagemInterna& img) { //o "&" depois de ImagemInterna aponta que a funçao altera diretamente a estrutura original do vetor
    for (size_t i = 0; i < img.pixels.size(); i += 3) {//percorre o vetor pixels, avançando de 3 em 3, até chegar no valor total de pixels na linha.
        unsigned char b = img.pixels[i];//azul
        unsigned char g = img.pixels[i + 1];//verde
        unsigned char r = img.pixels[i + 2];//vermelho

        // Fórmula da luminância: Y = 0.299R + 0.587G + 0.114B
        unsigned char cinza = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);//descarta a parte decimal do calculo e encaixa no intervalo de 0-255.

        img.pixels[i]     = cinza; // Blue
        img.pixels[i + 1] = cinza; // Green
        img.pixels[i + 2] = cinza; // Red
    }
}

// 4. Recorta uma sub-região da imagem e gera uma nova ImagemInterna
bool RecImagem(const ImagemInterna& origem, ImagemInterna& destino, int x, int y, int larguraCorte, int alturaCorte) {
                //^-impede que a Imagem interna seja modificada na função
    if (x < 0 || y < 0 || x + larguraCorte > origem.largura || y + alturaCorte > origem.altura) {//validações de segurança
        cerr << "Erro: Dimensoes de corte fora dos limites da imagem original." << endl;
        return false;
    }
    //atribui os novos valores de largura e altura após o recorte
    destino.largura = larguraCorte;
    destino.altura = alturaCorte;
    destino.pixels.resize(larguraCorte * alturaCorte * 3);//redimensiona o vetor de pixels da struct destino.

    for (int lin = 0; lin < alturaCorte; lin++) {
        for (int col = 0; col < larguraCorte; col++) {//laços que percorrem a nova região recortada linha por linha e coluna por coluna
            int idxOrigem  = ((y + lin) * origem.largura + (x + col)) * 3;//mapeia a posição da coordenada bidimensional no vetor em que foi alocada.
            int idxDestino = (lin * larguraCorte + col) * 3;

            destino.pixels[idxDestino]     = origem.pixels[idxOrigem];     // B
            destino.pixels[idxDestino + 1] = origem.pixels[idxOrigem + 1]; // G
            destino.pixels[idxDestino + 2] = origem.pixels[idxOrigem + 2]; // R
        }
    }
    return true;
}

// 5. Salva a ImagemInterna em um novo arquivo BMP (gerando o cabeçalho e reinsirindo o padding)
bool SaveBMP(const string& caminho, const ImagemInterna& img) {//o"const evita que a função mude os valores das structs e variaveis.
    ofstream arq(caminho, ios::binary);
    if (!arq) {
        cerr << "Erro: Nao foi possivel criar o arquivo BMP de saida." << endl;
        return false;
    }

    int bytesPorLinha = img.largura * 3;//calcula a quantidade de bytes uteis de cor por linha, como cada pixel utiliza 3 bytes, multiplica por 3.
    int padding = (4 - (bytesPorLinha % 4)) % 4; //o padding ja foi explicado anteriormente
    uint32_t tamImagem = (bytesPorLinha + padding) * img.altura;//calcula o tamanho total da matriz em bytes, considerando os ocupados com "lixo"

    CabecalhoArquivo cab = {}; //inicializa cab com zeros em todos os campos.
    //define todos os campos do cabeçalho conforme os padroes de BMP
    cab.tipo[0] = 'B'; cab.tipo[1] = 'M';
    cab.tamArquivo = 54 + tamImagem;
    cab.offset = 54;
    cab.tamCab = 40;
    cab.largura = img.largura;
    cab.altura = img.altura;
    cab.planos = 1;
    cab.bpp = 24;
    cab.compressao = 0;
    cab.tamImagem = tamImagem;

    arq.write((char*)&cab, sizeof(cab));//gravação direta da struct em uma unica linha

    unsigned char paddingZero[3] = {0, 0, 0};//prepara um vetor de 3bytes com valor = 0 para ser usado no final de cada linha.
    for (int y = 0; y < img.altura; y++) {//laço que percorre todas as linhas do arwquivo
        int inicioLinha = y * img.largura * 3;//calcula a posição no vetor pixels onde começam os dados da linha atual.
        arq.write((char*)&img.pixels[inicioLinha], bytesPorLinha);//escreve no arquivo todos os bytes de cor referentes à linha atual.
        if (padding > 0) {
            arq.write((char*)paddingZero, padding);//adiciona os paddings necessários para o numero de bytes na linha ser multiplo de 4.
        }
    }

    arq.close();//grava as informações no arquivo
    return true;
}

// 6. Exporta a matriz de pixels para um arquivo em formato texto (.txt)
bool exportarParaTexto(const string& caminho, const ImagemInterna& img) {
    ofstream arq(caminho);
    if (!arq) {
        cerr << "Erro: Nao foi possivel criar o arquivo de texto." << endl;
        return false;
    }

    arq << "Largura: " << img.largura << "\n";
    arq << "Altura: " << img.altura << "\n\n";

    for (int y = 0; y < img.altura; y++) {
        for (int x = 0; x < img.largura; x++) {
            int idx = (y * img.largura + x) * 3;
            int b = img.pixels[idx];
            int g = img.pixels[idx + 1];
            int r = img.pixels[idx + 2];

            arq << "(" << r << "," << g << "," << b << ") ";
        }
        arq << "\n";
    }

    arq.close();//grava as informações no arquivo
    return true;
}

int main() {
    string caminhoArquivo = "batman.bmp";
    CabecalhoArquivo cab;
    ImagemInterna img;

    if (AbreArq(caminhoArquivo, cab)) {
        if (carregarImagemInterna(caminhoArquivo, cab, img)) {
            cout << "Imagem carregada com sucesso na RAM!" << endl;

            // Exemplo de uso das funções:
            // 1. Converter para cinza e salvar BMP
            // ConvGray(img);
            // SaveBMP("batman_cinza.bmp", img);

            // 2. Exportar matriz para texto
            // exportarParaTexto("batman_matriz.txt", img);
        }
    }

    return 0;
}