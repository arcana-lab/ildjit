#ifndef CACHE_ARRAY_H
#define CACHE_ARRAY_H

#include <map>
#include <vector>

using namespace std;

struct Entry
{
  unsigned int address;
  unsigned long long lastTouched;
};

struct Set
{
  map<unsigned int, Entry> entries;
};

class CacheArray{
 public:
  CacheArray(int numEntries, int assoc);
  ~CacheArray();
  bool doMemoryRequest(unsigned int addr, int coreId);
  
 private:
  int numEntries_;
  int assoc_;
  
  int numSets_;
  int log2Sets_;

  vector<Set> sets_;
  unsigned long long numRequests_;
  unsigned long long numHits_;
  int coreId_;

  // Helper functions
  int getSet(unsigned int address);
  unsigned int getReplacementAddress(int setNum);

  // Replacement policies
  unsigned int random(int setNum);
  unsigned int lru(int setNum);
};

#endif
