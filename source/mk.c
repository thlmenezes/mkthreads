/**
 * @date 18 NOV 2020
 * @author Thales Menezes @thlmenezes 17/0045919
 * Observação: Argumentos CLI -> ./mk --help
 * 
 * Objetivo: Simular um torneio de luta
 * 
 * Comportamento desejado:
 * - [OK] Cada ringue tem um juiz;
 * - [OK] Um juiz só pode assistir uma luta por vez;
 * - [OK] Cada luta só pode acontecer com na presença de um juiz;
 * - [OK] Mais de uma luta pode acontecer ao mesmo tempo;
 * - [fail] Torcedores buscam assistir lutadores;
 * - [fail] Há um limite de torcedores por luta classificatória;
 * - [fail] Caso um torcedor não consiga ingresso para assistir uma luta, espera na praça de alimentação;
 * 
 * Garantias iniciais:
 * - Nenhuma
 */
/* --------------------------------------------------------------------------------------------- */
/* ========================================= INCLUDES ========================================== */
/* --------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>


/* --------------------------------------------------------------------------------------------- */
/* ========================================== DEFINES ========================================== */
/* --------------------------------------------------------------------------------------------- */
// Booleanos
#define TRUE 1
#define FALSE 0
#define bool int
// Preferência Pessoal
#define print printf
#define RESET_COLOR "\033[0m"
// Incrementa índice pilha
#define INCREMENTA(idx) idx = (idx + 1) % LUTADORES;
typedef struct {
  int id;
  int round;
} fight;
typedef struct {
  bool vida;
  int round;
} status;

/* --------------------------------------------------------------------------------------------- */
/* ========================================== THREADS ========================================== */
/* ============================================= E ============================================= */
/* ================================== VARIAVEIS COMPARTILHADAS ================================= */
/* --------------------------------------------------------------------------------------------- */
// Número de atores padrão
int LUTADORES = 2, JUIZES = 1, TORCEDORES = 1, EQUIPES = 2, CADEIRAS = 20;
// Array para controle do status dos inscritos
status * INSCRITOS;
// Número de inscritos que continuam no torneio
int VIVOS;
// Pilha Circular para percorrer o torneio por largura
fight * TORNEIO;
int torneio_TAMANHO = 0;
int torneio_leitura_idx = 0;
int torneio_escrita_idx = 0;
// LOCKS
// Controlador da região crítica
pthread_mutex_t mutex    = PTHREAD_MUTEX_INITIALIZER;
// VARIÁVEIS CONDIÇÃO
pthread_cond_t  juiz_cond = PTHREAD_COND_INITIALIZER;
// SEMAPHORES
// Controla os lutadores, aguardam o fim de suas lutas
sem_t sem_vivos;
sem_t * LUTANDO;


/* --------------------------------------------------------------------------------------------- */
/* =========================================== HEADER ========================================== */
/* --------------------------------------------------------------------------------------------- */
// Atores
void * juiz     (void * pid);
void * lutador  (void * pid);
void * torcedor (void * pid);
// Ações
// std++
bool prefix     (const char *pre, const char *str);
void print_help ();
void print_man  (const char* nome, const char* description, int len, const char ** options);


/* --------------------------------------------------------------------------------------------- */
/* ============================================ MAIN =========================================== */
/* --------------------------------------------------------------------------------------------- */
int main(int argc, char *argv[]){
  int idx;
  int * ptr_int;
  // char input_char;
  char * ptr_char;
  
  // Tratando os argumentos CLI

  for(idx = 1; idx < argc; idx++, ptr_int=NULL){
    ptr_char = argv[idx];

    if(prefix("l=",ptr_char))
      ptr_int = &LUTADORES;

    else if(prefix("j=",ptr_char))
      ptr_int = &JUIZES;

    else if(prefix("t=",ptr_char))
      ptr_int = &TORCEDORES;

    else if(prefix("e=",ptr_char))
      ptr_int = &EQUIPES;

    else if(prefix("c=",ptr_char))
      ptr_int = &CADEIRAS;

    else if(strcmp("--help",ptr_char) == 0 || strcmp("-H",ptr_char) == 0){
      print_help();
      return 0;
    }

    if(ptr_int != NULL)
      *ptr_int = atoi(ptr_char+2*(sizeof(char)));
  }

  // Tratamento do caso de nenhum argumento CLI
  /*
  if (argc == 1){
    print("Imprimir Ajuda ? [S/n]\n");
    scanf("%c", &input_char);
    if(input_char != 'n' && input_char != 'N'){
      print_help();
      return 0;
    }
    print("Loading defaults\n");
  }
  // */

  // O número de vivos no começo é o número de lutadores
  VIVOS = LUTADORES;

  // Inicializando arrays
  TORNEIO   =  (fight *) calloc(LUTADORES,sizeof(fight));
  INSCRITOS = (status *) calloc(LUTADORES,sizeof(status));
  LUTANDO   =  (sem_t *) calloc(LUTADORES,sizeof(sem_t));

  for (idx = 0; idx < LUTADORES; idx++){
  // Inicializa o array com vida=true,round=0
    INSCRITOS[idx].vida  = TRUE;
    INSCRITOS[idx].round = 0;
  // Inicializa torneio com lutadores
    TORNEIO[idx].id = idx;
    TORNEIO[idx].round = 0;
  // Inicializando semáforos
    sem_init(&LUTANDO[idx], 0, FALSE);
  }

  torneio_TAMANHO = LUTADORES;
  sem_init(&sem_vivos, 0, FALSE);

  // Criação das threads de juízes
  pthread_t tjid[JUIZES];

  for (idx = 0; idx < JUIZES; idx++) {
      ptr_int = (int *) malloc(sizeof(int));
      *ptr_int = idx;
      if(pthread_create(&tjid[idx], NULL, juiz, (void*) (ptr_int))){
        print("erro na criacao do thread %d\n", idx);
        exit(1);
      }
  }

  // Criação das threads de lutadores
  pthread_t tlid[LUTADORES];

  for (idx = 0; idx < LUTADORES; idx++) {
      ptr_int = (int *) malloc(sizeof(int));
      *ptr_int = idx;
      pthread_create(&tlid[idx], NULL, lutador, (void*) (ptr_int));
  }

  sem_wait(&sem_vivos);
  return 0;
}


/* --------------------------------------------------------------------------------------------- */
/* ========================================== FUNÇÕES ========================================== */
/* --------------------------------------------------------------------------------------------- */
void * juiz     (void * pid){
  int round, esquerda, direita, ganhador, perdedor,
  id = *((int *) pid);

  while(TRUE){

    print("JUIZ %d chegou ao ringue\n",id);

    pthread_mutex_lock(&mutex);
      // Se tem menos de 2, torneio acabou
      if( VIVOS < 2 ){
        sem_post(&sem_vivos);
        break;
      }

      while (torneio_TAMANHO < 2)
        pthread_cond_wait(&juiz_cond,&mutex);

      // Pega o primeiro da pilha TORNEIO
      esquerda = TORNEIO[torneio_leitura_idx].id;
      round = TORNEIO[torneio_leitura_idx].round;
      INCREMENTA(torneio_leitura_idx);

      // Pega o indice do segundo da pilha
      // Nessa posição será escrito lutador
      direita = torneio_leitura_idx;
      INCREMENTA(torneio_leitura_idx);

      torneio_TAMANHO -= 2;
      // Atualizando indices (SIZE, read)

      // Enquanto os rounds forem diferentes, aguarda
      while (TORNEIO[direita].round < round)
        pthread_cond_wait(&juiz_cond,&mutex);

      direita = TORNEIO[direita].id;

      // Valida lutadores usando status
      // Lutadores mortos?
      if(!INSCRITOS[esquerda].vida || !INSCRITOS[direita].vida)
        break;
      // Luta já aconteceu?
      if(INSCRITOS[esquerda].round > round || INSCRITOS[direita].round > round)
        break;

      print("JUIZ %d: convocando lutadores %d e %d\n",
                  id,             esquerda, direita);
    pthread_mutex_unlock(&mutex);
    
    // Assiste luta
    sleep(3);
    // Decide ganhador
    ganhador = (esquerda+direita) % 2 ? esquerda : direita;
    
    pthread_mutex_lock(&mutex);
      // Informa perdedor - Atualiza Inscritos
      perdedor = ganhador == direita? esquerda : direita;
      INSCRITOS[perdedor].vida = FALSE;
      VIVOS -= 1;
      sem_post(&LUTANDO[perdedor]);
      // Insere ganhador no final da pilha
      TORNEIO[torneio_escrita_idx].id = ganhador;
      TORNEIO[torneio_escrita_idx].round += 1;
      // Ganhador avança um round
      INSCRITOS[ganhador].round = TORNEIO[torneio_escrita_idx].round;
      print("JUIZ %d: luta definida, ganhador %d\n",
                  id,                    ganhador);
      // Acorda juizes para conferir lutadores
      pthread_cond_broadcast(&juiz_cond);
      // Atualiza indices (SIZE, write)
      INCREMENTA(torneio_escrita_idx);
      torneio_TAMANHO++;
    pthread_mutex_unlock(&mutex);
  }
  print("JUIZ %d: encerrei o expediente\n", id);
  pthread_exit(0);
}

void * lutador  (void * pid){
  int id = *((int *) pid);

  while(TRUE){
    // Aguarda o resultado da luta
    sem_wait(&LUTANDO[id]);
    // Se meu inscrito.status for FALSO -> morri
    if(!INSCRITOS[id].vida){
      print("Lutador %d morreu\n", id);
      break;
    }
  }
  // TODO: MORREU? VIRA TORCEDOR
  pthread_exit(0);
}

void * torcedor (void * pid){
  int id = *((int *) pid);

  while(TRUE){
    // Tenta assistir luta
    // Cada torcedor assiste as lutas de um ringue
  }
  // Sem vaga? Praça de alimentação e Saída
  pthread_exit(0);
}

/* Source: https://stackoverflow.com/questions/4770985/how-to-check-if-a-string-starts-with-another-string-in-c#answer-4770992 */
bool prefix(const char *pre, const char *str){
  return strncmp(pre, str, strlen(pre)) == 0;
}

void print_help() {
  const char * options[] = {"-H, --help",
"Imprime essa mensagem de ajuda",
"--lutadores=LUTADORES",
"Número de participantes inscritos",
"--juizes=JUÍZES",
"Número de juízes ou ringues",
"--torcedores=TORCEDORES",
"Número de torcedores",
"--equipes=EQUIPES",
"Número de equipes competindo (lutadores serão dividos em equipes,\n\t\tmas o número de participantes por equipe pode variar)"};
  const char * description = "\n\
\t\033[1;32mTorneio de Artes Marciais Mistas\033[0m\n\
\t* Regras:\n\
\t* - Cada ringue tem um juiz;\n\
\t* - Lutadores são divididos em equipes;\n\
\t* - Cada luta só pode acontecer com na presença de um juiz;\n\
\t* - Um juiz só pode assistir uma luta por vez;\n\
\t* - Mais de uma luta pode acontecer ao mesmo tempo;\n\
\t* - Torcedores buscam assistir lutadores;\n\
\t* - Há um limite de torcedores por luta classificatória;\n\
\t* - Caso um torcedor não consiga ingresso para assistir uma luta, vai para a praça de alimentação;\n\
";
  print_man("./mk",description,10,options);
}

void print_man(const char* nome, const char* description, int len, const char ** options){
  int idx;
  const char * cyan = "\033[1;32m";

  print("\033[1;32mSYNOPSIS\n");
  if(strlen(nome))
    print("\t%s%s [\033[1;36mOPTIONS%s...]\n\n", nome, RESET_COLOR, RESET_COLOR);

  print("\033[1;32mDESCRIPTION\n");
  if(strlen(description))
    print("\t%s%s",RESET_COLOR, description);

  print("\033[1;32mOPTIONS\n");
  for(idx = 0; idx < len; idx++){
    print("\t%s", !(idx % 2) ? "" : "\t");
    print("%s", !(idx % 2) ? cyan : RESET_COLOR);
    print("%s\n",options[idx]);
  }
}