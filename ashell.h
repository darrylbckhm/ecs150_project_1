#ifndef ASHELL_H
#define ASHELL_H

#include <string>
#include <list>

using namespace std;

pid_t newChild();
void pwd(pid_t* pid, int* status, vector<string>* tokens);
void ff(pid_t* pid, int* status, vector<string>* tokens, int level);
bool isADirectory(string* path);
void cd(vector<string>* tokens);
void runCommand(char* raw_input_string, vector<vector<string>* >* tokens, bool redirect_output, string output_file, bool redirect_intput, string input_file);
vector<string> tokenize(const char* raw_input_string);
void ls(int* status, vector<string>* tokens);
void downHistory(list<string>* commands, int* commands_current_index, char *raw_input_string, int* raw_input_string_index);
void upHistory(list<string>* commands, int* commands_current_index, char* raw_input_string, int* raw_input_string_index);
void commandHistory(char* raw_input, list<string>* commands, int* commands_current_index, char *raw_input_string, int* raw_input_string_index);
bool processInput(char* raw_input, list<string>* commands, int* commands_current_index, char* raw_input_string, int* raw_input_string_index, vector<string>* tokens, vector<string>* redirectTokens);
bool writeInput(char* raw_input, list<string>* commands, int* commands_current_index, char* raw_input_string, int* raw_input_string_index,  vector<string>* tokens, vector<string>* redirectTokens);
char readInput(char* raw_input);
string get_working_dir();
void reset_canonical_mode(int fd, struct termios *savedattributes);
void set_non_canonical_mode(int fd, struct termios *savedattributes);
void writePrompt();
int main(int argc, char* argv[]);

#endif
