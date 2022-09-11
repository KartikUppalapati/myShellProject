#ifndef shell_hh
#define shell_hh

#include "command.hh"

struct Shell {

  static void prompt();
  static char** myargv;
  static Command _currentCommand;
};

#endif
