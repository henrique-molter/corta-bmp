#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
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

// Estrutura para armazenar a imagem na memória
struct ImagemInterna {
    int largura = 0;
    int altura = 0;
    vector<unsigned char> pixels; // Vetor contínuo de pixels BGR
};

// Leitor JSON
int encontraChave(const string& json, const string& chave);
string extraiValorJSON(const string& json, const string& chave);
void extraiVetorValores(const string& strVetor, int& v1, int& v2);

//Imagem
bool AbreArq(const string& caminho, CabecalhoArquivo& cab);
bool carregarImagemInterna(const string& caminho, const CabecalhoArquivo& cab, ImagemInterna& img);
void ConvGray(ImagemInterna& img);
bool RecImagem(const ImagemInterna& origem, ImagemInterna& destino, int x, int y, int larguraCorte, int alturaCorte);
bool SaveBMP(const string& caminho, const ImagemInterna& img);
bool exportarParaTexto(const string& caminho, const ImagemInterna& img);

// Grava JSON
void GravaCanalMatriz(ofstream& arq, const ImagemInterna& img, int deslocamentoCanal);
bool GravaJSON(const string& caminho, const ImagemInterna& img, char canal);


int main(int argc, char* argv[])
{
    // Verifica a linha de comando
    if (argc < 2) {
        cerr << "Erro: Informe o arquivo de script JSON (.mpi)" << endl;
        return -1;
    }

    string nomeScript = argv[1];
    // verifica se a chave não foi encontrada com "string::npos"
    // string::npos = nenhuma posição encontrada
    if (nomeScript.find('.') == string::npos) {
        // Se a extensão for omitida, assume .mpi como padrão
        nomeScript += ".mpi";
    }

    // Abre o arquivo de script para leitura
    ifstream arqScript(nomeScript);
    if (!arqScript.is_open()) {
        cerr << "Erro: Nao foi possivel abrir o arquivo de script: " << nomeScript << endl;
        return -1;
    }

    ImagemInterna imgAtual;
    string linha;

    cerr << "Iniciando execucao do script: " << nomeScript << endl;

    // Leitura linha por linha do script
    while (getline(arqScript, linha)) {
        if (linha.find('{') == string::npos) continue; // Pula linhas sem objeto JSON

        string cmd = extraiValorJSON(linha, "cmd");

        // verificação dos comandos de cada função
        if (cmd == "Abra") {
            string arq = extraiValorJSON(linha, "arq");
            if (arq.find('.') == string::npos) arq += ".bmp";

            cerr << "Executando comando 'Abra': " << arq << endl;

            CabecalhoArquivo cab;
            if (AbreArq(arq, cab)) {
                carregarImagemInterna(arq, cab, imgAtual);
            }
        }
        else if (cmd == "ConvGray") {
            cerr << "Executando comando 'ConvGray': " << endl;
            ConvGray(imgAtual);
        }
        else if (cmd == "Recorta") {
            int x = 0, y = 0, larg = 0, alt = 0;
            string strIni = extraiValorJSON(linha, "ini");
            string strTam = extraiValorJSON(linha, "tam");
            string strFim = extraiValorJSON(linha, "fim");

            extraiVetorValores(strIni, x, y);

            if (!strTam.empty()) {
                extraiVetorValores(strTam, larg, alt);
            } else if (!strFim.empty()) {
                int xFim = 0, yFim = 0;
                extraiVetorValores(strFim, xFim, yFim);
                larg = xFim - x;
                alt = yFim - y;
            }

            cerr << "Executando comando 'Recorta' de (" << x << "," << y << ") - tamanho (" << larg << "x" << alt << ")" << endl;

            ImagemInterna imgRecortada;
            if (RecImagem(imgAtual, imgRecortada, x, y, larg, alt)) {
                imgAtual = imgRecortada; // Atualiza a imagem corrente com a recortada
            }
        }
        else if (cmd == "GravaBMP") {
            string arq = extraiValorJSON(linha, "arq");
            if (arq.find('.') == string::npos) arq += ".bmp";

            cerr << "Executando comando 'GravaBMP': " << arq << endl;
            SaveBMP(arq, imgAtual);
        }
        else if (cmd == "GravaJson") {
            string arq = extraiValorJSON(linha, "arq");
            if (arq.find('.') == string::npos) arq += ".jbmp";

            string canalStr = extraiValorJSON(linha, "canal");
            char canal = canalStr.empty() ? 'A' : canalStr[0];

            cerr << "Executando comando '" << cmd << "' canal '" << canal << "': " << arq << endl;
            GravaJSON(arq, imgAtual, canal);
        }
    }

    arqScript.close();
    cerr << "Execucao concluida com sucesso!" << endl;

    return 0;
}


// MANIPULAÇÃO DE ARQUIVO DE TEXTO - JSON
int encontraChave(const string& json, const string& chave){
    // procurando pelas "chaves"
    string alvo = "\"" + chave + "\"";
    int posicaoAlvo = json.find(alvo);

    // verifica se a chave não foi encontrada com "string::npos"
    // string::npos = nenhuma posição encontrada
    if(posicaoAlvo == string::npos){
        return -1;
        // retorna o valor numérico padrão caso não encontre a chave
    }
    // retorna a posição da chave
    return posicaoAlvo;
}

string extraiValorJSON(const string& json, const string& chave) {
    // verifica a existência da chave para começar a extrair o valor
    int posicaoAlvo = encontraChave(json, chave);
    if (posicaoAlvo == -1) {
        return ""; // retorna formato inválido se a chave não existir
    }

    // busca pular os dois pontos
    string alvo = "\"" + chave + "\"";
    // os ':' começam após o início das '{'
    int posicao = json.find(':', posicaoAlvo + alvo.length());

    if (posicao == string::npos){
        return "";// retorna formato inválido
    }
    posicao++; // pula os ':'

    // após os ':', retira(pula) o espaço em branco que há
    while (posicao < json.length() && isspace(json[posicao])){
        posicao++; // pula os espaços
    }

    if (posicao >= json.length()) return "";

    // extrai o valor da posição atual
    char valor = json[posicao];

    // se for '"' então é uma string
    if (valor == '"'){
        posicao++;// pula aspa inicial e parte para a leitura dos caracteres
        int indice = posicao;

        while (posicao < json.length()){
            // verifica se o valor atual é '"' e
            // se o valor antes do valor atual não é '\'
            if (json[posicao] == '"' && json[posicao - 1] != '\\'){
                break; // sai da string (fim)
            }
            posicao++;// avança uma posição
        }
        // retorna valor da string extraída
        return json.substr(indice, posicao - indice);
    }
    // tratamento para vetores/arrays JSON - [x, y]
    else if (valor == '[') {
        int indice = posicao;
        int colcheteFecha = json.find(']', posicao);
        if (colcheteFecha != string::npos) {
            return json.substr(indice + 1, colcheteFecha - indice - 1);
        }
    }
    else if (valor == 't' || valor == 'f' || valor == 'n') {
        // verifica se o valor é booleano ou nulo
        int indice = posicao;
        while (posicao < json.length() && isalpha(json[posicao])){
            posicao++;// avança para a próxima posição
        }
        return json.substr(indice, posicao - indice);
    } else if (valor == '-' || isdigit(valor)){
        int indice = posicao;

        while (posicao < json.length()) {
            char digito = json[posicao];
            // "e"/"E" serve para verificar se caso o digito estiver em notação científica
            if (digito == '-' || digito == '.' || isdigit(digito) || digito == 'e' || digito == 'E') {
                posicao++;
            } else {break;}
        }
        return json.substr(indice, posicao - indice);
    }
    return "";
}

void extraiVetorValores(const string& strVetor, int& v1, int& v2) {
    v1 = 0; v2 = 0;
    // sscanf() = extrai os valores de um texto em vez do teclado
    sscanf(strVetor.c_str(), "%d, %d", &v1, &v2);
    if (v1 == 0 && v2 == 0) {
        sscanf(strVetor.c_str(), "%d %d", &v1, &v2);
    }
}

void GravaCanalMatriz(ofstream& arq, const ImagemInterna& img, int deslocamentoCanal) {
    for (int y = 0; y < img.altura; y++) {
        arq << "    [";
        for (int x = 0; x < img.largura; x++) {
            // Índice do canal específico (0=Blue, 1=Green, 2=Red)
            int idx = (y * img.largura + x) * 3 + deslocamentoCanal;
            int valorCor = static_cast<int>(img.pixels[idx]);

            arq << valorCor;
            if (x < img.largura - 1) arq << ", "; // Separa os valores da linha
        }
        arq << "]";
        if (y < img.altura - 1) arq << ",\n"; // Separa as linhas da matriz
        else arq << "\n";
    }
}

bool GravaJSON(const string& caminho, const ImagemInterna& img, char canal) {
    ofstream arq(caminho);
    if (!arq) {
        cerr << "Erro: Nao foi possivel criar o arquivo de texto JSON." << endl;
        return false;
    }

    arq << "{\n";
    arq << "  \"largura\": " << img.largura << ",\n";
    arq << "  \"altura\": " << img.altura << ",\n";

    // Grava apenas os canais solicitados no parâmetro 'canal' ('R', 'G', 'B' ou 'A' para todos os canais)
    if (canal == 'R' || canal == 'A') {
        arq << "  \"resultado_R\": [\n";
        GravaCanalMatriz(arq, img, 2); // 2 é o deslocamento do Vermelho (Red)
        arq << "  ]";
        if (canal == 'A' || canal == 'G' || canal == 'B') arq << ",\n";
        else arq << "\n";
    }

    if (canal == 'G' || canal == 'A') {
        arq << "  \"resultado_G\": [\n";
        GravaCanalMatriz(arq, img, 1); // 1 é o deslocamento do Verde (Green)
        arq << "  ]";
        if (canal == 'A' || canal == 'B') arq << ",\n";
        else arq << "\n";
    }

    if (canal == 'B' || canal == 'A') {
        arq << "  \"resultado_B\": [\n";
        GravaCanalMatriz(arq, img, 0); // 0 é o deslocamento do Azul (Blue)
        arq << "  ]\n";
    }

    arq << "}\n";

    arq.close();
    return true;
}

// MANIPULAÇÃO DE ARQUIVO DE IMAGEM - BMP
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
}

// 2. Carrega os pixels para a ImagemInterna sem exibir nada no console
bool carregarImagemInterna(const string& caminho, const CabecalhoArquivo& cab, ImagemInterna& img) {
    ifstream arq(caminho, ios::binary);
    if (!arq) return false;

    img.largura = cab.largura;
    img.altura = cab.altura;

    int bytesPorLinha = img.largura * 3; // bpp = 24 bits = 3 bytes -> bytes por linha = bpp*numero de pixels da linha
    int padding = (4 - (bytesPorLinha % 4)) % 4; // caso os bytes por linha não forem multiplos de 4, o arquivo insere bytes nulos no final da linha.

    img.pixels.resize(img.largura * img.altura * 3); // tamanho total do vetor na RAM, alocado em um bloco contínuo da memória.
    arq.seekg(cab.offset, ios::beg); // move os ponteiros de leitura do arquivo para logo depois dos 54bytes padrão de arquivos bmp

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
void ConvGray(ImagemInterna& img) {
    for (size_t i = 0; i < img.pixels.size(); i += 3) {
        unsigned char b = img.pixels[i];      // azul
        unsigned char g = img.pixels[i + 1];  // verde
        unsigned char r = img.pixels[i + 2];  // vermelho

        // Fórmula da luminância: Y = 0.299R + 0.587G + 0.114B
        unsigned char cinza = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);

        img.pixels[i]     = cinza; // Blue
        img.pixels[i + 1] = cinza; // Green
        img.pixels[i + 2] = cinza; // Red
    }
}

// 4. Recorta uma sub-região da imagem e gera uma nova ImagemInterna
bool RecImagem(const ImagemInterna& origem, ImagemInterna& destino, int x, int y, int larguraCorte, int alturaCorte) {
    if (x < 0 || y < 0 || x + larguraCorte > origem.largura || y + alturaCorte > origem.altura) {
        cerr << "Erro: Dimensoes de corte fora dos limites da imagem original." << endl;
        return false;
    }

    destino.largura = larguraCorte;
    destino.altura = alturaCorte;
    destino.pixels.resize(larguraCorte * alturaCorte * 3);

    for (int lin = 0; lin < alturaCorte; lin++) {
        for (int col = 0; col < larguraCorte; col++) {
            int idxOrigem  = ((y + lin) * origem.largura + (x + col)) * 3;
            int idxDestino = (lin * larguraCorte + col) * 3;

            destino.pixels[idxDestino]     = origem.pixels[idxOrigem];     // B
            destino.pixels[idxDestino + 1] = origem.pixels[idxOrigem + 1]; // G
            destino.pixels[idxDestino + 2] = origem.pixels[idxOrigem + 2]; // R
        }
    }
    return true;
}

// 5. Salva a ImagemInterna em um novo arquivo BMP (gerando o cabeçalho e reinsirindo o padding)
bool SaveBMP(const string& caminho, const ImagemInterna& img) {
    ofstream arq(caminho, ios::binary);
    if (!arq) {
        cerr << "Erro: Nao foi possivel criar o arquivo BMP de saída." << endl;
        return false;
    }

    int bytesPorLinha = img.largura * 3;
    int padding = (4 - (bytesPorLinha % 4)) % 4;
    uint32_t tamImagem = (bytesPorLinha + padding) * img.altura;

    CabecalhoArquivo cab = {};
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

    arq.write((char*)&cab, sizeof(cab));

    unsigned char paddingZero[3] = {0, 0, 0};
    for (int y = 0; y < img.altura; y++) {
        int inicioLinha = y * img.largura * 3;
        arq.write((char*)&img.pixels[inicioLinha], bytesPorLinha);
        if (padding > 0) {
            arq.write((char*)paddingZero, padding);
        }
    }

    arq.close();
    return true;
}