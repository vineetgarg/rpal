/* Last modified: Fri Apr 21 00:51:14 EDT 1995				   */
/* The following program takes an RPAL filename from the command line and, 
   depending upon the associated switches, will produces a listing and/or an
   abstract syntax tree and/or a (reduced) syntax tree. All trees conform to
   the standards outlined in the RPAL lexicon, phrase structure, and 
   transformatinal grammar, as modified by the handout and subsequent e-mail
   and in-class discussions.
   
   The overall program is divided into four parts in addition to this p1.c
   shell:

   token.c contains all the tokenization routines which take the file data and
   return a linked list of tokens. 

   build.c contains all the routines for a recursive decent parser based on the
   RPAL phrase structure on page 38 and 39 of the class notes.

   transform.c contains all the rotuines associated with transforms an AST to
   a ST, as defined by the notes on page 40 of the class notes.

   eval.c contains the CSE execution from the ST tree (project 2)

*/

#include "definitions.h"/* Constants, structures, and support routines */
#include "token.c"	/* Token construction */
#include "build.c"	/* AST construction */
#include "transform.c"  /* ST construction */
#include "eval.c"	/* RPAL evaluation */

int main(argc, argv)
 int argc;
 char *argv[];
{ 

 FILE *fp;
 char free_space[MAX_LINE_LENGTH+1];
 char *strptr;
 struct token *current,*incoming;
 struct tree *ast,*st;
 int print_listing=0, print_ast=0, print_st=0,no_out=0;
 int i;

 if (argc < 2) {
	printf("[Version 1.1 by Steven V. Walstra 4/21/95]\n\n");
	printf("Usage: %s [-l] [-ast] [-st] [-noout] filename\n\n",argv[0]);
	exit(-1);
 }

 for (i=2;i<argc;i++) {
    if (strcmp("-l",argv[i-1])==0) print_listing++;
    else if (strcmp("-st",argv[i-1])==0) print_st++;
    else if (strcmp("-ast",argv[i-1])==0) print_ast++;
    else if (strcmp("-db",argv[i-1])==0) debug3++;
    else if (strcmp("-db2",argv[i-1])==0) debug2++;
    else if (strcmp("-noout",argv[i-1])==0) no_out++;
 }

 if ((fp=fopen(argv[argc-1],"r"))==NULL) {
	printf("File \"%s\" not found!\n",argv[argc-1]);
	exit(-1);
 }

 if ((incoming=make_token())==TNULL) panic("No more memory for tokens!");
 current=head=incoming;
 strptr=free_space; 
 while(fgets(strptr,MAX_LINE_LENGTH,fp)!=NULL) { /* Read in lines */
	if (print_listing) printf("%s",strptr);  /* Print them if needed */
	line_num++;
 	while(*strptr!=10) { /* Tokenize the line, ends on an EOL (^J) */
		if ((makespace(strptr,incoming)) ||
 		    (makeidentifier(strptr,incoming)) ||
 		    (makestring(strptr,incoming)) ||
		    (makecomment(strptr,incoming)) ||
 		    (makeoperator(strptr,incoming)) ||
		    (makeinteger(strptr,incoming)) ||
		    (makedelimiter(strptr,incoming)) ) { 
			strptr=process(strptr,&head,&current,&incoming);}
		else { 
		     printf("Unable to parse near the following:\n");
		     printf("Line %d: %s\n",line_num,strptr);
		     exit(-1); 
		}
	}
 strptr=free_space;/* Start the pointer at the beginning of free-space again */
 }
 close(fp);

 head=trim(head); 			/* Remove DELETE nodes */
 ast=rpal(); 	 			/* Create the AST */
 if (print_ast) traverse_tree(ast,0);	/* Prints the AST */
 st=transform(ast);			/* Create the ST */
 if (print_st) traverse_tree(st,0);	/* Prints the ST */
 if (!no_out) eval(st);			/* Evaluates the ST */
 return(0);
}
