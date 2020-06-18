%top{
	#include <xanlib.h>
	#include <ir_method.h>
        #include <compiler_memory_manager.h>
	#include <chiara.h>

	#define YY_DECL int loop_lex (XanHashTable *loopsNames, XanHashTable *loopsNamesTable)
	int loop_lex (XanHashTable *loopsNames, XanHashTable *loopsNamesTable);
}
	static JITINT32		num	= -1;

%option noyywrap
%option noinput
%option nounput
%option header-file="loop_scanner.h"
%option outfile="loop_scanner.c"
%option prefix="loop"

INST_ID	[0-9]+

%%

{INST_ID}		{
	num	= atoi(looptext);
}

[a-zA-Z](.)+[a-zA-Z](.)+ {
	JITINT8 		*buf;
	XanHashTableItem	*hashItem;

	assert(num > 0);
	buf = (JITINT8 *) allocFunction(yyleng + 1);
	SNPRINTF(buf, yyleng+1, "%s", looptext);
	hashItem	= xanHashTable_first(loopsNames);
	while (hashItem != NULL){
		char	*name;
		name		= hashItem->element;
		if (STRCMP(name, buf) == 0){
			assert(xanHashTable_lookup(loopsNamesTable, name) == NULL);
			xanHashTable_insert(loopsNamesTable, name, (void *)(JITNUINT)num);
			break ;
		}
		hashItem	= xanHashTable_next(loopsNames, hashItem);
	}
	if (hashItem == NULL){
		fprintf(stderr, "WARNING: Loop %s has not been found in the loopsNames table.\n", buf);
	}
	free(buf);
}

\n		{
	num	= -1;
}

[ ] { }

%%

