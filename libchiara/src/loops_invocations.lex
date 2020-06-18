%top{
	#include <xanlib.h>
	#include <chiara.h>
        #include <compiler_memory_manager.h>
	#include <iljit-utils.h>

	#define YY_DECL int loops_invocationslex (XanGraph *g, XanHashTable *loops, XanHashTable *loopsNames)
	int loops_invocationslex (XanGraph *g, XanHashTable *loops, XanHashTable *loopsNames);
}
	static int state = 0;
	static loop_t *loop = NULL;
	static JITUINT64 minInv = 0;
	static JITUINT64 maxInv = 0;
	static loop_invocations_information_t	*l = NULL;
	static XanGraphNode *n = NULL;
	static loop_invocations_t *edgeData = NULL;

%option noyywrap
%option noinput
%option nounput
%option header-file="loops_invocations.h"
%option outfile="loops_invocations.c"
%option prefix="loops_invocations"

NUMBER	[0-9]+

%%

Node {
	state		= 1;
	edgeData	= NULL;
}

Edge {
	state	= 4;
}

{NUMBER}	{
	JITUINT64			num;
	XanGraphNode			*edgeNode;

	/* Fetch the number	*/
	num	= atoll(loops_invocationstext);

	/* Use the number	*/
	switch (state){
		case 1:
			minInv	= num;
			break;
		case 2:
			assert(loop != NULL);
			maxInv			= num;

			/* Fetch the node, which may	*
		 	 * have been created previously	*
			 * by some edge.		*/
			n	= LOOPS_getLoopInvocation(g, loop, minInv);
			if (n == NULL){
				l			= allocFunction(sizeof(loop_invocations_information_t));
				l->minInvocationsNumber	= minInv;
				l->loop			= allocFunction(sizeof(loop_profile_t));
				l->loop->loop		= loop;
				n			= xanGraph_addANewNode(g, l);
			}
			assert(n != NULL);

			/* Add the max invocations	*/
			l			= n->data;
			assert(l != NULL);
			l->maxInvocationsNumber	= maxInv;

			/* Erase variables		*/
			loop			= NULL;
			minInv			= 0;
			maxInv			= 0;
			break;
		case 4:
			assert(n != NULL);
			assert(l != NULL);
			assert(loop != NULL);

			/* Fetch the loop		*/
			edgeNode	= LOOPS_getLoopInvocation(g, loop, num);
			if (edgeNode == NULL){
				loop_invocations_information_t	*edgeL;

				/* Create the node		*/
				edgeL				= allocFunction(sizeof(loop_invocations_information_t));
				edgeL->minInvocationsNumber	= num;
				edgeL->loop			= allocFunction(sizeof(loop_profile_t));
				edgeL->loop->loop		= loop;
				edgeNode			= xanGraph_addANewNode(g, edgeL);
			}
			assert(edgeNode != NULL);

			/* Add the edge			*/
			edgeData			= allocFunction(sizeof(loop_invocations_t));
			edgeData->closed		= JITTRUE;
			edgeData->invocationsTop	= 0;
			edgeData->invocationsSize	= 1;
			edgeData->invocations		= allocFunction(sizeof(loop_invocations_range_t) * (edgeData->invocationsSize));
			xanGraph_addDirectedEdge(g, n, edgeNode, edgeData);
			break;
		case 5:
			assert(edgeData != NULL);

			/* Allocate space		*/
			if (edgeData->invocationsTop == edgeData->invocationsSize){
				(edgeData->invocationsSize)	+= 2;
				edgeData->invocations		= dynamicReallocFunction(edgeData->invocations, sizeof(loop_invocations_range_t) * (edgeData->invocationsSize));
			}
			assert(edgeData->invocationsTop < edgeData->invocationsSize);

			/* Fill up the data		*/
			edgeData->invocations[edgeData->invocationsTop].startInvocation	= num;
			break;
		case 6:
			assert(edgeData != NULL);
			assert(edgeData->invocationsTop < edgeData->invocationsSize);

			/* Add data			*/
			edgeData->invocations[edgeData->invocationsTop].adjacentInvocationsNumber	= (num + 1 - edgeData->invocations[edgeData->invocationsTop].startInvocation);
			assert(edgeData->invocations[edgeData->invocationsTop].adjacentInvocationsNumber > 0);
			break ;
		case 7:
			assert(edgeData != NULL);
			assert(edgeData->invocationsTop < edgeData->invocationsSize);

			/* Add data			*/
			edgeData->invocations[edgeData->invocationsTop].fatherInvocation		= num;
			(edgeData->invocationsTop)++;
		
			/* Erase variables		*/
			state				= 4;
			break;
		default:
			abort();
	}
	state++;
}

(.)+[^0-9\n ](.)+ {
	JITINT8 *buf;

	/* Fetch the name	*/
       	buf = (JITINT8 *) allocFunction(loops_invocationsleng + 1);
        SNPRINTF(buf, loops_invocationsleng+1, "%s", loops_invocationstext);

	/* Fetch the loop	*/
	loop	= LOOPS_getLoopFromName(loops, loopsNames, buf);
	if (loop == NULL){
		XanHashTableItem *item;
		fprintf(stderr, "Known loops:\n");
		item	= xanHashTable_first(loopsNames);
		while (item != NULL){
			fprintf(stderr, "%s\n", (char *)item->element);
			item	= xanHashTable_next(loopsNames, item);
		}
	}
	assert(loop != NULL);

	/* Free the memory	*/
        free(buf);
}

.|\n 		{ } 

%%

