
%top{
  #include "cam_system.h"
  #include <map>
  #include <vector>
  using namespace std;

  #ifdef yytext_ptr
    #undef yytext_ptr
  #endif
  #ifdef YY_DECL
    #undef YY_DECL
  #endif
	#define YY_DECL int xcalltracelex (map<uintptr_t, map<pair<uint64_t, uint64_t>, char *>>& t)
	int xcalltracelex (map<uintptr_t, map<pair<uint64_t, uint64_t>, char *>>& t);
}

%{
struct CompressedCallTraceLex{
  int state;
  uint64_t num;
  uintptr_t callid;
  uint64_t numlines;
  vector<pair<uint64_t, uint64_t>> pattern;
  CompressedCallTraceLex() : state(0)  {}
};
CompressedCallTraceLex cctl;


%}

%option noyywrap
%option noinput
%option nounput
%option header-file="compressed_call_trace_parser.h"
%option outfile="compressed_call_trace_parser.cpp"
%option prefix="xcalltrace"

CLOSEBRACE \}
NUMBER	[0-9]+
LOOPNAME [a-zA-Z][^ \t\n\[\]]+

%%

{NUMBER} {
  if(cctl.state == 0){
    cctl.callid = strtoull(xcalltracetext, NULL, 10);
    cctl.state++;
  }
  else if(cctl.state == 1){
    cctl.numlines = strtoull(xcalltracetext, NULL, 10);
    cctl.state++;
  }
  else if(cctl.state == 2){
    cctl.num = strtoull(xcalltracetext, NULL, 10);
    cctl.state++;
  }
  else if(cctl.state == 4){
    cctl.pattern.push_back(pair<uint64_t, uint64_t>(cctl.num, strtoull(xcalltracetext, NULL, 10)));
    cctl.state = 2;
  }
  else{
    printf("Can't process a number in state %i: %s\n", cctl.state, xcalltracetext);
    abort();
  }
    
}

\- {
  cctl.state++;
}

,|{CLOSEBRACE} {
  if(cctl.state == 3){
    cctl.pattern.push_back(pair<uint64_t, uint64_t>(cctl.num, cctl.num));
    cctl.state = 2;
  }
}

({NUMBER}[ ]*\[.*\]\n)|0[ ]*\n {
  char *newPattern = (char *)malloc(sizeof(char)*(strlen(xcalltracetext)+1));
  strcpy(newPattern, xcalltracetext);
  for(auto i = cctl.pattern.begin(); i != cctl.pattern.end(); i++){
    t[cctl.callid][*i] = newPattern;
  }
  cctl.pattern.clear();
  cctl.numlines--;
  if(cctl.numlines > 0)
    cctl.state = 2;
  else 
    cctl.state = 0;
}

. {
}



%%
