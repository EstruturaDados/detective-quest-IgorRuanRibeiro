#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NOME 50
#define MAX_PISTA 100
#define HASH_SIZE 101  // tamanho da tabela hash (primo recomendado)

/* ------------------ Estruturas ------------------ */

/* Nó da árvore binária: cada sala tem nome, pista e caminhos */
typedef struct Sala {
    char nome[MAX_NOME];
    char pista[MAX_PISTA];
    struct Sala *esquerda;
    struct Sala *direita;
} Sala;

/* Nó da BST de pistas coletadas */
typedef struct NoPista {
    char pista[MAX_PISTA];
    struct NoPista *esq;
    struct NoPista *dir;
} NoPista;

/* Entrada da tabela hash (lista encadeada) */
typedef struct HashEntry {
    char *pista;
    char *suspeito;
    struct HashEntry *prox;
} HashEntry;

/* Estrutura da tabela hash */
typedef struct HashTable {
    HashEntry *tabela[HASH_SIZE];
} HashTable;

/* ------------------ Funções auxiliares ------------------ */

/* Cria e inicializa uma nova sala */
Sala* criarSala(const char *nome, const char *pista) {
    Sala *nova = (Sala*) malloc(sizeof(Sala));
    if (!nova) { fprintf(stderr, "Falha de memória.\n"); exit(EXIT_FAILURE); }

    strncpy(nova->nome, nome, MAX_NOME - 1);
    nova->nome[MAX_NOME - 1] = '\0';
    if (pista && pista[0] != '\0')
        strncpy(nova->pista, pista, MAX_PISTA - 1);
    else
        nova->pista[0] = '\0';
    nova->pista[MAX_PISTA - 1] = '\0';
    nova->esquerda = nova->direita = NULL;
    return nova;
}

/* Cria um nó de pista (BST) */
NoPista* criarNoPista(const char *pista) {
    NoPista *n = (NoPista*) malloc(sizeof(NoPista));
    if (!n) { fprintf(stderr, "Falha de memória.\n"); exit(EXIT_FAILURE); }
    strncpy(n->pista, pista, MAX_PISTA - 1);
    n->pista[MAX_PISTA - 1] = '\0';
    n->esq = n->dir = NULL;
    return n;
}

/* Insere pista na BST em ordem alfabética (ignora duplicatas) */
NoPista* inserirPista(NoPista *raiz, const char *pista) {
    if (!pista || pista[0] == '\0') return raiz;
    if (!raiz) return criarNoPista(pista);

    int cmp = strcmp(pista, raiz->pista);
    if (cmp < 0) raiz->esq = inserirPista(raiz->esq, pista);
    else if (cmp > 0) raiz->dir = inserirPista(raiz->dir, pista);
    return raiz;
}

/* Wrapper alternativo */
NoPista* adicionarPista(NoPista *raiz, const char *pista) {
    return inserirPista(raiz, pista);
}

/* Exibe pistas em ordem alfabética */
void exibirPistas(NoPista *raiz) {
    if (!raiz) return;
    exibirPistas(raiz->esq);
    printf("- %s\n", raiz->pista);
    exibirPistas(raiz->dir);
}

/* Libera memória da árvore de pistas */
void liberarPistas(NoPista *raiz) {
    if (!raiz) return;
    liberarPistas(raiz->esq);
    liberarPistas(raiz->dir);
    free(raiz);
}

/* Libera memória do mapa (salas) */
void liberarMapa(Sala *raiz) {
    if (!raiz) return;
    liberarMapa(raiz->esquerda);
    liberarMapa(raiz->direita);
    free(raiz);
}

/* ------------------ Implementação da Hash ------------------ */

/* Função hash djb2 */
static unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    return hash;
}

/* Inicializa tabela hash com NULL */
void inicializarHash(HashTable *h) {
    for (int i = 0; i < HASH_SIZE; ++i) h->tabela[i] = NULL;
}

/* Insere ou atualiza par (pista, suspeito) na hash */
void inserirNaHash(HashTable *h, const char *pista, const char *suspeito) {
    if (!pista || !suspeito) return;
    unsigned long hval = hash_djb2(pista) % HASH_SIZE;
    HashEntry *cur = h->tabela[hval];

    while (cur) { // atualiza se já existir
        if (strcmp(cur->pista, pista) == 0) {
            free(cur->suspeito);
            cur->suspeito = strdup(suspeito);
            if (!cur->suspeito) { perror("strdup"); exit(EXIT_FAILURE); }
            return;
        }
        cur = cur->prox;
    }

    // cria nova entrada
    HashEntry *novo = (HashEntry*) malloc(sizeof(HashEntry));
    if (!novo) { fprintf(stderr, "Falha de memória.\n"); exit(EXIT_FAILURE); }
    novo->pista = strdup(pista);
    novo->suspeito = strdup(suspeito);
    novo->prox = h->tabela[hval];
    h->tabela[hval] = novo;
}

/* Busca o suspeito associado à pista */
const char* encontrarSuspeito(HashTable *h, const char *pista) {
    unsigned long hval = hash_djb2(pista) % HASH_SIZE;
    for (HashEntry *cur = h->tabela[hval]; cur; cur = cur->prox)
        if (strcmp(cur->pista, pista) == 0) return cur->suspeito;
    return NULL;
}

/* Libera toda a memória da tabela hash */
void liberarHash(HashTable *h) {
    for (int i = 0; i < HASH_SIZE; ++i) {
        HashEntry *cur = h->tabela[i];
        while (cur) {
            HashEntry *prox = cur->prox;
            free(cur->pista);
            free(cur->suspeito);
            free(cur);
            cur = prox;
        }
    }
}

/* Remove espaços iniciais e finais */
static void str_trim(char *s) {
    if (!s) return;
    char *start = s;
    while (isspace((unsigned char)*start)) start++;
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    size_t len = (size_t)(end - start + 1);
    memmove(s, start, len);
    s[len] = '\0';
}

/* Comparação case-insensitive */
static int str_iequals(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++; b++;
    }
    return *a == *b;
}

/* ------------------ Exploração da mansão ------------------ */

/* Explora salas e coleta pistas */
void explorarSalas(Sala *atual, NoPista **raizPistas) {
    if (!atual) return;
    char buffer[64];

    while (1) {
        printf("\nVocê está em: %s\n", atual->nome);

        // coleta pista se disponível
        if (atual->pista[0] != '\0') {
            printf(">> Pista encontrada: \"%s\"\n", atual->pista);
            *raizPistas = adicionarPista(*raizPistas, atual->pista);
            atual->pista[0] = '\0'; // marca como coletada
        } else {
            printf("Nenhuma pista nesta sala.\n");
        }

        // mostra caminhos possíveis
        printf("\nEscolha um caminho:\n");
        if (atual->esquerda) printf(" (e) Esquerda -> %s\n", atual->esquerda->nome);
        if (atual->direita)  printf(" (d) Direita -> %s\n", atual->direita->nome);
        printf(" (s) Sair\n> ");

        if (!fgets(buffer, sizeof(buffer), stdin)) continue;
        if (buffer[0] == '\n') continue;

        char opcao = tolower((unsigned char)buffer[0]);
        if (opcao == 'e' && atual->esquerda) atual = atual->esquerda;
        else if (opcao == 'd' && atual->direita) atual = atual->direita;
        else if (opcao == 's') break;
        else printf("Opção inválida.\n");
    }
}

/* Conta quantas pistas associam-se ao suspeito */
int contarPistasDoSuspeitoRec(NoPista *r, HashTable *h, const char *suspeito) {
    if (!r) return 0;
    int c = 0;
    c += contarPistasDoSuspeitoRec(r->esq, h, suspeito);
    const char *s = encontrarSuspeito(h, r->pista);
    if (s && str_iequals(s, suspeito)) c++;
    c += contarPistasDoSuspeitoRec(r->dir, h, suspeito);
    return c;
}

/* Avalia se o acusado é culpado com base nas pistas */
void verificarSuspeitoFinal(NoPista *pistas, HashTable *h, const char *acusado) {
    if (!acusado || !*acusado) { printf("Nome inválido.\n"); return; }

    char nome[100];
    strncpy(nome, acusado, sizeof(nome)-1);
    nome[sizeof(nome)-1] = '\0';
    str_trim(nome);

    int total = contarPistasDoSuspeitoRec(pistas, h, nome);

    printf("\nVocê acusou: %s\n", nome);
    printf("Pistas relacionadas: %d\n", total);

    if (total >= 2)
        printf("DESFECHO: %s é o culpado.\n", nome);
    else if (total == 1)
        printf("DESFECHO: Evidência insuficiente contra %s.\n", nome);
    else
        printf("DESFECHO: Nenhuma pista contra %s.\n", nome);
}

/* ------------------ Função principal ------------------ */

int main(void) {
    // criação do mapa fixo
    Sala *hall = criarSala("Hall de Entrada", "Pegada de sapato na porta");
    Sala *salaEstar = criarSala("Sala de Estar", "Copo quebrado proximo ao sofa");
    Sala *cozinha = criarSala("Cozinha", "Facas fora do lugar");
    Sala *biblioteca = criarSala("Biblioteca", "Livro aberto com anotacoes suspeitas");
    Sala *jardim = criarSala("Jardim", "Terra recem-remexida perto da estatua");
    Sala *porao = criarSala("Porao", "Caixa trancada com iniciais gravadas");

    // conexões das salas
    hall->esquerda = salaEstar;
    hall->direita = cozinha;
    salaEstar->esquerda = biblioteca;
    salaEstar->direita = jardim;
    cozinha->direita = porao;

    printf("=== Detective Quest: A Mansao Enigma ===\n");

    // inicialização das estruturas
    NoPista *pistas = NULL;
    HashTable tabela;
    inicializarHash(&tabela);

    // associação pista -> suspeito
    inserirNaHash(&tabela, "Pegada de sapato na porta", "Carlos");
    inserirNaHash(&tabela, "Copo quebrado proximo ao sofa", "Mariana");
    inserirNaHash(&tabela, "Facas fora do lugar", "Ricardo");
    inserirNaHash(&tabela, "Livro aberto com anotacoes suspeitas", "Mariana");
    inserirNaHash(&tabela, "Terra recem-remexida perto da estatua", "Carlos");
    inserirNaHash(&tabela, "Caixa trancada com iniciais gravadas", "Henrique");

    // início da exploração
    explorarSalas(hall, &pistas);

    // exibe pistas coletadas
    printf("\n=== Pistas Coletadas ===\n");
    if (pistas) exibirPistas(pistas);
    else printf("Nenhuma pista coletada.\n");

    // fase de acusação
    char acusado[100];
    printf("\nDigite o nome do suspeito que deseja acusar: ");
    if (fgets(acusado, sizeof(acusado), stdin)) {
        size_t L = strlen(acusado);
        if (L > 0 && acusado[L-1] == '\n') acusado[L-1] = '\0';
        str_trim(acusado);
        verificarSuspeitoFinal(pistas, &tabela, acusado);
    }

    // libera memória
    liberarMapa(hall);
    liberarPistas(pistas);
    liberarHash(&tabela);

    printf("\nObrigado por jogar Detective Quest!\n");
    return 0;
}

