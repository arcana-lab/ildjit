%top{
	#include <xanlib.h>
	#include <assert.h>
	
	#define YY_DECL int loop_names_lex (XanList *loop_names_list)
	int loop_names_lex (XanList *loop_names_list);
}

%option noyywrap
%option noinput
%option nounput
%option header-file="loop_names_scanner.h"
%option outfile="loop_names_scanner.c"
%option prefix="loop_names"

%%

.*	{
	char *loop_name;
	loop_name = strdup(loop_namestext);
	assert(loop_name != NULL);
	xanList_append(loop_names_list, loop_name);
}

\n 		{ }

%%

