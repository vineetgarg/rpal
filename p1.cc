/* File - p1.cc
 * Entry point for the RPAL parser
 * @Vineet Garg, 7 Oct, 2011
 */

#include "parser.h"
#include <cstdlib>

int main(int argc, char *argv[] ) {

	char *filename, *deb_level;
	DEBUG_LEVEL user_level;

	if( argc < 2 ){
		cout<<"Usage: ./p1 [-ast][-debug][-verbose] <filename>"<<endl;
		exit(0);
	}


	if( argc == 2 ) {//p1 filename
		filename = argv[1];
		user_level = off;
	}
	
	else if( argc == 3) {//p1 -flag filename
		filename = argv[2];

		if( ((string)argv[1]).compare("-verbose") == 0 ) //if flag is verbose
			user_level = verbose;
		else if( ((string)argv[1]).compare("-debug") == 0 )
			user_level = debug;
		else
			user_level = ast;

	}

	else {
		cout<<"Usage: ./p1 [-ast][-debug][-verbose] <filename>"<<endl;
		exit(0);
	}
	

	

	Parser parserObj( (string)filename );
	parserObj.parser(user_level);

}
