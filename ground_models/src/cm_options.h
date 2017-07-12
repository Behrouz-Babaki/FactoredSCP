#ifndef CM_OPTIONS_H
#define CM_OPTIONS_H

#pragma GCC diagnostic ignored "-Wunused-function"
#include <string>
using std::string;

extern struct Cm_opt PROG_OPT;

struct Cm_opt{
  int verbose;
  int depth_or;
  int depth_and;
  int branch_or;
  int branch_and;
  int capacity;
  int timeout;
  char* ac_file;
  char* lm_file;
  char* names_file;
};

void getCmOptions(int, char**);

#endif //CM_OPTIONS_H
