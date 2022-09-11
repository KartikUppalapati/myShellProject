
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

/* 
 * Sets terminal into raw mode. 
 * This causes having the characters available
 * immediately instead of waiting for a newline. 
 * Also there is no automatic echo.
 */

// Struct for resetting on exit
struct termios saved_attributes;
void reset_input_mode (void)
{
    tcsetattr(0, TCSAFLUSH, &saved_attributes);
}

void tty_raw_mode(void)
{
	// Change mode struct
	struct termios tty_attr;

	if (!isatty (STDIN_FILENO))
    	{
      	  fprintf (stderr, "Not a terminal.\n");
          exit (EXIT_FAILURE);
    	}

	// Save for resetting terminal mode
	tcgetattr(0, &saved_attributes);
  	atexit(reset_input_mode);

	/* Set raw mode. */
	tcgetattr(0, &tty_attr);
	tty_attr.c_lflag &= (~(ICANON|ECHO));
	tty_attr.c_cc[VTIME] = 0;
	tty_attr.c_cc[VMIN] = 1;
     
	tcsetattr(0, TCSAFLUSH, &tty_attr);
}
