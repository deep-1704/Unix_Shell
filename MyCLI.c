#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define HISTORY_SIZE 10
#define MAXLIST 100

char* HistoryArr[HISTORY_SIZE];
int begin = 0;
int end = 0;
int size = 0;

void printHistory() {
  int i = begin;
  int len = 0;
  while (len < size) {
    printf("%s\n", HistoryArr[i]);
    i++;
    if (i == HISTORY_SIZE) i = 0;
    len++;
  }
}
void pushHistory(char* cmd) {
  if (end == HISTORY_SIZE) end = 0;
  if ((end == begin) && (size > 0)) {
    begin++;
    size--;
  }
  HistoryArr[end] = cmd;
  end++;
  size++;
  if (begin == HISTORY_SIZE) begin = 0;
  //   printf("%s\n",HistoryArr[end]);
}

int compare(char* s1, char* s2) {
  int l1 = strlen(s1);
  int l2 = strlen(s2);
  if (l1 != l2) return 0;

  for (int i = 0; i < l2; i++) {
    if (s1[i] != s2[i]) return 0;
  }
  return 1;
}

void execArgs(char** parsed) {
  pid_t pid = fork();

  if (pid == -1) {
    printf("\nFork error\n");
    return;
  } else if (pid == 0) {
    if (execvp(parsed[0], parsed) < 0) {
      printf("No such Command!\n");
    }
    exit(0);
  } else {
    wait(NULL);
    return;
  }
}

void execInOut(char** parsed) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    close(pipefd[1]);

    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);

    freopen(parsed[2], "w", stdout);

    execl(parsed[0], parsed[0], NULL);

    perror("exec");
    exit(EXIT_FAILURE);
  } else {
    close(pipefd[0]);

    FILE* inputFile = fopen(parsed[1], "r");
    if (inputFile == NULL) {
      perror("Error opening input.txt");
      exit(EXIT_FAILURE);
    }

    char buffer[1024];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), inputFile)) > 0) {
      write(pipefd[1], buffer, bytesRead);
    }

    close(pipefd[1]);
    fclose(inputFile);

    wait(NULL);
  }
  wait(NULL);
}

void openHelp() {
  puts(
      "\nList of Commands supported:\n\n"
      "\n>Custom commands:\n"
      "\nexit -- To exit the session"
      "\ngreet -- Check your username"
      "\ncd -- to change working directory"
      "\nhistory -- to revisit previous 10 executed commands"
      "\n[.exe file]<[input.txt]>[output.txt] -- Takes input from input.txt "
      "and prints output to output.txt"
      "\n\n>all other general commands available in UNIX shell");
  return;
}

int ownCmdHandler(char** parsed) {
  int NoOfOwnCmds = 5, i, OwnArg = 0;
  char* ListOfOwnCmds[NoOfOwnCmds];

  ListOfOwnCmds[0] = "exit";
  ListOfOwnCmds[1] = "cd";
  ListOfOwnCmds[2] = "guide";
  ListOfOwnCmds[3] = "greet";
  ListOfOwnCmds[4] = "history";

  for (i = 0; i < NoOfOwnCmds; i++) {
    if (compare(parsed[0], ListOfOwnCmds[i]) == 1) {
      OwnArg = i + 1;
      break;
    }
  }

  if (OwnArg == 1) {
    printf("\nShell session closed\n");
    exit(0);
  }
  if (OwnArg == 2) {
    chdir(parsed[1]);
    return 1;
  }
  if (OwnArg == 3) {
    openHelp();
    return 1;
  }
  if (OwnArg == 4) {
    printf("\nHello %s.\n", getenv("USER"));
    return 1;
  }
  if (OwnArg == 5) {
    printHistory();
    return 1;
  }
  return 0;
}

int parseInOut(char* str, char** parsed) {
  int itr = 0;
  while (str[itr] != '\0') {
    if (str[itr] == '<') break;
    itr++;
  }
  if (str[itr] == '\0') return 0;

  parsed[0] = strsep(&str, "<");
  char* rest = strsep(&str, "<");
  parsed[1] = strsep(&rest, ">");
  parsed[2] = strsep(&rest, ">");

  return 1;
}

void parseSpace(char* str, char** parsed) {
  // parsed = NULL;
  for (int i = 0; i < MAXLIST; i++) {
    parsed[i] = strsep(&str, " ");

    if (parsed[i] == NULL) break;
    if (strlen(parsed[i]) == 0) i--;
  }
}

int processString(char* str, char** parsed) {
  int inOut = parseInOut(str, parsed);

  if (inOut) {
    return 2;
  } else {
    parseSpace(str, parsed);
    if (ownCmdHandler(parsed))
      return 0;
    else
      return 1;
  }
}

int main() {
  int iter = 1;
  int execFlag;
  char* Parsed[MAXLIST];

  printf("\n Welcome to my shell.\nType 'guide' for more info\n");
  while (1) {
    char wd[1024];
    getcwd(wd, sizeof(wd));
    printf("\nubuntu@%s:%s>>", getenv("USER"), wd);

    char* CmdLine;
    size_t buffSize = 50;
    size_t characters;
    CmdLine = malloc(buffSize * sizeof(char));
    characters = getline(&CmdLine, &buffSize, stdin);
    CmdLine[strlen(CmdLine) - 1] = '\0';

    execFlag = processString(CmdLine, Parsed);
    if (execFlag == 1) execArgs(Parsed);
    if (execFlag == 2) execInOut(Parsed);

    pushHistory(CmdLine);
  }

  return 0;
}