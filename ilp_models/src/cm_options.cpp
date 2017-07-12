#include "cm_options.h"
#include <stdlib.h>
#include <argp.h>

#include <string>
#include <sstream>
#include <iostream>
using std::string;
using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;

struct Cm_opt PROG_OPT;
const char *argp_program_version =
  "FSCP solver 0.1";
const char *argp_program_bug_address =
  "<Behrouz.Babaki@cs.kuleuven.be>";
     
/* Program documentation. */
static char doc[] =
  "FSCP solver -- a program for solving factored SCPs";
     
/* A description of the arguments we accept. */
static char args_doc[] = "";
     
/* The options we understand. */
static struct argp_option options[] = {
  {"verbose",  'v', "NUM",     0,  "Verbosity level" },
  {"depth_or", 'o', "NUM", 0,    "depth to compute bound for OR nodes for (default: 6)"},
  {"depth_and",'p', "NUM", 0,    "depth to compute bound for AND nodes for (default: 2) (-1=disable)"},
  {"branch_or", 'b', "NUM", 0, "value branching over OR vars: 0=default, -1=min, 1=max"},
  {"branch_and", 'c', "NUM", 0, "value branching over AND vars: 0=default, -1=min, 1=max"},
  {"acfile",   'a', "FILE", 0,
   "Read the arithmetic circuit from FILE"},
  {"lmfile",   'l', "FILE", 0,
   "Read the literal map from FILE"},
  {"namesfile", 'n', "FILE", 0,
   "Write number, names, and values of network variables to FILE"},
  {"datafile", 'd', "FILE", 0,
    "Read the data from FILE"},    
   {"capacity", 't', "NUM", 0, "capacity of knapsack"},
   {"timeout" , 'u', "TIME", 0, "set the timeout to TIME (seconds)"},
  { 0 }
};
     
/* Used by main to communicate with parse_opt. */
struct arguments
{
  int verbose;
  int depth_or;
  int depth_and;
  int branch_or;
  int branch_and;
  int capacity;
  int timeout;
  char *ac_file;
  char *lm_file;
  char* names_file;
  char* data_file;
};


/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
   *          know is a pointer to our arguments structure. */
  struct arguments *arguments = static_cast<struct arguments*> (state->input);
  switch (key)
    {
    case 'v':
      arguments->verbose = atoi(arg);
      break;
    case 'o':
      arguments->depth_or = atoi(arg);
      break;
    case 'p':
      arguments->depth_and = atoi(arg);
      break;
    case 'b':
      arguments->branch_or = atoi(arg);
      break;
    case 'c':
      arguments->branch_and = atoi(arg);
      break;
    case 't':
      arguments->capacity = atoi(arg);
      break;
    case 'a':
      arguments->ac_file = arg;
      break;
    case 'l':
      arguments->lm_file = arg;
      break;
    case 'n':
      arguments->names_file = arg;
      break;
    case 'd':
      arguments->data_file = arg;
      break;
    case 'u':
      arguments->timeout = atoi(arg);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void getCmOptions(int argc, char** argv) {
  
  struct arguments _arguments;

  /* arguments default values. */
  _arguments.verbose = 0;
  _arguments.depth_or = 6;
  _arguments.depth_and = 2;
  _arguments.branch_or = 0;
  _arguments.branch_and = 0;
  _arguments.capacity = -1;
  _arguments.timeout = -1;
  _arguments.ac_file = NULL;
  _arguments.lm_file = NULL;
  _arguments.names_file = NULL;
  _arguments.data_file = NULL;

  argp_parse (&argp, argc, argv, 0, 0, &_arguments);

  if (_arguments.ac_file == NULL || _arguments.lm_file == NULL) {
    cerr << "error: input files for arithmetic circuit and literal map should be specified." << endl;
    exit(1);
  }
  
  PROG_OPT.verbose = _arguments.verbose;
  PROG_OPT.depth_or = _arguments.depth_or;
  PROG_OPT.depth_and = _arguments.depth_and;
  PROG_OPT.branch_or = _arguments.branch_or;
  PROG_OPT.branch_and = _arguments.branch_and;
  PROG_OPT.capacity = _arguments.capacity;
  PROG_OPT.ac_file = _arguments.ac_file;
  PROG_OPT.lm_file = _arguments.lm_file;
  PROG_OPT.names_file = _arguments.names_file;
  PROG_OPT.data_file = _arguments.data_file;
  PROG_OPT.timeout = _arguments.timeout;
  
}

     
     
     
