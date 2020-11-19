/**
 * @date 18 NOV 2020
 * @author Thales Menezes @thlmenezes 17/0045919
 * Observação: Argumentos CLI -> ./fight --help
 * 
 * Objetivo: Simular um torneio de luta
 * 
 * Comportamento desejado:
 * - [fail] Cada ringue tem um juiz;
 * - [fail] Um juiz só pode assistir uma luta por vez;
 * - [fail] Cada luta só pode acontecer com na presença de um juiz;
 * - [fail] Mais de uma luta pode acontecer ao mesmo tempo;
 * - [fail] Torcedores buscam assistir lutadores;
 * - [fail] Há um limite de torcedores por luta classificatória;
 * - [fail] Caso um torcedor não consiga ingresso para assistir uma luta, espera na praça de alimentação;
 * - [fail] Todos os torcedores podem assistir a final do torneio;
 * - [fail] Lutadores são divididos em equipes;
 * - [fail] Lutadores quando perdem viram torcedores, mas só se interessam por partidas da sua equipe;
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


/* --------------------------------------------------------------------------------------------- */
/* ========================================== DEFINES ========================================== */
/* --------------------------------------------------------------------------------------------- */
#define TRUE 1
#define FALSE 0
#define bool int
#define print printf
#define RESET_COLOR "\033[0m"
typedef struct {
  int id;
  bool status;
} inscrito;

/* --------------------------------------------------------------------------------------------- */
/* ========================================== THREADS ========================================== */
/* ============================================= E ============================================= */
/* ================================== VARIAVEIS COMPARTILHADAS ================================= */
/* --------------------------------------------------------------------------------------------- */
int * TORNEIO;// TORNEIO = (int *) calloc(LUTADORES+1,sizeof(char));


/* --------------------------------------------------------------------------------------------- */
/* =========================================== HEADER ========================================== */
/* --------------------------------------------------------------------------------------------- */
// Atores
void * juiz     (void * pid);
void * lutador  (void * pid);
// Ações
// std++
bool prefix     (const char *pre, const char *str);
void print_help ();
void print_man  (const char* nome, const char* description, int len, const char ** options);


/* --------------------------------------------------------------------------------------------- */
/* ============================================ MAIN =========================================== */
/* --------------------------------------------------------------------------------------------- */
int main(int argc, char *argv[]){
  int idx, LUTADORES = 2, JUIZES = 1, TORCEDORES = 1, EQUIPES = 2;
  int * value;
  char input;
  char * command;
  
  for(idx = 1; idx < argc; idx++, value=NULL){
    command = argv[idx];

    if(prefix("l=",command))
      value = &LUTADORES;

    else if(prefix("j=",command))
      value = &JUIZES;

    else if(prefix("t=",command))
      value = &TORCEDORES;

    else if(prefix("e=",command))
      value = &EQUIPES;

    else if(strcmp("--help",command) == 0 || strcmp("-H",command) == 0){
      print_help();
      return 0;
    }

    if(value != NULL)
      *value = atoi(command+2*(sizeof(char)));
  }

  // Verify if user wants to use default args
  /*
  if (argc == 1){
    print("Print Help ? [Y/n]\n");
    scanf("%c", &input);
    if(input != 'n' && input != 'N'){
      print_help();
      return 0;
    }
    print("Loading defaults\n");
  }
  // */

  pthread_t tjid[JUIZES];

  // Criacao das threads de juízes
  for (idx = 0; idx < JUIZES; idx++) {
      value = (int *) malloc(sizeof(int));
      *value = idx;
      pthread_create(&tjid[idx], NULL, juiz, (void*) (value));
  }

  pthread_t tlid[LUTADORES];

  // Criacao das threads de lutadores
  for (idx = 0; idx < LUTADORES; idx++) {
      value = (int *) malloc(sizeof(int));
      *value = idx;
      pthread_create(&tlid[idx], NULL, lutador, (void*) (value));
  }

  pthread_join(tjid[0],NULL);

  return 0;
}


/* --------------------------------------------------------------------------------------------- */
/* ========================================== FUNÇÕES ========================================== */
/* --------------------------------------------------------------------------------------------- */
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
\t* - Caso um torcedor não consiga ingresso para assistir uma luta, espera na praça de alimentação;\n\
\t* - Todos os torcedores podem assistir a final do torneio;\n\
\t* - Lutadores quando perdem viram torcedores, mas só se interessam por partidas da sua equipe;\n\
";
  print_man("./fight",description,10,options);
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