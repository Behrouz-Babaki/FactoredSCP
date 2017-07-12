#pragma once
#pragma GCC diagnostic ignored "-Wsign-compare"

#include <vector>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cmath>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

using std::vector;
using std::string;
using std::set;
using std::map;
using std::unordered_map;
using std::istream;
using std::ifstream;
using std::hash;
using std::exception;
using std::invalid_argument;
using std::runtime_error;


class Variable{
private:
  string fName;
  vector<string> fDomainNames;
  
public: //formerly protected
  Variable(string name, vector<string> domainNames){
    fName = name;
    fDomainNames.assign(domainNames.begin(), domainNames.end());
  }
  
public:
  string name() const {
    return fName;
  }
  
  vector<string> domainNames() const {
    return fDomainNames;
  }
  
  //TODO overload << instead
  string toString() {
    return name();
  }
  
  // functions to be used by std::map

  bool operator< (const Variable& other) const {
    return this->fName < other.name();
  }
  
  size_t operator() (const Variable& self) const {
    return hash<string>()(self.name());
  }
  
  Variable(){
  }
  
  Variable(const Variable& other){
    fName = other.name();
    fDomainNames = other.domainNames();
  }
};

class Potential {
private:
    string fName;
    int fNumPositions;
    
public: //formerly protected
    Potential(string name, int numPositions) {
        fName = name;
        fNumPositions = numPositions;
    }
    
public:
    string name() const{
        return fName;
    }
    
    int numPositions() const{
        return fNumPositions;
    }
    
    //TODO overload << instead
    string toString() {
        return "T (" + name() + ")";
    }
    
    
    // functions to be used by std::map
    
    bool operator<(const Potential& other) const {
      return this->fName < other.name();
    }
    size_t operator()(const Potential& self) const {
      return (hash<string>()(self.name()));
    }
    
    Potential(){
    }
    
    Potential(const Potential& other){
      fName = other.name();
      fNumPositions = other.numPositions();
    }
    
};

class OnlineEngine;

class Evidence {
  
private:
  OnlineEngine& fEngine;
  
public: //formerly protected:
  vector<double> fVarToCurrentNegWeight;
  vector<double> fVarToCurrentPosWeight;
  
private:
  double defaultWeight (int l);
  void setCurrentWeight (int l, double w);
  void setCurrentWeightToDefault (int l);
  void setCurrentWeightsToDefaults (const vector<int>&);
  void setCurrentWeights (double w, const vector<int>&);
  
public:
  void retractAll ();
  Evidence (OnlineEngine& engine);
  void varCommit (const Variable& v, int u);
  void valCommit (const Variable& v, int u, bool allow);
  void varRetract (const Variable& v);
  void varSet (const Variable& v, double w);
  void parmCommit (const Potential& t, int p, double w);
  void parmRetract (const Potential& t, int p);
  
};

class OnlineEngine {
public: // formerly protected:
    static const char CONSTANT = 0;
    static const char LITERAL = 1;
    static const char MULTIPLY = 2;
    static const char ADD = 3;
    vector<char> fNodeToType;
    vector<int> fNodeToLastEdge;
    vector<int> fNodeToLit;
    vector<int> fEdgeToTailNode;
    vector<int> fVarToNegLitNode;
    vector<int> fVarToPosLitNode;
    string READ_DELIMITER;
    string DELIMITER;
    set<Variable> fVariables;
    set<Potential> fPotentials;
    unordered_map<string, Variable> fNameToSrcVar;
    unordered_map<string, Potential> fNameToSrcPot;
    vector<double> fLogicVarToDefaultNegWeight;
    vector<double> fLogicVarToDefaultPosWeight;
    map<Variable, vector<int>* > fSrcVarToSrcValToIndicator;
    map<Potential, vector<int>* > fSrcPotToSrcPosToParameter;
    vector<double> fNodeToValue;
    vector<double> fNodeToDerivative;
    vector<bool> fNodeToOneZero;
    bool fUpwardPassCompleted;
    bool fTwoPassesCompleted;
    vector<double> fAcVarToMostRecentNegWeight;
    vector<double> fAcVarToMostRecentPosWeight;

    vector<double> clone(const vector<double>& a);
    void readArithmeticCircuit(istream& r);
    void readLiteralMap(istream& r);
    void upwardPass (const Evidence& ev);
    void twoPasses (const Evidence& ev);
    double computedValue (int n);
    int first (int n);
    int rootNode ();
    int numAcNodes ();

public:    
    OnlineEngine (string acFilename, string lmFilename);
    void initialize(istream& acReader, istream& lmReader);
    Variable varForName (const string& n);
    Potential potForName (const string& n);
    set<Variable> variables ();
    set<Potential> potentials ();
    void assertEvidence (const Evidence& e, bool secondPass);
    double probOfEvidence ();
    vector<double> varPartials (const Variable& v);
    map<Variable,vector<double> > varPartials (const set<Variable>& vs);
    vector<double> varMarginals (const Variable& v);
    map<Variable,vector<double> > varMarginals (const set<Variable>& vs);
    vector<double> varPosteriors (const Variable& v);
    map<Variable,vector<double> > varPosteriors (const set<Variable>& vs);
    vector<double> potPartials (const Potential& pot);
    map<Potential,vector<double> > potPartials (const set<Potential>& ps);
    vector<double> potMarginals (const Potential& p);
    map<Potential,vector<double> > potMarginals (const set<Potential>& ps);
    vector<double> potPosteriors (const Potential& p);
    map<Potential,vector<double> > potPosteriors (const set<Potential>& ps);
};

inline
double Evidence::defaultWeight (int l) {
    return l < 0 ? fEngine.fLogicVarToDefaultNegWeight[-l]:
           fEngine.fLogicVarToDefaultPosWeight[l];
}

inline
void Evidence::setCurrentWeight (int l, double w) {
    if (l < 0) {
        fVarToCurrentNegWeight[-l] = w;
    } else {
        fVarToCurrentPosWeight[l] = w;
    }
}

inline
void Evidence::setCurrentWeightToDefault (int l) {
    setCurrentWeight (l, defaultWeight (l));
}

inline
void Evidence::setCurrentWeightsToDefaults (const vector<int>& lits) {
    for (int l : lits) {
        setCurrentWeightToDefault (l);
    }
}

inline
void Evidence::setCurrentWeights (double w, const vector<int>& lits) {
    for (int l : lits) {
        setCurrentWeight (l, w);
    }
}


inline
void Evidence::retractAll () {
    fVarToCurrentNegWeight.assign(fEngine.fLogicVarToDefaultNegWeight.begin(),
                                  fEngine.fLogicVarToDefaultNegWeight.end());
    fVarToCurrentPosWeight.assign(fEngine.fLogicVarToDefaultPosWeight.begin(),
                                  fEngine.fLogicVarToDefaultPosWeight.end());
}

inline
Evidence::Evidence (OnlineEngine& engine) : fEngine(engine){
    fVarToCurrentNegWeight.resize(
        fEngine.fLogicVarToDefaultNegWeight.size());
    fVarToCurrentPosWeight.resize(
        fEngine.fLogicVarToDefaultPosWeight.size());
    retractAll ();
}

inline
void Evidence::varCommit (const Variable& v, int u) {
    varSet (v, 0.0);
    setCurrentWeightToDefault (
        fEngine.fSrcVarToSrcValToIndicator[v]->at(u));
}

inline
void Evidence::valCommit (const Variable& v, int u, bool allow) {
    int l = fEngine.fSrcVarToSrcValToIndicator[v]->at(u);
    if (allow) {
        setCurrentWeightToDefault (l);
    } else {
        setCurrentWeight (l, 0.0);
    }
}

inline
void Evidence::varRetract (const Variable& v) {
    setCurrentWeightsToDefaults (*(fEngine.fSrcVarToSrcValToIndicator[v]));
}

inline
void Evidence::varSet (const Variable& v, double w) {
    setCurrentWeights (w, *(fEngine.fSrcVarToSrcValToIndicator[v]));
}

inline
void Evidence::parmCommit (const Potential& t, int p, double w) {
    int l = fEngine.fSrcPotToSrcPosToParameter[t]->at(p);
    if (p == 0) {
        throw invalid_argument("Attempt to set value of parameter illegally!");
    }
    setCurrentWeight (l, w);
}

inline
void Evidence::parmRetract (const Potential& t, int p) {
    int l = fEngine.fSrcPotToSrcPosToParameter[t]->at(p);
    if (p == 0) {
        throw invalid_argument("Attempt to set value of parameter illegally!");
    }
    setCurrentWeightToDefault (l);
}

inline
vector<double> OnlineEngine::clone(const vector<double>& a) {
    vector<double> ans;
    ans.reserve(a.size());
    ans.assign(a.begin(), a.end());
    return ans;
}

inline
void OnlineEngine::upwardPass (const Evidence& ev) {
    const vector<double>& negValues = ev.fVarToCurrentNegWeight;
    const vector<double>& posValues = ev.fVarToCurrentPosWeight;
    int numNodes = numAcNodes ();
    for (int n = 0; n < numNodes; n++) {
        char type = fNodeToType[n];
        if (type == MULTIPLY) {
            double v = 1.0;
            int last = fNodeToLastEdge[n];
            for (int e = first (n); e < last; e++) {
                int ch = fEdgeToTailNode[e];
                double chVal = fNodeToValue[ch];
                if (chVal == 0.0) {
                    v = 0.0;
                    break;
                }
                v *= chVal;
                if (v == 0.0) {
		  throw runtime_error("Underflow");
                }
            }
            fNodeToValue[n] = v;
        } else if (type == ADD) {
            double v = 0.0;
            int last = fNodeToLastEdge[n];
            for (int e = first (n); e < last; e++) {
                int ch = fEdgeToTailNode[e];
                v += fNodeToValue[ch];
            }
            fNodeToValue[n] = v;
        } else if (type == LITERAL) {
            int l = fNodeToLit[n];
            fNodeToValue[n] = (l < 0 ? negValues[-l] : posValues[l]);
        }
        // do nothing for a constant
    }
}

inline
void OnlineEngine::twoPasses (const Evidence& ev) {

    // Upward pass.

    const vector<double>& negValues = ev.fVarToCurrentNegWeight;
    const vector<double>& posValues = ev.fVarToCurrentPosWeight;
    int numNodes = numAcNodes ();
    for (int n = 0; n < numNodes; n++) {
        fNodeToDerivative[n] = 0.0;
        char type = fNodeToType[n];
        if (type == MULTIPLY) {
            int numZeros = 0;
            double v = 1.0;
            int last = fNodeToLastEdge[n];
            for (int e = first (n); e < last; e++) {
                int ch = fEdgeToTailNode[e];
                double chVal = computedValue (ch);
                if (chVal == 0.0) {
                    if (++numZeros > 1) {
                        v = 0;
                        break;
                    }
                } else {
                    v *= chVal;
                    if (v == 0.0) {
                        throw runtime_error("Underflow");
                    }
                }
            }
            fNodeToValue[n] = v;
            fNodeToOneZero[n] = numZeros == 1;
        } else if (type == ADD) {
            double v = 0.0;
            int last = fNodeToLastEdge[n];
            for (int e = first (n); e < last; e++) {
                int ch = fEdgeToTailNode[e];
                double chVal = computedValue (ch);
                v += chVal;
            }
            fNodeToValue[n] = v;
        } else if (type == LITERAL) {
            int l = fNodeToLit[n];
            fNodeToValue[n] = (l < 0 ? negValues[-l] : posValues[l]);
        }
        // do nothing for a constant
    }

    // Downward pass.

    fNodeToDerivative[numNodes - 1] = 1.0;
    for (int n = numNodes - 1; n >= 0; n--) {
        char type = fNodeToType[n];
        if (type == LITERAL || type == CONSTANT) {
            continue;
        }
        int last = fNodeToLastEdge[n];
        if (type == MULTIPLY) {
            double value = fNodeToValue[n];
            if (value == 0.0) {
                continue;   // more than one zero
            }
            double x = fNodeToDerivative[n];
            if (x == 0.0) {
                continue;
            }
            x *= value;
            if (x == 0.0) {
                throw runtime_error("Underflow");
            }
            if (fNodeToOneZero[n]) { // exactly one zero
                for (int e = first (n); e < last; e++) {
                    int ch = fEdgeToTailNode[e];
                    double chVal = computedValue (ch);
                    if (chVal == 0.0) {
                        fNodeToDerivative[ch] += x;
                        break;
                    }
                }
            } else { // no zeros
                for (int e = first (n); e < last; e++) {
                    int ch = fEdgeToTailNode[e];
                    double chVal = computedValue (ch);
                    fNodeToDerivative[ch] += x / chVal;
                }
            }
        } else { /* PLUS NODE */
            double x = fNodeToDerivative[n];
            for (int e = first (n); e < last; e++) {
                int ch = fEdgeToTailNode[e];
                fNodeToDerivative[ch] += x;
            }
        }
    }

}

inline
double OnlineEngine::computedValue (int n) {
    return fNodeToOneZero[n] ? 0 : fNodeToValue[n];
}

inline
int OnlineEngine::first (int n) {
    return (n == 0) ? 0 : fNodeToLastEdge[n-1];
}

inline
int OnlineEngine::rootNode () {
    return fNodeToType.size() - 1;
}

inline
int OnlineEngine::numAcNodes () {
    return fNodeToType.size();
}

inline
OnlineEngine::OnlineEngine (string acFilename, string lmFilename) {
	READ_DELIMITER  = "\\$";
	DELIMITER = "$";  
	ifstream ac_fs (acFilename, ifstream::in);
        ifstream lm_fs (lmFilename, ifstream::in);
        initialize (ac_fs, lm_fs);
        ac_fs.close();
        lm_fs.close();
}

inline
void OnlineEngine::initialize (istream& acReader, istream& lmReader) {
    readArithmeticCircuit(acReader);
    readLiteralMap(lmReader);
    fNodeToValue.resize(numAcNodes());
    fNodeToDerivative.resize(numAcNodes());
    fNodeToOneZero.resize(numAcNodes());
    fUpwardPassCompleted = false;
    fTwoPassesCompleted = false;
}

inline
Variable OnlineEngine::varForName (const string& n) {
    return fNameToSrcVar[n];
}

inline
Potential OnlineEngine::potForName (const string& n) {
    return fNameToSrcPot[n];
}

inline
set<Variable> OnlineEngine::variables () {
    return fVariables;
}

inline
set<Potential> OnlineEngine::potentials () {
    return fPotentials;
}

inline
void OnlineEngine::assertEvidence (const Evidence& e, bool secondPass) {
    if (secondPass) {
        fAcVarToMostRecentNegWeight = clone (e.fVarToCurrentNegWeight);
        fAcVarToMostRecentPosWeight = clone (e.fVarToCurrentPosWeight);
        // XXX check that function takes reference as argument
        // looks like copying in profiler
        twoPasses (e);
    } else {
        fAcVarToMostRecentNegWeight.clear();
        fAcVarToMostRecentPosWeight.clear();
        upwardPass (e);
    }
    fUpwardPassCompleted = true;
    fTwoPassesCompleted = secondPass;
}

inline
double OnlineEngine::probOfEvidence () {
    if (!fUpwardPassCompleted) {
        throw runtime_error ("assertEvidence () must be called!");
    }
    int root = rootNode ();
    return fTwoPassesCompleted ? computedValue (root) : fNodeToValue[root];
}

inline
vector<double> OnlineEngine::varPartials (const Variable& v) {
    if (!fTwoPassesCompleted) {
        throw runtime_error (
            "assertEvidence () must be called with second pass flag set!");
    }
    vector<double> ans(v.domainNames().size());
    vector<int>& inds = *(fSrcVarToSrcValToIndicator[v]);
    for (int u = 0; u < ans.size(); u++) {
        int l = inds[u];
        ans[u] =
            (l < 0) ?
            fNodeToDerivative[fVarToNegLitNode[-l]] :
            fNodeToDerivative[fVarToPosLitNode[l]];
    }
    return ans;
}

inline
map<Variable,vector<double> > OnlineEngine::varPartials (const set<Variable>& vs) {
    map<Variable,vector<double> > ans;
    for (Variable v : vs) {
        ans[v] = varPartials (v);
    }
    return ans;
}

inline
vector<double> OnlineEngine::varMarginals (const Variable& v) {
    if (!fTwoPassesCompleted) {
        throw runtime_error (
            "assertEvidence () must be called with marginals flag set!");
    }
    vector<double> ans(v.domainNames ().size ());
    vector<int>& inds = *(fSrcVarToSrcValToIndicator[v]);
    for (int u = 0; u < ans.size(); u++) {
        int l = inds[u];
        ans[u] =
            (l < 0) ?
            fAcVarToMostRecentNegWeight[-l] *
            fNodeToDerivative[fVarToNegLitNode[-l]] :
            fAcVarToMostRecentPosWeight[l] *
            fNodeToDerivative[fVarToPosLitNode[l]];
    }
    return ans;
}

inline
map<Variable,vector<double> > OnlineEngine::varMarginals (const set<Variable>& vs) {
    map<Variable,vector<double> > ans;
    for (Variable v : vs) {
        ans[v] = varMarginals (v);
    }
    return ans;
}

inline
vector<double> OnlineEngine::varPosteriors (const Variable& v) {
    double pe = probOfEvidence ();
    vector<double> ans = varMarginals (v);
    for (int i = 0; i < ans.size(); i++) {
        ans[i] /= pe;
    }
    return ans;
}

inline
map<Variable,vector<double> > OnlineEngine::varPosteriors (const set<Variable>& vs) {
    map<Variable,vector<double> > ans;
    for (Variable v : vs) {
        ans[v] = varPosteriors (v);
    }
    return ans;
}

inline
vector<double> OnlineEngine::potPartials (const Potential& pot) {
    vector<int>& parms = *(fSrcPotToSrcPosToParameter[pot]);
    vector<double> ans(parms.size());
    for (int pos = 0; pos < ans.size(); pos++) {
        int l = parms[pos];
        ans[pos] =
            l == 0  ? NAN :
            l < 0 ? fNodeToDerivative[fVarToNegLitNode[-l]] :
            fNodeToDerivative[fVarToPosLitNode[l]];
    }
    return ans;
}

inline
map<Potential,vector<double> > OnlineEngine::potPartials (
    const set<Potential>& ps) {
    map<Potential,vector<double> > ans;
    for (Potential p : ps) {
        ans[p] = potPartials (p);
    }
    return ans;
}

inline
vector<double> OnlineEngine::potMarginals (const Potential& p) {
    vector<int>& parms = *(fSrcPotToSrcPosToParameter[p]);
    vector<double> ans = potPartials (p);
    for (int pos = 0; pos < ans.size(); pos++) {
        int l = parms[pos];
        if (!isnan (ans[pos])) {
            ans[pos] *=
                l < 0 ?
                fAcVarToMostRecentNegWeight[-l] : fAcVarToMostRecentPosWeight[l];
        }
    }
    return ans;
}

inline
map<Potential,vector<double> > OnlineEngine::potMarginals (const set<Potential>& ps) {
    map<Potential,vector<double> > ans;
    for (Potential p : ps) {
        ans[p] = potMarginals (p);
    }
    return ans;
}

inline
vector<double> OnlineEngine::potPosteriors (const Potential& p) {
    vector<double> ans = potMarginals (p);
    double pe = probOfEvidence ();
    for (int pos = 0; pos < ans.size(); pos++) {
        if (!isnan (ans[pos])) {
            ans[pos] /= pe;
        }
    }
    return ans;
}

inline
map<Potential,vector<double> > OnlineEngine::potPosteriors (
    const set<Potential>& ps) {
    map<Potential,vector<double> > ans;
    for (Potential p : ps) {
        ans[p] = potPosteriors (p);
    }
    return ans;
}



inline
void OnlineEngine::readArithmeticCircuit(istream& r) {

    int numNodes = __INT_MAX__;
    int nextEdge = 0;
    int nextNode = 0;
    
    // Process each line.
    
    while (nextNode < numNodes) {
      
      // Read the line.  Quit if eof.  Skip if comment or blank line.
      // Otherwise, split into tokens.
      
      string line;
      if (!getline(r, line)) {break;} // eof
      if (boost::starts_with(line, "c")) {continue;} // comment
      boost::trim(line);
      if (line.length () == 0) {continue;} // blank line
      vector<string> tokens;
      boost::split(tokens, line, boost::is_any_of("\t "), boost::token_compress_on);
      
      // A header line looks like: "nnf" numNodes numEdges numVars
      
      if (tokens[0] == "nnf") {
	numNodes = boost::lexical_cast<int>(tokens[1]);
        int numEdges = boost::lexical_cast<int>(tokens[2]);
        int numAcVars = boost::lexical_cast<int>(tokens[3]);
        fEdgeToTailNode.resize(numEdges);
        fNodeToLastEdge.resize(numNodes);
        fNodeToType.resize(numNodes);
        fNodeToLit.resize(numNodes, 0);
        fVarToNegLitNode.assign(numAcVars + 1, -1);
        fVarToPosLitNode.assign(numAcVars + 1, -1);
        continue;
      }
      
      // This is not a header line, so it must be a node line, which looks
      // like one of the following:
      //   "A" numChildren child+
      //   "O" splitVar numChildren child+
      //   "L" literal
      char ch = tokens[0].at(0);
      if (ch == 'A') {
        fNodeToType[nextNode] = MULTIPLY;
        for (int chIndex = 2; chIndex < tokens.size(); chIndex++) {
          fEdgeToTailNode[nextEdge++] = boost::lexical_cast<int> (tokens[chIndex]);
        }
      } else if (ch == 'O') {
        fNodeToType[nextNode] = ADD;
        for (int chIndex = 3; chIndex < tokens.size(); chIndex++) {
          fEdgeToTailNode[nextEdge++] = boost::lexical_cast<int> (tokens[chIndex]);
        }
      } else /* ch == 'L' */ {
        fNodeToType[nextNode] = LITERAL;
        int l = boost::lexical_cast<int> (tokens[1]);
        fNodeToLit[nextNode] = l;
        (l < 0 ? fVarToNegLitNode : fVarToPosLitNode)[abs (l)] =
          nextNode;
      }
      fNodeToLastEdge[nextNode] = nextEdge;
      nextNode++;

    }

}

inline
void OnlineEngine::readLiteralMap(istream& r) {
    
    // Prepare to parse.
    
    int numLits = __INT_MAX__;
    int litsFinished = 0;

    // Process each line.
    
    while (litsFinished < numLits) {
      
      // Read the line.  Quit if eof.  Skip if comment (including blank
      // lines).  Otherwise, split into tokens.
      
      
      string line;
      if (!getline(r, line)) {break;} // eof
      if (!boost::starts_with(line, ("cc" + DELIMITER))) {continue;} // comment
      boost::trim(line);
      vector<string> tokens;
      boost::split(tokens, line, boost::is_any_of(READ_DELIMITER), boost::token_compress_on);
      
      // If the line is a header, it is of the form: "cc" "N" numLogicVars.
      string type = tokens[1];
      if (type == "N") {
	int n = boost::lexical_cast<int>(tokens[2]);
        numLits = n * 2;
        fLogicVarToDefaultNegWeight.resize(n+1);
        fLogicVarToDefaultPosWeight.resize(n+1);
        continue;
      }
      
      // If the line is a variable line, then it is of the form:
      // "cc" "V" srcVarName numSrcVals (srcVal)+

      if (type == "V") {
        string vn = tokens[2];
        int valCount = boost::lexical_cast<int>(tokens[3]);
        vector<string> valNames(valCount, "");
        for (int i = 0; i < valCount; i++) {valNames[i] = tokens[4 + i];}
        Variable v(vn, valNames);
        fSrcVarToSrcValToIndicator[v] = new vector<int>(valCount);
        fNameToSrcVar[vn] = v;
        continue;
      }
      
      // If the line is a potential line, then it is of the form:
      // "cc" "T" srcPotName parameterCnt.
      
      if (type == "T") {
        string tn = tokens[2];
        int parmCount = boost::lexical_cast<int> (tokens[3]);
        Potential pot(tn, parmCount);
        fSrcPotToSrcPosToParameter[pot] = new vector<int>(parmCount);
        fNameToSrcPot[tn] = pot;
        continue;
      }

      // This is not a header line, a variable line, or potential line, so
      // it must be a literal description, which looks like one of the
      // following:
      //   "cc" "I" literal weight srcVarName srcValName srcVal
      //   "cc" "P" literal weight srcPotName pos+
      //   "cc" "A" literal weight
      
      int l = boost::lexical_cast<int>(tokens[2]);
      double w = boost::lexical_cast<double>(tokens[3]);
      (l < 0 ? fLogicVarToDefaultNegWeight : fLogicVarToDefaultPosWeight)
        [abs (l)] = w;
      if (type == "I") {
        string vn = tokens[4];
        //String un = tokens[5];
        int u = boost::lexical_cast<int>(tokens[6]);
        fSrcVarToSrcValToIndicator[fNameToSrcVar[vn]]->at(u) = l;
      } else if (type == "P") {
        string tn = tokens[4];
	vector<string> posStrings;
	boost::split(posStrings, tokens[5], boost::is_any_of(","));
        if (posStrings.size() == 1) {
	  int pos = boost::lexical_cast<int>(posStrings[0]);
          fSrcPotToSrcPosToParameter[fNameToSrcPot[tn]]->at(pos) = l;
        }
      } else if (type == "A") {
      } else {
        throw runtime_error (
          "\"cc\" must be followed by \"N\", \"V\", \"T\", \"I\", \"P\", or \"A\"");
      }
      ++litsFinished;
      
    }

    // Now create the variables, the map from variable name to variable, and
    // the map from variable to value to indicator.
    for (auto p : fNameToSrcVar)
      fVariables.insert(p.second);
    for (auto p : fNameToSrcPot)
      fPotentials.insert(p.second);

}
