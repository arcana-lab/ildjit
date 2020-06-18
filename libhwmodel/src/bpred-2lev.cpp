/* bpred-2lev.cpp: 2-Level predictor (GAg, GAp, PAg, PAp, gshare, pshare)
   [Yeh and Patt, MICRO 1991; McFarling DEC-WRL TN36]  */
/*
 * __COPYRIGHT__ GT
 */

#include <math.h>
#include "valcheck.h"
#include "zesto-bpred.h"
#include <iostream>
#define COMPONENT_NAME "2lev"


using namespace std;

#define BPRED_STAT(x) if(!frozen) {x}
#define BPRED_LOOKUP_HEADER \
  bool lookup(class bpred_sc_t * const scvp, const md_addr_t PC, const md_addr_t tPC, const md_addr_t oPC, const bool outcome)
#define BPRED_UPDATE_HEADER \
  void update(class bpred_sc_t * const scvp, const md_addr_t PC, const md_addr_t tPC, const md_addr_t oPC, const bool outcome, const bool our_pred)
#define BPRED_SPEC_UPDATE_HEADER \
  void spec_update(class bpred_sc_t * const scvp, const md_addr_t PC, const md_addr_t tPC, const md_addr_t oPC, const bool outcome, const bool our_pred)
#define BPRED_RECOVER_HEADER \
  void recover(class bpred_sc_t * const scvp, const bool outcome)
#define BPRED_FLUSH_HEADER \
  void flush(class bpred_sc_t * const scvp)
#define BPRED_GET_CACHE_HEADER \
  class bpred_sc_t * get_cache(void)
#define BPRED_RET_CACHE_HEADER \
  void ret_cache(class bpred_sc_t * const scvp)


class bpred_2lev_t:public bpred_dir_t
{
  class bpred_2lev_sc_t:public bpred_sc_t
  {
    public:
    my2bc_t * current_ctr;
    int * bhr;
    int lookup_bhr;
  };

  protected:

  int bht_size;
  int bht_mask;
  int * bht;
  int pht_size;
  int pht_mask;
  my2bc_t * pht;
  int history_length;
  int history_mask;
  int Xor; /* all lower-case "xor" is reserved keyword in C++. yuck! >:-P */
  int xorshift;

  public:

  /* CREATE */
  bpred_2lev_t(char * const arg_name,
               const int arg_bht_size,
               const int arg_pht_size,
               const int arg_history_length,
               const int arg_Xor
              )
  {
    name = NULL;
    type = NULL;
    num_hits = 0;
    lookups = 0;
    updates = 0;
    spec_updates = 0;
    frozen = 0;

    /* verify arguments are valid */
    CHECK_PPOW2(arg_bht_size);
    CHECK_PPOW2(arg_pht_size);
    CHECK_NNEG(arg_history_length);
    CHECK_BOOL(arg_Xor);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc 2lev name (strdup)");
    type = strdup("2lev");
    if(!type)
      fatal("couldn't malloc 2lev type (strdup)");

    bht_size = arg_bht_size;
    bht_mask = arg_bht_size-1;
    pht_size = arg_pht_size;
    pht_mask = arg_pht_size-1;
    history_length = arg_history_length;
    history_mask = (1<<arg_history_length)-1;
    Xor = arg_Xor;
    if(Xor)
    {
      xorshift = log_base2(pht_size)-history_length;
      if(xorshift < 0)
        xorshift = 0;
    }

    bht = (int*) calloc(bht_size,sizeof(*bht));
    if(!bht)
      fatal("couldn't malloc 2lev BHT");
    pht = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    if(!pht)
      fatal("couldn't malloc 2lev PHT");
    for(int i=0;i<pht_size;i++)
      pht[i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;

    bits = bht_size*history_length + pht_size*2;

  }

  /* DESTROY */
  ~bpred_2lev_t()
  {
    if(pht) free(pht); pht = NULL;
    if(bht) free(bht); bht = NULL;
    cerr << "HITS:" << num_hits << endl;
    cerr << "LOOKUPS:" << lookups << endl;
    cerr << "UPDATE:" << updates << endl;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;
    int l1index = PC&bht_mask;
    int l2index;
    sc->bhr = &bht[l1index];

    if(Xor)
    {
      l2index = PC^(*sc->bhr<<xorshift);
    }
    else
    {
      l2index = (PC<<history_length)|*sc->bhr;
    }

    l2index &= pht_mask;
    cerr << hex << l2index << dec << endl;
    sc->current_ctr = &pht[l2index];
    sc->updated = false;

    BPRED_STAT(lookups++;)
    sc->lookup_bhr = *sc->bhr;

    return MY2BC_DIR(*sc->current_ctr);
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;

    MY2BC_UPDATE(*sc->current_ctr,outcome);

    if(!sc->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)
      sc->updated = true;
    }
    bht[0] = ((sc->lookup_bhr<<1)|outcome)&history_mask;;
  }

  /* SPEC UPDATE */
  BPRED_SPEC_UPDATE_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;

    BPRED_STAT(spec_updates++;)

    *sc->bhr = ((*sc->bhr<<1)|our_pred)&history_mask;
  }

  BPRED_RECOVER_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;
    if(bht_size == 1) // global history recovery only
        bht[0] = (sc->lookup_bhr<<1)|outcome;
  }

  BPRED_FLUSH_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;
    if(bht_size == 1) // global history recovery only
        bht[0] = sc->lookup_bhr;
  }

  /* GETCACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_2lev_sc_t * sc = new bpred_2lev_sc_t();
    if(!sc)
      fatal("couldn't malloc 2lev State Cache");
    return sc;
  }


  int get_local_size(void) { return pht_size; }
  int get_local_width(int lev) { return lev ? 0 : 20; } //bits
  int get_global_size(void) { return bht_size; }
  int get_global_width(void) { return history_length; }
};

#undef COMPONENT_NAME
