/* File - token.cc
 * It implements the token class (see token.h)
 * @Vineet Garg, 7 Oct, 2011
 */

#include "token.h"

bool debug_on = false;
bool verbose_on = false;
bool ast_on = false;


//note - value is NULL but token_type is initialized to OTHER
/*
token::token():value(NULL), type(OTHER) {
	cout<<"Token is intilialzed to NULL & OTHER "<<endl;
	//TODO -insert debug information here
}
*/
//token value and toke type is set
token::token(string tvalue, tokenType t_type):value(tvalue), type(t_type) {
	if(debug_on || verbose_on){
		switch (t_type){
			case 0: cout<<" IDENTIFIER";
				break;
			
			case 1: cout<<" INTEGER";
				break;

			case 2: cout<<" STRING";
				break;

			case 3: cout<<" RESERVED_WORD";
				break;

			case 4: cout<<" OPERATOR";
				break;
			
			case 5: cout<<" PUNCTUATION";
				break;
		}

		cout<<" type of token is created with value: "<<value<<endl;

	}
}

//return the token's value
string token::get_value(){
	return value;
}


//return the token type
tokenType token::get_type(){
	return type;
}
