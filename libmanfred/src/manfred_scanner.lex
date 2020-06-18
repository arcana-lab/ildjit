%option noyywrap
%option nounput
%option noinput
%{
#include <stdlib.h>
#include <compiler_memory_manager.h>
#include <ir_method.h>
#include <ilmethod.h>
#include <manfred_load.h>
#include <manfred_parser.tab.h>
%}

NUMBER 			[0-9]+
IRTYPE			[A-Z]+[0-9]*
INSTRUCTION		[a-z_]+[a-z_0-9]*

%%

"\r\n"			{}
"\n" 			{}
[ \t\f\v]+ 		{}
"(" 			{ return irLPAR; }
")"				{ return irRPAR; }
"["				{ return irLEFTSQUAREBRACKET; }
"]"				{ return irRIGHTSQUAREBRACKET; }
"BEGIN"		{ return irMETHOD; }
"CCTOR"		{ return irCCTOR; }
"LOCALS"	{ return irLOCALS; }
"SIGNATURE"	{ return irSIGNATURE; }
"MAXVAR"	{ return irMAXVAR; }
"END"		{ return irENDMETHOD; }
"NOPARAM"	{ return irNOPARAM; }
"CLASSID"	{ return irCLASSID; }
"METHODID"	{ return irMETHODID; }
"TYPE"		{ return irTYPE; }
"SYMBOL"	{ return irSYMBOL; }
"LABELITEM" { return irLABELITEM; }
"VAR"		{ return irOFFSET; }
"STRING"	{ return irSTRING; }

{INSTRUCTION}	{
	manfred_yylval.ir_value = IRMETHOD_getValueFromIRKeyword((JITINT8 *)manfred_yytext);
	return irINSTRUCTION;
}

{IRTYPE} {
	manfred_yylval.ir_value = IRMETHOD_getValueFromIRKeyword((JITINT8 *)manfred_yytext);
	return irTYPE_VALUE;
}

{NUMBER} {
	manfred_yylval.int_value = strtoull (manfred_yytext,NULL,0);
	return irNUMBER;
}

