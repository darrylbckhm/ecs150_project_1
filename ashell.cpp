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
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <vector>
#include <sstream>

#include "ashell.h"

using namespace std;

string formatString(char* raw_input_string)
{

  string str = raw_input_string;
  string buf;
  istringstream ss(str);

  //loop through raw input to add spaces where necessary
  //all code from here to the second while loop is of similar format

  size_t pos = str.find_first_of("|"); //index of first pipe character found

  while (pos != string::npos)
  {

    if(str[pos - 1] != ' ') //if the character before the pipe isn't a space
    {

      str.insert(pos, " "); //add a space before the pipe
      ++pos; //increment back to current pipe index

    }

    if(str[pos + 1] != ' ') //if the character after the pipe isn't a space
    {

      str.insert(pos + 1, " "); //add a space after the pipe
      pos = str.find_first_of("|", pos + 2); //search string for another pipe
      continue;
 
    }

    pos = str.find_first_of("|", pos + 1); //if there are spaces, search for another pipe

  }

  pos = str.find_first_of("<");

  while (pos != string::npos)
  {

    if(str[pos - 1] != ' ')
    {

      str.insert(pos, " ");
      ++pos;

    }

    if(str[pos + 1] != ' ')
    {

      str.insert(pos + 1, " ");
      pos = str.find_first_of("<", pos + 2);

    }

    pos = str.find_first_of('<', pos + 1);

  }

  pos = str.find_first_of(">");

  while (pos != string::npos)
  {

    if(str[pos - 1] != ' ')
    {

      str.insert(pos, " ");
      ++pos;

    }

    if(str[pos + 1] != ' ')
    {

      str.insert(pos + 1, " ");
      pos = str.find_first_of(">", pos + 2);

    }

    pos = str.find_first_of('>', pos + 1);

  }

  pos = str.find_first_of("\\");

  while (pos != string::npos)
  {

    if(str[pos - 1] != ' ')
    {

      str.insert(pos, " ");
      ++pos;

    }

    if(str[pos + 1] != ' ')
    {

      str.insert(pos + 1, " ");
      pos = str.find_first_of("\\", pos + 2);

    }

    pos = str.find_first_of('\\', pos + 1);

  }

  return str;

} // formats string to be parsed by spaces 

void createPipe() //http://tldp.org/LDP/lpg/node11.html
{

}

pid_t newChild()
{

  pid_t pid = fork();
  return pid;

}

void pwd(pid_t* pid, int* status)
{

  write(1, get_working_dir().c_str(), get_working_dir().size());
  write(1, "\n", 1);

}

void ff(pid_t* pid, int* status, vector<string> cmdStr, const char* dirname, int level, int end) //source: http://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux
{
  DIR *dir;
  struct dirent *entry;

  string absStem;

  if(level == 0)
    absStem = dirname;

  const char* file = cmdStr[1].c_str();

  // try to open directory
  if (!(dir = opendir(dirname)))
  {

    write(1, "Failed to open directory.\n", 26);
    return;

  }

  if (!(entry = readdir(dir))) //if directory is readable
  {

    write(1, "Failed to open directory.\n", 26);
    return;

  }

  while ((entry = readdir(dir)) != NULL)
  {
    if (entry->d_type == DT_DIR) 
    {
      char path[1024];

      int len = strlen(dirname) + 1 + strlen(entry->d_name);
      string s1(dirname);
      string s2(entry->d_name);
      string tmp = s1 + "/" + s2;
      strcpy(path, tmp.c_str());

      path[len] = 0;

      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;

      // continue to search directory for next file
      ff(pid, status, cmdStr, path, level + 1, ++end);
      end--; 
    }

    else // if not a directory
    {
      char* fname = entry->d_name;
      if(!strcmp(fname, file))
      {
        write(1, dirname, sizeof(dirname));
        write(1, "/", 1);
        write(1, file, sizeof(file));
        write(1, "\n", 1);
      }

    }
  }

  closedir(dir);

  if (end == 0)
    exit(0);


}

void cd(vector<string> cmdStr)
{

  const char* path;

  if(cmdStr.size() == 1)
    path = getenv("HOME");

  else
    path = cmdStr[1].c_str();

  string tmp = path;

  if(isADirectory(&tmp))
  {

    chdir(path);

  }

  else
  {

    write(1,"Error changing directory.\r\n", 27);
  }

}

bool isADirectory(string* path)
{

  const char* tmp = (*path).c_str();
  struct stat path_stat;
  stat(tmp, &path_stat);
  return S_ISDIR(path_stat.st_mode);

}

vector<vector<string> >* tokenize(string formattedString, vector<vector<string> >* all_tokens, vector<string>* redirectTokens) //source: 4
{

  //vector<vector<string> > all_tokens;

  vector<string> cmdTokens;
    
  //size_t pos = 0;

  //const char* str = formattedString.c_str();
  //int size = strlen(str); // variable for the size of formattedString
  char cstr[1024]; // char array to hold formattedString

  string tmp = formattedString.substr(0, formattedString.size() - 1);

  strcpy(cstr, tmp.c_str()); // copy bytes from str to the new array

  int escape = 0;
  int input = 0;
  int output = 0; // true when escape character is seen
  int numPipes = 0;
  int numRedir = 0;
  char* token;
  (*redirectTokens)[0] = "";
  (*redirectTokens)[1] = "";

  token = strtok(cstr, " ");

  while(token != NULL)
  {

    string tmp = token;

    if(tmp.compare(">") == 0) // if a backslash is found, set escape to true
    {

      ++numRedir;
      output = 1;

    }

    else if(tmp.compare("<") == 0) // if a backslash is found, set escape to true
    {

      ++numRedir;
      input = 1;

    }

    else if(tmp.compare("\\") == 0) // if a backslash is found, set escape to true
    {

      escape = 1;

    }

    else if(input)
    {

      (*redirectTokens)[0] = tmp;
      input = 0;

    }

    else if(output)
    {

      (*redirectTokens)[1] = tmp;
      output = 0;

    }

    else if(escape) //if escape is true add a space and the current token to the last element of cmdTokens
    {

      cmdTokens.back().append(" ");
      cmdTokens.back().append(tmp);
      cmdTokens.back().append("\0");
      escape = 0; //set escape to false for next token

    }

    else if(tmp.compare("|") == 0) //if there is a pipe add this command vector to the vector of vectors
    {

      ++numPipes;
      all_tokens->push_back(cmdTokens);
      cmdTokens.clear(); //clear vector for next command string

    }

    else //otherwise, add this token to the current command vector
    {

      cmdTokens.push_back(tmp);
      cmdTokens.back().append("\0");

    }

    token = strtok(NULL, " ");

  }

  all_tokens->push_back(cmdTokens);
  cmdTokens.clear();

  //for(size_t i = 0; i < all_tokens->size(); i++)
    //for(size_t j = 0; j < (*all_tokens)[i].size(); j++)
      //cout << (*all_tokens)[i][j] << endl;
  
  return all_tokens;

}

void ls(int* status, vector<string> cmdStr) //source used: #2,3
{

  DIR *dir;
  struct dirent *dp;

  string path;

  // if just ls
  if (cmdStr.size() == 1)
  {
    path = ".";
  }
  else // if directory specified
  {
    path = cmdStr[1];
  }

  // checks if specified argument is a file or nonexistent
  if(isADirectory(&path))
  {
    dir = opendir(path.c_str());

    // reads dir
    while((dp = readdir(dir)) != NULL)
    {
      // create full pathname
      string filename = dp->d_name;
      string working_directory = get_working_dir();

      string path = working_directory + "/" + filename;

      if (cmdStr.size() == 2)
        path =  working_directory + "/" + cmdStr[1] + "/" + filename;

      // grab permissions info and fill in permissions
      struct stat file_stats;
      stat(path.c_str(), &file_stats);
      mode_t mode = file_stats.st_mode;
      char permissions[11] = "----------";

      if (S_ISDIR(mode)) permissions[0] = 'd';

      // using source: #4
      if (mode & S_IRUSR) permissions[1] = 'r';
      if (mode & S_IWUSR) permissions[2] = 'w';
      if (mode & S_IXUSR) permissions[3] = 'x';

      if (mode & S_IRGRP) permissions[4] = 'r';
      if (mode & S_IWGRP) permissions[5] = 'w';
      if (mode & S_IXGRP) permissions[6] = 'x';

      if (mode & S_IROTH) permissions[7] = 'r';
      if (mode & S_IWOTH) permissions[8] = 'w';
      if (mode & S_IXOTH) permissions[9] = 'x';

      permissions[10] = '\0';
      write(1, permissions, strlen(permissions));
      write(1, " ", 1);
      write(1, filename.c_str(), filename.length());
      write(1, "\n", 1);
    }
  }
  else
  {
    write(1, "Failed to open directory ", 24);
    write(1, " \"", 1);
    write(1, path.c_str(), sizeof(path));
    write(1, "\n", 1);
  }

  exit(0);
}

void downHistory(list<string>* commands, int* commands_current_index, char *raw_input_string, int* raw_input_string_index)
{
  if (commands->size() == 0)
  {
    write(1, "\a", 1);
    return;
  }

  // increment if at beginning of list
  if (*commands_current_index == 0)
  {

    (*commands_current_index)++;

  }

  // check if at end of command list (at prompt)
  if ((int)(*commands).size() == *commands_current_index)
  {
    write(1, "\a", 1);
    for (int i = 0; i < *raw_input_string_index; i++)
    {
       write(1, "\b \b", 3);
    }
    *raw_input_string_index = 0;
  }
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

  // nothing in commands, exit and do nothing
  if (commands->size() == 0)
  {
    write(1, "\a", 1);
    return;
  }

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
    char* raw_input_string, int* raw_input_string_index, vector<vector<string> >* all_tokens, vector<string>* redirectTokens)
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
    else
    {
      write(1, "\a", 1);
    }
    return true;
  }

  else
  {

    return writeInput(raw_input, commands, commands_current_index, raw_input_string, raw_input_string_index, all_tokens, redirectTokens);

  }

}

void runCommand(char* raw_input_string, vector<vector<string> >* all_tokens, vector<string>* redirectTokens)
{

  // calculate pipes and children
  int num_children = all_tokens->size();

  int num_pipes = num_children - 1;

  // create pipes and add to array
  int pipes[num_pipes][2];

  for (int i = 0; i < num_pipes; i++) //for all pipes created
  {

    int fd[2]; // create int array for file descriptors

    pipe(fd); // open pipe

    pipes[i][0] = fd[0]; // hook up file descriptor output to pipe

    pipes[i][1] = fd[1]; // hook up file descriptor input to pipe

  }

  // iterate through command and fork each
  int child_num = 0;

  vector<vector<string> >::iterator itr;

  dup2(STDOUT_FILENO,3);

  for (itr = all_tokens->begin(); itr != all_tokens->end(); itr++) //for all commands entered
  {

    vector<string> tokens = *itr;

    string cmd = tokens[0]; 

    const char* input_file = (*redirectTokens)[0].c_str();
    const char* output_file = (*redirectTokens)[1].c_str();

    pid_t pid;
    pid = newChild();

    int status;

    // if child
    if (pid == 0)
    {

      // if more than one pipe
      if (num_pipes > 0)
      {

        // if first command, hook up to first pipe
        if (child_num == 0)
        {

          if (strcmp(input_file, ""))
          {

            int fd = open(input_file, O_RDONLY); //open and return file descriptor for given input file

            dup2(fd, 0); // creates a copy of the file descriptor fd and hooks it to stdin

            close(fd); //

          }

          dup2(pipes[0][1], 1);

        }
        // else hook up to pipe before and after
        else
        {

          dup2(pipes[child_num - 1][0], 0);

          if (child_num != num_children - 1)
          {

            dup2(pipes[child_num][1], 1);
          }

          else
          {
            if (strcmp(output_file, "")) //if redirect input 
            {

              mode_t mode = S_IRUSR | S_IWUSR;
              int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, mode );

              dup2(fd, 1);

              close(fd);


            }

          }
        }

      }

      else
      {

        if (strcmp(input_file, "")) //if redirect output
        {

          int fd = open(input_file, O_RDONLY);

          dup2(fd, 0);

          close(fd);

        }

        if (strcmp(output_file, ""))
        {

          mode_t mode = S_IRUSR | S_IWUSR;
          int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, mode );

          dup2(fd, 1);

          close(fd);

        }

      }


      if(cmd == "ls")
      {

        ls(&status, tokens);

      }

      else if(cmd == "cd")
      {

        cd(tokens);

      }

      else if(cmd == "pwd")
      {

        pwd(&pid, &status);

      }

      else if(cmd == "ff")
      {

        const char* dirname;

        if(tokens.size() == 2)
          dirname = ".";
        else
          dirname = tokens[2].c_str();

        ff(&pid, &status, tokens, dirname, 0, 0);

      }

      else
      {

        char* argv[tokens.size() + 1];

        /*strcpy could be problematic*/

        for (size_t i = 0; i < tokens.size(); i++)
        {
          argv[i] = (char *)malloc(sizeof(tokens[i]) + 1);
          strcpy(argv[i], tokens[i].c_str());
          //argv[i] = (char)malloc(sizeof(tokens[i]) + 1);
          //const char* tmp = tokens[i].c_str();
          //size_t size = sizeof(char) * strlen(tmp);
          //memcpy(argv[i], tmp, size);

        }

        argv[tokens.size()] = NULL;

        for (int i = 0; i < num_pipes; i++)
        {
          close(pipes[i][0]);
          close(pipes[i][1]);
        }

        execvp(argv[0], argv);

        exit(0);

      }

    }

    else
    {

      child_num++;

    }

  }

  for (int i = 0; i < num_pipes; i++)
  {

    close(pipes[i][0]);

    close(pipes[i][1]);

  }

  for (int i = 0; i < num_children; i++)
  {
    int status;
    wait(&status);
  }

              dup2(3, STDOUT_FILENO);
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
    char* raw_input_string, int* raw_input_string_index, vector<vector<string> >* all_tokens, vector<string>* redirectTokens)
{

  // write input character out for user to see
  write(1, raw_input, 1);

  //copy char to input string 
  raw_input_string[(*raw_input_string_index)++] = *raw_input;

  // on enter, end string and display back to screen
  if (*raw_input == '\n')
  {

    raw_input_string[(*raw_input_string_index)] = '\0';
    //*tokens = tokenize(raw_input_string);

    if (strcmp(raw_input_string, "exit\n") == 0)
      return false;

    else
    {

      addHistory(commands, commands_current_index, raw_input_string, raw_input_string_index);

      all_tokens = tokenize(formatString(raw_input_string), all_tokens, redirectTokens);
      
      runCommand(raw_input_string, all_tokens, redirectTokens);
      (*all_tokens).clear();

    }

    writePrompt();

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
  if(tmp == 0x04)
    exit(0);
  else
  {

    if(tmp == 0x7C)
      //createPipe(); 

      return *raw_input = tmp;
  }

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

    write(2, "Not a terminal.\n", 16);
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

void writePrompt()
{

  // set prompt
  string working_directory = get_working_dir();

  size_t last_slash = working_directory.find_last_of("/");
  string last_folder = working_directory.substr(last_slash, working_directory.length()-1);


  if (working_directory.length() > 16)
  {
    working_directory = "/..." + last_folder;
  }

  write(1, working_directory.c_str(), working_directory.length());
  write(1, "%", 1);

}

int main(int argc, char* argv[])
{

  struct termios SavedTermAttributes;

  // set to noncanonical mode, treat input as characters
  // instead of lines ending on newline or EOF
  set_non_canonical_mode(STDIN_FILENO, &SavedTermAttributes);

  list<string> commands;

  vector<vector<string> > all_tokens;

  //bool redirect_output, redirect_input;
  string output_file, input_file;

  vector<string> redirectTokens;

  redirectTokens.push_back("");
  redirectTokens.push_back("");

  //createPipe(); 

  int commands_current_index = 0;

  writePrompt();

  char raw_input_string[128];
  int raw_input_string_index = 0;
  char raw_input = readInput(&raw_input);

  while(processInput(&raw_input, &commands, &commands_current_index, raw_input_string, &raw_input_string_index, &all_tokens, &redirectTokens))
  {

    raw_input = readInput(&raw_input);

  }

  return 0;

}
