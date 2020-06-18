#include <cache_array.h>
#include <assert.h>
#include <cmath>
#include <algorithm>
#include <iostream>

CacheArray::CacheArray(int numEntries, int assoc)
{
  numRequests_ = 0;
  numHits_ = 0;
  numEntries_ = numEntries;
  assoc_ = assoc;

  assert(numEntries_ % assoc == 0);
  assert(numEntries_ >= assoc);

  numSets_ = numEntries / assoc;
  log2Sets_ = (int)floor(log2(numSets_) + 0.5);

  for(int i = 0; i < numSets_; i++) {
    sets_.push_back(Set());
  }
}

CacheArray::~CacheArray()
{
  cerr << numHits_ << "\t" << numRequests_ << endl;
  cerr << "HITRATE:" << double(numHits_) / double(numRequests_) << endl;
}

bool CacheArray::doMemoryRequest(unsigned int addr, int coreId)
{
  numRequests_++;
  map<unsigned int, Entry>::iterator it;
  int setNum = getSet(addr);
  bool foundAddress = false;

  it = sets_[setNum].entries.find(addr);
  foundAddress = (it != sets_[setNum].entries.end());

  if(foundAddress) {  // hit
    it->second.lastTouched = numRequests_;
    numHits_++;
  }
  else { //  miss - allocate new entry
    Entry entry;
    entry.address = addr;
    entry.lastTouched = numRequests_;
    
    if((int)sets_[setNum].entries.size() >= assoc_) { // need to evict
      unsigned int replacedAddress = getReplacementAddress(setNum);
      int replaced = sets_[setNum].entries.erase(replacedAddress);
      assert(replaced == 1);
    }
    sets_[setNum].entries[addr] = entry;
  }

  return foundAddress;
}

unsigned int CacheArray::getReplacementAddress(int setNum)
{
  unsigned int replaced = lru(setNum);
  return replaced;
}

unsigned int CacheArray::lru(int setNum)
{
  Set* set = &(sets_[setNum]);
  assert((int)set->entries.size() == assoc_);

  int leastRecent = numRequests_ + 1;
  int count = 0;
  map<unsigned int, Entry>::iterator it = set->entries.begin();
  unsigned int choice = it->first;

  for(count = 0, it = set->entries.begin(); it != set->entries.end(); it++, count++) {
    int mostRecentAccess = it->second.lastTouched;
    if(mostRecentAccess < leastRecent) {
      leastRecent = mostRecentAccess;
      choice = it->first;
    }
  }

  return choice;
}

int CacheArray::getSet(unsigned int address)
{
  if(numSets_ == 1) {
    return 0;
  }

  int byteOffset = 2;

  unsigned int index = address >> byteOffset;
  index = index << (32 - log2Sets_);
  index = index >> (32 - log2Sets_);

  if(index >= sets_.size()) {
    unsigned int key = address >> byteOffset;
    key = key >> log2Sets_;
    index = key % numSets_;
  }

  return index;
}
