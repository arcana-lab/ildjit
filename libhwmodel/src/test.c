#include <iostream>
#include "bpred-2lev.cpp"


using namespace std;

int main(int argc, char* argv[]) 
{
  bpred_2lev_t mybp = bpred_2lev_t("name", 1, 4096, 12, 1);
  bpred_sc_t * scvp = mybp.get_cache();
  
  md_addr_t pc = 0x0;
  md_addr_t tpc = 0x100;

  bool mypred;

  for(int i = 0; i < 100; i++) {
    mypred = mybp.lookup(scvp, pc, tpc, -1, false);
    mybp.update(scvp, pc, tpc, -1, false, mypred);
  }
  cerr << endl;
  for(int i = 0; i < 12; i++) {
    mypred = mybp.lookup(scvp, pc, tpc, -1, false);
    mybp.update(scvp, pc, tpc, -1, true, mypred);
  }
  cerr << endl;


  cerr << "done!" << endl;
}
