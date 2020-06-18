%option noyywrap
%option header-file="lex.yy.h"

%x METHOD_NAME
%x METHOD_PARAMETER
%x METHOD_PARAMETER_2
%x GARBAGE

%%

<INITIAL>ENTER:[ ]* {
	BEGIN(METHOD_NAME);
}

<METHOD_NAME>[(]([a-zA-Z]|[ ]|[-])+[)][ ]* {
}

<METHOD_NAME>[a-zA-Z._-]+[:][a-zA-Z._-]+[ ]* {
        ECHO ;
        BEGIN(METHOD_PARAMETER);
}

<METHOD_PARAMETER>[(] {
        ECHO ;
        BEGIN(METHOD_PARAMETER_2);
}

<METHOD_PARAMETER_2>[)] {
        ECHO ;
        BEGIN(GARBAGE);
}

<METHOD_PARAMETER_2>. {
        ECHO ;
}

<GARBAGE>. { }

<GARBAGE>\n {
        ECHO ;
        BEGIN(INITIAL);
}

<INITIAL>. { }

%%
