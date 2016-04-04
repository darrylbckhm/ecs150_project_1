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

using namespace std;

void cd(vector<string>* tokens)
{

  string path = (*tokens)[1];

  if(isADirectory(&path))
  {

    write(1,"is dir",6);
    chdir((*tokens)[1].c_str());

  }

  else
  {
   
     write(1,"Not a directory", 20);
  }

}

bool isADirectory(string* path)
{

  const char* tmp = (*path).c_str();
  struct stat path_stat;
  stat(tmp, &path_stat);
  return S_ISDIR(path_stat.st_mode);

}

vector<string> tokenize(char* raw_input_string) //source: 4
{

  string str = raw_input_string;
  string buf;
  stringstream ss(str);

  vector<string> tokens;

  while (ss >> buf)
    tokens.push_back(buf);

  return tokens;

}

void ls(vector<string>* tokens) //source used: #2,3
{

  DIR *dir;
  struct dirent *dp;
  pid_t pid = fork();

  if(pid == 0) //child process
  {

      for(int i = 1; i < (*tokens).size(); i++)
      {

        string path = (*tokens)[i];

        if(isADirectory(&path))
        {

          dir = opendir(path.c_str());
          while((dp = readdir(dir)) != NULL)
            cout << dp->d_name << '\n';
      
        }

        else
        {

          cout << "Not a directory\n";

        }

      }

  }

}
void downHistory(list<string>* commands, int* commands_current_index, char *raw_input_string, int* raw_input_string_index)
{

   // increment if at beginning of list
   if (*commands_current_index == 0)
   {

     (*commands_current_index)++;

   }

   // check if at end of command list (at prompt)
   if ((int)(*commands).size() == *commands_current_index)
     write(1, "\a", 1);
   else
   {

      // start at beginning of commands list and move to index
      // move index to the later command
      list<string>::iterator iter = (*commands).begin();

      advance(iter, *(commands_current_index));
      (*commands_current_index)++;
      
      for (int i = 0; i < *raw_input_string_index; i++)
      {
        write(1, "\b \b", 3);
      }
        
      string raw_command = *iter;
      string command = raw_command.substr(0, raw_command.size()-1); // source used: #3
      write(1, command.c_str(), command.size());
      strcpy(raw_input_string, command.c_str());
      *raw_input_string_index = command.size();

      fflush(stdout);

    }

}

void upHistory(list<string>* commands, int* commands_current_index, char *raw_input_string, int* raw_input_string_index)
{

   // check if any commands in history or if current spot in history is at top
  if (*commands_current_index == 0)
  {

    (*commands_current_index)++;

    write(1, "\a", 1);

  }
 
 // start at beginning of commands list and move to one before
  // the current index. then move the index to the earlier command
  list<string>::iterator iter = (*commands).begin();

  advance(iter, (*commands_current_index)-1);
  (*commands_current_index)--;


  for (int i = 0; i < *raw_input_string_index; i++)
  {
    write(1, "\b \b", 3);
  }

  string raw_command = *iter;
  string command = raw_command.substr(0, raw_command.size()-1); // source used: #3
  write(1, command.c_str(), command.size());
  strcpy(raw_input_string, command.c_str());
  *raw_input_string_index = command.size();

}

void commandHistory(char* raw_input, list<string>* commands, int* commands_current_index, char *raw_input_string, int* raw_input_string_index)
{
 
   // skip second escape character and check which arrow it is
  read(0, raw_input, 1);
  read(0, raw_input, 1);
      
  // Up Arrow => 0x1B 0x5B 0x41
  // Down Arrow => 0x1B 0x5B 0x42
  // source used: #1
  if (*raw_input == 0x41)
  {

    upHistory(commands, commands_current_index, raw_input_string, raw_input_string_index);

  }

  else if (*raw_input == 0x42)
  {

    downHistory(commands, commands_current_index, raw_input_string, raw_input_string_index);

  }


}

bool processInput(char* raw_input, list<string>* commands, int* commands_current_index,
                  char* raw_input_string, int* raw_input_string_index, vector<string>* tokens)
{
  if (*raw_input == 0x1B)
  {
  
    commandHistory(raw_input, commands, commands_current_index, raw_input_string, raw_input_string_index);
    return true;

  }

  else if (*raw_input == 0x7F)
  {
    if (*raw_input_string_index > 0)
    {
      write(1, "\b \b", 3);
      (*raw_input_string_index)--;
    }
    return true;
  }

  else
  {

    return writeInput(raw_input, commands, commands_current_index, raw_input_string, raw_input_string_index, tokens);

  }

}

void runCommand(char* raw_input_string, vector<string>* tokens)
{

  string cmd = (*tokens)[0];

  //cout << cmd << '\n';

  if(cmd == "ls")
  {

    cout << "exec ls\n";
    ls(tokens);

  }

  else if(cmd == "cd")
  {

    cout << "exec cd\n";
    cd(tokens);

  }

  else if(cmd == "pwd")
  {

    cout << "exec pwd";
    write(1, get_working_dir().c_str(), get_working_dir().size());
    write(1, "\n", 1);

  }

  else if(cmd == "ff")
  {

    cout << "exec ff";

  }

  else
    cout << "exec external command";

  raw_input_string = "";
  (*tokens).clear();

}

void addHistory(list<string>* commands, int* commands_current_index, char* raw_input_string, int* raw_input_string_index)
{

  string command(raw_input_string);

  // if commands history full, remove the oldest
  if ((*commands).size() == 10)
    (*commands).pop_front();

  // add command to end of history list
  (*commands).push_back(command);
  *commands_current_index = (*commands).size();

  // write(1, raw_input_string, raw_input_string_index);
  // reset index to zero for new input
  *raw_input_string_index = 0;

}

bool writeInput(char* raw_input, list<string>* commands, int* commands_current_index,
                char* raw_input_string, int* raw_input_string_index, vector<string>* tokens)
{

  // write input character out for user to see
  write(1, raw_input, 1);

  //copy char to input string 
  raw_input_string[(*raw_input_string_index)++] = *raw_input;
  
  // on enter, end string and display back to screen
  if (*raw_input == '\n')
  {

    raw_input_string[(*raw_input_string_index)] = '\0';
    *tokens = tokenize(raw_input_string);
  
    if (strcmp(raw_input_string, "exit\n") == 0)
      return false;

    else
    {
      addHistory(commands, commands_current_index, raw_input_string, raw_input_string_index);
      runCommand(raw_input_string, tokens);
      (*tokens).clear();
      raw_input_string = "";

    }
    
    string working_directory = get_working_dir();
    
    int working_directory_length = working_directory.length();
    
    writePrompt(&working_directory, &working_directory_length);

  }

  return true;

}

char readInput(char* raw_input)
{

  struct termios SavedTermAttributes;

  // set to noncanonical mode, treat input as characters
  // instead of lines ending on newline or EOF
  set_non_canonical_mode(STDIN_FILENO, &SavedTermAttributes);

  char tmp;
  read(0, &tmp, 1);
  return *raw_input = tmp;

}

string get_working_dir()
{
  //string working_directory = string(getcwd());
  char getcwd_buff[100];
  getcwd(getcwd_buff, 100);

  string working_directory = string(getcwd_buff);

  return working_directory;

}

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

void writePrompt(string* working_directory, int* working_directory_length)
{

  // set prompt
  //write(1, (*working_directory).c_str(), *working_directory_length);
  write(1, working_directory->c_str(), *working_directory_length);
  write(1, "%", 1);

}

int main(int argc, char* argv[])
{

  struct termios SavedTermAttributes;

  // set to noncanonical mode, treat input as characters
  // instead of lines ending on newline or EOF
  set_non_canonical_mode(STDIN_FILENO, &SavedTermAttributes);
  
  list<string> commands;

  vector<string> tokens;

  int commands_current_index = 0;

  char raw_input_string[128];
  int raw_input_string_index = 0;
  char raw_input = readInput(&raw_input);

  string working_directory = get_working_dir();
  
  int working_directory_length = working_directory.length();

  writePrompt(&working_directory, &working_directory_length);

  while(processInput(&raw_input, &commands, &commands_current_index, raw_input_string, &raw_input_string_index, &tokens))
  {

    raw_input = readInput(&raw_input);

  }

  return 0;

}


































/*int main()
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
}*/
