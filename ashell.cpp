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
#include <sys/wait.h>

#include <vector>
#include <sstream>

#include "ashell.h"

using namespace std;

vector<string> redirectDelimiter(char* raw_input_string)
{

  string str = raw_input_string;
  string buf;
  istringstream ss(str);

  vector<string> redirectTokens;

  while (getline(ss, buf, '|'))
  {

    redirectTokens.push_back(buf);

  }

  //cout << redirectTokens[0] << '\n';
  //cout << redirectTokens[1] << '\n';

  return redirectTokens;

} // tokenizes piped commands

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

void ff(pid_t* pid, int* status, vector<string>* tokens, const char* dirname, int level, int end) //source: http://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux
{
  DIR *dir;
  struct dirent *entry;

  string absStem;

  if(level == 0)
    absStem = dirname;

  const char* file = (*tokens)[1].c_str();

  // try to open directory
  if (!(dir = opendir(dirname)))
    return;
  if (!(entry = readdir(dir))) //if directory is readable
    return;

  while ((entry = readdir(dir)) != NULL)
  {
    if (entry->d_type == DT_DIR) 
    {
      char path[1024];

      int len = snprintf(path, sizeof(path)-1, "%s/%s", dirname, entry->d_name);

      path[len] = 0;

      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;

      // continue to search directory for next file
      ff(pid, status, tokens, path, level + 1, ++end);
      end--; 
    }

    else // if not a directory
    {
      char* fname = entry->d_name;
      if(!strcmp(fname, file))
      {
        cout << dirname << "/" << file << "\n";
      }

    }
  }

  closedir(dir);

  if (end == 0)
    exit(0);


}

void cd(vector<string>* tokens)
{

  const char* path;

  if((*tokens).size() == 1)
    path = getenv("HOME");

  else
    path = (*tokens)[1].c_str();

  string tmp = path;

  if(isADirectory(&tmp))
  {

    chdir(path);

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

vector<string> tokenize(const char* raw_input_string) //source: 4
{

  string str = raw_input_string;
  string buf;
  stringstream ss(str);

  vector<string> tokens;

  while (ss >> buf)
  {

    //if(str.find_first_of("\\"))
    //continue;

    //else
    tokens.push_back(buf);

  }

  return tokens;

}

void ls(int* status, vector<string>* tokens) //source used: #2,3
{

  DIR *dir;
  struct dirent *dp;

  string path;

  // if just ls
  if (tokens->size() == 1)
  {
    path = ".";
  }
  else // if directory specified
  {
    path = (*tokens)[1];
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

      if (tokens->size() == 2)
        path =  working_directory + "/" + (*tokens)[1] + "/" + filename;

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
    cout << "Not a directory\n";
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
    char* raw_input_string, int* raw_input_string_index, vector<string>* tokens, vector<string>* redirectTokens)
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

    return writeInput(raw_input, commands, commands_current_index, raw_input_string, raw_input_string_index, tokens, redirectTokens);

  }

}

void runCommand(char* raw_input_string, vector<vector<string>* >* all_tokens)
{
  // calculate pipes and children
  int num_children = all_tokens->size();
  int num_pipes = num_children - 1;

  cout << "num_pipes: " << num_pipes << endl;

  // create pipes and add to array
  int pipes[num_pipes][2];
  for (int i = 0; i < num_pipes; i++)
  {
    int fd[2];
    pipe(fd);
    cout << "fd[0]: " << fd[0] << endl;
    cout << "fd[1]: " << fd[1] << endl;
    pipes[i][0] = fd[0];
    pipes[i][1] = fd[1];
  }
    
  cout << "pipes[0][0]: " << pipes[0][0] << endl;
  cout << "pipes[0][1]: " << pipes[0][1] << endl;

  int fd2[2];
  pipe(fd2);


  // iterate through command and fork each
  int child_num = 0;
  vector<vector<string> *>::iterator itr;
  for (itr = all_tokens->begin(); itr != all_tokens->end(); itr++)
  {

    vector<string>* tokens = *itr;
    string cmd = (*tokens)[0];

    if(cmd == "ls")
    {
      cout << "child_num: " << child_num << endl;
      pid_t pid;
      pid = newChild();

      int status;
      if (pid == 0)
      {
        cout << "a" << endl;
        //dup2(1, pipes[0][1]);
        write(fd2[1], "hello\n", 6);

        child_num++;

        //ls(&status, tokens);
      }
      else
        waitpid(pid, &status, WCONTINUED | WUNTRACED);

    }

    else if(cmd == "cd")
    {

      cd(tokens);

    }

    else if(cmd == "pwd")
    {

      pid_t pid;
      pid = newChild();

      int status;
      if (pid == 0)
      {
        child_num++;
        pwd(&pid, &status);
      }
      else
        waitpid(pid, &status, WCONTINUED | WUNTRACED);

    }

    else if(cmd == "ff")
    {

      pid_t pid;
      pid = newChild();
      child_num++;
      int status;

      const char* dirname;

      if((*tokens).size() == 2)
        dirname = ".";
      else
        dirname = (*tokens)[2].c_str();

      if (pid == 0)
      {
        ff(&pid, &status, tokens, dirname, 0, 0);
      }
      else
        waitpid(pid, &status, WCONTINUED | WUNTRACED);
    }

    else
    {

      pid_t pid;
      pid = newChild();
      child_num++;
      int status;

      if(pid == 0)
      {
        cout << "b" << endl;
        //dup2(0, pipes[0][0]);
        dup2(0, fd2[0]);
        child_num++;

        char* argv[tokens->size() + 1];

        for (size_t i = 0; i < tokens->size(); i++)
        {
          argv[i] = (char *)malloc(sizeof((*tokens)[i]) + 1);
          strcpy(argv[i], (*tokens)[i].c_str());
        }

        argv[tokens->size()] = NULL;

        execvp(argv[0], argv);

        exit(0);

      }

      else
      {

        wait(&status);

      }

    }
  }
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
    char* raw_input_string, int* raw_input_string_index, vector<string>* tokens, vector<string>* redirectTokens)
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

      (*redirectTokens) = redirectDelimiter(raw_input_string);

      for(size_t i = 0; i < (*redirectTokens).size(); i++)
      {

        const char* tmp = (*redirectTokens)[i].c_str();
        (*tokens) = tokenize(tmp);

      }

      // for testing
      /*string str1("ls");
      string str2("grep");
      string str3("README");

      vector<string> cmd1;
      cmd1.push_back(str1);

      vector<string> cmd2;
      cmd2.push_back(str2);
      cmd2.push_back(str3);

      vector<vector<string>* > tokens2;
      tokens2.push_back(&cmd1);
      tokens2.push_back(&cmd2);*/

      // to use for old tokenize
      vector<vector<string>* > tokens2;
      tokens2.push_back(tokens);

      runCommand(raw_input_string, &tokens2);
      (*tokens).clear();

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

  vector<string> tokens, redirectTokens;

  //createPipe(); 

  int commands_current_index = 0;

  writePrompt();

  char raw_input_string[128];
  int raw_input_string_index = 0;
  char raw_input = readInput(&raw_input);

  while(processInput(&raw_input, &commands, &commands_current_index, raw_input_string, &raw_input_string_index, &tokens, &redirectTokens))
  {

    raw_input = readInput(&raw_input);

  }

  return 0;
}
