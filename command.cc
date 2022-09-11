/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "command.hh"
#include "shell.hh"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/syscall.h>
 
Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _appendOut = false;
    _appendErr = false;
}

// Helper function for qsort
int compare(const void * a, const void *b)
{
    // Get s1 and s2
    char *s1 = *(char**)a;
    char *s2 = *(char **)b;

    // Compare s1 and s2
    return strcmp(s1, s2);
}

// Helper function to sort array
void Command::sortArrayStrings(char** array, int nEntries)
{
    qsort((void *)&array[0], nEntries, sizeof (char*), &compare);
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile ) {
	if ( _outFile == _errFile) 
	{
	  _errFile = NULL;
	}
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;
    _appendOut = false;
    _appendErr = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

// Helper function for source
void doSource(char * line)
{
    // Save default temp in and out
    int tmpin = dup(0);
    int tmpout = dup(1);
    
    // Create pipes
    int pin[2];
    int pout[2];
    pipe(pin);
    pipe(pout);

    // Get line as std::string and remove /n
    std::string newLine = std::string(line);
    newLine = newLine.substr(0, newLine.size() - 1);

    // If cd command used
    if (newLine.find("cd") != -1)
    {
	// If no argument given then do default path
	if (newLine.size() == 2)
	{
	    // Default path 
	    const char* homedir = getenv("HOME");

	    // Check result
	    if (chdir(homedir) == -1)
	    {
		// Set errno
		errno = ENOMEM;
		
		// Print to stderr
		fprintf( stderr, "cd: can't cd to %s\n", homedir );
	    }
	}
	// Else provide second argument as argument for chdir
	else
	{
	    // Check result
	    if (chdir(newLine.substr(newLine.find(" ") + 1, newLine.size()).c_str()) == -1)
	    {
		// Set errno
		errno = ENOMEM;
		
		// Print to stderr
		fprintf( stderr, "cd: can't cd to %s\n", newLine.substr(newLine.find(" ") + 1, newLine.size()).c_str());
	    }
	}

	// Stop if builtin function
	return;
    }
    // Check if setenv or printenv
    else if (std::string(line).find("setenv") != -1)
    {
      // Remove preceding setenv and space
      newLine = newLine.substr(7, newLine.size());

      // Setenv
      std::string a = newLine.substr(0, newLine.find(" "));
      std::string b = newLine.substr(newLine.find(" ") + 1, newLine.size()); 
      if(setenv(a.data(), b.data(), 1) == -1)
      {
	  errno = ENOMEM;
      }
      
      // End function
      return;
    }
    else if (std::string(line).find("unsetenv") != -1)
    {
      // Unsetenv
      std::string a = newLine.substr(newLine.find(" ") + 1, newLine.size()); 
      if(unsetenv(a.data()) == -1)
      {
	  errno = EINVAL;
      }
      
      // End function
      return;
    }

    // Parent writes to pin[1]
    std::string pipeWrite = newLine + "\nexit\n";
    write(pin[1], pipeWrite.c_str(), pipeWrite.size());    
    close(pin[1]);

    // Redirect input and output
    dup2(pin[0], 0);
    close(pin[0]);
    dup2(pout[1], 1);
    close(pout[1]);

    // Create child process
    int ret;
    ret = fork();

    // In child process
    if (ret == 0)
    {
      // Execute self
      std::string* selfPath = new std::string("/proc/self/exe");
      execvp(selfPath->c_str(), NULL);         
 
      // Exit
      close(pout[0]);
      _exit(1);
    }
    // Fork error
    else if (ret < 0)
    {
      perror("fork");
      exit(1);
    }

    // Wait for child to finish
    waitpid(ret, NULL, 0);   

    // Create buffer for reading
    char returnBuffer[4096];

    // Read from pout[0] into buffer
    int readValue = read(pout[0], returnBuffer, sizeof(returnBuffer));
    
    // Restore in and out
    dup2(tmpin, 0);
    dup2(tmpout, 1);
    
    // Clear good bye message from buffer
    for (int i = readValue - 13; i < readValue; i++)
    {
      returnBuffer[i] = '\0';
    }

    // Print
    printf("%s", returnBuffer);

    // Flush
    fflush(stdout);

    // Clear buffer
    for (int i = 0; i < readValue; i++)
    {
      returnBuffer[i] = '\0';
    }
}

void printExit(void)
{
    printf("\nGood Bye!!\n\n");
}
void printNoExit(void)
{
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::prompt();
        return;
    }

    // Else if command is exit
    if (!strcmp(_simpleCommands[0]->_arguments[0]->c_str(), "exit"))
    {
      atexit(printExit);
      exit(EXIT_SUCCESS);
    }
    if (!strcmp(_simpleCommands[0]->_arguments[0]->c_str(), "exitsub"))
    {
      atexit(printNoExit);
      exit(EXIT_SUCCESS);
    }

    // Print contents of Command data structure
    //print();

    // Save in/out and err
    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);

    // Set up input file
    int fdin;
    if (_inFile)
    {
        fdin = open(_inFile->data(), O_RDONLY);
	//perror("open in");
    }
    else
    {
        fdin = dup(tmpin);
    }

    // Go through all commands
    int ret;
    int fdout;
    int fderr;
    for ( int i = 0; i < _simpleCommands.size(); i++)
    {
	// Redirect input
	dup2(fdin, 0);
	close(fdin);

	// Last simple command
	if (i == _simpleCommands.size() - 1)
	{
	    // Set output
	    if (_outFile)
	    {
		if (_appendOut == true)
		{
		    fdout = open(_outFile->data(), O_CREAT | O_WRONLY | O_APPEND, 0664);
		}
		else
		{
		    fdout = open(_outFile->data(), O_CREAT | O_WRONLY, 0664);
		}
		//perror("open out");
	    }
	    else
	    {
		fdout = dup(tmpout);
	    }
	    // Set error
	    if (_errFile)
	    {
		if (_appendErr == true)
		{
		    fderr = open(_errFile->data(), O_CREAT | O_WRONLY | O_APPEND, 0664);
		}
		else
		{
		    fderr = open(_errFile->data(), O_CREAT | O_WRONLY, 0664);
		}
		//perror("open err");
	    }
	    else
	    {
		fderr = dup(tmperr);
	    }

	}
	// Else not last command
	else
	{
	    int fdpipe[2];
	    pipe(fdpipe);
	    fdout = fdpipe[1];
	    fdin = fdpipe[0];
	}

	// Redirect output and error
	dup2(fdout, 1);
	close(fdout);
	dup2(fderr, 2);
	close(fderr);	
     
        // Const char* vector to hold arguments
	std::vector<const char*> newArguments;

	// Copy
	for ( const auto& string : _simpleCommands[i]->_arguments ) 
	{
	    // If tilde stuff
	    if (string->find("~") != -1)
	    {
	        // If only the tilde
	        if (string->size() == 1)
	        {
	 	  newArguments.push_back( getenv("HOME") ); 
	        }
		// Else if tilde with string	
		else if (string->find("/") == -1)
		{
		    // Get home directory of argument
		    std::string user = string->substr(1, string->size());
		    const char* userHomedir = getpwnam(user.data())->pw_dir;
	       	    
		    // Put exanded argument back into new arguments list
		    newArguments.push_back( userHomedir );
		}
		// Else tilde with /
		else
		{
		    // Get home directory of argument
		    std::string user = string->substr(1, string->find("/") - 1);
		    const char* userHomedir = getpwnam(user.data())->pw_dir;
	       	 
		    // Find second argument past /
		    int foundLocation = string->find("/");
		    std::string userHomedirDir = string->substr(foundLocation, string->size());
		    std::string fullpath = userHomedir + userHomedirDir;
		    std::string* fullpathPointer = new std::string(fullpath);
		
		    // Put expanded argument into new arguments list
		    newArguments.push_back( fullpathPointer->data() );
		}
	    }
	    // Else just put into newArgument vector
	    else
	    {
	        // Put string data into new argument vector
    	        newArguments.push_back( string->data() );
	    }

	    // If string is an ampersand then update previousBackgroundPid value
	    if (strcmp(string->data(), "$") == 0)
	    {
    		_simpleCommands[i]->previousBackgroundPid = syscall(SYS_getpid);
	    }
	}

        // Add null to end
	newArguments.push_back(NULL);
	newArguments.shrink_to_fit();
	
        // If cd command used
        if (strcmp(newArguments[0], "cd") == 0)
	{
	    // If no argument given then do default path
	    if (newArguments.size() == 2)
	    {
	        // Default path 
		const char* homedir = getenv("HOME");

		// Check result
		if (chdir(homedir) == -1)
		{
		    // Set errno
		    errno = ENOMEM;
		    
		    // Print to stderr
		    fprintf( stderr, "cd: can't cd to %s\n", homedir );
		}
	    }
	    // Else provide second argument as argument for chdir
	    else
	    {
	        // Check result
	        if (chdir(newArguments[1]) == -1)
	        {
		    // Set errno
	            errno = ENOMEM;
		    
		    // Print to stderr
		    fprintf( stderr, "cd: can't cd to %s\n", newArguments[1]);
	        }
	    }
	}
	// Else if setenv command
	else if (strcmp(newArguments[0], "setenv") == 0)
	{
	    // Use setenv()
	    if (setenv(newArguments[1], newArguments[2], 1) == -1)
	    {
	        errno = ENOMEM;
	    }
	}
	// Else if unsetenv command
	else if (strcmp(newArguments[0], "unsetenv") == 0)
	{
	    // Use unsetenv()
	    if (unsetenv(newArguments[1]) == -1)
	    {
	        errno = EINVAL;
	    }
	}
	// Else if source command
	else if (strcmp(newArguments[0], "source") == 0)
	{
	    // Open file for reading
	    FILE * fp;
	    char * line = NULL;
	    size_t len = 0;
	    ssize_t read;

	    // Open file pointer and make sure its not null
	    fp = fopen(newArguments[1], "r");
	    if (fp == NULL)
	    {
	        fprintf( stderr, "%s: no such file or directory\n", newArguments[1]);
	    }
	    // Else 
	    else
	    { 
	        // Read all lines 
	        while ((read = getline(&line, &len, fp)) != -1) 
	        {
		    doSource(line);
	        }

	        // Close file pointer
	        fclose(fp);
	    }
	}


	// Call fork if not builtin command
	if (strcmp(newArguments[0], "cd" ) != 0 && strcmp(newArguments[0], "setenv") != 0 &&
            strcmp(newArguments[0], "unsetenv") != 0 && strcmp(newArguments[0], "source") != 0)
	{
	  ret = fork();
	}
	// Else set ret value to something not used
	else
	{
	  ret = 5;
	}
	
	// Get environ variable for printenv
	extern char ** environ;
	
	// Check child
	if (ret == 0)
	{
	    // If printenv
            if (strcmp(newArguments[0], "printenv") == 0)
	    {
		// Get double pointer to environ variables
		char ** p = environ;

		// Print all
		while (*p != NULL)
		{
		    printf("%s\n", *p);
		    p++;
		}
		
		// Exit
		exit(1);	
	    }
	    // If not builtin command
	    else if (strcmp(newArguments[0], "cd" ) != 0 && strcmp(newArguments[0], "setenv") != 0 &&
		strcmp(newArguments[0], "unsetenv") != 0 && strcmp(newArguments[0], "source") != 0)
	    {
		// Execute command
	        execvp(newArguments[0], const_cast<char* const *>(newArguments.data()));
	        perror("execvp");

		// Put exit code in struct
		int es;	
		int previousReturnCode;
 
		// Check if exited 
		if ( waitpid(ret, &previousReturnCode, 0) == -1 ) 
		{
		  perror("waitpid() failed");
		}

		if (WIFEXITED(previousReturnCode)) 
		{
		  es = WEXITSTATUS(previousReturnCode);
		  fprintf(stderr, "Exit status was %d\n", es);
		}

		// Put exit code in struct
	        _simpleCommands[i]->previousCommandReturnCode = es;
	        
		// Exit
		_exit(1);
	    }
	}
	else if (ret < 0)
	{
	    perror("fork");
	    return;
	}
    }

    // Make parent wait if background set to true
    if (_background == false)
    {
	waitpid(ret, NULL, 0);
    }

    // Restore in/out and err defaults
    dup2(tmpin, 0);
    dup2(tmpout, 1);
    dup2(tmperr, 2);
    close(tmpin);
    close(tmpout);
    close(tmperr);

    // Clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
