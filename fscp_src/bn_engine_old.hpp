#ifndef BN_ENGINE_H
#define BN_ENGINE_H

#include <iostream>
#include <boost/dynamic_bitset.hpp>

using std::cerr;
using boost::dynamic_bitset;

#define CALL_CLIENT(client, url, method, params, result) do { \
    try { \
      client.call(url, method, params, result); \
    } \
    catch(exception& err) { \
      cerr << err.what() << endl; \
      this->terminate(); \
      exit(1); \
    } \
} while (0)

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <functional>

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/client_simple.hpp>

using std::vector;
using std::string;
using std::pair;
using std::unordered_map;
using std::pair;

// from http://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
class int_vector_hasher {
public:
  std::size_t operator()(std::vector< std::pair<int,int> > const& vec) const {
    std::size_t seed = vec.size();
    for(auto& i : vec) {
      // hack: only the values will contribute to the hash...
      seed ^= i.second + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

class BNEngine {
 public:
  BNEngine (string, string, int cache_level=1, int verbosity=0);
  ~BNEngine();
  
 public:
  int verbose;
  
  // get names:ids of random variables 
  const unordered_map<string,int> get_var_ids();
  // get domain values of random variables
  const vector<vector<int> >* get_val_ids();
  
  int num_vars();
  size_t domain_size(int);
  
  // get value,prob pairs of all values of variable_index given the evidence
  vector< pair<int,double> > var_partials(const vector< pair< int, int > >& evidence, int variable);
  // get probability of evidence
  double pr(const vector< pair< int, int > >& evidence);
  double pr(const vector< pair< int, int > >& evidence, int next_var, int next_val); // more efficient then the above
  
 private:
  void initialize_server(void);
  void terminate(void);
  void get_value_map(void);
  
  double update_evidence(const vector< pair< int, int > >& new_evidence);
  vector<double> _var_partials(const vector< pair< int, int > >& new_evidence, int variable); // raw, uncached, xmlrpc

  xmlrpc_c::clientSimple bn_client;
  string serverUrl;
  int port;
  int cache_level;
  
  // mappings
  vector<vector<int>  > bn_val_ids;
  vector<unordered_map<int,int> > bn_val_map; // per var, from domain value to index of the value (according to the BN engine)
                                    // inverse of bn_val_ids
                                    
  vector< pair<int,int> > ac_evidence; // evidence that the AC currently has: (varID, val), SORTED
  unordered_map<vector< pair<int,int> >, vector<double>, int_vector_hasher> cache_partials; // caching results in _var_partials
 
};

#endif // BN_ENGINE_H
