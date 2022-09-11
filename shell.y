
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>
#include <unistd.h>
#include <sys/types.h>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD WORD_QUOTES SUBSHELL_WORD
%token NOTOKEN GREAT NEWLINE GREATGREAT AMPERSAND PIPE LESS TWOGREAT GREATAMPERSAND GREATGREATAMPERSAND

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"
#include <sys/syscall.h>
#include <cstring>
#include <dirent.h>
#include <sys/wait.h>
#include <cassert>

void yyerror(const char * s);
int yylex();

// Global variables to keep track of nEntries and maxEntries
int nEntries = 0;
int maxEntries = 20;
char ** array;

// Helper function to do recursive search
void doRecursiveSearch(std::string path, std::string pattern)
{
    // Get found location
    std::string* word = new std::string(path + "/" + pattern);
    std::string* originalWord = new std::string(path + "/" + pattern);
    int foundStar = word->find("*");
    int foundLastBackslash = word->substr(0, foundStar).find_last_of("/");
    int notFullPath = 1;
    std::string relativePath = path + "/";

    // Get directory
    struct dirent *fileInDirectory;
    DIR *directory;

    // Open directory
    directory = opendir(path.data());

    // Check that given path is a directory and not a file
    if (directory == NULL)
    {
	return;
    }

    // Check for backslash afer found star location and get / to next / substring  by itself
    if (word->find("/", foundStar) != -1)
    {
	word = new std::string(word->substr(foundLastBackslash + 1, foundStar - foundLastBackslash));
    }
    else
    {
	word = new std::string(word->substr(foundLastBackslash + 1, word->size()));
    }

    //printf("new path is: %s pattern to match is: %s\n", path.c_str(), pattern.c_str());
    //printf("new word is: %s\n", word->c_str());

    // Process each entry.
    while ((fileInDirectory = readdir(directory)) != NULL) 
    {
	// If a star after star then find anywhere in string
	int firstStar = word->find("*");
	if (word->find("*", firstStar + 1) != -1)
	{
	  // Initialize variable
	  int contains = -1;
	    
	  // Get word to find
	  std::string wordToFind = word->substr(firstStar + 1, word->find("*", firstStar + 1) - firstStar - 1);
	  std::string fileAsString = std::string(fileInDirectory->d_name);	     

	  // Check if it contains it
	  contains = fileAsString.find(wordToFind);

	  // If need to do wildcard recursively
	  if (contains >= 0 && originalWord->find(word->data()) != originalWord->size() - word->size())
	  {
	    // Get next wildcard pattern
	    std::string newPattern = originalWord->substr(originalWord->find(word->data()) + word->size() + 1, originalWord->size());

	    // Do recursive serach with current path
	    if (notFullPath == 0)
	    {
	      doRecursiveSearch("./", newPattern);
	    }
	    // Else relative path so add it to file name
	    else
	    {
	      // Get relative path by concatenating buffers
	      char locationBuffer[relativePath.size() + 256];
	      for (int i = 0; i < relativePath.size(); i++)
	      {
		  locationBuffer[i] = relativePath.at(i);
	      }
	      for (int i = 0; i < 256; i++)
	      {
		  locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
	      }

	      // Do recursive search with relative path
	      doRecursiveSearch(std::string(locationBuffer), newPattern);
	    }
	  }
	  // If it matches
	  else if (contains >= 0)
	  {
	    // If max file length reached then increase it
	    if (nEntries == maxEntries)
	    {
	      maxEntries = maxEntries * 2;
	      array = (char**) realloc(array, maxEntries * sizeof(char*));
	      assert(array != NULL);
	    }

	    // Put in array
	    if (notFullPath == 0)
	    {
	      array[nEntries] = strdup(fileInDirectory->d_name); 
	      nEntries++;
	    }
	    // Else relative path so add it to file name
	    else
	    {
	      // Get relative path by concatenating buffers
	      char locationBuffer[relativePath.size() + 256];
	      for (int i = 0; i < relativePath.size(); i++)
	      {
		  locationBuffer[i] = relativePath.at(i);
	      }
	      for (int i = 0; i < 256; i++)
	      {
		  locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
	      }

	      // Put concatenated buffer into array
	      array[nEntries] = strdup(locationBuffer); 
	      nEntries++;
	    }
	  }
	}
	// Check for post characters after *
	else if (word->find("*") + 1 != word->size())
	{
	  // Initialize variable
	  int contains = 1;
	    
	  // Get word to find
	  int foundStar = word->find("*");
	  std::string wordToFind = word->substr(foundStar + 1, word->size() - foundStar);
	  std::string fileAsString = std::string(fileInDirectory->d_name);	     

	  // Check for matches
	  for (int i = 0; i < wordToFind.size(); i++)
	  {
	    if (wordToFind.at(i) != fileAsString.at(fileAsString.size() - i - 1))
	    {
	      contains = 0;
	      break;
	    }	
	  }
    
	  // If need to do wildcard recursively
	  if (contains == 1 && originalWord->find(word->data()) != originalWord->size() - word->size())
	  {
	    // Get next wildcard pattern
	    std::string newPattern = originalWord->substr(originalWord->find(word->data()) + word->size() + 1, originalWord->size());

	    // Do recursive serach with current path
	    if (notFullPath == 0)
	    {
	      doRecursiveSearch("./", newPattern);
	    }
	    // Else relative path so add it to file name
	    else
	    {
	      // Get relative path by concatenating buffers
	      char locationBuffer[relativePath.size() + 256];
	      for (int i = 0; i < relativePath.size(); i++)
	      {
		  locationBuffer[i] = relativePath.at(i);
	      }
	      for (int i = 0; i < 256; i++)
	      {
		  locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
	      }

	      // Do recursive search with relative path
	      doRecursiveSearch(std::string(locationBuffer), newPattern);
	    }
	  }
	  // If it matches
	  else if (contains == 1)
	  {
	    // If max file length reached then increase it
	    if (nEntries == maxEntries)
	    {
	      maxEntries = maxEntries * 2;
	      array = (char**) realloc(array, maxEntries * sizeof(char*));
	      assert(array != NULL);
	    }

	    // Put in array
	    if (notFullPath == 0)
	    {
	      array[nEntries] = strdup(fileInDirectory->d_name); 
	      nEntries++;
	    }
	    // Else relative path so add it to file name
	    else
	    {
	      // Get relative path by concatenating buffers
	      char locationBuffer[relativePath.size() + 256];
	      for (int i = 0; i < relativePath.size(); i++)
	      {
		  locationBuffer[i] = relativePath.at(i);
	      }
	      for (int i = 0; i < 256; i++)
	      {
		  locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
	      }

	      // Put concatenated buffer into array
	      array[nEntries] = strdup(locationBuffer); 
	      nEntries++;
	    }
	  }
	}
	// Check for preceding characters of *
	else if(word->find("*") != 0)
	{
	  // Initialize variable
	  int contains = 1;
	    
	  // Get word to find
	  std::string wordToFind = word->substr(0, word->find("*"));

	  // Check for matches
	  for (int i = 0; i < wordToFind.size(); i++)
	  {
	    if (wordToFind.at(i) != fileInDirectory->d_name[i])
	    {
	      contains = 0;
	      break;
	    }	
	  }

	  // If need to do wildcard recursively
	  if (contains == 1 && originalWord->find(word->data()) != originalWord->size() - word->size())
	  {
	    // Get next wildcard pattern
	    std::string newPattern = originalWord->substr(originalWord->find(word->data()) + word->size() + 1, originalWord->size());

	    // Do recursive serach with current path
	    if (notFullPath == 0)
	    {
	      doRecursiveSearch("./", newPattern);
	    }
	    // Else relative path so add it to file name
	    else
	    {
	      // Get relative path by concatenating buffers
	      char locationBuffer[relativePath.size() + 256];
	      for (int i = 0; i < relativePath.size(); i++)
	      {
		  locationBuffer[i] = relativePath.at(i);
	      }
	      for (int i = 0; i < 256; i++)
	      {
		  locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
	      }

	      // Do recursive search with relative path
	      doRecursiveSearch(std::string(locationBuffer), newPattern);
	    }
	  }
	  // If it matches
	  else if (contains == 1)
	  {
	    // If max file length reached then increase it
	    if (nEntries == maxEntries)
	    {
	      maxEntries = maxEntries * 2;
	      array = (char**) realloc(array, maxEntries * sizeof(char*));
	      assert(array != NULL);
	    }

	    // Put in array
	    if (notFullPath == 0)
	    {
	      array[nEntries] = strdup(fileInDirectory->d_name); 
	      nEntries++;
	    }
	    // Else relative path so add it to file name
	    else
	    {
	      // Get relative path by concatenating buffers
	      char locationBuffer[relativePath.size() + 256];
	      for (int i = 0; i < relativePath.size(); i++)
	      {
		  locationBuffer[i] = relativePath.at(i);
	      }
	      for (int i = 0; i < 256; i++)
	      {
		  locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
	      }

	      // Put concatenated buffer into array
	      array[nEntries] = strdup(locationBuffer); 
	      nEntries++;
	    }
	  }
	}
	// Else just a star
	else if (fileInDirectory->d_name[0] != '.')
	{
	  // If need to do wildcard recursively
	  if (originalWord->find(word->data()) != originalWord->size() - word->size())
	  {
	    // Get next wildcard pattern
	    std::string newPattern = originalWord->substr(originalWord->find(word->data()) + word->size() + 1, originalWord->size());

	    // Do recursive serach with current path
	    if (notFullPath == 0)
	    {
	      doRecursiveSearch("./", newPattern);
	    }
	    // Else relative path so add it to file name
	    else
	    {
	      // Get relative path by concatenating buffers
	      char locationBuffer[relativePath.size() + 256];
	      for (int i = 0; i < relativePath.size(); i++)
	      {
		  locationBuffer[i] = relativePath.at(i);
	      }
	      for (int i = 0; i < 256; i++)
	      {
		  locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
	      }

	      // Do recursive search with relative path
	      doRecursiveSearch(std::string(locationBuffer), newPattern);
	    }
	  }
	  // Else just add it into array
	  else
	  {
	      // If max file length reached then increase it
	      if (nEntries == maxEntries)
	      {
		  maxEntries = maxEntries * 2;
		  array = (char**) realloc(array, maxEntries * sizeof(char*));
		  assert(array != NULL);
	      }

	      // Put in array
	      if (notFullPath == 0)
	      {
		  array[nEntries] = strdup(fileInDirectory->d_name); 
		  nEntries++;
	      }
	      // Else relative path so add it to file name
	      else
	      {
		  // Get relative path by concatenating buffers
		  char locationBuffer[relativePath.size() + 256];
		  for (int i = 0; i < relativePath.size(); i++)
		  {
		      locationBuffer[i] = relativePath.at(i);
		  }
		  for (int i = 0; i < 256; i++)
		  {
		      locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
		  }

		  // Put concatenated buffer into array
		  array[nEntries] = strdup(locationBuffer); 
		  nEntries++;
	      }
	  }
	}
    }

    //printf("recursively added %d entries\n", nEntries);
    // Close diretory
    closedir(directory);
}


%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: simple_command
       ;

simple_command:	
  command_and_args multiple_commands_and_args iomodifiers_list background NEWLINE {
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | NEWLINE
  | error NEWLINE { yyerrok; }
  ;

// Grammer for multiple iomodifiers lists
iomodifiers_list:
  iomodifiers
  | iomodifiers_list iomodifiers
  | /* can be empty */
  ;


// Grammar for list iomodifier commands
iomodifiers:
  iomodifier_out iomodifier_in iomodifier_err iomodifier_out_and_err 
  iomodifier_append_out iomodifier_append_out_and_err
  ;

// Grammar for multiple piped commands
multiple_commands_and_args:
  multiple_commands_and_args PIPE command_and_args
  | /* can be empty */
  ;

// Grammar for background ampersand
background:
  AMPERSAND {
    Shell::_currentCommand._background = true;
  }
  | /* can be empty */


command_and_args:
  command_word argument_list {
    for (int i = 0; i < Command::_currentSimpleCommand->_arguments.size(); i++)
    {
      //fprintf(stderr, "%s\n", Command::_currentSimpleCommand->_arguments[i]->c_str());
    }
    //fprintf(stderr, "inserting here\n");
    //Shell::_currentCommand.print();
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    // Get word
    std::string* word = $1;
    int dontAddExtra = 0;

    // Check if word has > in it 
    if (word->find(">") != -1)
    {
      // Parse command and outfile from string
      std::string delimiter = ">";
      std::string argument = (word->substr(0, word->find(delimiter)));
      std::string outfile = word->substr(word->find(delimiter) + 1, word->size());
    
      // Set outfile 
      word = new std::string(argument);
      Shell::_currentCommand._outFile = new std::string(outfile);
    }
    // Else if word has a \ in it
    else if (word->find("\\") != -1)
    {
      // Find all \'s and parse
      int position = 0;
      while (word->find("\\", position) != -1)
      {
	// Parse left and right side of \ symbol
        std::string delimiter = "\\";
        std::string left = word->substr(0, word->find(delimiter));
        std::string right = word->substr(word->find(delimiter) + 1, word->size());

        // Set word = to left and right sides concatenated
        word = new std::string(left + right);
        position = word->find(delimiter) + 1;
      }
	
    }
    // Do wild carding here
    else if (word->find("*") != -1 || word->find("?") != -1)
    {
      // If just a *
      if (word->find("*") != -1)
      {
        // Get found location
	int foundStar = word->find("*");
	int foundLastBackslash = word->substr(0, foundStar).find_last_of("/");
	int notFullPath = 0;

        // Get directory
        struct dirent *fileInDirectory;
        DIR *directory;
	std::string relativePath;
	array = (char **) malloc(maxEntries * sizeof(char*));
	nEntries = 0;
	maxEntries = 20;

        // Open directory
	if (foundStar == 0 || foundLastBackslash == -1)
	{
            directory = opendir("./");
	}
	else
	{
	    notFullPath = 1;
	    directory = opendir(word->substr(0, foundLastBackslash + 1).data());
	    relativePath = word->substr(0, foundLastBackslash + 1).data();
	}

	// Check for backslash afer found star location and get / to next / substring  by itself
	if (word->find("/", foundStar) != -1)
	{
	    word = new std::string(word->substr(foundLastBackslash + 1, foundStar - foundLastBackslash));
	}
	else
	{
	    word = new std::string(word->substr(foundLastBackslash + 1, word->size()));
	}

	//printf("new word is: %s\n", word->c_str());

        // Process each entry.
        while ((fileInDirectory = readdir(directory)) != NULL) 
        {
	    // If a star after star then find anywhere in string
	    int firstStar = word->find("*");
	    if (word->find("*", firstStar + 1) != -1)
	    {
	      // Initialize variable
	      int contains = -1;
		
	      // Get word to find
	      std::string wordToFind = word->substr(firstStar + 1, word->find("*", firstStar + 1) - firstStar - 1);
	      std::string fileAsString = std::string(fileInDirectory->d_name);	     

	      // Check if it contains it
	      contains = fileAsString.find(wordToFind);

	      // If need to do wildcard recursively
	      if (contains >= 0 && $1->find(word->data()) != $1->size() - word->size())
	      {
	 	// Get next wildcard pattern
		std::string newPattern = $1->substr($1->find(word->data()) + word->size() + 1, $1->size());
 
		// Do recursive serach with current path
	        if (notFullPath == 0)
	        {
		  doRecursiveSearch("./", newPattern);
	        }
	        // Else relative path so add it to file name
	        else
	        {
		  // Get relative path by concatenating buffers
		  char locationBuffer[relativePath.size() + 256];
		  for (int i = 0; i < relativePath.size(); i++)
		  {
		      locationBuffer[i] = relativePath.at(i);
		  }
		  for (int i = 0; i < 256; i++)
		  {
		      locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
		  }

		  // Do recursive search with relative path
		  doRecursiveSearch(std::string(locationBuffer), newPattern);
		}
	      }
	      // If it matches
	      else if (contains >= 0)
	      {
		// If max file length reached then increase it
	        if (nEntries == maxEntries)
	        {
		  maxEntries = maxEntries * 2;
		  array = (char**) realloc(array, maxEntries * sizeof(char*));
		  assert(array != NULL);
	        }

	        // Put in array
	        if (notFullPath == 0)
	        {
		  array[nEntries] = strdup(fileInDirectory->d_name); 
		  nEntries++;
	        }
	        // Else relative path so add it to file name
	        else
	        {
		  // Get relative path by concatenating buffers
		  char locationBuffer[relativePath.size() + 256];
		  for (int i = 0; i < relativePath.size(); i++)
		  {
		      locationBuffer[i] = relativePath.at(i);
		  }
		  for (int i = 0; i < 256; i++)
		  {
		      locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
		  }

		  // Put concatenated buffer into array
		  array[nEntries] = strdup(locationBuffer); 
		  nEntries++;
	        }
	      }
	    }
	    // Check for post characters after *
	    else if (word->find("*") + 1 != word->size())
	    {
	      // Initialize variable
	      int contains = 1;
		
	      // Get word to find
	      int foundStar = word->find("*");
	      std::string wordToFind = word->substr(foundStar + 1, word->size() - foundStar);
	      std::string fileAsString = std::string(fileInDirectory->d_name);	     

	      // Check for matches
	      for (int i = 0; i < wordToFind.size(); i++)
	      {
		if (wordToFind.at(i) != fileAsString.at(fileAsString.size() - i - 1))
		{
		  contains = 0;
		  break;
		}	
	      }
	
	      // If need to do wildcard recursively
	      if (contains == 1 && $1->find(word->data()) != $1->size() - word->size())
	      {
	 	// Get next wildcard pattern
		std::string newPattern = $1->substr($1->find(word->data()) + word->size() + 1, $1->size());
 
		// Do recursive serach with current path
	        if (notFullPath == 0)
	        {
		  doRecursiveSearch("./", newPattern);
	        }
	        // Else relative path so add it to file name
	        else
	        {
		  // Get relative path by concatenating buffers
		  char locationBuffer[relativePath.size() + 256];
		  for (int i = 0; i < relativePath.size(); i++)
		  {
		      locationBuffer[i] = relativePath.at(i);
		  }
		  for (int i = 0; i < 256; i++)
		  {
		      locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
		  }

		  // Do recursive search with relative path
		  doRecursiveSearch(std::string(locationBuffer), newPattern);
		}
	      }
	      // If it matches
	      else if (contains == 1)
	      {
		// If max file length reached then increase it
	        if (nEntries == maxEntries)
	        {
		  maxEntries = maxEntries * 2;
		  array = (char**) realloc(array, maxEntries * sizeof(char*));
		  assert(array != NULL);
	        }

	        // Put in array
	        if (notFullPath == 0)
	        {
		  array[nEntries] = strdup(fileInDirectory->d_name); 
		  nEntries++;
	        }
	        // Else relative path so add it to file name
	        else
	        {
		  // Get relative path by concatenating buffers
		  char locationBuffer[relativePath.size() + 256];
		  for (int i = 0; i < relativePath.size(); i++)
		  {
		      locationBuffer[i] = relativePath.at(i);
		  }
		  for (int i = 0; i < 256; i++)
		  {
		      locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
		  }

		  // Put concatenated buffer into array
		  array[nEntries] = strdup(locationBuffer); 
		  nEntries++;
	        }
	      }
	    }
	    // Check for preceding characters of *
	    else if(word->find("*") != 0)
	    {
	      // Initialize variable
	      int contains = 1;
		
	      // Get word to find
	      std::string wordToFind = word->substr(0, word->find("*"));
	     
	      // Check for matches
	      for (int i = 0; i < wordToFind.size(); i++)
	      {
		if (wordToFind.at(i) != fileInDirectory->d_name[i])
		{
		  contains = 0;
		  break;
		}	
	      }

	      // If need to do wildcard recursively
	      if (contains == 1 && $1->find(word->data()) != $1->size() - word->size())
	      {
	 	// Get next wildcard pattern
		std::string newPattern = $1->substr($1->find(word->data()) + word->size() + 1, $1->size());
 
		// Do recursive serach with current path
	        if (notFullPath == 0)
	        {
		  doRecursiveSearch("./", newPattern);
	        }
	        // Else relative path so add it to file name
	        else
	        {
		  // Get relative path by concatenating buffers
		  char locationBuffer[relativePath.size() + 256];
		  for (int i = 0; i < relativePath.size(); i++)
		  {
		      locationBuffer[i] = relativePath.at(i);
		  }
		  for (int i = 0; i < 256; i++)
		  {
		      locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
		  }

		  // Do recursive search with relative path
		  doRecursiveSearch(std::string(locationBuffer), newPattern);
		}
	      }
	      // If it matches
	      else if (contains == 1)
	      {
		// If max file length reached then increase it
	        if (nEntries == maxEntries)
	        {
		  maxEntries = maxEntries * 2;
		  array = (char**) realloc(array, maxEntries * sizeof(char*));
		  assert(array != NULL);
	        }

	        // Put in array
	        if (notFullPath == 0)
	        {
		  array[nEntries] = strdup(fileInDirectory->d_name); 
		  nEntries++;
	        }
	        // Else relative path so add it to file name
	        else
	        {
		  // Get relative path by concatenating buffers
		  char locationBuffer[relativePath.size() + 256];
		  for (int i = 0; i < relativePath.size(); i++)
		  {
		      locationBuffer[i] = relativePath.at(i);
		  }
		  for (int i = 0; i < 256; i++)
		  {
		      locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
		  }

		  // Put concatenated buffer into array
		  array[nEntries] = strdup(locationBuffer); 
		  nEntries++;
	        }
	      }
	    }
	    // Else just a star
	    else if (fileInDirectory->d_name[0] != '.')
	    {
	      // If need to do wildcard recursively
	      if ($1->find(word->data()) != $1->size() - word->size())
	      {
	 	// Get next wildcard pattern
		std::string newPattern = $1->substr($1->find(word->data()) + word->size() + 1, $1->size());
 
		// Do recursive serach with current path
	        if (notFullPath == 0)
	        {
		  doRecursiveSearch("./", newPattern);
	        }
	        // Else relative path so add it to file name
	        else
	        {
		  // Get relative path by concatenating buffers
		  char locationBuffer[relativePath.size() + 256];
		  for (int i = 0; i < relativePath.size(); i++)
		  {
		      locationBuffer[i] = relativePath.at(i);
		  }
		  for (int i = 0; i < 256; i++)
		  {
		      locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
		  }

		  // Do recursive search with relative path
		  doRecursiveSearch(std::string(locationBuffer), newPattern);
		}
	      }
	      // Else just add it into array
	      else
	      {
	          // If max file length reached then increase it
	          if (nEntries == maxEntries)
	          {
	              maxEntries = maxEntries * 2;
		      array = (char**) realloc(array, maxEntries * sizeof(char*));
		      assert(array != NULL);
	          }

	          // Put in array
	          if (notFullPath == 0)
	          {
	              array[nEntries] = strdup(fileInDirectory->d_name); 
	              nEntries++;
	          }
	          // Else relative path so add it to file name
	          else
	          {
		      // Get relative path by concatenating buffers
		      char locationBuffer[relativePath.size() + 256];
		      for (int i = 0; i < relativePath.size(); i++)
		      {
		          locationBuffer[i] = relativePath.at(i);
		      }
		      for (int i = 0; i < 256; i++)
		      {
		          locationBuffer[i + relativePath.size()] = fileInDirectory->d_name[i];
		      }

		      // Put concatenated buffer into array
		      array[nEntries] = strdup(locationBuffer); 
	              nEntries++;
	          }
	      }
	    }
        }

	// Close diretory
	closedir(directory);

	// Sort array
	Command::sortArrayStrings(array, nEntries);

	// Add entries into current simple commmand
	for (int i = 0; i < nEntries; i++)
	{
	    Command::_currentSimpleCommand->insertArgument(new std::string(array[i]));
	}

	// Free array
	free(array);

	// Set don't add to 1 
	dontAddExtra = 1;
      } 
    }

    // If not a wildcard 
    if (dontAddExtra == 0)
    {  
      // Add argument to command
      Command::_currentSimpleCommand->insertArgument( word );
    }
  }
  |
  WORD_QUOTES {
    Command::_currentSimpleCommand->insertArgument( new std::string($1->substr(1, $1->size() - 2)) );
  }
  |
  SUBSHELL_WORD {
    // Get just command and arguments
    std::string commandAndArgs = $1->substr(2, $1->size() - 3);  
   
    // Save default temp in and out
    int tmpin = dup(0);
    int tmpout = dup(1);
    
    // Create pipes
    int pin[2];
    int pout[2];
    pipe(pin);
    pipe(pout);

    // Parent writes to pin[1]
    std::string pipeWrite = commandAndArgs + "\nexitsub\n";
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
    int readValueSize = read(pout[0], returnBuffer, sizeof(returnBuffer));
   
    // Find all newlines in buffer
    std::string* subshellReturn = new std::string(returnBuffer);
    
    // Clear buffer
    for (int i = 0; i < readValueSize; i++)
    {
      returnBuffer[i] = '\0';
    }

    while (subshellReturn->find("\n") != -1)
    {
      // Get line
      int newlineLocation = subshellReturn->find("\n");
      std::string left = subshellReturn->substr(0, newlineLocation);
      std::string right = subshellReturn->substr(newlineLocation + 1, subshellReturn->size());
      
      // If line has a space in it then parse it and add each word of line into argument list
      if (left.find(" ") != -1)
      {
	// Variable to hold last remaining word from line
	std::string rightOfLeft;

	// While loop to find all words
  	while (left.find(" ") != -1)
	{
	  // Parse first word from line
	  int spaceLocation = left.find(" ");
	  std::string leftOfLeft = left.substr(0, spaceLocation);
	  rightOfLeft = left.substr(spaceLocation + 1, left.size());

	  // Insert word into argument list
	  Command::_currentSimpleCommand->insertArgument( new std::string(leftOfLeft) );
	  
	  // Set left to remaining
	  left = leftOfLeft;
	}

	// Add last word of line
	Command::_currentSimpleCommand->insertArgument( new std::string(rightOfLeft) );
      }
      else
      {
        Command::_currentSimpleCommand->insertArgument( new std::string(left) );
      }

      // Remove word from subshell and continue while loop
      subshellReturn = new std::string(right);
    } 
 
    // Restore in and out
    dup2(tmpin, 0);
    dup2(tmpout, 1);
  }
  ;


command_word:
  WORD 
  {
    //fprintf(stderr,"   Yacc: insert command \"%s\"\n", $1->c_str());
    // Check if word has > in it 
    if ($1->find(">") != -1)
    {
      // Parse command and outfile from string
      std::string delimiter = ">";
      std::string argument = $1->substr(0, $1->find(delimiter));
      std::string outfile = $1->substr($1->find(delimiter) + 1, $1->size());
   
      // Create new command and command and set outfile
      Command::_currentSimpleCommand = new SimpleCommand();
      Command::_currentSimpleCommand->insertArgument(new std::string(argument));
      Shell::_currentCommand._outFile = new std::string(outfile); 
    }
    // Check if word has | in it 
    else if ($1->find("|") != -1)
    {
      // Parse command and outfile from string
      std::string delimiter = "|";
      std::string argument1 = $1->substr(0, $1->find(delimiter));
      std::string argument2 = $1->substr($1->find(delimiter) + 1, $1->size());
   
      // Create first new command and add commands
      Command::_currentSimpleCommand = new SimpleCommand();
      Command::_currentSimpleCommand->myargv = Shell::myargv;
      Command::_currentSimpleCommand->insertArgument(new std::string(argument1));
      Shell::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
      
      // Create new second command and add commands
      Command::_currentSimpleCommand = new SimpleCommand();
      Command::_currentSimpleCommand->myargv = Shell::myargv;
      Command::_currentSimpleCommand->insertArgument(new std::string(argument2));
    }
    // Else just create new command and add command to it
    else
    {
      Command::_currentSimpleCommand = new SimpleCommand();
      Command::_currentSimpleCommand->myargv = Shell::myargv;
      Command::_currentSimpleCommand->insertArgument( $1 );
    }
  }
  ;

iomodifier_out:
  GREAT WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    if (!Shell::_currentCommand._outFile)
    {
      Shell::_currentCommand._outFile = $2;
    }
    else
    {
      printf("Ambiguous output redirect.\n");
    }
  }
  | /* can be empty */ 
  ;

// Grammar for input file 
iomodifier_in:
  LESS WORD {
    //printf("   Yacc: insert input \"%s\"\n", $2->c_str());
    Shell::_currentCommand._inFile = $2;
  }
  | /* can be empty */ 
  ;

// Grammar for output file
iomodifier_err:
  TWOGREAT WORD {
    //printf("   Yacc: insert error \"%s\"\n", $2->c_str());
    Shell::_currentCommand._errFile = $2;
  }
  | /* can be empty */ 
  ;

// Grammar for output and error file
iomodifier_out_and_err:
  GREATAMPERSAND WORD {
    //printf("   Yacc: insert error and output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._errFile = $2;
  }
  | /* can be empty */ 
  ;

// Grammar for append to output file
iomodifier_append_out:
  GREATGREAT WORD {
    //printf("   Yacc: insert append output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._appendOut = true;
  }
  | /* can be empty */ 
  ;

// Grammar for append to output and error file
iomodifier_append_out_and_err:
  GREATGREATAMPERSAND WORD {
    //printf("   Yacc: insert append error and output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._errFile = $2;
    Shell::_currentCommand._appendOut = true;
    Shell::_currentCommand._appendErr = true;
  }
  | /* can be empty */ 
  ;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
