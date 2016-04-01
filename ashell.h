#ifndef ASHELL_H
#define ASHELL_H

#include <string>
#include <list>

using namespace std;

void ls();
void downHistory(list<string>* commands, int* commands_current_index);
void upHistory(list<string>* commands, int* commands_current_index);
void commandHistory(char* raw_input, list<string>* commands, int* commands_current_index);
bool processInput(char* raw_input, list<string>* commands, int* commands_current_index, char* raw_input_string, int* raw_input_string_index);
bool writeInput(char* raw_input, list<string>* commands, int* commands_current_index, char* raw_input_string, int* raw_input_string_index);
char readInput(char* raw_input);
string get_working_dir();
void reset_canonical_mode(int fd, struct termios *savedattributes);
void set_non_canonical_mode(int fd, struct termios *savedattributes);
void writePrompt(string* working_directory, int* working_directory_length);
int main(int argc, char* argv[]);

#endif
