all: method.lex methods_study.c
	flex method.lex
	gcc -I. -o methods_study methods_study.c lex.yy.c

clean: 
	rm -f lex.yy* methods_study
