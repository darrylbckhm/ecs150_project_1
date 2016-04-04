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
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <sstream>

#include "ashell.h"

int main()
{
  struct termios SavedTermAttributes;
  
  list<string> commands;
  int commands_current_index = 0;

  char raw_input_string[128];
  int raw_input_string_index = 0;
  char raw_input;

  int exit = false;
  
  get_working_dir()
  
  int working_directory_length = working_directory.length();

  // set to noncanonical mode, treat input as characters
  // instead of lines ending on newline or EOF
  set_non_canonical_mode(STDIN_FILENO, &SavedTermAttributes);

  // set prompt
  write(1, working_directory.c_str(), working_directory_length);
  write(1, "%", 1);

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
        // check if any commands in history or if current spot in history is at top
        if (commands_current_index == 0)
        {
          commands_current_index++;
          cout << "play bell" << endl;
        }
        // start at beginning of commands list and move to one before
        // the current index. then move the index to the earlier command
        list<string>::iterator iter = commands.begin();
        advance(iter, commands_current_index-1);
        commands_current_index--;
        cout << "command before: " << *iter;
      }
      else if (raw_input == 0x42)
      {

        // increment if at beginning of list
        if (commands_current_index == 0)
        {
          commands_current_index++;
        }

        // check if at end of command list (at prompt)
        if ((int)commands.size() == commands_current_index)
          cout << "end of line" << endl;
        else
        {
          // start at beginning of commands list and move to index
          // move index to the later command
          list<string>::iterator iter = commands.begin();
          advance(iter, commands_current_index);
          commands_current_index++;
          cout << "command after: " << *iter;
        }
      }
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

          // if commands history full, remove the oldest
          if (commands.size() == 10)
            commands.pop_front();

          // add command to end of history list
          commands.push_back(command);
          commands_current_index = commands.size();

          // write(1, raw_input_string, raw_input_string_index);
          // reset index to zero for new input
          raw_input_string_index = 0;
       
          // write prompt
          write(1, working_directory.c_str(), working_directory_length);
          write(1, "%", 1);
        }
      }

    }
  }

  // reset mode to canonical
  reset_canonical_mode(STDIN_FILENO, &SavedTermAttributes); 

  return 0;
}
