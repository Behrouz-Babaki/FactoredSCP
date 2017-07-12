#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <utility>

using std::unordered_map;
using std::string;
using std::vector;
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
    // get names:ids of random variables
    virtual const unordered_map<string,int> get_var_ids() = 0;
    // get domain values of random variables
    virtual const vector<vector<int> >* get_val_ids() = 0;

    virtual int num_vars() = 0;
    virtual size_t domain_size(int) = 0;

    // get value,prob pairs of all values of variable_index given the evidence
    virtual vector< pair<int,double> > var_partials(const vector< pair< int, int > >& evidence, int variable) = 0;
    // get probability of evidence
    virtual double pr(const vector< pair< int, int > >& evidence) = 0;
    virtual double pr(const vector< pair< int, int > >& evidence, int next_var, int next_val) = 0; // more efficient then the above

protected:
    int verbose;
    int cache_level;
    // mappings
    vector<vector<int>  > bn_val_ids;
    vector<unordered_map<int,int> > bn_val_map; // per var, from domain value to index of the value (according to the BN engine)
    // inverse of bn_val_ids
    vector< pair<int,int> > ac_evidence; // evidence that the AC currently has: (varID, val), SORTED
    unordered_map<vector< pair<int,int> >, vector<double>, int_vector_hasher> cache_partials; // caching results in _var_partials
    virtual void set_verbose(int _verbose) {verbose = _verbose;}
    virtual void set_cache_level(int _cache_level) {cache_level = _cache_level;}
    
};
