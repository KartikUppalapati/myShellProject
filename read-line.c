/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);
extern void reset_input_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.

int history_length = 0;
int history_index = 0;

char ** history;

void printHistory()
{
  for (int i = 0; i < history_length; i++)
  {
    printf("history[%d] is: %s", i, history[i]);
  }
}

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line(void) {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  history_index = history_length;
  int cursorPos = line_length;

  if (history_length == 0)
  {
    // Malloc for history array
    history = (char **) malloc((history_length + 1) * sizeof(char *));
  }

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);
    
    if (ch>=32 && ch != 127) {
      // It is a printable character. 

      // If char position isn't the same then shift all characters ahead to the right
      if (cursorPos != line_length)
      {
	  // Save original cursor position 
	  int originalCursorPos = cursorPos;

	  // Go to end of line
	  for (int i = cursorPos; i < line_length; i++)
	  {
	      // Go forward one char
	      write(1,&line_buffer[cursorPos],1);

	      // Adjust cursor pos 
	      cursorPos++;
	  }

	  // Print backspaces
	  int i = 0;
	  for (i =0; i < line_length; i++) {
	    char goback = 8;
	    write(1,&goback,1);
	  }

	  // Print spaces on top
	  for (i =0; i < line_length; i++) {
	    char space = ' ';
	    write(1,&space,1);
	  }

	  // Print backspaces
	  for (i =0; i < line_length; i++) {
	    char goback = 8;
	    write(1,&goback,1);
	  }

	  // Reset cursor pos to what it was before
	  cursorPos = originalCursorPos;

	  // Put all characters right of original cursor pos into new line buffer
	  for (int j = line_length; j >= cursorPos; j--)
	  {
	    // If place where inserted then increase index position
	    line_buffer[j] = line_buffer[j - 1];
	  }

	  // Put user input character at cursor place
	  line_buffer[cursorPos] = ch;
	
	  // Adjust line length and cursor pos
	  line_length++;

	  // Write new line to terminal
	  write(1, line_buffer, line_length);

	  // For loop to put cursor back where it was
	  for (int k = 0; k < line_length - cursorPos -  1; k++)
	  {
	    char goBack = 8;
	    write(1, &goBack, 1);
	  }

	  // Adjust cursor pos
	  cursorPos++;
      }
      // Else normal insertion
      else
      {
	// Do echo
	write(1,&ch,1);

	// If max number of character reached return.
	if (line_length==MAX_BUFFER_LINE-2) break; 

	// add char to buffer.
	line_buffer[cursorPos]=ch;
	line_length++;
	cursorPos++;
      }
    }
    // Else if tab was pressed
    else if (ch == 9 && line_length != 0)
    {
      // Save original cursor position
      int originalCursorPos = cursorPos;

      // Go back till space is found
      while (line_buffer[cursorPos - 1] != ' ')
      {
	// Go back one character
        ch = 8;
        write(1,&ch,1);
	cursorPos--;

	// If at start of line break
	if (cursorPos == 0)
	{
	  break;
	}
      }

      // Get word to match by itself
      int wordLength = originalCursorPos - cursorPos;
      char wordToMatch[wordLength + 1];

      // Put chars into word to match
      for (int i = 0; i < wordLength; i++)
      {
	 // Add char at cursor pos to wordToMmatch
	 wordToMatch[i] = line_buffer[cursorPos];

	 // Go to right 
	 write(1,&line_buffer[cursorPos],1);

	 // Increment cursor pos
	 cursorPos++;
      }

      // Put null terminator
      wordToMatch[wordLength] = '\0';

      // Make dir variables
      struct dirent * fileInDirectory;
      DIR * directory;

      // Open dir
      directory = opendir("./");

      // Check that its not null
      if (directory == NULL)
      {
	 break;
      }
 
      // Go through files in directory and check for matches
      int totalFiles = 0;
      int numberOfMatches = 0;
      while ((fileInDirectory = readdir(directory)) != NULL) 
      {
	 // Variable to hold if file matches word
	 int contains = 1;

	 // Go through and check for any matches
	 for (int i = 0; i < wordLength; i++)
	 {
	    if (wordToMatch[i] != fileInDirectory->d_name[i])
	    {
	      contains = 0;
	      break;
	    }	
	 }

	 // Increment matches
	 if (contains == 1)
         {
	   numberOfMatches++;
	 }

	 // Increment total numeber of files
	 totalFiles++;
      }
      
      // Close dir
      closedir(directory);

      // Open dir
      directory = opendir("./");

      // Check that its not null
      if (directory == NULL)
      {
	 break;
      }
 
      // If number of matches is 1 then just write it to line
      if (numberOfMatches == 1)
      {
	while ((fileInDirectory = readdir(directory)) != NULL) 
	{
	   // Variable to hold if file matches word
	   int contains = 1;

	   // Go through and check for any matches
	   for (int i = 0; i < wordLength; i++)
	   {
	     if (wordToMatch[i] != fileInDirectory->d_name[i])
	     {
	       contains = 0;
	       break;
	     }	
	   }

	   // If match found then write it to line
	   if (contains == 1)
	   {
	     // Go back to start of wordToMatch
	     for (int i = 0; i < wordLength; i++)
	     {
	       // Go back one character
               ch = 8;
               write(1,&ch,1);
	       cursorPos--;
	     }

	     // Write to line
	     write(1, fileInDirectory->d_name, strlen(fileInDirectory->d_name));

	     // Put in line buffer
	     for (int i = 0; i < strlen(fileInDirectory->d_name); i++)
	     {
		line_buffer[i + cursorPos] = fileInDirectory->d_name[i];
	     }
	     
	     // Adjust line length ( - 1 for null terminator at end of file directory name)
	     line_length += strlen(fileInDirectory->d_name) - wordLength;
	     cursorPos = line_length;

	     // Leave while loop
	     break;
	   }
	}
      }
      // Else if number of matches > 0 find max match string in all the files
      else if (numberOfMatches > 0)
      {
	// Create matching files array to hold names of files
	char * matchingFiles[numberOfMatches];	
	int matchingFileCount = 0;
	int minWordLengthMatched = wordLength; 
	int maxWordLengthMatched;

	// Use while loop to put all matches in array
        while ((fileInDirectory = readdir(directory)) != NULL) 
	{
	   // Variable to hold if file matches word
	   int contains = 1;

	   // Go through and check for any matches
	   for (int i = 0; i < wordLength; i++)
	   {
	     if (wordToMatch[i] != fileInDirectory->d_name[i])
	     {
	       contains = 0;
	       break;
	     }	
	   }

	   // If match found then put it in the matching files array
	   if (contains == 1)
	   {
	     matchingFiles[matchingFileCount] = fileInDirectory->d_name;
	     matchingFileCount++;
	   }
	 }

	 // Make max word length match the length of the first string in the matching array
	 maxWordLengthMatched = strlen(matchingFiles[0]);

         // Go through all files in matching files array
	 for (int i = 1; i < numberOfMatches; i++)
	 {
	   // Create variable to keep track of number of chars the file matched
	   int charsMatched = 0;

	   // Go through each char of file
	   for (int j = 0; j < maxWordLengthMatched; j++)
	   {
	     //printf("j index is: %d original j char is: %c check j char is: %c\n", j, matchingFiles[0][j], matchingFiles[i][j]);
	     // If the j char matches then increase the chars matched variable
	     if (matchingFiles[i][j] == matchingFiles[0][j])
	     {
	       charsMatched++;
	     }
	     // Else no match so just break
	     else
	     {
	       break;
	     }
	   }

	   // Reset the max chars matched variable
	   maxWordLengthMatched = charsMatched;
	 }

	 // If max word length matched is greater than the original
	 if (maxWordLengthMatched > minWordLengthMatched)
	 {
	     // Go back to start of wordToMatch
	     for (int i = 0; i < wordLength; i++)
	     {
	       // Go back one character
               ch = 8;
               write(1,&ch,1);
	       cursorPos--;
	     }

	     // Write to line
	     write(1, matchingFiles[0], maxWordLengthMatched);

	     // Put in line buffer
	     for (int i = 0; i < maxWordLengthMatched; i++)
	     {
		line_buffer[i + cursorPos] = matchingFiles[0][i];
	     }
	     
	     // Adjust line length ( - 1 for null terminator at end of file directory name)
	     line_length += maxWordLengthMatched - minWordLengthMatched;
	     cursorPos = line_length;
	     //fprintf(stderr, "\nnew length is: %d new position is: %d\n", line_length, cursorPos);
	 }
      }

      // Close dir
      closedir(directory);
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      
      // Print newline
      write(1,&ch,1);
      
      if (line_length == 0)
      {
	fprintf(stderr, "myshell>");
      }

      // Add eol and null char at the end of string
      line_buffer[line_length]=10;
      line_length++;
      line_buffer[line_length]=0;

      if (line_length > 1)
      {      
        // Realloc history array
	history = (char **) realloc(history, (history_length + 1) * sizeof(char *));
	assert(history != NULL);

       // Malloc for line
	char * newHistoryLine = (char *) malloc(line_length + 1);

	// Strcpy stuff
	strcpy(newHistoryLine, line_buffer);

	// Put new command into history table
	history[history_length] = newHistoryLine;

	// Increment size of history table
	history_length++;
	history_index = history_length;
      }

      // Break 
      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    // ctrl a aka home key
    else if (ch == 1)
    {
	// For loop to go to start of line
	for (int i = 0; i < cursorPos; i++)
	{
	    // Go back one character
            ch = 8;
            write(1,&ch,1);
	}
	
	// Reset cursor pos
	cursorPos = 0;	
    }
    // ctr e aka end key
    else if (ch == 5)
    {
	// Go to end of line
	for (int i = cursorPos; i < line_length; i++)
	{
	    // Go forward one char
            write(1,&line_buffer[cursorPos],1);

	    // Adjust cursor pos 
	    cursorPos++;
	}
    }
    else if (ch == 4) 
    {
      // Delete pressed on line length - 1 
      if (line_length != 0 && cursorPos <= line_length - 1)
      {      
        // Write a space to erase the last character read
        ch = ' ';
        write(1,&ch,1);

        // Go back one character
        ch = 8;
        write(1,&ch,1);

	// Remove char at cursor
	line_buffer[cursorPos] = ' ';

        // Remove one character from buffer
        line_length--;
      }
      // If deleted somewhere in middle
      if (cursorPos != line_length)
      {
	  // Save original cursor position 
	  int originalCursorPos = cursorPos;

	  // Temporarily increase line length
	  line_length++;

	  // Go to end of line
	  for (int i = cursorPos; i < line_length; i++)
	  {
	      // Go forward one char
	      write(1,&line_buffer[cursorPos],1);

	      // Adjust cursor pos 
	      cursorPos++;
	  }

	  // Print backspaces
	  int i = 0;
	  for (i =0; i < line_length; i++) {
	    char goback = 8;
	    write(1,&goback,1);
	  }

	  // Print spaces on top
	  for (i =0; i < line_length; i++) {
	    char space = ' ';
	    write(1,&space,1);
	  }

	  // Print backspaces
	  for (i =0; i < line_length; i++) {
	    char goback = 8;
	    write(1,&goback,1);
	  }

	  // Reset cursor pos to what it was before
	  cursorPos = originalCursorPos;

	  // Decrease line length after clearing line
	  line_length--;

	  // Shift chars right of original cursor pos into new line buffer
	  for (int j = cursorPos; j < line_length; j++)
	  {
	    // If place where inserted then increase index position
	    line_buffer[j] = line_buffer[j + 1];
	  }

	  // Write new line to terminal
	  write(1, line_buffer, line_length);

	  // For loop to put cursor back where it was
	  for (int k = 0; k < line_length - cursorPos; k++)
	  {
	    char goBack = 8;
	    write(1, &goBack, 1);
	  }
      }

    }
    else if (ch == 8 || ch == 127) 
    {
      // Backspace pressed
      if (line_length != 0 && cursorPos != 0)
      {      
        // Go back one character
        ch = 8;
        write(1,&ch,1);

        // Write a space to erase the last character read
        ch = ' ';
        write(1,&ch,1);

        // Go back one character
        ch = 8;
        write(1,&ch,1);

        // Remove one character from buffer
        line_length--;
	cursorPos--;
      }
      // If backspaced somewhere in middle
      if (cursorPos != line_length && cursorPos != 0)
      {
	  // Save original cursor position 
	  int originalCursorPos = cursorPos;

	  // Temporarily increase line length
	  line_length++;

	  // Go to end of line
	  for (int i = cursorPos; i < line_length; i++)
	  {
	      // Go forward one char
	      write(1,&line_buffer[cursorPos],1);

	      // Adjust cursor pos 
	      cursorPos++;
	  }

	  // Print backspaces
	  int i = 0;
	  for (i =0; i < line_length; i++) {
	    char goback = 8;
	    write(1,&goback,1);
	  }

	  // Print spaces on top
	  for (i =0; i < line_length; i++) {
	    char space = ' ';
	    write(1,&space,1);
	  }

	  // Print backspaces
	  for (i =0; i < line_length; i++) {
	    char goback = 8;
	    write(1,&goback,1);
	  }

	  // Reset cursor pos to what it was before
	  cursorPos = originalCursorPos;

	  // Decrease line length after clearing line
	  line_length--;

	  // Shift chars right of original cursor pos into new line buffer
	  for (int j = cursorPos; j < line_length; j++)
	  {
	    // If place where inserted then increase index position
	    line_buffer[j] = line_buffer[j + 1];
	  }

	  // Write new line to terminal
	  write(1, line_buffer, line_length);

	  // For loop to put cursor back where it was
	  for (int k = 0; k < line_length - cursorPos; k++)
	  {
	    char goBack = 8;
	    write(1, &goBack, 1);
	  }
      }
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      // Up arrow. Print next line in history.
      if (ch1==91 && ch2==65) 
      {
	// If out of bounds then go back in bounds
	if (history_index == 0)
	{
	  history_index++;
	}
	// Check if can go up
	if (history_index > 0 && history_length != 0)
	{
	  // Go to end of line
	  for (int i = cursorPos; i < line_length; i++)
	  {
	      // Go forward one char
	      write(1,&line_buffer[cursorPos],1);

	      // Adjust cursor pos 
	      cursorPos++;
	  }

	  // Erase old line
	  // Print backspaces
	  int i = 0;
	  for (i =0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	  }

	  // Print spaces on top
	  for (i =0; i < line_length; i++) {
	    ch = ' ';
	    write(1,&ch,1);
	  }

	  // Print backspaces
	  for (i =0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	  }	

	  // Copy line from history
	  strcpy(line_buffer, history[history_index - 1]);

	  // Adjust variables
	  line_length = strlen(line_buffer) - 1;
	  cursorPos = line_length;
	  history_index--;

	  // echo line
	  write(1, line_buffer, line_length);
	}
     }
      // Down arrow. Print prev line in history.
      else if (ch1==91 && ch2==66) 
      {
	// Check if can go up
	if (history_index < history_length)
	{
	  // Go to end of line
	  for (int i = cursorPos; i < line_length; i++)
	  {
	      // Go forward one char
	      write(1,&line_buffer[cursorPos],1);

	      // Adjust cursor pos 
	      cursorPos++;
	  }

	  // Erase old line
	  // Print backspaces
	  int i = 0;
	  for (i =0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	  }

	  // Print spaces on top
	  for (i =0; i < line_length; i++) {
	    ch = ' ';
	    write(1,&ch,1);
	  }

	  // Print backspaces
	  for (i =0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	  }	

	  // If not out of bounds of history length
	  if (history_index + 1 != history_length)
	  {
	    // Copy line from history
	    strcpy(line_buffer, history[history_index + 1]);

	    // Adjust variables
	    line_length = strlen(line_buffer) - 1;
	    cursorPos = line_length;
	    history_index++;

	    // echo line
	    write(1, line_buffer, line_length);
	  }
	  else
	  {
	    // Else are out of bounds so reset length and cursor position
	    line_length = 0;
	    cursorPos = 0;
	    history_index++;
	  }
	}	
      }
      else if (ch1==91 && ch2==68) 
      {
	// Left arrow
	if (cursorPos != 0)
	{
	    // Go back one character
            ch = 8;
            write(1,&ch,1);

	    // Adjust cursor pos
	    cursorPos--;
	}
      }
      else if (ch1==91 && ch2==67) 
      {
	// Right arrow
	if (cursorPos < line_length)
	{
	    // Go forward one char
            write(1,&line_buffer[cursorPos],1);

	    // Adjust cursor pos 
	    cursorPos++;
	}
      }
    }
  }

  // Print history stuff
  //printHistory();

  // Reset terminal mode
  reset_input_mode();

  return line_buffer;
}

