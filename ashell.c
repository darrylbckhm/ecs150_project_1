#include <unistd.h> 
#include <string.h>
#include <string>
#include <iostream>

using namespace std;

void reset_canonical_mode(int fd, struct termios *savedattributes)
{
  tcsetattr(fd, TCSANOW, savedattributes);
}

void set_non_canonical_mode(int fd, struct termios *savedattributes) 
{
  struct termios TermAttributes;
  char *name;

  // make sure stdin is a terminal
  if (!isatty(fd)) 
  {
    fprintf (stderr, "Not a terminal.\n");
    exit(0);
  }

  // save the terminal attributes so we can restore them later
  tcgetattr(fd, savedattributes);

  // set the terminal modes
  tcgetattr (fd, &TermAttributes);
  TermAttributes.c_lflag &= ~(ICANON | ECHO); // clear ICANON and ECHO
  TermAttributes.c_cc[VMIN] = 1;
  TermAttributes.c_cc[VTIME] = 0;
  tcsetattr(fd, TCSAFLUSH, &TermAttributes);
}

int main()
{
  struct termios SavedTermAttributes;
  char raw_input[128];
  int exit = false;

  while (exit == false)
  {
    // read raw input for 128 characters
    read(0, raw_input, 128);

    // find first newline character position in raw_input
    int i;

    for (i = 0; i < 128; i++)
    {
      if (raw_input[i] == '\n')
        break;
    }

    // turn the next character at that position to a null character
    raw_input[i+1] = '\0';

    if (strcmp(raw_input, "exit\n") == 0)
      exit = true;
    else
      // write the full string to stdout
      write(1, raw_input, i+1);

  }
  return 0;
}
