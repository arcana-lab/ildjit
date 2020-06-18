#ifndef COMPRESSIONPATTERN_H
#define COMPRESSIONPATTERN_H

#include <iostream>
#include <vector>
#include <deque>
#include <map>
using namespace std;

typedef uintptr_t c_symbol;

class CompressionPattern;

class CompressionPatternChild{
  vector<CompressionPattern> pattern;
  uint64_t numRepetitions;

public:
  friend ostream& operator<<(ostream& os, const CompressionPatternChild& p);
  bool operator==(const CompressionPatternChild& other) const ;
  CompressionPatternChild();
  void push_back(CompressionPattern cp);
  void setNumRepetitions(uint64_t n);
  void incRepetitions();
  unsigned int getPatternSize();
  map<c_symbol, uint64_t> getNumberOfInstancesMap();
};

class CompressionPattern{
  c_symbol sym;
  CompressionPatternChild* child;

public:
  friend ostream& operator<<(ostream& os, const CompressionPattern& p);
  bool operator==(const CompressionPattern& other) const ;
  bool operator!=(const CompressionPattern& other) const ;
  //CompressionPattern(vector<CompressionPattern*> pattern_p, uint64_t numRepetitions_p);
  CompressionPattern(const CompressionPattern& other);
  CompressionPattern();
  CompressionPattern& operator=(const CompressionPattern& other);
  ~CompressionPattern();
  CompressionPattern(c_symbol sym_p);
  //bool patternMatchesSymbols(deque<CompressionPattern*> &symbols);
  void incRepetitions();
  unsigned int getPatternSize();
  //vector<c_symbol> decompress();
  void addPattern(CompressionPattern& cp);
  void setNumRepetitions(uint64_t n);
  void setSymbol(c_symbol s);
  map<c_symbol, uint64_t> getNumberOfInstancesMap();
  bool hasChild() const;
};

#endif
