#include <cstdio>
#include <unistd.h>
#include "shell.hh"
#include <sys/wait.h>

int yyparse(void);
char** newargv;

void Shell::prompt() {
  if (isatty(0))
  {
    printf("myshell>");
    fflush(stdout);
  }
}

// Function for zombies
void kill (int sig) 
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// Function for ctrl c
extern "C" void ctrlc( int sig )
{
  // Put new line
  printf("\n");

  // If command list is empty then print prompt
  if (Shell::_currentCommand._simpleCommands.size() == 0)
  {
    Shell::prompt();
  }
}

int main(int argc, char** argv)
{    
  // Code for handling zombies
  struct sigaction signalAction; 
  signalAction.sa_handler = kill; 
  sigemptyset(&signalAction.sa_mask); 
  signalAction.sa_flags = SA_RESTART; 

  int error = sigaction(SIGCHLD, &signalAction, NULL ); 
  if ( error ) 
  { 
    perror( "sigaction" ); 
    exit( -1 ); 
  } 
  
  // Code for handling ctrl c
  struct sigaction sa;
  sa.sa_handler = ctrlc;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if(sigaction(SIGINT, &sa, NULL))
  {
      perror("sigaction");
      exit(2);
  }

  // Put argv in shell's struct
  Shell::myargv = argv;

  // Prompt for user input
  Shell::prompt();
  
  // Parse user input
  yyparse();

}

char** Shell::myargv = newargv;
Command Shell::_currentCommand;
