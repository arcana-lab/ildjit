%{
#include <manfred_system.h>
#include <stdio.h>
#include <xanlib.h>
#include <ilmethod.h>
#include <ir_method.h>
#include <compiler_memory_manager.h>
#include <manfred_load.h>

%}

%union {
	IR_ITEM_VALUE int_value;
	JITINT32 ir_value;
	Method method;
	XanList* list;
	ir_signature_t* signature;
	ir_item_t* item;
	ir_instruction_t* instruction;
	ir_type_definition_t* typeDefinition;
}

%start irfile

%type <typeDefinition> irTypeDefinition
%type <method> method
%type <list> cctorList
%type <list> cctorListContent
%type <list> localList
%type <list> localListContent
%type <signature> signature
%type <list> signatureParams
%type <instruction> irInstruction
%type <item> irItem
%type <item> irItemContent
%type <list> paramList
%type <list> paramListContent
%type <list> instructionsList

%token irNEWLINE
%token irLPAR
%token irRPAR
%token irLEFTSQUAREBRACKET
%token irRIGHTSQUAREBRACKET
/**
 * Special keyword
 */
%token irMETHOD
%token irCCTOR
%token irLOCALS
%token irMAXVAR
%token irENDMETHOD

/**
 * Special keyword
 */
%token irSIGNATURE
/**
 * Special IR Type
 */
%token irCLASSID
%token irMETHODID 
%token irSTRING
%token irTYPE
%token irSYMBOL
%token irLABELITEM
%token irOFFSET
%token irNOPARAM

%token <ir_value> irINSTRUCTION
%token <int_value> irNUMBER
%token <ir_value> irTYPE_VALUE
%token <int_value> irSTRING_VALUE

%%

irfile : 
methods {
	YYACCEPT;
};

methods :
methods method {} | method {};
;	

method : irMETHOD irNUMBER irCCTOR cctorList irSIGNATURE signature irLOCALS localList irMAXVAR irNUMBER instructionsList irENDMETHOD
{
	XanListItem *item;
	ir_symbol_t *methodSymbol = IRSYMBOL_loadSymbol($2);
	Method method = (Method)(JITNUINT)(IRSYMBOL_resolveSymbol(methodSymbol).v);
	item =xanList_first($4);
	while(item != NULL){
		Method cctor = (Method) item->data;
		method->addCctorToCall(method, cctor);
		item = item->next;
	}
	xanList_destroyList($4);
	
	ir_method_t * irMethod = method->getIRMethod(method);
	if (method->isAnonymous(method)){
		memcpy(&(irMethod->signature), $6, sizeof(ir_signature_t));
		IRMETHOD_createMethodTrampoline(irMethod);
	}
	IRMETHOD_setNumberOfVariablesNeededByMethod(irMethod, $6->parametersNumber);
	freeFunction($6);
	
	item = xanList_first($8);
	while(item != NULL){
		ir_type_definition_t *local = (ir_type_definition_t *)item->data;
		IRMETHOD_insertLocalVariableToMethod( irMethod, local->internal_type, local->type_infos);
		item = item->next;
	}
	xanList_destroyListAndData($8);
	
	IRMETHOD_setNumberOfVariablesNeededByMethod(irMethod, (JITUINT32)$10);

	item = xanList_first($11);
	while(item != NULL){
		ir_instruction_t *inst = (ir_instruction_t *)item->data;
		ir_instruction_t *newInst = IRMETHOD_newInstruction(irMethod);
		JITUINT32 id = newInst->ID;
		memcpy(newInst, inst, sizeof(ir_instruction_t));
		newInst->ID = id;
		item = item->next;
	}
	xanList_destroyListAndData($11);

	method->setState(method, IR_STATE);
};

cctorList: irLPAR cctorListContent irRPAR
{
	$$ = $2;
}
| irLPAR irRPAR
{
	$$=xanList_new(allocFunction, freeFunction, NULL);
};

cctorListContent: cctorListContent irNUMBER
{
	ir_symbol_t *methodSymbol = IRSYMBOL_loadSymbol($2);
	Method method = (Method)(JITNUINT)(IRSYMBOL_resolveSymbol(methodSymbol).v); 	
	xanList_append($1, method); 
 	$$=$1;
}
| irNUMBER
{
 	$$=xanList_new(allocFunction, freeFunction, NULL);
	ir_symbol_t *methodSymbol = IRSYMBOL_loadSymbol($1);
	Method method = (Method)(JITNUINT)(IRSYMBOL_resolveSymbol(methodSymbol).v); 	
	xanList_append($$, method); 
};

localList: irLPAR localListContent irRPAR
{
	$$ = $2;
}
| irLPAR irRPAR
{
	$$=xanList_new(allocFunction, freeFunction, NULL);
};

localListContent: localListContent irTypeDefinition {
	xanList_append($1, $2); 
 	$$ = $1;
}
| irTypeDefinition
{
	$$ = xanList_new(allocFunction, freeFunction, NULL);
	xanList_append($$, $1);
};


signature: irLEFTSQUAREBRACKET irTypeDefinition irLPAR signatureParams irRPAR irRIGHTSQUAREBRACKET
{
	$$ = allocFunction(sizeof(ir_signature_t));
	$$->resultType = $2->internal_type;
	assert($$->resultType != NOPARAM);
	$$->ilResultType = $2->type_infos;
	freeFunction($2);
	IRMETHOD_initSignature($$, xanList_length($4));
	XanListItem *item = xanList_first($4);
	JITUINT32 count=0;
	while(item != NULL){
		ir_type_definition_t *typeDefinition = (ir_type_definition_t *) item->data;
		$$->ilParamsTypes[count] = typeDefinition->type_infos;
		$$->parameterTypes[count] = typeDefinition->internal_type;
		item = item->next;
		count++;
	}
	xanList_destroyListAndData($4);
}
| irLEFTSQUAREBRACKET irTypeDefinition irLPAR irRPAR irRIGHTSQUAREBRACKET
{
	$$ = allocFunction(sizeof(ir_signature_t));
	$$->resultType = $2->internal_type;
	$$->ilResultType = $2->type_infos;
	freeFunction($2);
	$$->parametersNumber = 0;
	$$->ilParamsTypes = NULL;
	$$->parameterTypes = NULL;
};

signatureParams: signatureParams irTypeDefinition
{
 	xanList_append($1, $2);
 	$$=$1;
}
| irTypeDefinition
{
 	$$=xanList_new(allocFunction, freeFunction, NULL);
	xanList_append($$, $1); 
};

irTypeDefinition: irLEFTSQUAREBRACKET irTYPE_VALUE irNUMBER irRIGHTSQUAREBRACKET
{
	$$ = allocFunction(sizeof(ir_type_definition_t));
	$$->internal_type = $2;
	ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol($3);
	$$->type_infos = (TypeDescriptor *)(JITNUINT)(IRSYMBOL_resolveSymbol(typeSymbol).v);
};

paramList: paramList irItem
{
 	xanList_append($1, $2);
 	$$=$1;
}
| irItem
{
 	$$=xanList_new(allocFunction, freeFunction, NULL);
	xanList_append($$, $1); 
};

irItem: irLEFTSQUAREBRACKET irItemContent irRIGHTSQUAREBRACKET{
	$$ = $2;
}
| irNOPARAM
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = NOPARAM;
	$$->internal_type = NOPARAM;
	$$->type_infos = NULL;
	$$->value.v = 0;
};

irItemContent: irSTRING irNOPARAM
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IRSTRING;
	$$->internal_type = IRSTRING;
	$$->type_infos = NULL;
	$$->value.v = (JITNUINT)"";
}
| irCLASSID irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol($2);
	IRSYMBOL_storeSymbolToIRItem($$, typeSymbol, IRCLASSID, NULL);
}
| irMETHODID irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IRMETHODID;
	$$->internal_type = IRMETHODID;
	$$->type_infos = NULL;
	ir_symbol_t *methodSymbol = IRSYMBOL_loadSymbol($2);
	$$->value = IRSYMBOL_resolveSymbol(methodSymbol);

}
| irSIGNATURE signature
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IRSIGNATURE;
	$$->internal_type = IRSIGNATURE;
	$$->type_infos = NULL;
 	$$->value.v = (JITNUINT)$2;
 }
| irTYPE irTYPE_VALUE irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IRTYPE;
	$$->internal_type = IRTYPE;
	$$->value.v = $2;
	ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol($3);
	$$->type_infos = (TypeDescriptor *)(JITNUINT)(IRSYMBOL_resolveSymbol(typeSymbol).v);
}
| irSYMBOL irTYPE_VALUE irNUMBER irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IRSYMBOL;
	$$->internal_type = $2;
	ir_symbol_t *valueSymbol = IRSYMBOL_loadSymbol($3);
	$$->value.v = (JITNUINT)valueSymbol;
	ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol($4);
	$$->type_infos = (TypeDescriptor *)(JITNUINT)(IRSYMBOL_resolveSymbol(typeSymbol).v);
}
| irSYMBOL irCLASSID irNUMBER irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IRSYMBOL;
	$$->internal_type = IRCLASSID;
	ir_symbol_t *valueSymbol = IRSYMBOL_loadSymbol($3);
	$$->value.v = (JITNUINT)valueSymbol;
	ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol($4);
	$$->type_infos = (TypeDescriptor *)(JITNUINT)(IRSYMBOL_resolveSymbol(typeSymbol).v);
}
| irSYMBOL irMETHODID irNUMBER irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IRSYMBOL;
	$$->internal_type = IRMETHODID;
	ir_symbol_t *valueSymbol = IRSYMBOL_loadSymbol($3);
	$$->value.v = (JITNUINT)valueSymbol;
	ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol($4);
	$$->type_infos = (TypeDescriptor *)(JITNUINT)(IRSYMBOL_resolveSymbol(typeSymbol).v);
}
| irSYMBOL irSTRING irNUMBER irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IRSYMBOL;
	$$->internal_type = IRSTRING;
	ir_symbol_t *valueSymbol = IRSYMBOL_loadSymbol($3);
	$$->value.v = (JITNUINT)valueSymbol;
	ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol($4);
	$$->type_infos = (TypeDescriptor *)(JITNUINT)(IRSYMBOL_resolveSymbol(typeSymbol).v);
}
| irLABELITEM irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IRLABELITEM;
	$$->internal_type = IRLABELITEM;
	$$->value.v = $2;
}
| irOFFSET irTYPE_VALUE irNUMBER irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = IROFFSET;
	$$->internal_type = $2;
	$$->value.v = $3;
	ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol($4);
	$$->type_infos = (TypeDescriptor *)(JITNUINT)(IRSYMBOL_resolveSymbol(typeSymbol).v);
}
| irTYPE_VALUE  irNUMBER irNUMBER
{
	$$ = allocFunction(sizeof(ir_item_t));
	$$->type = $1;
	$$->internal_type = $1;
	$$->value.v = $2;
	ir_symbol_t *typeSymbol = IRSYMBOL_loadSymbol($3);
	$$->type_infos = (TypeDescriptor *)(JITNUINT)(IRSYMBOL_resolveSymbol(typeSymbol).v);
};

instructionsList:  instructionsList irInstruction
{
 	xanList_append($1, $2);
 	$$=$1;
}
| irInstruction
{
 	$$=xanList_new(allocFunction, freeFunction, NULL);
	xanList_append($$, $1); 
};

irInstruction: irINSTRUCTION irItem irItem irItem irItem paramList irNUMBER irNUMBER
{
	$$ = allocFunction(sizeof(ir_instruction_t));
	$$->type = $1;
	$$->ID = -1;
	$$->byte_offset = (JITUINT32)$7;
	$$->flags = (JITUINT32)$8;
	memcpy (&($$->result), $2, sizeof(ir_item_t));
	freeFunction($2);
	memcpy (&($$->param_1), $3, sizeof(ir_item_t));
	freeFunction($3);
	memcpy (&($$->param_2), $4, sizeof(ir_item_t));
	freeFunction($4);
	memcpy (&($$->param_3), $5, sizeof(ir_item_t));
	freeFunction($5);
	if (xanList_length($6) > 0){
		$$->callParameters = $6;
	} else {
		xanList_destroyList($6);
	}
};

paramList: irLPAR paramListContent irRPAR
{
	$$=$2;
}
| irLPAR irRPAR
{
	$$=xanList_new(allocFunction, freeFunction, NULL);
};

paramListContent: paramListContent irItem
{
	xanList_append($1, $2);
	$$=$1;
}
| irItem
{
	$$=xanList_new(allocFunction, freeFunction, NULL);
	xanList_append($$, $1); 
};
%%
