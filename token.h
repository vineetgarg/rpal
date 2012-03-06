/* File - token.h
 * It represents the structure of a token
 * @Vineet Garg, 7 Oct, 2011
 */

#ifndef TOKEN_H
#define TOKEN_H

#include<iostream>
#include<string>
#include<cstdlib>


using namespace std;

//define types of token, other includes punctuation, operator etc
enum tokenType  {IDENTIFIER, INTEGER, STRING, RESERVED_WORD, OPERATOR, PUNCTUATION, END_OF_FILE};


class token {

	string value; //value of token
	tokenType type;
  
public:
	//constructor
	//token();
	token(string, tokenType);

	//destructor
	//~token();

	//setters
	void set_value(string);
	void set_token_type(tokenType);

	//getters
	string get_value();
	tokenType get_type();
};

#endif 
