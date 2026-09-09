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