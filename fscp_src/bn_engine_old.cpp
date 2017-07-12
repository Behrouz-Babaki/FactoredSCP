#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <utility>
#include <algorithm>
#include <cassert>

#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/client_simple.hpp>

#include "bn_engine.h"

using std::string;
using std::vector;
using std::cerr;
using std::cout;
using std::endl;
using std::stringstream;
using std::exception;
using std::pair;
using std::make_pair;
using std::sort;

void BNEngine::initialize_server(void) {
  /* (http://beej.us/guide/bgnet/output/html/multipage/ipstructsdata.html) */

  int sockfd;
  struct addrinfo hints;
  struct addrinfo *res;  // will point to the results

  this->port = -1;
  while(this->port < 1024) {
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    int status;
    if ((status = getaddrinfo(NULL, "0", &hints, &res)) != 0) {
      fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
      exit(1);
    }
 
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0){
      perror("socket");
      exit(1);
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0){
      perror("bind");
      exit(1);
    }
  
    if (getsockname(sockfd, (struct sockaddr *)res->ai_addr, &res->ai_addrlen) < 0){
      perror("getsocketname");
      exit(1);
    }

    this-> port = ((struct sockaddr_in*)res->ai_addr)->sin_port;
    if (this->port < 1024)
      close(sockfd);
  }

  stringstream ss;
  ss << "http://localhost:" << port
     << "/xmlrpc";
  this->serverUrl = ss.str();

  ss.str("");
  ss << "java -jar bns.jar --port "
     << port << " &";
  if (verbose >= 3) {
    cout << "initializing BN engine on port " << port << endl;
    cout << ss.str() << endl;
  }

  int retcode = system(ss.str().c_str());
  if (retcode != 0) {
    cerr << "command returned with value " << retcode << endl;
    exit(1);
  }

  close(sockfd);
}

BNEngine::BNEngine(string ac_filename, 
		   string lm_filename,
		   int c_level,
                   int verbosity
		  ) : verbose(verbosity), cache_level(c_level)
{

  this->initialize_server();
  if (verbose >= 3)
    cout << this->serverUrl << endl;

  // initialize the engine
  xmlrpc_c::paramList params(2);
  params.add(xmlrpc_c::value_string(ac_filename));
  params.add(xmlrpc_c::value_string(lm_filename));
  xmlrpc_c::value init_result;
  bool succeed = false;
  
  // Give server some time to set up
  while (!succeed) {
    try {
      bn_client.call(serverUrl, "Engine.load_ac", params, &init_result);
      succeed = true;
      cout << endl;
    }
    catch(exception& err) {
      usleep(1000);
    }
  }
  
  // Store mapping from value indices to domain values
  get_value_map();  
}

void BNEngine::get_value_map(){
  
  xmlrpc_c::paramList val_params;
  xmlrpc_c::value val_result;
  CALL_CLIENT(bn_client, serverUrl, "Engine.get_val_ids", val_params, &val_result);
  vector<xmlrpc_c::value> val_array = 
  xmlrpc_c::value_array (val_result).cvalue();
  for (auto inner_arr : val_array) {
    vector<xmlrpc_c::value> inner_array = 
    xmlrpc_c::value_array (inner_arr).cvalue();
    vector<int> inner_val_ids;
    unordered_map<int,int> inner_val_map;
    int j=0;
    for (auto entry : inner_array) {
      int const valId ((xmlrpc_c::value_int(entry)));
      inner_val_ids.push_back(valId); // at position j
      inner_val_map[valId] = j++;
    }
    bn_val_ids.push_back(inner_val_ids);
    bn_val_map.push_back(inner_val_map);
  }
}

void BNEngine::terminate(){
  if (verbose >= 3)
    cout << "terminating BN engine" << endl;
  stringstream ss;
  ss << "fuser --kill "
     << port << "/tcp ";

  int retcode = system(ss.str().c_str());
  if (retcode != 0) {
    cerr << "command returned with value " << retcode << endl;
    exit(1);
  }
}

int BNEngine::num_vars()
{
  xmlrpc_c::paramList numvars_params;
  xmlrpc_c::value nv_result;
  CALL_CLIENT(bn_client, serverUrl, "Engine.get_num_vars", numvars_params, &nv_result);
  return (int)(xmlrpc_c::value_int(nv_result));
}


const unordered_map< string, int > BNEngine::get_var_ids()
{
  unordered_map<string,int> bn_var_names;
  
  xmlrpc_c::paramList var_params;
  xmlrpc_c::value var_result;
  CALL_CLIENT(bn_client, serverUrl, "Engine.get_var_ids", var_params, &var_result);
  vector<xmlrpc_c::value> var_array = 
  xmlrpc_c::value_array (var_result).cvalue();
  int i=0;
  for (auto entry : var_array) {
    string varName ((xmlrpc_c::value_string(entry)));
    bn_var_names[varName] = i++; // string to id
  }
  
  return bn_var_names;
}

const vector< vector< int > >* BNEngine::get_val_ids()
{
  // pre-fetched in get_value_map();
  return &this->bn_val_ids;
}

size_t BNEngine::domain_size(int var_id)
{
  // pre-fetched in get_value_map();
  return this->bn_val_ids[var_id].size();
}

BNEngine::~BNEngine(){
  this->terminate();  
}


double BNEngine::pr(const vector<pair<int, int> >& evidence){
  if (verbose >= 5) {
    cout << "pr()\n";
    cout << "Evidence:\n";
    for (size_t i=0; i!=evidence.size(); i++)
      cout << " " << evidence[i].first << "," << evidence[i].second;
    cout << "\n";
  }
  
  //  // query inference engine
  //   xmlrpc_c::paramList pr_params(3);
  //   pr_params.addc(commit_vars);
  //   pr_params.addc(commit_vals);
  //   pr_params.addc(retract_vars);
  //   
  //   xmlrpc_c::value pr_result;
  //   CALL_CLIENT(bn_client, serverUrl, "Engine.pr", pr_params, &pr_result);
  //   return xmlrpc_c::value_double(pr_result);
  
  if (evidence.size() == 0) {
    // empty, so probably 1 but use cache anyway (can't do pop_back trick)
    const vector<double>& probs = _var_partials(evidence, 0);
    double pr = 0.0;
    for (auto& v: probs)
      pr += v;
    return pr;
  }
  
  vector< pair<int,int> > par_evidence(evidence);
  par_evidence.pop_back(); // remove parent from par_evidence
  return pr(par_evidence, evidence.back().first, evidence.back().second); // evidence, par_var, par_val
}

inline
double BNEngine::pr(const vector<pair<int, int> >& evidence, int par_var, int par_val) {
  // get all probs of parent and extract the one of this child
  const vector<double>& probs = _var_partials(evidence, par_var);
  return probs[bn_val_map[par_var][par_val]];
}

vector<pair<int, double> > BNEngine::var_partials(const vector<pair<int, int> >& evidence, int variable){
  if (verbose >= 5) {
    cout << "var_partials for var idx: " << variable << "\n";
    cout << "Evidence:\n";
    for (size_t i=0; i!=evidence.size(); i++)
      cout << " " << evidence[i].first << "," << evidence[i].second;
    cout << "\n";
  }
  
  const vector<double>& probs = _var_partials(evidence, variable);
  
  // convert to (value,prob) instead of (id,prob)
  const vector<int>& order = this->bn_val_ids[variable];
  if (order.size() != probs.size())
    cout << "var="<<variable<<" order.size="<<order.size()<<" != "<<probs.size()<<" probs.size()\n";
  assert(order.size() == probs.size());
  vector< pair<int,double> > ret(probs.size());
  for (size_t i=0; i!=order.size(); i++) {
    ret[i].first = order[i];
    ret[i].second = probs[i];
  }
  return ret;
}


// find the commit and retract var/vals
inline
vector<double> BNEngine::_var_partials(const vector< pair< int, int > >& evidence, int variable){
  
  vector<double>& lookup = cache_partials[evidence];
  if (lookup.size() == 0) {
    // cache miss, create
    
    vector<int> commit_vars, commit_vals, retract_vars;
    // New: no need to retract+commit same var: commit overwites previous commit
    // (and remember: evidence is always in exactly the same order: varBNorder)
    
    // overwrite new values of already committed vars
    size_t sizeboth = std::min(evidence.size(), ac_evidence.size());
    for (size_t i=0; i!=sizeboth; i++) {
      if (evidence[i].second != ac_evidence[i].second) {
        commit_vars.push_back(evidence[i].first);
        commit_vals.push_back(bn_val_map[evidence[i].first][evidence[i].second]);
        if (verbose >= 5)
          cout << "commit var="<<evidence[i].first<<" val_dom="<<evidence[i].second<<" val_idx="<<bn_val_map[evidence[i].first][evidence[i].second]<<"\n";
      }
    }
    // add new values of new vars
    for (size_t i=sizeboth; i!=evidence.size(); i++) {
      commit_vars.push_back(evidence[i].first);
      commit_vals.push_back(bn_val_map[evidence[i].first][evidence[i].second]);
      if (verbose >= 5)
        cout << "commit var="<<evidence[i].first<<" val_dom="<<evidence[i].second<<" val_idx="<<bn_val_map[evidence[i].first][evidence[i].second]<<"\n";
    }
    // retract old vars
    for (size_t i=sizeboth; i!=ac_evidence.size(); i++) {
      retract_vars.push_back(ac_evidence[i].first);
      if (verbose >= 5)
        cout << "retract var="<<ac_evidence[i].first<<" val_dom="<<ac_evidence[i].second<<"\n";
    }
    
    // store new evidence
    ac_evidence = evidence;
    
    
    // query inference engine
    xmlrpc_c::paramList vp_params(4);
    vp_params.addc(commit_vars);
    vp_params.addc(commit_vals);
    vp_params.addc(retract_vars);
    vp_params.addc(variable);
    xmlrpc_c::value vp_result;
    CALL_CLIENT(bn_client, serverUrl, "Engine.varPartials", vp_params, &vp_result);
    vector<xmlrpc_c::value> vp_array = xmlrpc_c::value_array (vp_result).vectorValueValue();
    
    // convert and return
    lookup.resize(vp_array.size());
    for (size_t i=0; i!=vp_array.size(); i++)
      lookup[i] = xmlrpc_c::value_double(vp_array[i]);
  } // end of cache miss, create
  
  return lookup;
}