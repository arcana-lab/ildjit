%top{
	#include <xanlib.h>
	#include <ir_method.h>
        #include <compiler_memory_manager.h>
	#include <chiara.h>

	#define YY_DECL int loop_id_lex (XanHashTable *loopTable)
	int loop_id_lex (XanHashTable *loopTable);
}
	static JITINT32		num	= -1;

%option noyywrap
%option noinput
%option nounput
%option header-file="loop_scanner.h"
%option outfile="loop_scanner.c"
%option prefix="loop_id"

INST_ID	[0-9]+

%%

{INST_ID}		{
	num	= atoi(loop_idtext);
}

(([A-Fa-f0-9]*\*\ )|[a-zA-Z_])(.)+[a-zA-Z](.)+ {
	JITINT8 		*buf;

	assert(num > 0);
	buf = (JITINT8 *) allocFunction(yyleng + 1);
	SNPRINTF(buf, yyleng+1, "%s", loop_idtext);
	assert(xanHashTable_lookup(loopTable, buf) == NULL);
	xanHashTable_insert(loopTable, buf, (void *)(JITNUINT)num);
}

\n		{
	num	= -1;
}

[ ] { }

%%

