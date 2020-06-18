%top{
	#include <xanlib.h>
	#include <chiara.h>
        #include <compiler_memory_manager.h>
	#include <iljit-utils.h>

	#define YY_DECL int status_file_lex (JITUINT64 *kb)
	int status_file_lex (JITUINT64 *kb);
}
	static int state = 0;

%option noyywrap
%option noinput
%option nounput
%option header-file="status_file_scanner.h"
%option outfile="status_file_scanner.c"
%option prefix="status_file"

NUMBER	[0-9]+

%%

{NUMBER}	{
	if (state != 0){

		/* Fetch the number	*/
		(*kb)	= atoll(status_filetext);
		state	= 0;
	}
}

([a-zA-Z]|[:])+ {
	if (strcmp(status_filetext, "VmSize:") == 0){
		state	= 1;
	}
}

.|\n 		{ } 

%%

