%top{
	#include <xanlib.h>
	#include <ir_method.h>
        #include <compiler_memory_manager.h>
	#include <chiara.h>

	#define YY_DECL int instruction_lex (XanHashTable *insts, XanHashTable *methods)
	int instruction_lex (XanHashTable *insts, XanHashTable *methods);
}
	static ir_method_t 	*currentMethod	= NULL;
	static JITUINT32	instID		= 0;

%option noyywrap
%option noinput
%option nounput
%option header-file="instruction_scanner.h"
%option outfile="instruction_scanner.c"
%option prefix="instruction"

INST_ID	[0-9]+

%%

{INST_ID}		{
	if (instID == 0){
		instID	= atoi(instructiontext);
		assert(instID > 0);

	} else if (currentMethod != NULL){
		JITUINT32		inst_id;
		ir_instruction_t	*inst;

		assert(instID > 0);
		inst_id	= atoi(instructiontext);
		inst 	= IRMETHOD_getInstructionAtPosition(currentMethod, inst_id);
		assert(inst != NULL);

		if (xanHashTable_lookup(insts, inst) == NULL){
			xanHashTable_insert(insts, inst, (void *)(JITNUINT)instID);
		}
		currentMethod	= NULL;

	} else {
		instID		= 0;
	}
}

[^\n ](.)+[a-zA-Z](.)+ {
	JITINT8 *buf;
	buf = (JITINT8 *) allocFunction(yyleng + 1);
	SNPRINTF(buf, yyleng+1, "%s", instructiontext);
	currentMethod	= IRPROGRAM_getMethodWithSignatureInString(buf);
	assert(currentMethod != NULL);
	if (xanHashTable_lookup(methods, currentMethod) == NULL){
		xanHashTable_insert(methods, currentMethod, currentMethod);
	}
	free(buf);
}

\n		{ }

[ ] { }

%%

