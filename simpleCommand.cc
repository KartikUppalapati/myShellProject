#include <cstdio>
#include <cstdlib>
#include <regex.h>
#include <iostream>
#include <cstring>
#include "simpleCommand.hh"
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>

SimpleCommand::SimpleCommand() {
  _arguments = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
  // iterate over all the arguments and delete them
  for (auto & arg : _arguments) {
    delete arg;
  }
}


// Global variable for last and last to last argument
int lastArgNumber = 0;
std::string lastArg;
std::string lastLastArg;

void SimpleCommand::insertArgument( std::string * argument ) 
{
  // Get word
  std::string * word = argument;

  // Check for variable expansions
  if (word->find("${") != -1)
  {
      // Find all $('s and parse
      while (word->find("${") != -1)
      {
	// Find positions of $() 
        int first = word->find("${");
	int second = word->find("}");

	// Get left variable and right of $()
	std::string variable = word->substr(first + 2, second - first - 2); 
	std::string left = word->substr(0, first);
	std::string right = word->substr(second + 1, word->size());

	// Check if variable is shell
	if (strcmp(variable.c_str(), "SHELL") == 0)
	{
	  // Get path	
	  //char buf[PATH_MAX]; /* PATH_MAX incudes the \0 so +1 is not required */
   	  char *path = realpath(myargv[0], NULL);
	  
	  // Make new word
	  word = new std::string(left + path + right);
	}
	// Else if $ variable
	else if (strcmp(variable.c_str(), "$") == 0)
	{
	  // Get pid
	  std::string pid = std::to_string(syscall(SYS_getpid));
	  
	  // Make new word
	  word = new std::string(left + pid + right);
	}
	// Else if ? variable
	else if (strcmp(variable.c_str(), "?") == 0)
	{
	  // Get ? from struct and waitpid
	  //int previousReturnCode;
	  // Get exit status as std::string
	  std::string exitStatus = std::to_string(previousCommandReturnCode);

	  // Make new word
	  word = new std::string(left + exitStatus + right);
	}
	// Else if ! variable
	else if (strcmp(variable.c_str(), "!") == 0)
	{
	  // Get previous pid from struct
	  std::string backgroundPid = std::to_string(previousBackgroundPid);
	  
	  // Make new word
	  word = new std::string(left + backgroundPid + right);
	}
	// Else if _ variable
	else if (strcmp(variable.c_str(), "_") == 0)
	{
	  // Get previous pid from struct
	  std::string previousArgument;

	  // Put into global last argument based on number
          if (lastArgNumber % 2 == 0)
          {
	    previousArgument = lastArg;
          }
          else
          {
	    previousArgument = lastLastArg;
          }

	  // Make new word
	  word = new std::string(left + previousArgument + right);
	}
	// Else just get variable
	else
	{
	  // Make new word
	  word = new std::string(left + getenv(variable.data()) + right);
	}
      }
    
      // Add new word to argument vector
      _arguments.push_back(word);
      // Put into global last argument based on number
      if (lastArgNumber % 2 == 0)
      {
	lastArg = word->data();
      }
      else
      {
	lastLastArg = word->data();
      }

      // Increment last arg number
      lastArgNumber++;
    }
    // Else just add the argument to the vector
    else
    {
       // Just add it
       _arguments.push_back(argument);
    
      // Put into global last argument based on number
      if (lastArgNumber % 2 == 0)
      {
	lastArg = word->data();
      }
      else
      {
	lastLastArg = word->data();
      }

      // Increment last arg number
      lastArgNumber++;
    }
}

// Print out the simple command
void SimpleCommand::print() {
  for (auto & arg : _arguments) {
    std::cout << "\"" << *arg << "\" \t";
  }
  // effectively the same as printf("\n\n");
  std::cout << std::endl;
}

