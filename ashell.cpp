#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <iostream>
#include <list>
#include <iterator>

using namespace std;

void reset_canonical_mode(int fd, struct termios *savedattributes)
{
  tcsetattr(fd, TCSANOW, savedattributes);
}

void set_non_canonical_mode(int fd, struct termios *savedattributes) 
{
  struct termios TermAttributes;

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
  
  list<string> commands;
  int commands_current_index = 0;

  char raw_input_string[128];
  int raw_input_string_index = 0;
  char raw_input;

  int exit = false;

  // set to noncanonical mode, treat input as characters
  // instead of lines ending on newline or EOF
  set_non_canonical_mode(STDIN_FILENO, &SavedTermAttributes);

  // endless loop until exit typed
  while (exit == false)
  {
    // read raw input one character at a time
    read(0, &raw_input, 1);

    // if escape character found, check which arrow key was pressed
    if (raw_input == 0x1B)
    {
      // skip second escape character and check which arrow it is
      read(0, &raw_input, 1);
      read(0, &raw_input, 1);
      
      // Up Arrow => 0x1B 0x5B 0x41
      // Down Arrow => 0x1B 0x5B 0x42
      // source used: #1
      if (raw_input == 0x41)
      {
        cout << "command size: " << commands.size() << endl;
        // check if any commands in history or if current spot in history is at top
        if (commands_current_index == 0)
          cout << "play bell" << endl;
        else
        {
          list<string>::iterator iter = commands.begin();
          cout << "earlier command: " << *iter;
        }
      }
      else if (raw_input == 0x42)
        write(1, "down\n", 5);
    }
    else
    {
      // write input character out for user to see
      write(1, &raw_input, 1);
  
      // copy char to input string 
      raw_input_string[raw_input_string_index++] = raw_input;
  
      // on enter, end string and display back to screen
      if (raw_input == '\n')
      {
        raw_input_string[raw_input_string_index] = '\0';
  
        if (strcmp(raw_input_string, "exit\n") == 0)
          exit = true;
        else
        {
          string command(raw_input_string);
          if (commands.size() == 10)
            commands.pop_front();
          cout << "size before add: " << commands.size() << endl;
          commands.push_back(command);
          commands_current_index = commands.size();
          cout << "size after add: " << commands.size() << endl;
          // write(1, raw_input_string, raw_input_string_index);
          // reset index to zero for new input
          raw_input_string_index = 0;
        }
      }

    }
  }

  // reset mode to canonical
  reset_canonical_mode(STDIN_FILENO, &SavedTermAttributes);

  return 0;
}
