/*File - parser.cc
 *Implementation of parser's functionality
 @Vineet Garg
 */

#include"parser.h"

//reserved words
string reserved_words[] = {"let", "where", "in", "fn", "and", "rec", "within", "aug", "not", "or", "gr", "ge", "ls", "le", "eq", "ne", "true", "false", "nil", "dummy" };

stack<AST_node*> AST_stack; //stack to hold temp AST nodes

//constructor for control structure data struct
control_st::control_st():node(NULL), next(NULL), delta(NULL), bound_var(NULL), env(NULL), num_children(0) {}

//destructor for control structure
control_st::~control_st(){
	//I don't think I need memory management as of now
/*	if( !node)
		delete node;
	if( !next)
		delete next;
	if( !delta)
		delete delta;
	if( !bound_var)
		delete bound_var;
	//may be later if i feel like doing it and have some time in hand */
}

void control_st::set_num_children(int num_child){ num_children = num_child ; }

int control_st::get_num_children(){ return num_children ; }

environment::environment():parent(NULL) {}

/* Initialize Control & Execution stack
 * Put initial environment in both of the stacks
 * Load first control_stack in CONTROL STACK */
CSE_machine::CSE_machine(control_st *cntrl_st){
	if(debug_on || verbose_on)
		cout<<"Initialize CSE_machine........"<<endl;
	
	//create primitive environment
	current_env = new environment();
	i=0;
	
	//We need to put environment first of all in both stacks
	//create new execution_element of type environment
	execution_element *temp_exe_ele = new execution_element; //this is for EXECUTION ELEMENT
	temp_exe_ele->type = EXE_ENV;
	temp_exe_ele->env_var =  current_env;
	execution_stack.push(temp_exe_ele);

	//create new control struct for CONTROL STACK
	AST_node *new_ast_node = new AST_node;
	new_ast_node->set_node_type(ENV);

	control_st *new_control_st = new control_st;
	new_control_st->node = new_ast_node; //this node will tell the type of control struct in execution
	new_control_st->env = current_env; //this stores the environment
	control_stack.push(new_control_st);

	if(verbose_on)
		cout<<"CSE_machine - Initialization - Pushed environment on both of the stacks"<<endl;
	//push all control stack elements in control stack
	while(cntrl_st != NULL){
		if(debug_on || verbose_on){
			cout<<"Pushing on control stack: ";  print_ast_node(cntrl_st->node);
		}

		control_stack.push(cntrl_st);
		cntrl_st = cntrl_st->next;
	}

	if(debug_on || verbose_on)
		cout<<"CSE_machine initialization...........[done]"<<endl;

}

//It initialize the parser by opening the input file
//It initialize the next_token by calling scan()
Parser::Parser(string filename):saved_char(' '), ast_tree(NULL) {
	if(debug_on || verbose_on)
		cout<<"Initializing parser with filename: "<<filename<<endl;

	input_file.open(filename.c_str(), ifstream::in); //open file in input mode only

	if(input_file.fail() ){
		cout<<"ERROR - not able to open file: "<<filename<<" !!!"<<endl;
		exit(0);
	}

	next_token =scan();

	//if(debug_on || verbose_on)


}

//TODO
Parser::~Parser() {}

/* Initializes AST_node left children and right sibling */
AST_node::AST_node(): left_child(NULL), right_sibling(NULL) {}

/* PENDING */
AST_node::~AST_node() {}

/* set the AST_node's value */
void AST_node::set_node_value(string node_value){ this->node_value = node_value; }

/* return the AST_node's value */
string AST_node::get_node_value() {return node_value ; }

/* set AST node's type */
void AST_node::set_node_type(AST_node_type node_type) { this->node_type = node_type ; }

/* return the AST_node's type */
AST_node_type AST_node::get_node_type() { return node_type ; }

/* public function to begin the parsing */
void Parser::parser(DEBUG_LEVEL level) { 
	if(level == verbose)
		verbose_on = true;
	else if (level == debug)
		debug_on = true;
	else if (level == ast)
		ast_on = true;
	
	if(debug_on || verbose_on)
		cout<<"Beginging parsing............"<<endl;

	E(); //call the starting grammar procedure

	//next_token should be EOF
	if( !(next_token->get_type() == END_OF_FILE) )
		cerr<<"Expected END OF FILE but received : '"<<next_token->get_value()<<"'";

	//cout<<"YAYYYYY !!! Stack size: "<<AST_stack.size()<<endl;
	if(verbose_on)
		cout<<"Parsing done....will traverse the tree now...."<<endl;

	if(verbose_on || debug_on || ast_on || off) {
		ast_tree = AST_stack.top();
	
		/*cout<<"-----------------------------AST----------------------------------->"<<endl;
		preorder_traversal(ast_tree, 0);
		
		cout<<"-----------------------------STANDARIZED AST----------------------------------->"<<endl; */
		standardize_tree(ast_tree);
		
		if(ast_on)
			preorder_traversal(ast_tree, 0);

		//make a copy of CS
		//control_st **CS_copy = &CS;
		//CS = new control_st;
		generate_control_struct(ast_tree, &CS); //we have control struc in CS.next
		if(verbose_on || debug_on){
			print_control_struct(CS.next);
			getchar();
		}
		CSE_machine cse_machine(CS.next);
		cse_machine.execute();
		cout<<endl;
	}
}

/* Starting grammar rule of parser */
void Parser::E() {
	if(verbose_on)
		cout<<"Starting with procedure E.."<<endl;
	
	if( next_token->get_value().compare("let") == 0 && next_token->get_type()== RESERVED_WORD){ //if next_token is 'let'
		read("let");
		D();
		read("in");
		E();

		build_tree(LET, "let", 2);
	}

	else if( next_token->get_value().compare("fn") == 0 && next_token->get_type()== RESERVED_WORD) { //if next_token is 'fn'
		read("fn");
		
		int n=0;
		while(( ( (next_token->get_value()).compare("(") == 0) && next_token->get_type()== PUNCTUATION) || next_token->get_type() == IDENTIFIER ) {
			n++;
			Vb();
		}
		read(".");
		E();
		build_tree(LAMBDA, "lambda", n+1);
	}

	else
		Ew();

	if(verbose_on)
		cout<<"E - E rule ends.............."<<endl;

}


//Ew() grammar rule
void Parser::Ew() {
	if(verbose_on)
		cout<<"Applying Ew grammar rule.........."<<endl;

	T();
	if( (next_token->get_value()).compare("where") == 0 && next_token->get_type()== RESERVED_WORD) { //if nt is 'where'
		read("where");
		Dr();
		build_tree(WHERE, "where", 2);
	}

	if(verbose_on)
		cout<<"Ew rule end............."<<endl;
}


//T() grammar rule
void Parser::T() {
	if(verbose_on)
		cout<<"Applying T grammar rule............."<<endl;

	Ta();
	int n=0;
	while( (next_token->get_value()).compare(",") == 0 && next_token->get_type()== PUNCTUATION ){
		n++; //to keep track of nodes
		read(",");
		Ta();
	}

	if( n>0 )
		build_tree(TAU, "tau", n+1); 

	if(verbose_on)
		cout<<"T rule end............."<<endl;
}


//Ta() grammar rule
void Parser::Ta() {
	if(verbose_on)
		cout<<"Applying Ta() rule..........."<<endl;

	Tc();
	while( (next_token->get_value()).compare("aug") == 0 && next_token->get_type() == RESERVED_WORD){ //as long as nt is 'aug'
		read("aug");
		Tc();
		build_tree(AUG, "aug", 2);
	}

	if(verbose_on)
		cout<<"Ta rule end............."<<endl;
}

//Tc() grammar rule
void Parser::Tc() {
	if(verbose_on)
		cout<<"Applying Tc() rule....."<<endl;

	B();
	if( (next_token->get_value()).compare("->") == 0 && next_token->get_type() == OPERATOR) { //if nt is '->'
		read("->");
		Tc();
		read("|");
		Tc();
		build_tree(COND, "->", 3);
	}

	if(verbose_on)
		cout<<"Tc rule end............."<<endl;
}


//B grammar rule
void Parser::B() {
	if(verbose_on)
		cout<<"Applying B grammar rule............"<<endl;

	Bt();
	while( (next_token->get_value()).compare("or") == 0 && next_token->get_type() == RESERVED_WORD) { //as long as nt is 'or'
		read("or");
		Bt();
		build_tree(OR, "or", 2);
	}

	if(verbose_on)
		cout<<"B rule end............."<<endl;
}


//Bt grammar rule
void Parser::Bt() {
	if(verbose_on)
		cout<<"Applying Bt grammar rule.............."<<endl;

	Bs();
	while( (next_token->get_value()).compare("&") == 0 && next_token->get_type() == OPERATOR){ //as long as next token is '&'
		read("&");
		Bs();
		build_tree(AMP, "&", 2);
	}

	if(verbose_on)
		cout<<"Bt rule end............."<<endl;
}


//Bs grammar rule
void Parser::Bs() {
	if(verbose_on)
		cout<<"Applying Bs grammar rule.............."<<endl;

	if( (next_token->get_value()).compare("not") == 0 && next_token->get_type() == RESERVED_WORD ){
		read("not");
		Bp();
		build_tree(NOT, "not", 1);
	}

	else
		Bp();


	if(verbose_on)
		cout<<"Bs rule end............."<<endl;
}


//Bp grammar rule
void Parser::Bp() {
	if(verbose_on)
		cout<<"Applying Bp grammar rule............."<<endl;

	A();
	
	if( ( (next_token->get_value()).compare("gr") == 0 && next_token->get_type() == RESERVED_WORD ) 
			|| ( (next_token->get_value()).compare(">") == 0 && next_token->get_type() == OPERATOR ) ){
		string temp_string = next_token->get_value();
		read(temp_string);
		A();
		build_tree(GR, "gr", 2);
	}

	else if( ( (next_token->get_value()).compare("ge") == 0 && next_token->get_type() == RESERVED_WORD )
			|| ( (next_token->get_value()).compare(">=") == 0 && next_token->get_type() == OPERATOR)){
		string temp_string = next_token->get_value();
		read(temp_string);
		A();
		build_tree(GE, "ge", 2);
	}

	else if( ( (next_token->get_value()).compare("ls") == 0 && next_token->get_type() == RESERVED_WORD ) 
			|| ( (next_token->get_value()).compare("<") == 0 && next_token->get_type() == OPERATOR)){
		string temp_string = next_token->get_value();
		read(temp_string);
		A();
		build_tree(LS, "ls", 2);
	}
	
	else if(( (next_token->get_value()).compare("le") == 0 && next_token->get_type() == RESERVED_WORD )
			|| ( (next_token->get_value()).compare("<=") == 0 && next_token->get_type() == OPERATOR)) {
		string temp_string = next_token->get_value();
		read(temp_string);
		A();
		build_tree(LE, "le", 2);
	}

	else if( (next_token->get_value()).compare("eq") == 0 && next_token->get_type() == RESERVED_WORD ){
		string temp_string = next_token->get_value();
		read(temp_string);
		A();
		build_tree(EQ, "eq", 2);
	}

	else if( (next_token->get_value()).compare("ne") == 0 && next_token->get_type() == RESERVED_WORD){
		string temp_string = next_token->get_value();
		read(temp_string);
		A();
		build_tree(NE, "ne", 2);
	}


	if(verbose_on)
		cout<<"Bp rule end............."<<endl;
}

//A grammar rule
void Parser::A() {
	if(verbose_on)
		cout<<"Applying A rule....."<<endl;

	if( (next_token->get_value()).compare("+") == 0 && next_token->get_type() == OPERATOR ){ //if nt is '+'
		read("+");
		At();
	}

	else if( (next_token->get_value()).compare("-") == 0 && next_token->get_type() == OPERATOR ){ //if nt is '+'
		read("-");
		At();
		build_tree(NEG, "neg", 1);
	}

	else{ //ACCORDING TO PROF WHILE SHOULD BE OUT OF ELSE  !!!
		At(); 
	}
	
	while( ((next_token->get_value()).compare("+") == 0 && next_token->get_type() == OPERATOR ) 
			|| ( (next_token->get_value()).compare("-") == 0 && next_token->get_type() == OPERATOR  )) {
		string temp_string = next_token->get_value();
		read(temp_string);
		At();
		if(temp_string.compare("+") == 0 )
			build_tree(PLUS, temp_string, 2);
		else
			build_tree(MINUS, temp_string, 2);
	}

	

	if(verbose_on)
		cout<<"A rule end............."<<endl;
}

//At grammar rule
void Parser::At() {
	if(verbose_on)
		cout<<"Applying At rule........."<<endl;

	Af();
	while( ((next_token->get_value()).compare("*") == 0 && next_token->get_type() == OPERATOR ) 
			|| ( (next_token->get_value()).compare("/") == 0 && next_token->get_type() == OPERATOR) ) {
			string temp_string = next_token->get_value();
			read(temp_string);
			Af();
			if( temp_string.compare("*") == 0) 
				build_tree(MULTI, temp_string, 2);
			else
				build_tree(DIV, temp_string, 2);
	}


	if(verbose_on)
		cout<<"At rule end............."<<endl;
}

void Parser::Af() {
	if(verbose_on)
		cout<<"Applying Af rule............."<<endl;

	Ap();
	if( (next_token->get_value()).compare("**") == 0 && next_token->get_type() == OPERATOR ){
		read("**");
		Af();
		build_tree(EXPO, "**", 2);
	}

	if(verbose_on)
		cout<<"Af rule end............."<<endl;
}

void Parser::Ap() {
	if(verbose_on)
		cout<<"Applying Ap rule.........."<<endl;
	R();

	while((next_token->get_value()).compare("@") == 0 && next_token->get_type() == OPERATOR ){
		read("@");
		read(IDENTIFIER);
		R();
		build_tree(AT, "@", 3);
	}

	if(verbose_on)
		cout<<"Ap rule end............."<<endl;
}


//R grammar rule
void Parser::R() {
	if(verbose_on)
		cout<<"Applying R rule.............."<<endl;
	Rn();
	while( ((next_token->get_value()).compare("true") == 0 && next_token->get_type() == RESERVED_WORD )
			|| ((next_token->get_value()).compare("false") == 0 && next_token->get_type() == RESERVED_WORD )
			|| ((next_token->get_value()).compare("nil") == 0 && next_token->get_type() == RESERVED_WORD )
			|| ((next_token->get_value()).compare("(") == 0 && next_token->get_type() == PUNCTUATION)
			|| ((next_token->get_value()).compare("dummy") == 0 && next_token->get_type() == RESERVED_WORD)
			|| next_token->get_type() == IDENTIFIER 
			|| next_token->get_type() == STRING 
			|| next_token->get_type() == INTEGER 
			
			) {
		Rn();
		build_tree(GAMMA, "gamma", 2);
	}


	if(verbose_on)
		cout<<"R rule end............."<<endl;
}

//Rn grammar rule
void Parser::Rn() {
	if(verbose_on)
		cout<<"Applying Rn rule.............."<<endl;

	if( next_token->get_type() == IDENTIFIER )
		read(IDENTIFIER);

	else if( next_token->get_type() == INTEGER)
		read(INTEGER);

	else if( next_token->get_type() == STRING)
		read(STRING);

	else if( (next_token->get_value()).compare("true") == 0 && next_token->get_type() == RESERVED_WORD) {
		read("true");
		build_tree(TRUE, "true", 0); //passing 0 because read('true') will not create any node but we need node for 'true'
	}

	else if( (next_token->get_value()).compare("false") == 0 && next_token->get_type() == RESERVED_WORD) {
		read("false");
		build_tree(FALSE, "false", 0);
	}

	else if( (next_token->get_value()).compare("nil") == 0 && next_token->get_type() == RESERVED_WORD) {
		read("nil");
		build_tree(NIL, "nil", 0);
	}

	else if( (next_token->get_value()).compare("(") == 0 && next_token->get_type() == PUNCTUATION) {
		read("(");
		E();
		read(")");
	}

	else {
		read("dummy");
		build_tree(DUMMY, "dummy", 0);
	}


	if(verbose_on)
		cout<<"Rn rule end............."<<endl;
}

//D() grammar rule
void Parser::D() {
	if(verbose_on)
		cout<<"Applying D rule..."<<endl;
	Da();

	if( next_token->get_value().compare("within") == 0 && next_token->get_type() == RESERVED_WORD) { //if next_token is 'within'
		read("within");
		D();
		build_tree(WITHIN, "within", 2);
	}


	if(verbose_on)
		cout<<"D rule end............."<<endl;
}

//Da grammar rule
void Parser::Da() {
	if(verbose_on)
		cout<<"Applying Da rule..."<<endl;
	Dr();
	int n=0;

	while( next_token->get_value().compare("and") == 0 && next_token->get_type() == RESERVED_WORD) { //while next token is '('
		n++;
		read("and");
		Dr();
	}

	if( n>0 )
		build_tree(AND, "and", n+1 ); 


	if(verbose_on)
		cout<<"Da rule end............."<<endl;
}


//Dr grammar rule
void Parser::Dr() {
	if(verbose_on)
		cout<<"Applying Dr rule....."<<endl;

	if( next_token->get_value().compare("rec") == 0 && next_token->get_type() == RESERVED_WORD) { //if next_token is 'rec'
		read("rec");
		Db();
		build_tree(REC, "rec", 1);
	}

	else
		Db();


	if(verbose_on)
		cout<<"Dr rule end............."<<endl;
}
//Db grammar rule
void Parser::Db() {
	if(verbose_on)
		cout<<"Applying Db rule.........."<<endl;

	if( (next_token->get_value()).compare("(") == 0 && next_token->get_type() == PUNCTUATION) { //if next_token is '('
		read("(");
		D();
		read(")");
	}

	else if ( next_token->get_type() == IDENTIFIER ) { //if next_token is identifier
		read(IDENTIFIER); //will call the overloaded version
		
		if ( (( next_token->get_value() ).compare(",") == 0  && next_token->get_type() == PUNCTUATION)
				|| (( next_token->get_value() ).compare("=") == 0 && next_token->get_type() == OPERATOR ) 
				) { //Vl '=' E is applied
			Vl(); //Vl will not read the first IDENTIFIER which is already read by Db() above
			read("=");
			E();

			build_tree(EQUAL, "=", 2);

		}

		else{//IDENTIFIER is alread read	
			int n=0;
			while( ( (next_token->get_value().compare("(") == 0) && next_token->get_type() == PUNCTUATION )
					|| next_token->get_type() == IDENTIFIER ) {
				n++;
				Vb();
			}

			read("=");
			E();
			build_tree(FUN_FORM, "fcn_form", n+2);
		}
	}

	if(verbose_on)
		cout<<"Db rule end............."<<endl;
}

//Vl grammar rule
//Keep in mind to read IDENTIFIER always before calling Vl()
void Parser::Vl() {
	if(verbose_on)
		cout<<"Applying Vl rule...."<<endl;

	//don't read identifier because it is already read by Db but do take it into consideration while building tree
	int n=1; //1 is for indentifier
	while( (next_token->get_value()).compare(",") == 0 && next_token->get_type() == PUNCTUATION){ //as long as there is ',' operator
		n++;
		read(",");
		read(IDENTIFIER);
	}
	
	if( n > 1)
		build_tree(COMMA, ",", n);

	if(verbose_on)
		cout<<"Vl rule end............."<<endl;
}

//Vb grammar rule
void Parser::Vb() {
	if(verbose_on)
		cout<<"Appling Vb rule...."<<endl;
	if( next_token->get_type() == IDENTIFIER ) { //
		read(IDENTIFIER);
	}

	else { //next_token is '('
		read("(");

		if( (next_token->get_value() ).compare(")") == 0 && next_token->get_type() == PUNCTUATION) { 
			read("(");
			build_tree(PARAN, "()", 2);
		}

		else{ //next_token is IDENTIFIER
			read(IDENTIFIER);
			Vl();
			read(")");
		}
	}

	if(verbose_on)
		cout<<"Vb rule end............."<<endl;
}






/* Parse the given input file to tokenize the file
 * There are four types of token - IDENTIFIER, INTEGER, STRING & OTHER
 * Limitations - Do not take care of ''
 */
token* Parser::scan() {
	char c;
	if(verbose_on)
		cout<<"\n\nBegining scanning of file....."<<endl;

	if( saved_char == ' ')
		c = input_file.get();
	else
		c = saved_char; //scanner might have already scanned a character in previous scanning; in the very first call value of saved_char is " "

	char temp_char_holder[256]; //to temporarily store string or identifier

	while (true) {

		//ignore all white spaces
		while (c == ' ' || c == '\t' || c == '\n' ) {
			
			/*if(verbose_on)
				cout<<"Ignoring white space - received "<<c<<" just now"<<endl; */

			c = input_file.get();
			
			//if EOF occurred while scanning
			/*if (c == EOF) {
				cout<<"ERROR - received EOF while ignoring white space !!"<<endl;
				exit(0);
			}*/
		}

		//handle identifier
		if( (c >= 65 && c <= 90) || (c >=97 && c <= 122) ) { //identifier begins with a-z, A-Z
			if(verbose_on)
				cout<<"Indentifier found..."<<endl;

			int pos = 0;
			temp_char_holder[pos]= c;
			c = input_file.get(); //get next char

			while( (c == 95) || (c >= 65 && c <= 90) || (c >=97 && c <= 122) || (c >=48 && c <=57) ){ //as long as the char is _, a-z, A-Z or 0-9
				pos++;
				temp_char_holder[pos] = c;

				c = input_file.get();
			}

			temp_char_holder[++pos] = '\n';

			string token_string(temp_char_holder, pos); //create string

			if(verbose_on)
				cout<<"Idenifier token created: "<<token_string<<endl;
			
			//there is a char in c; save it
			saved_char = c;
			
			//find if the identifier is a reserved word
			int size_of_words = sizeof(reserved_words)/sizeof(string);

			if(verbose_on)
				cout<<"Size of reserved words array: "<<size_of_words<<endl;
			
			bool match = false;
			for(int i=0; i<size_of_words; i++) { //find the indentifier in reserved word array
				if( reserved_words[i].compare(token_string) == 0 ) {//if identifier found
					if(verbose_on)
						cout<<"Match found..it is a reserved word"<<endl;
					match = true;
					break;
				}

			}
			//create new token
			token *identifier_token = NULL;
			if(match)
				identifier_token = new token(token_string, RESERVED_WORD);
			else
				identifier_token = new token(token_string, IDENTIFIER);

			return identifier_token;
		}



		//handle digits
		if( c >=48 && c<=57) { //we got integer
			if(verbose_on)
				cout<<"Integer encountered..."<<endl;

			int pos=0;

			temp_char_holder[pos] = c;

			c = input_file.get();

			while( c>=48 && c<=57 ) { //get all the digits
				pos++;
				temp_char_holder[pos] = c;
				c = input_file.get();
			}
			
			temp_char_holder[++pos] = '\n' ;
			string int_string(temp_char_holder, pos); //create integer string

			if(verbose_on)
				cout<<"Integer token created: "<<int_string<<endl;;

			saved_char = c;

			//create new token
			token *int_token = new token(int_string, INTEGER);
			return int_token;
		}
		
		//Handle '**' operator 
		if( c=='*' ) {
			if(verbose_on)
				cout<<"Encountered * operator..."<<endl;

			if( (c=input_file.get()) == '*' ) {// '**' operator
				//create ** token
				string temp_string(2, c); //create ** string
				token *temp_token = new token(temp_string, OPERATOR);
				return temp_token;
			}
			else{ //we have * token
				saved_char = c; //save the already scanned character
				string temp_string(1, '*'); //create * string
				token *temp_token = new token(temp_string, OPERATOR);
				return temp_token;
				//create * token
			}
		}
		
		//Handle '>=' operator
		if( c=='>' || c=='<' ) {
			if(verbose_on)
				cout<<"Encountered "<<c<<" operator..."<<endl;

			char temp_char = c;
			if( (c=input_file.get()) == '=' ) { // '>=' or '<=' operator
				//create ** token
				if( temp_char == '>' ){
					saved_char = ' ';
					token *temp_token = new token(">=", OPERATOR);
					return temp_token;
				}

				else {
					saved_char = ' ';
					token *temp_token = new token("<=", OPERATOR);
					return temp_token;
				}
			}
			else{ //we have either '>' or '<'  token
				saved_char = c; //save the already scanned character 
				string temp_string(1, temp_char); //create * string
				token *temp_token = new token(temp_string, OPERATOR);
				return temp_token;
				//create * token
			}
		}

		//handle -> operator
		if( c=='-' ) {
			if(verbose_on)
				cout<<"Encountered "<<c<<" operator..."<<endl;

			char temp_char = c;
			if( (c=input_file.get()) == '>' ) { // -> operator
				//create ** token
				saved_char = ' ';
				token *temp_token = new token("->", OPERATOR);
				return temp_token;
			}

			else {
				saved_char = c;

				string oper_string(1, temp_char);
				token *temp_token = new token(oper_string, OPERATOR);
				return temp_token;
			}
			
			
		}

		//look for operator symbols or punctuation
		/* Note that ', / operators are handled above */
		if( c=='+' || c=='-'|| c=='*'|| c=='<'|| c=='>' || c=='&' || c=='.' || c=='@' || c==':' || c=='=' || c=='~'|| c=='|'|| c=='$'|| c=='!' || c=='#' || c=='%' || c=='^' || c=='_' || c=='[' || c==']'|| c=='{'|| c=='}'|| c=='"' || c=='?' ) { //remember we have already handled '/' operator
			if(verbose_on)
				cout<<"Received OPERATOR SYMBOL or PUNCTUATION: "<<c<<endl;
			saved_char = ' ';	
			string temp_string(1, c); //create a temp string for characater, string is being copied(deep) in token so don't worry about memory
			token *temp_token = new token(temp_string, OPERATOR);
			return temp_token;

		}


		//handle strings
		//PENDING - double quotes what to do ?
		//LIMITATION - As of now it continousily scans for end quote even if there is an end of line. So if it doesn't encounter end quote it will keep going on and will encountered end of file and throw an error
		if( c=='\'' ) {
			if(verbose_on)
				cout<<"Encountered the beginging of a string. Scanning....."<<endl;

			c = input_file.get(); //get the next character TODO - this could be the end of file
			int pos = 0; //to keep track of no of characters in string
			
			
			//get the string in temp_string
			while( c != '\'' ){
				
				//c could be '\'
				//scan next c and check if it is single quote or not
				//if it is single quote ignore the '\' and save the single quote
				//else if it isn't save the '\'
				if( c == '\\'){
					char temp_char = c;
					c = input_file.get();

					if( c != '\''){ //it isn't single quote
						//save the back slash
						temp_char_holder[pos++] = temp_char;
					//	temp_char_holder[pos++] = c;
					//	c = input_file.get();
					}
					else{ //it is single quote
						temp_char_holder[pos++] =  c; //ignore the back slash and save the quote
						c = input_file.get();
					}
				}

				else{ //it isn't back slash
					temp_char_holder[pos++] = c;
					c = input_file.get(); //TODO - EOF
				}

				if( c == EOF){
					cout<<"ERROR - while scanning for string end quotes weren't found !"<<endl;
					exit(0);
				}


			}
			temp_char_holder[pos] = '\n'; //to end the string

			string convert_string ( temp_char_holder, pos );

			if(verbose_on)
				cout<<"Scanned string is: "<<convert_string<<endl;
			
			saved_char =  ' ';

			token *temp_token = new token( convert_string, STRING);
			return temp_token;
			
		}

		//handle punctuations
		if( c=='(' || c==')' || c==';' || c==',' ) {
			if(verbose_on)
				cout<<"Punctuation encountered..."<<endl;

			saved_char = ' ';	
			string temp_string(1, c); //create a temp string for characater, string is being copied(deep) in token so don't worry about memory
			token *temp_token = new token(temp_string, PUNCTUATION);
			return temp_token;

		}

		//look for comments
		if( c == '/' ){
			c = input_file.get();
			if( c == '/'){ //we have got comment here
				if(verbose_on)
					cout<<"Encountered comment. Ignoring..."<<endl;
				while( c != '\n' )  //ignore the current line
					c = input_file.get(); //TODO - what if we get an EOF here
			}

			else{ //it was / operator
				saved_char = c;

				//create token for /
				token *temp_token = new token( (string)"/", OPERATOR);

				return temp_token;
			}

		}

		//tokenize end of file
		if( c == EOF) {
			if(verbose_on)
				cout<<"Scanning: EOF encountered.."<<endl;

			token *eof_token = new token("", END_OF_FILE);
			return eof_token;
		}

		//we have non-white space & non-comment character here

		
		
	
		//Handle Digits
		/*if ( c=='0' || c=='1' || c=='2' || c=='3' || c=='4' || c=='5' || c=='6' || c=='7' || c=='8' || c=='9' ){
			if(debug_on)
				cout<<"Digit encountered..."<<endl;
			int pos = 0;
			temp_char_holder[pos] = c;

			while


		}*/


	}

	//while( (c = input_file.get() ) != EOF )
	//	cout<<c;
}


/* read(string) - it compare the token with next_token. If they don't match it throws the error and exit the system
 * if they match and are either integer, identifier or string it creates a node. It then scans the next token and put it in next token
 */
void Parser::read(string token_value) {
	if(verbose_on)
		cout<<"Reading "<<token_value<<" token"<<endl;

	//compare the token
	//if don't match throw error and exit
	if(!((next_token->get_value()).compare(token_value) == 0 )) { //if they match
		cout<<"ERROR - Expected '"<<token_value<<"' but received '"<<next_token->get_value()<<"' .Exiting...."<<endl;
		exit(0);
	}

	else{ 
		if(verbose_on )
			cout<<token_value<<" and "<<next_token->get_value()<<" matches as expected "<<endl;

		//if identifier, string or intger build a node
		if( next_token->get_type()==IDENTIFIER || next_token->get_type()==INTEGER || next_token->get_type()==STRING ){
			if(verbose_on || debug_on)
				cout<<"Creating node for token: "<<token_value<<endl;

			//call build_tree
		}
	}

	delete next_token;
	next_token = NULL;
	//scan the next token in next_token;
	next_token = scan();

	if(verbose_on)
		cout<<"Scanned next token: "<<next_token->get_value()<<endl;
}


/* read(tokenType) - it compare the token with next_token. If they don't match it throws the error and exit the system
 * if they match and are either integer, identifier or string it creates a node. It then scans the next token and put it in next token
 */
void Parser::read(tokenType token_type) {
	if(verbose_on)
		cout<<"Reading token type: "<<token_type<<endl;

	//compare the token
	//if don't match throw error and exit
	if(!((next_token->get_type()) == token_type )) { //if they don't match
		cout<<"ERROR - Expected "<<token_type<<" but received "<<next_token->get_type()<<" ! Exiting...."<<endl;
		exit(0);
	}

	else{ 
		if(verbose_on )
			cout<<token_type<<" and "<<next_token->get_type()<<" matches as expected "<<endl;

		//if identifier, string or intger build a node; this check is reduntant because this fun will be called only in these cases
		if( next_token->get_type()==IDENTIFIER || next_token->get_type()==INTEGER || next_token->get_type()==STRING ){
			if(verbose_on || debug_on)
				cout<<"Creating node for token: "<<next_token->get_value()<<endl;

			//call build_tree
			if( next_token->get_type()==IDENTIFIER )
				build_tree(IDT, next_token->get_value(), 0);

			else if( next_token->get_type()==STRING)
				build_tree(STR, next_token->get_value(), 0);

			else if( next_token->get_type()==INTEGER)
				build_tree(INT, next_token->get_value(), 0);
			
			delete next_token;
			next_token = NULL;
		}
	}


	//scan the next token in next_token;
	next_token = scan();

	if(verbose_on)
		cout<<"Scanned next token: "<<next_token->get_value()<<endl;
}

/*build_tree(node_type, node_value, num_node) - It pop num_node nodes from stack and create a new node
 with popped node as children. It then pushes back the newly created node
 */
void Parser::build_tree(AST_node_type node_type, string node_value, int num_node) {
	if(verbose_on || debug_on)
		cout<<"Building tree with node type: "<<node_type<<", value: "<<node_value<<" and number of nodes: "<<num_node<<endl;

	//create new node
	AST_node *new_node = new AST_node;

	//set type & value of new node
	new_node->set_node_type(node_type);
	new_node->set_node_value(node_value);

	//create temp AST_node to hold temp node
	AST_node *temp_node = NULL;

	//for i 1 to n pop and create sibling list
	for(int i=1; i<=num_node; i++){
		AST_node *saved_popped = AST_stack.top(); //save the popped node
		AST_stack.pop();

		if(verbose_on || debug_on)
			cout<<"build_tree - node popped with value: "<<saved_popped->get_node_value()<<" and type: "<<saved_popped->get_node_type()<<endl;

		if( temp_node != NULL) {
			saved_popped->right_sibling = temp_node;
			temp_node = saved_popped;
		}

		else{ //first popped node
			temp_node = saved_popped;
		}
	}

	//set new node's left children
	new_node->left_child = temp_node;

	//push the new node into stack
	AST_stack.push(new_node);

	if(debug_on || verbose_on)
		cout<<"build_tree - size of stack: "<<AST_stack.size()<<endl;
}

/*void print_dots(int n) {
	while(n > 0 ){
		cout<<".";
		n--;
	}
}*/

void Parser::preorder_traversal(AST_node *tree, int num_dots){
	AST_node *temp = tree;
	for(int i=1;i<=num_dots; i++)
		cout<<".";
	//print_dots(num_dots);

	print_AST_node(tree);
	if( temp->left_child != NULL){
		preorder_traversal(temp->left_child, num_dots+1);
	}
	if( temp->right_sibling != NULL)
		preorder_traversal(temp->right_sibling, num_dots);
}

/* prints the content of node based on its type */
void Parser::print_AST_node(AST_node *node) {
	if(node->get_node_type() == STR)
		cout<<"<STR:'"<<node->get_node_value()<<"'>"<<endl;
	else if(node->get_node_type() == IDT)
		cout<<"<ID:"<<node->get_node_value()<<">"<<endl;
	else if(node->get_node_type() == INT)
		cout<<"<INT:"<<node->get_node_value()<<">"<<endl;
	else if(node->get_node_type() == YSTAR)
		cout<<"<"<<node->get_node_value()<<">"<<endl;
	else if(node->get_node_type() == FUN_FORM)
		cout<<"function_form"<<endl;
	else if(node->get_node_type() == TRUE)
		cout<<"<true>"<<endl;
	else if(node->get_node_type() == FALSE)
		cout<<"<false>"<<endl;
	else if(node->get_node_type() == DUMMY)
		cout<<"<dummy>"<<endl;
	else if(node->get_node_type() == NIL)
		cout<<"<nil>"<<endl;
	else
		cout<<node->get_node_value()<<endl;
}

/* This procedure will tranverse the AST tree in post-order fashion standardizing each node */
AST_node* Parser::standardize_tree(AST_node *node){ //this is to standardized let node 
	if(node->left_child != NULL )
		standardize_tree(node->left_child);	
	if( node->right_sibling != NULL )
		standardize_tree(node->right_sibling);
	if( node->get_node_type() == LET)
		standardize_let(node);

	else if( node->get_node_type() == WHERE)
		standardize_where(node);

	else if( node->get_node_type() == FUN_FORM)
		standardize_funform(node);

	else if( node->get_node_type() == WITHIN)
		standardize_within(node);

	else if( node->get_node_type() == AND)
		standardize_and(node);

	else if( node->get_node_type() == AT)
		standardize_at(node);

	else if( node->get_node_type() == REC)
		standardize_rec(node);
}	



/* standardized LET node
 * note - it do no create new node. It only standarized the exisiting node and return it
 */
AST_node* Parser::standardize_let(AST_node *& let_node){

	if(let_node == NULL){
		cout<<"standardized_let - node is NULL !!"<<endl;
		exit(0);
	}
	if(verbose_on|| debug_on) {
		cout<<"standardized_let - standardizing let node......."<<endl;
		cout<<"node_type: "<<let_node->get_node_type()<<endl;
		cout<<"node_value: "<<let_node->get_node_value()<<endl;
	}

	//change AST_node's value and type to gamma
	let_node->set_node_value("gamma");
	let_node->set_node_type(GAMMA);

	//change AST_node's left child's value and type to lambda
	if( (let_node->left_child) != NULL){
		(let_node->left_child)->set_node_value("lambda");
		(let_node->left_child)->set_node_type(LAMBDA);
	}
	else{
		cout<<"standardized_let - let node's left child is NULL !"<<endl;
		exit(0);
	}

	//save AST_node's right child
	AST_node *save_right_child = (let_node->left_child)->right_sibling;
	if(save_right_child == NULL){
		cout<<"standardized_let - let node's right child is NULL !"<<endl;
		exit(0);
	}

	//swap AST_node's right child with AST_node's left child's right child (yup bit confusing read carefully)
	if( ((let_node->left_child)->left_child)->right_sibling != NULL ) {
		(let_node->left_child)->right_sibling = ((let_node->left_child)->left_child)->right_sibling;
		((let_node->left_child)->left_child)->right_sibling = save_right_child;
	}

	else{
		cout<<"standardized_let -  Expression is NULL !"<<endl;
		exit(0);
	}

	return let_node;
}


/* This is to standardize the WHERE node
 * This standarize in place i.e. in the end the same node is standardized
 */
AST_node* Parser::standardize_where(AST_node *& where_node){
	if(where_node == NULL){
		cout<<"standardized_where- node is NULL !!"<<endl;
		exit(0);
	}

	if(verbose_on || debug_on){
		cout<<"standardized_where- standardizing where node......."<<endl;
		cout<<"node_type: "<<where_node->get_node_type()<<endl;
		cout<<"node_value: "<<where_node->get_node_value()<<endl;
	}

	//change AST_node's value and type to gamma
	where_node->set_node_value("gamma");
	where_node->set_node_type(GAMMA);

	AST_node *P, *E, *X; //create temps to store org node's childrena

	//First pass - change & save the values of node
	//save P
	if(where_node->left_child != NULL)
		P = where_node->left_child;
	else{
		cout<<"standardized_where- left_child is NULL !"<<endl;
		exit(0);
	}

	//change the equal node to lambda
	if(P->right_sibling != NULL){
		
		//check if it is equal
		if( (P->right_sibling)->get_node_type() == EQUAL ){
			(P->right_sibling)->set_node_type(LAMBDA);
			(P->right_sibling)->set_node_value("lambda");
		}

		else{
			cout<<"standarized_where- right child is not equal as expected ! It is :"<<(P->right_sibling)->get_node_value()<<endl;
			exit(0);
		}
	}

	else{
		cout<<"standarized_where- right child(sibling) is NULL !"<<endl;
		exit(0);
	}

	//save X
	X = (P->right_sibling)->left_child ;
	if( X == NULL){
		cout<<"standarized_where- X is NULL !"<<endl;
		exit(0);
	}

	//save E
	E = X->right_sibling;
	if( E == NULL){
		cout<<"standarized_where- E is NULL !"<<endl;
		exit(0);
	}

	//second pass - change the pointers
	where_node->left_child = P->right_sibling; //make lambda a left child of gamma
	P->right_sibling = NULL;

	where_node->left_child->right_sibling= E;
	X->right_sibling = P;

	return where_node;

}

/* This is to standardize WITHIN node*/
AST_node* Parser::standardize_within(AST_node* within_node){
	if(within_node == NULL){
			cout<<"standardized_within - node is NULL !!"<<endl;
			exit(0);
	}

	if(verbose_on || debug_on){
		cout<<"standardized_within- standardizing where node......."<<endl;
		cout<<"node_type: "<<within_node->get_node_type()<<endl;
		cout<<"node_value: "<<within_node->get_node_value()<<endl;
	}

	AST_node *X1=NULL, *X2=NULL, *E1=NULL, *E2=NULL; //some temps to hold children
	
	//save X1
	if( within_node->left_child != NULL )
		X1 = within_node->left_child->left_child;
	else{
		cout<<"standardized_within - left child is NULL ! "<<endl;
		exit(0);
	}

	//save X2
	if( within_node->left_child->right_sibling != NULL )
		X2 = within_node->left_child->right_sibling->left_child; //yeah a little complex
	else{
		cout<<"standardized_within - Right children (left's right sibling) is NULL! "<<endl;
		exit(0);
	}

	//save E2
	if( X2 != NULL)
		E2 = X2->right_sibling;
	else{
		cout<<"standardized_within - X2 is NULL! "<<endl;
		exit(0);
	}

	//save E1
	if(X1 != NULL)
		E1 = X1->right_sibling;
	else{
		cout<<"standardized_within - X1 is NULL ! "<<endl;
		exit(0);
	}


	//delete nodes
	/*delete within_node->left_child->right_sibling; //delete right equal
	delete within_node->left_child; //delete left equal
	delete within_node; //delete within node
	within_node = NULL;


	//create node
	within_node = new AST_node; */
	within_node->set_node_value("=");
	within_node->set_node_type(EQUAL);

	within_node->left_child = X2; //make X2 left child

	AST_node *temp = new AST_node;
	temp->set_node_value("gamma");
	temp->set_node_type(GAMMA);

	X2->right_sibling = temp; //make new gamma node a right children of eq

	temp = NULL;
	temp = new AST_node;
	temp->set_node_value("lambda");
	temp->set_node_type(LAMBDA);

	X2->right_sibling->left_child = temp; //make lambda node the left children of gamma
	temp->right_sibling = E1;
	temp->left_child = X1;

	X1->right_sibling = E2;

	return within_node;

}


/* This is to standardize FUNCTION_FORM node */
AST_node* Parser::standardize_funform(AST_node *& funform_node){
	if(funform_node == NULL){
		cout<<"standardize_funform- node is NULL !!"<<endl;
		exit(0);
	}

	if(verbose_on || debug_on){
		cout<<"standardize_funform- standardizing function form node......."<<endl;
		cout<<"node_type: "<<funform_node->get_node_type()<<endl;
		cout<<"node_value: "<<funform_node->get_node_value()<<endl;
	}


	//convert fcn_form node to equal
	funform_node->set_node_type(EQUAL);
	funform_node->set_node_value("=");

	AST_node *P=NULL, *V=NULL, *temp_lambda=NULL; //some temps to hold children

	P = funform_node->left_child; //P could be NULL but it shouldn't be
	if( P == NULL ) {
		cout<<"standarize_funform - left child P is NULL ! "<<endl;
		exit(0);
	}

	V = P->right_sibling; //V could be NULL but it shouldn't be
	if( V == NULL ) {
		cout<<"standarize_funform - right child V is NULL ! "<<endl;
		exit(0);
	}
	
	//leave P as it is it shouldn't be modified
	while( V->right_sibling != NULL){
		//create a new lambda node
		temp_lambda = new AST_node;
		temp_lambda->set_node_value("lambda");
		temp_lambda->set_node_type(LAMBDA);

		//set V as right sibling of lambda
		temp_lambda->left_child= V;

		//set newly created lambda node as right sibling of P
		P->right_sibling = temp_lambda;
		
		P = V;
		V = V->right_sibling;
	}

	//we have E here in V
	P->right_sibling = V;

	return funform_node;
}

/* This is to standardize @ node */
AST_node* Parser::standardize_at(AST_node *& at_node){
	
	if(at_node == NULL){
		cout<<"standardize_at- node is NULL !!"<<endl;
		exit(0);
	}

	if(verbose_on || debug_on){
		cout<<"standardize_at- standardizing at node......."<<endl;
		cout<<"node_type: "<<at_node->get_node_type()<<endl;
		cout<<"node_value: "<<at_node->get_node_value()<<endl;
	}

	//save E1,N & E2
	AST_node *E1=NULL, *N=NULL, *E2=NULL; //some temps

	E1 = at_node->left_child; //shouldn't be NULL
	if(E1 == NULL){
		cout<<"standardize_at - Left child E1 is NULL !"<<endl;
		exit(0);
	}

	N = E1->right_sibling; //shouldn't be NULL
	if(N == NULL){
		cout<<"standardize_at - Child N is NULL !"<<endl;
		exit(0);
	}

	E2 = N->right_sibling; //shouldn't be NULL
	if(E2 == NULL){
		cout<<"standardize_at - Child E2 is NULL !"<<endl;
		exit(0);
	}

	//change the at node to gamma
	at_node->set_node_value("gamma");
	at_node->set_node_type(GAMMA);

	//create a new gamma node
	AST_node *temp_gamma = new AST_node;
	temp_gamma->set_node_type(GAMMA);
	temp_gamma->set_node_value("gamma");

	//make the newly created gamma node a left child of main gamma node
	at_node->left_child = temp_gamma;
	
	//make E2 right sibling of new gamma node
	temp_gamma->right_sibling = E2;

	//make N right sibling of new gamma node
	temp_gamma->left_child = N;

	//make E1 right siblingn of N
	N->right_sibling = E1;

	E1->right_sibling = NULL; //prior to standardization E1's right sibling was N

	return at_node;
}

/* This is to standardize lambda node w/o comma version
 * This node will not be standardized if it contains only one identifier
 */
AST_node* Parser::standardize_lambda(AST_node *& lambda_node){
	if(lambda_node == NULL){
		cout<<"standardize_lambda- node is NULL !!"<<endl;
		exit(0);
	}

	if(verbose_on || debug_on){
		cout<<"standardize_at- standardizing lambda node......."<<endl;
		cout<<"node_type: "<<lambda_node->get_node_type()<<endl;
		cout<<"node_value: "<<lambda_node->get_node_value()<<endl;
	}

	AST_node *V=NULL, *V_next=NULL; //some temps
	bool standardize = false; //will decide if node needs to be standardized or not

	V = lambda_node->left_child; //this shouldn't be NULL
	if(V == NULL){
		cout<<"standardize_lambda - left child V is NULL !"<<endl;
		exit(0);
	}

	V_next = V->right_sibling; //this shouldn't be NULL either, either E or another identifier
	if(V_next == NULL){
		cout<<"standardize_lambda - right child V or E is NULL !"<<endl;
		exit(0);
	}

	while( V_next->right_sibling != NULL){
		standardize = true; //node has more than one identifier so it needs to be standarized

		//create new lambda node
		AST_node *temp_lambda = new AST_node;
		temp_lambda->set_node_type(LAMBDA);
		temp_lambda->set_node_value("lambda");

		V->right_sibling = temp_lambda; //make new lambda node a right sibling of V
		temp_lambda->left_child = V_next;

		//Advance
		V = V_next;
		V_next = V_next->right_sibling;
	}

	if(standardize)
		V->right_sibling = V_next;

	return lambda_node;
}

/* This is to standardize and node */
AST_node* Parser::standardize_and(AST_node *& and_node){
	if(and_node == NULL){
		cout<<"standardize_and- node is NULL !!"<<endl;
		exit(0);
	}

	if(verbose_on || debug_on){
		cout<<"standardize_and- standardizing and node......."<<endl;
		cout<<"node_type: "<<and_node->get_node_type()<<endl;
		cout<<"node_value: "<<and_node->get_node_value()<<endl;
	}

	AST_node *eq=NULL, *comma=NULL, *tau=NULL; //some temp
	eq = and_node->left_child; //EQUAL node - this shouldn't be NULL
	if(eq == NULL){
		cout<<"standardize_and - left child EQUAL is null !"<<endl;
		exit(0);
	}

	//create EQUAL, COMMA & TAU node
	AST_node *new_eq=NULL, *new_comma=NULL, *new_tau=NULL, *temp=NULL;
	new_eq = new AST_node;
	new_eq->set_node_type(EQUAL);
	new_eq->set_node_value("=");

	//change and_node to EQUAL
	and_node->set_node_type(EQUAL);
	and_node->set_node_value("=");

	new_comma= new AST_node;
	new_comma->set_node_type(COMMA);
	new_comma->set_node_value(",");
	
	new_tau= new AST_node;
	new_tau->set_node_type(TAU);
	new_tau->set_node_value("tau");
	
	//make comma and tau children of equal
	//new_eq->left_child = new_comma;
	and_node->left_child = new_comma;
	new_comma->right_sibling = new_tau;

	/*comma = new_comma->left_child;
	tau = new_tau->left_child; */

	new_comma->left_child = eq->left_child; //shouldn't be NULL
	new_tau->left_child= new_comma->left_child->right_sibling; //shouldn't be NULL
	if(new_comma == NULL || new_tau == NULL){
		cout<<"standardize_and = one of equal's children is NULL !"<<endl;
		exit(0);
	}

	//advance the pointers
	new_comma = new_comma->left_child;
	new_tau = new_tau->left_child;
	
	//delete and_node
	/*delete and_node;
	and_node = new_eq; */

	//delete eq node
	temp = eq;
	eq = eq->right_sibling; //go to next equal node
	delete temp;

	while( eq != NULL){
		new_comma->right_sibling = eq->left_child; //shouldn't be NULL
		new_tau->right_sibling = new_comma->right_sibling->right_sibling; //shouldn't be NULL
		if(new_comma == NULL || new_tau == NULL){
			cout<<"standardize_and = one of equal's children is NULL !"<<endl;
			exit(0);
		}

		//advance the pointers
		new_comma = new_comma->right_sibling;
		new_tau = new_tau->right_sibling;
		
		//delete eq node
		temp = eq;
		eq = eq->right_sibling; //go to next equal node
		delete temp;
	}

	new_comma->right_sibling = NULL; //prior to standardization it was pointing to Expression

	return and_node;

}

/* This is to standardize rec */
void Parser::standardize_rec(AST_node *& rec_node){
	if(rec_node== NULL){
		cout<<"standardize_rec- node is NULL !!"<<endl;
		exit(0);
	}

	if(verbose_on || debug_on){
		cout<<"standardize_rec- standardizing rec node......."<<endl;
		cout<<"node_type: "<<rec_node->get_node_type()<<endl;
		cout<<"node_value: "<<rec_node->get_node_value()<<endl;
	}

	//change rec to EQUAL
	rec_node->set_node_type(EQUAL);
	rec_node->set_node_value("=");

	AST_node *eq=NULL, *X=NULL, *E=NULL; //to store temps

	eq = rec_node->left_child; //it shouldn't be NULL
	if(eq == NULL){
		cout<<"Standardize_rec - left child = is NULL!"<<endl;
		exit(0);
	}

	X = eq->left_child; //shouldn't be NULL
	if(X == NULL){
		cout<<"Standardize_rec - left child X of = is NULL!"<<endl;
		exit(0);
	}

	E = X->right_sibling; 
	if(E == NULL){
		cout<<"Standardize_rec - right child E of = is NULL!"<<endl;
		exit(0);
	}

	//make X left child of main EQUAL
	rec_node->left_child = X;

	//create new gamma node
	AST_node *temp_gamma = new AST_node;
	temp_gamma->set_node_type(GAMMA);
	temp_gamma->set_node_value("gamma");

	//put this new node as right children of EQUAL
	X->right_sibling = temp_gamma;

	//create new Y* node
	AST_node *ystar = new AST_node;
	ystar->set_node_type(YSTAR);
	ystar->set_node_value("Y*");

	//shove it in..i mean put it as left children of new gamma
	temp_gamma->left_child = ystar;

	//create new lambda node
	AST_node *temp_lambda = new AST_node;
	temp_lambda->set_node_type(LAMBDA);
	temp_lambda->set_node_value("lambda");

	//put it in as right child of gamma
	ystar->right_sibling = temp_lambda;

	/*conundrum - not sure how to handle the second X under lambda
	 * As of now creating new X node a copy of X whose left_child will point to where X's point to
	 * but right_silbing will be different */
	AST_node *copy_of_X = new AST_node;
	copy_of_X->set_node_type( X->get_node_type() );
	copy_of_X->set_node_value( X->get_node_value() );
	copy_of_X->left_child = X->left_child;

	temp_lambda->left_child = copy_of_X;
	copy_of_X->right_sibling= E;
}

/* this procedure recursively generate the control structure from standard_ast and save it in a tree structure in cntrl_st
 * To Keep in Mind - at the end cntrl_st will be pointing to the end of control structure not the begining
 * Various Cases:
 *	=>LAMBDA
 		-With comma
		-without comma
	=>TAU
	=>COND
	=>OTHER Nodes
 */
void Parser::generate_control_struct(AST_node *standard_ast, control_st *cntrl_st){ //this will recursively generate the control structure
	//standard_ast can't be NULL
	if( standard_ast == NULL ){
		cout<<"generate_control_struct - standard_ast is NULL! "<<endl;
		exit(0);
	}

	//if node is lambda. this case won't have any left  child only right sibling
	if( standard_ast->get_node_type() == LAMBDA){
		if(verbose_on){
			cout<<"\n==========================================================="<<endl;
			cout<<"generate_control_struct - LAMBDA type node "<<endl;
			cout<<"Node Type: "<<standard_ast->get_node_type()<<endl;
			cout<<"Node Value: "<<standard_ast->get_node_value()<<endl;
			cout<<"===========================================================\n"<<endl;
		}
		//create new CS node
		control_st *temp_cs = new control_st;
		
		//set its values
		temp_cs->node = standard_ast;  //lambda node
		AST_node *save_right_sibling = standard_ast->left_child->right_sibling; //because if there isn't comma next few lines will make it NULL
		if(standard_ast->left_child->get_node_type() != COMMA){
			temp_cs->bound_var = standard_ast->left_child; //if this is comma it would be left child of left child
			temp_cs->bound_var->right_sibling = NULL; //there could only be one variable
		}
		else
			temp_cs->bound_var = standard_ast->left_child->left_child; //if this is comman it would be left child of left child

		cntrl_st->next = temp_cs;
		cntrl_st = cntrl_st->next; //move forward

		control_st *new_cs = new control_st; //yeah design sucks. this will be a memory leak - NOPE resolved it :)
		generate_control_struct(save_right_sibling, new_cs); //this  will return the control struct of right child
		//link the new cs
		cntrl_st->delta = new_cs->next; 
		
		delete new_cs;

		if(standard_ast->right_sibling != NULL)
			generate_control_struct(standard_ast->right_sibling, cntrl_st);
	}

	//else if node is tau. this case won't have any left child only right sibling 
	else if( standard_ast->get_node_type() == TAU){
		if(verbose_on){
			cout<<"\n==========================================================="<<endl;
			cout<<"generate_control_struct - node other than lambda, tau or cond"<<endl;
			cout<<"Node Type: "<<standard_ast->get_node_type()<<endl;
			cout<<"Node Value: "<<standard_ast->get_node_value()<<endl;
			cout<<"===========================================================\n"<<endl;
		}

		//create new tau node
		control_st *temp_cs = new control_st;
		temp_cs->node = standard_ast;

		cntrl_st->next = temp_cs;
		cntrl_st = cntrl_st->next;

		AST_node *temp_ast_node = standard_ast->left_child;
		int num_children = 0;

		generate_control_struct(temp_ast_node, cntrl_st);
		
		while(temp_ast_node != NULL){
			num_children++;
			temp_ast_node = temp_ast_node->right_sibling;
		}

		//set the num of children in tau
		cntrl_st->set_num_children(num_children);

		while(cntrl_st->next != NULL)
			cntrl_st = cntrl_st->next; //because generate_control_struct above can generate more than one number of control structs
		

		if(debug_on || verbose_on)
			cout<<"generate_control_struct - Number of children of tau are: "<<num_children<<endl;

		

		if(standard_ast->right_sibling != NULL)
			generate_control_struct(standard_ast->right_sibling, cntrl_st);
	}

	//else if node is conditional. this case won't have any left  child only right sibling
	/* if node is CONDITIONAL it breaks the B, T & E part from the right sibling list
	   otherwise due to this procedure being a recurisive it will automatically generate the 
	   control structure for B, T & E */
	else if( standard_ast->get_node_type() ==  COND){
		if(verbose_on){
			cout<<"\n==========================================================="<<endl;
			cout<<"generate_control_struct - COND node"<<endl;
			cout<<"Node Type: "<<standard_ast->get_node_type()<<endl;
			cout<<"Node Value: "<<standard_ast->get_node_value()<<endl;
			cout<<"===========================================================\n"<<endl;
		}

		//Break the B, T & E and save each of them 
		AST_node *temp_B=NULL, *temp_T=NULL, *temp_E=NULL;
		
		temp_B = standard_ast->left_child; //shouldn't be NULL
		if(temp_B == NULL){
			cout<<"left child (B) is NULL! "<<endl;
			exit(0);
		}

		if(verbose_on){
			cout<<"generate_control_struct - temp_B's value: "<<endl;
			cout<<"node type: "<<temp_B->get_node_type()<<endl;
			cout<<"node value: "<<temp_B->get_node_value()<<endl;
		}
		temp_T = temp_B->right_sibling; //shoudln't be NULL
		if(temp_T == NULL){
			cout<<"middle child (T) is NULL! "<<endl;
			exit(0);
		}
		if(verbose_on){
			cout<<"generate_control_struct - temp_T's value: "<<endl;
			cout<<"node type: "<<temp_T->get_node_type()<<endl;
			cout<<"node value: "<<temp_T->get_node_value()<<endl;
		}

		temp_E = temp_T->right_sibling; //shouldn't be NULL
		if(temp_E == NULL){
			cout<<"right child (E) is NULL! "<<endl;
			exit(0);
		}
		if(verbose_on){
			cout<<"generate_control_struct - temp_E's value: "<<endl;
			cout<<"node type: "<<temp_E->get_node_type()<<endl;
			cout<<"node value: "<<temp_E->get_node_value()<<endl;
		}
		//break all right siblings	
		temp_B->right_sibling = NULL;
		temp_T->right_sibling = NULL;
		//temp_E should already be NULL

		//need to generate delta_then control structure
		//create new AST node of DELTA_THEN type
		AST_node *temp_delta_then = new AST_node;
		temp_delta_then->set_node_type(DELTA_THEN);
		temp_delta_then->set_node_value("delta_then");
		//create new control struct
		control_st *temp_cs = new control_st;
		temp_cs->node = temp_delta_then;
		cntrl_st->next = temp_cs; //link the newly created control struct in the list
		cntrl_st = cntrl_st->next;
		//generate control struct for temp_T;
		control_st *temp_cntst = new control_st;
		generate_control_struct(temp_T, temp_cntst); //temp_cntst's next have control struct
		cntrl_st->delta = temp_cntst->next; 
		delete temp_cntst;

		//need to generate delta_else control structure
		//create new AST node of DELTA_ELSE type
		AST_node *temp_delta_else = new AST_node;
		temp_delta_else->set_node_type(DELTA_ELSE);
		temp_delta_else->set_node_value("delta_else");
		//create new control struct
		control_st *temp_cs_else= new control_st;
		temp_cs_else->node = temp_delta_else;
		cntrl_st->next = temp_cs_else; //link the newly created control struct in the list
		cntrl_st = cntrl_st->next;
		//generate control struct for temp_T;
		control_st *temp_cstruct= new control_st;
		generate_control_struct(temp_E, temp_cstruct); //temp_cntst's next have control struct
		cntrl_st->delta = temp_cstruct->next; 
		delete temp_cstruct;
		
		//create BETA control structure
		AST_node *temp_beta = new AST_node;
		temp_beta->set_node_type(BETA);
		temp_beta->set_node_value("beta");

		control_st *temp_beta_cs = new control_st;
		temp_beta_cs->node = temp_beta;
		cntrl_st->next = temp_beta_cs;
		cntrl_st =cntrl_st->next;


		//generate control struct for B
		//AST_node *temp_delta_b= new AST_node;
		//temp_delta_b->set_node_type(DELTA_ELSE);
		//temp_delta_then->set_node_value("delta");
		//create new control struct
		//temp_cs = new control_st;
		//temp_cs->node = temp_B;
		//cntrl_st->next = temp_cs; //link the newly created control struct in the list
		//generate control struct for temp_T;
		control_st *temp_cstruct_B= new control_st;
		generate_control_struct(temp_B, temp_cstruct_B); //temp_cntst's next have control struct
		cntrl_st->next= temp_cstruct_B->next; 
		delete temp_cstruct_B;
		while(cntrl_st->next != NULL)
			cntrl_st = cntrl_st->next;

		if(standard_ast->right_sibling != NULL)
			generate_control_struct(standard_ast->right_sibling, cntrl_st);
	}

	//else - for rest of the nodes. this case will have both left child and right sibling
	else{
		if(verbose_on){
			cout<<"\n==========================================================="<<endl;
			cout<<"generate_control_struct - node other than lambda, tau or cond"<<endl;
			cout<<"Node Type: "<<standard_ast->get_node_type()<<endl;
			cout<<"Node Value: "<<standard_ast->get_node_value()<<endl;
			cout<<"===========================================================\n"<<endl;
		}
		//create new CS node
		control_st *temp_cs = new control_st;

		//set its values
		temp_cs->node = standard_ast; //save the current standardized tree node

		//move forward the CS pointer
		cntrl_st->next = temp_cs;
		cntrl_st = cntrl_st->next;
		//recursively do the same for both left_child and right_sibling
		if(standard_ast->left_child != NULL) //will generate only one CS node
			generate_control_struct(standard_ast->left_child, cntrl_st); //this will automatically change the value of cntrl_st
		while(cntrl_st->next != NULL) //introducing inefficiency. couldn't resolve the design issue :(
			cntrl_st = cntrl_st->next;
		if(standard_ast->right_sibling != NULL)
			generate_control_struct(standard_ast->right_sibling, cntrl_st);
	}
}

void CSE_machine::print_control_struct(control_st *cnt_st){
	//control_st *temp = cnt_st->next;
	cout<<"==========================================================="<<endl;
	while(cnt_st != NULL){
		//cout<<"[ "<<cnt_st->node->get_node_type()<<": "<<cnt_st->node->get_node_value()<<" ]"<<endl;
		print_ast_node(cnt_st->node);
		if( cnt_st->node->get_node_type() == TAU)
			cout<<"Number of children: "<<cnt_st->get_num_children()<<endl;
		
		AST_node *temp_bv = cnt_st->bound_var;
		while(temp_bv != NULL){
			cout<<"\t";
			cout<<" ("<<temp_bv->get_node_value()<<")";
			temp_bv = temp_bv->right_sibling;
		}
		cout<<endl;
		//cout<<"\n----------------------------------------------------"<<endl;

		if(cnt_st->delta != NULL)
			print_control_struct(cnt_st->delta);
		cnt_st = cnt_st->next;
	}
	cout<<"============================================================"<<endl;
}






/* this procedure print out the first control structure
 * FOR DEBUGGING PURPOSE ONLY
 */
void Parser::print_control_struct(control_st *cnt_st){
	//control_st *temp = cnt_st->next;
	cout<<"==========================================================="<<endl;
	while(cnt_st != NULL){
		//cout<<"[ "<<cnt_st->node->get_node_type()<<": "<<cnt_st->node->get_node_value()<<" ]"<<endl;
		print_ast_node(cnt_st->node);
		if( cnt_st->node->get_node_type() == TAU)
			cout<<"Number of children: "<<cnt_st->get_num_children()<<endl;
		
		AST_node *temp_bv = cnt_st->bound_var;
		while(temp_bv != NULL){
			cout<<"\t";
			cout<<" ("<<temp_bv->get_node_value()<<")";
			temp_bv = temp_bv->right_sibling;
		}
		cout<<endl;
		//cout<<"\n----------------------------------------------------"<<endl;

		if(cnt_st->delta != NULL)
			print_control_struct(cnt_st->delta);
		cnt_st = cnt_st->next;
	}
	cout<<"============================================================"<<endl;
}

/* this procedure will print out the node's type and value in a presentable manner */
void print_ast_node(AST_node *in_node){
	cout<<"<";
	switch(in_node->get_node_type()){
		case IDT: cout<<"IDENTIFIER";
			  break;

		case STR: cout<<"STRING";
			  break;

		case INT: cout<<"INTEGER";
			  break;

		case TAU: cout<<"TAU";
			  break;

		case LAMBDA: cout<<"LAMBDA";
			  break;

		case OR: cout<<"OR";
			  break;

		case NOT: cout<<"NOT";
			  break;

		case GR: cout<<"GR";
			  break;

		case GE: cout<<"GE";
			  break;

		case LS: cout<<"LS";
			  break;

		case LE: cout<<"LE";
			  break;

		case EQ: cout<<"EQ"; //this should be binary operator not EQUAL
			  break;

		case NE: cout<<"NE";
			  break;

		case PLUS: cout<<"PLUS";
			  break;

		case MINUS: cout<<"MINUS";
			  break;

		case NEG: cout<<"NEG";
			  break;

		case AMP: cout<<"AMP";
			  break;

		case AUG: cout<<"AUG";
			  break;

		case MULTI: cout<<"MULTI";
			  break;

		case DIV: cout<<"DIV";
			  break;

		case EXPO: cout<<"EXPO";
			  break;

		case GAMMA: cout<<"GAMMA";
			  break;

		case TRUE: cout<<"TRUE";
			  break;

		case FALSE: cout<<"FALSE";
			  break;

		case NIL: cout<<"NIL";
			  break;

		case DUMMY: cout<<"DUMMY";
			  break;

		/*case EQUAL: cout<<"EQUAL";
			  break; */

		case YSTAR: cout<<"YSTAR";
			  break;

		case DELTA_THEN: cout<<"DELTA_THEN";
			  break;

		case DELTA_ELSE: cout<<"DELTA_ELSE";
			  break;

		case BETA: cout<<"BETA";
			  break;
		
		case ENV: cout<<"ENVIRONMENT";
			  break;
		default:
			   cerr<<"Some other type of node !! - "<<in_node->get_node_type()<<endl;
			   exit(0);
	}
	cout<<":"<<in_node->get_node_value()<<">"<<endl;
}

/* Crux of CSE_machine
 * It will start executing the program by applying all the rules
 */
void CSE_machine::execute(){
	if(debug_on || verbose_on)
		cout<<"\nBeigning execution...Tight your seat belts folks......."<<endl;
	
	//while control is not empty
	while( control_stack.empty() != true ){
		//pop an element from control stack
		control_st *popped_cntrl_st = control_stack.top();
		control_stack.pop();

		if(verbose_on){
			cout<<"\nPopped node from control stack:" ; print_ast_node(popped_cntrl_st->node);
		}

		//apply rules
		if( popped_cntrl_st->node->get_node_type() == INT){ //if node is INTEGER
			if(verbose_on)
				cout<<"\tINTEGER node found..."<<endl;
			//create new execution_element
			execution_element *new_exe_ele = new execution_element();
			new_exe_ele->type = EXE_INT; //set type

			//convert string value to int
			int temp_int = atoi( (popped_cntrl_st->node->get_node_value() ).c_str() );
			if(debug_on || verbose_on)
				cout<<"\tPopped integer converted is: "<<temp_int<<endl;
			new_exe_ele->int_var =  temp_int;

			//push the new execution_element on exeuction stack
			execution_stack.push(new_exe_ele);
			

			if(verbose_on){
				cout<<"\tPushed "<<temp_int<<" on execution stack"<<endl;
				cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
				cout<<"\tPrinting pushed integer: ";
				print_execution_element(new_exe_ele);
				cout<<endl;
			}

			if(verbose_on || debug_on)
				getchar();
			//delete the control stack
			//delete popped_cntrl_st;
		}

		else if(popped_cntrl_st->node->get_node_type() == NIL){
			if(verbose_on)
				cout<<"\t<NIL> node on control stack..."<<endl;

			execution_element *new_nil = new execution_element;
			new_nil->type = EXE_NILL;
			execution_stack.push(new_nil);
			
			if(verbose_on){
				cout<<"\tPushed node: ";
				print_execution_element(new_nil);
				cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
			}
		}
	
		else if(popped_cntrl_st->node->get_node_type() == DUMMY){
			if(verbose_on)
				cout<<"\t<DUMMY> node on control stack..."<<endl;

			execution_element *new_dummy= new execution_element;
			new_dummy->type = EXE_DUMMY;
			execution_stack.push(new_dummy);
			
			if(verbose_on){
				cout<<"\tPushed node: ";
				print_execution_element(new_dummy);
				cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
			}
		}


		//TRUE
		else if(popped_cntrl_st->node->get_node_type() == TRUE){
			if(verbose_on)
				cout<<"\t<TRUE> node on control stack..."<<endl;

			execution_element *new_true= new execution_element;
			new_true->type = EXE_TRUE;
			execution_stack.push(new_true);
			
			if(verbose_on){
				cout<<"\tPushed node: ";
				print_execution_element(new_true);
				cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
			}
		}
		//FALSE
		else if(popped_cntrl_st->node->get_node_type() == FALSE){
			if(verbose_on)
				cout<<"\t<FALSE> node on control stack..."<<endl;

			execution_element *new_false= new execution_element;
			new_false->type = EXE_FALSE;
			execution_stack.push(new_false);
			
			if(verbose_on){
				cout<<"\tPushed node: ";
				print_execution_element(new_false);
				cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
			}
		}




		else if( popped_cntrl_st->node->get_node_type() == STR){ //if node is STRING 
			if(verbose_on)
				cout<<"\tSTRING node found..."<<endl;
			//create new execution_element
			execution_element *new_exe_ele = new execution_element();
			new_exe_ele->type = EXE_STR; //set type

			/*//convert string value to int
			int temp_int = atoi( (popped_cntrl_st->node->get_node_value() ).c_str() ); */
			if(debug_on || verbose_on)
				cout<<"\tPopped node's string value is: <"<<popped_cntrl_st->node->get_node_value()<<">"<<endl;
		
			new_exe_ele->string_var = popped_cntrl_st->node->get_node_value();
			//push the new execution_element on exeuction stack
			execution_stack.push(new_exe_ele);
			

			if(verbose_on){
				cout<<"\tPushed <"<<popped_cntrl_st->node->get_node_value()<<"> on execution stack"<<endl;
				cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
				cout<<"\tPrinting pushed string: ";
				print_execution_element(new_exe_ele);
				cout<<endl;
			}

			if(verbose_on || debug_on)
				getchar();
			//delete the control stack
			//delete popped_cntrl_st;
		}

		//if node is IDENTIFIER
		else if( popped_cntrl_st->node->get_node_type() == IDT) {
			if(verbose_on || debug_on)
				cout<<"\t<IDENTIFIER> node it is..."<<endl;

			//lookup in current environment
			execution_element *idt_exe_ele = current_env->lookup(popped_cntrl_st->node->get_node_value());

			//if lookup is unsuccessfull it might be primitive function
			if( idt_exe_ele == NULL){
				//check if its primitive function
				/* NOTE - I AM ASSUMING THAT PRIMTIVE FUNCTIONS ARE CASE SENSTITIVE SO print IS NOT A PRIMITIVE FUNCTION */
				if((popped_cntrl_st->node->get_node_value()).compare("Print") == 0){
					if(verbose_on || debug_on)
						cout<<"\tIt is primitive function: Print"<<endl;
					//do appropriate action
					//create new execution element
					execution_element *new_exe_ele = new execution_element;
					new_exe_ele->type = EXE_PRIMITIVE_FUNC;
					new_exe_ele->primitive_fun_var =  PRINT;
					execution_stack.push(new_exe_ele); //push
					if(verbose_on){
						cout<<"\tPushed <PRINT> function on execution stack"<<endl;
						cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
					}
					//delete popped_cntrl_st;
				}
				else if((popped_cntrl_st->node->get_node_value()).compare("ItoS") == 0){
					if(verbose_on || debug_on)
						cout<<"\tIt is primitive function: ItoS"<<endl;
					//do appropriate action
					//create new execution element
					execution_element *new_exe_ele = new execution_element;
					new_exe_ele->type = EXE_PRIMITIVE_FUNC;
					new_exe_ele->primitive_fun_var =  ITOS;
					execution_stack.push(new_exe_ele); //push
					if(verbose_on){
						cout<<"\tPushed <PRINT> function on execution stack"<<endl;
						cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
					}
					//delete popped_cntrl_st;
				}
				else if((popped_cntrl_st->node->get_node_value()).compare("Istruthvalue") == 0){
					if(verbose_on || debug_on)
						cout<<"\tIt is primitive function: Istruthvalue"<<endl;
					//do appropriate action
					//create new execution element
					execution_element *new_exe_ele = new execution_element;
					new_exe_ele->type = EXE_PRIMITIVE_FUNC;
					new_exe_ele->primitive_fun_var =  ISTRUTHVALUE;
					execution_stack.push(new_exe_ele); //push
					if(verbose_on){
						cout<<"\tPushed <ISTRUTHVALUE> function on execution stack";
						cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
					}
					//delete popped_cntrl_st;
				}

				else if((popped_cntrl_st->node->get_node_value()).compare("Isstring") == 0){
					if(verbose_on || debug_on)
						cout<<"\tIt is primitive function: Isstring"<<endl;
					//do appropriate action
					//create new execution element
					execution_element *new_exe_ele = new execution_element;
					new_exe_ele->type = EXE_PRIMITIVE_FUNC;
					new_exe_ele->primitive_fun_var =  ISSTRING;
					execution_stack.push(new_exe_ele); //push
					if(verbose_on){
						cout<<"\tPushed <ISSTRING> function on execution stack";
						cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
					}
					//delete popped_cntrl_st;
				}


				else if((popped_cntrl_st->node->get_node_value()).compare("Isinteger") == 0){
					if(verbose_on || debug_on)
						cout<<"\tIt is primitive function: Isinteger"<<endl;
					//do appropriate action
					//create new execution element
					execution_element *new_exe_ele = new execution_element;
					new_exe_ele->type = EXE_PRIMITIVE_FUNC;
					new_exe_ele->primitive_fun_var =  ISINTEGER;
					execution_stack.push(new_exe_ele); //push
					if(verbose_on){
						cout<<"\tPushed <ISINTEGER> function on execution stack";
						cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
					}
					//delete popped_cntrl_st;
				}

				else if((popped_cntrl_st->node->get_node_value()).compare("Istuple") == 0){
					if(verbose_on || debug_on)
						cout<<"\tIt is primitive function: Istuple"<<endl;
					//do appropriate action
					//create new execution element
					execution_element *new_exe_ele = new execution_element;
					new_exe_ele->type = EXE_PRIMITIVE_FUNC;
					new_exe_ele->primitive_fun_var =  ISTUPLE;
					execution_stack.push(new_exe_ele); //push
					if(verbose_on){
						cout<<"\tPushed <ISTUPLE> function on execution stack";
						cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
					}
					//delete popped_cntrl_st;
				}

				else if((popped_cntrl_st->node->get_node_value()).compare("Order") == 0){
					if(verbose_on || debug_on)
						cout<<"\tIt is primitive function: Order"<<endl;
					//do appropriate action
					//create new execution element
					execution_element *new_exe_ele = new execution_element;
					new_exe_ele->type = EXE_PRIMITIVE_FUNC;
					new_exe_ele->primitive_fun_var =  ORDER;
					execution_stack.push(new_exe_ele); //push
					if(verbose_on){
						cout<<"\tPushed <ORDER> function on execution stack"<<endl;
						cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
					}
					//delete popped_cntrl_st;
				}

				/*else if((popped_cntrl_st->node->get_node_value()).compare("Aug") == 0){
					if(verbose_on || debug_on)
						cout<<"\tIt is primitive function: <AUG>"<<endl;
					//do appropriate action
					//create new execution element
					execution_element *new_exe_ele = new execution_element;
					new_exe_ele->type = EXE_PRIMITIVE_FUNC;
					new_exe_ele->primitive_fun_var =  AUG;
					execution_stack.push(new_exe_ele); //push
					if(verbose_on)
						cout<<"\tPushed <AUG> function on execution stack"<<endl;
					delete popped_cntrl_st;
				} */
				else if( ((popped_cntrl_st->node->get_node_value()).compare("Stern") == 0 ) 
						|| ((popped_cntrl_st->node->get_node_value()).compare("Stem") == 0) 
					||((popped_cntrl_st->node->get_node_value()).compare("Conc") == 0) )
				{
					if(verbose_on || debug_on)
						cout<<"\tIt is primitive function: <STERN>"<<endl;
					//do appropriate action
					//create new execution element
					execution_element *new_exe_ele = new execution_element;
					new_exe_ele->type = EXE_PRIMITIVE_FUNC;
					if((popped_cntrl_st->node->get_node_value()).compare("Stern") == 0 )
						new_exe_ele->primitive_fun_var =  STERN;
					else if((popped_cntrl_st->node->get_node_value()).compare("Stem") == 0 )
						new_exe_ele->primitive_fun_var = STEM;	
					else{
						new_exe_ele->primitive_fun_var = CONC;
						new_exe_ele->conc_applied = false;
					}
					execution_stack.push(new_exe_ele); //push
					if(verbose_on)
						cout<<"\tPushed <STRING> function on execution stack"<<endl;
					//delete popped_cntrl_st;
				}


				else{  	//if it isn't then it isn't declared so throw some appropriate error to appease the GODs of RPAL
					cout<<"\nIDENTIFIER : <"<<(popped_cntrl_st->node->get_node_value())<<"> is not declared !"<<endl;
					exit(0);
				}
			}

			else{ //identifier's value found in some environment //we have an execution element in idt_exe_ele
				if(verbose_on || debug_on){
					cout<<"\tValue of <"<<popped_cntrl_st->node->get_node_value()<<"> found:";
					print_execution_element(idt_exe_ele);
					cout<<endl;
				}
				execution_stack.push(idt_exe_ele);
				if(verbose_on){
					cout<<"\tPushed execution element corresponding to: "<<popped_cntrl_st->node->get_node_value()<<" on execution stack"<<endl;
					cout<<"\tSize of Execution Stack: "<<execution_stack.size() ;
				}
			}

			if(verbose_on || debug_on)
				getchar();
		}

		else if (popped_cntrl_st->node->get_node_type() == LAMBDA){ //if node is LAMBDA
			if(verbose_on || debug_on)
				cout<<"\tLambda node..........."<<endl;

			

			//anoint it with current environment
			popped_cntrl_st->env = current_env;

			//create new execution_element
			execution_element *new_exe_ele = new execution_element();
			new_exe_ele->type = EXE_CTRL_STRUCT; //set type
			new_exe_ele->ctrl_st_var = popped_cntrl_st;
			
			//push it on execution stack
			execution_stack.push(new_exe_ele);
			if(verbose_on){
				cout<<"\tPushed <LAMBDA> on execution stack"<<endl;
				cout<<"\tSize of EXECUTION STACK: "<<execution_stack.size()<<endl;
			}
//DEBUGGIN
			/*if(verbose_on){
				cout<<"\t<Lambda>'s bound variable: "<<popped_cntrl_st->bound_var->get_node_value()<<endl;
				print_control_struct( (execution_stack.top())->ctrl_st_var->delta);

			} */
			if(verbose_on || debug_on)
				getchar();
		}
		
		/* TODO - Remember to update the current environment */
		else if(popped_cntrl_st->node->get_node_type() == ENV) { //if it is ENVIRONMENT
			if(verbose_on)
				cout<<"\tENVIRONMENT node on control stack..."<<endl;

			//pop from execution stack and save it
			execution_element *saved_exe_ele = execution_stack.top();
			if(saved_exe_ele != NULL){
				execution_stack.pop(); //pop another element
				execution_element *another_exe_ele = execution_stack.top();

				//it should be same environment as control stack's one
				if( !( (another_exe_ele->type == EXE_ENV) && ( (another_exe_ele->env_var) ==  (popped_cntrl_st->env) ) )){ //confirm if it is environment and same as the popped one
					cout<<"ERROR - was expecting an ENVIRONMENT element on execution stack !"<<endl;
					exit(0);
				}
				execution_stack.pop(); //ENV was matched so pop it	
				execution_stack.push(saved_exe_ele);
				if(debug_on || verbose_on){
					cout<<"\tConfirmed ENVIRONMENT node"<<endl;
					cout<<"\tPushed back the element. Size of Execution Stack: "<<execution_stack.size()<<endl;
				}

			}
			else{ //go NULL from stack
				cout<<"ERROR - Was expecting some element on execution stack but got NULL !"<<endl;
				exit(0);
			}

			//UPDATE current environment
			stack<execution_element*> temp_stack;
			while( execution_stack.size() != 0 && ((execution_stack.top())->type != EXE_ENV) ){
				temp_stack.push( execution_stack.top() );
				execution_stack.pop();
			}
			
			if( execution_stack.size() != 0){
				if(verbose_on)
					cout<<"\tUpdating current_env..."<<endl;
					current_env = (execution_stack.top())->env_var; //we have current environment
			}
			if(verbose_on)
				cout<<"\tCurrent Env updated "<<endl;
			//put back the elements
			while( temp_stack.size() != 0){
				execution_stack.push( temp_stack.top() );
				temp_stack.pop();
			}
			
			if(verbose_on)
				cout<<"\tPushed back all elements"<<endl;  

			if(verbose_on || debug_on)
				getchar();
				
		}

		else if(popped_cntrl_st->node->get_node_type() == TAU){ //if node is TAU
			if(verbose_on)
				cout<<"\tTAU node....."<<endl;
			
			//create an execution element
			execution_element *new_exec_ele = new execution_element; //it will have queue
			new_exec_ele->type = EXE_TUPPLE;
			
			//pop n element from execution stack
			int n = popped_cntrl_st->get_num_children();
			if(debug_on || verbose_on)
				cout<<"\tThis TAU node has "<<n<<" number of children"<<endl;
			
			for(int i=0; i<n; i++){
				execution_element *temp_ele = execution_stack.top();
				if(temp_ele == NULL) {
					cout<<"\nERROR: Processing TAU node. Expected number of children: "<<n<<" .But Execution Stack has less"<<endl;
					exit(0);
				}
				
				if(debug_on || verbose_on){
					cout<<"\tPopped element: ";
					print_execution_element(temp_ele);
					cout<<endl;
				}
				(new_exec_ele->queue_var).push(temp_ele); //LIFO
				execution_stack.pop();

				if(verbose_on)
					cout<<"\tPopped a children from execution stack. Size of execution stack: "<<execution_stack.size()<<endl;
			}

			//push back the queue into execution stack
			execution_stack.push(new_exec_ele);

			if(verbose_on){
				cout<<"\tPushed back the queue (Tupple): ";
				print_execution_element(new_exec_ele);
				cout<<"\tSize of execution stack: "<<execution_stack.size()<<endl;
			}

			if(verbose_on || debug_on)
				getchar();

		}
		

		//Handle <YSTAR> node
		else if(popped_cntrl_st->node->get_node_type() == YSTAR){
			if(verbose_on)
				cout<<"\t<YSTAR> node on control stack..."<<endl;
			
			//create a new execution element of type EXE_YSTAR and push it in execution stack
			execution_element *new_y_star = new execution_element;
			new_y_star->type = EXE_YSTAR;

			execution_stack.push(new_y_star);

			if(verbose_on || debug_on){
				cout<<"\tPushed <YSTAR> node on execution stack: ";
				print_execution_element(new_y_star);
				cout<<"\tSize of Execution Stack : "<<execution_stack.size();
			}
		}

		else if(popped_cntrl_st->node->get_node_type() == GAMMA){ //if node is GAMMA
			if(verbose_on)
				cout<<"\tGAMMA node..............."<<endl;
			//check the element on execution stack
			if( (execution_stack.top())->type == EXE_CTRL_STRUCT ) { //there is lambda on execution stack - RULE 3
				if(verbose_on || debug_on)
					cout<<"\tfound control stack i.e. lambda on execution stack"<<endl;

				//pop the element from execution stack
				execution_element *temp_exe_ele = execution_stack.top(); //temp_exe_ele contains control structure
				execution_stack.pop();
				if(verbose_on){
					cout<<"\tPopped the control struct from Execution stack"<<endl;
					cout<<"\tSize of Execution Stack: "<<execution_stack.size()<<endl;
				}

				//create new environment
				environment *new_env = new environment();

				//link the new environment to environment in control struct
				new_env->parent = temp_exe_ele->ctrl_st_var->env;
				
				//bound_var shouldn't be NULL
				if(temp_exe_ele->ctrl_st_var->bound_var == NULL) {
					cout<<"Bound variable of this popped control structure is NULL!"<<endl;
					exit(0);
				}
				//bound the new variable - could have two cases
					//assuming single binding
					//pop an element from exection stack
				execution_element *bind_exe_ele = execution_stack.top();
				execution_stack.pop();
				//if bind_exe_ele is TUPPLE  we must have linked list in popped control struct's bound_var
				//BUG - assuming that if there is a tupple on Execution Stack then there must be same number of variables to be bound which isn't true
				if(bind_exe_ele->type == EXE_TUPPLE){
					if(verbose_on)
						cout<<"\tPopped element from Execution stack is TUPPLE. so bound variable must be linked list"<<endl;
					//so we have Linked list in temp_exe_ele->ctrl_st_var->bound_var AND TUPPLE in bind_exe_ele
					AST_node *temp_bound_var = temp_exe_ele->ctrl_st_var->bound_var;

					//if bound variable is not linked list bound whole tupple to the variable
					if( temp_bound_var->right_sibling == NULL){
						if(verbose_on)
							cout<<"\tThere is only single bound variable.."<<endl;

						new_env->insert(temp_bound_var->get_node_value(), bind_exe_ele);
					}

					else{

						//BUG
						queue<execution_element*> temp_for_tuple;

						while(temp_bound_var != NULL){
							//get the AST_nodes value to use as key
							string temp_key = temp_bound_var->get_node_value();
							if(verbose_on || debug_on)
								cout<<"\tValue of key: "<<temp_key<<endl;


							//get the value from queue
							execution_element *popped_from_queue = (bind_exe_ele->queue_var).front();
							if(popped_from_queue == NULL){
								cout<<"ERROR - Binding variables - Tupple on execution has less number of elements ! (Yeah I know its not the world's best user friendly error message)"<<endl;
								exit(0);
							}
							temp_for_tuple.push(popped_from_queue); //preserve the tuple
							(bind_exe_ele->queue_var).pop(); //pop the element;
							//insert key and value
							new_env->insert(temp_key, popped_from_queue);
							if(debug_on || verbose_on) {
								cout<<"\tValue of bound variable: ";
								print_execution_element(popped_from_queue);
								cout<<endl; 
							}

							temp_bound_var = temp_bound_var->right_sibling;
							
						}

						while( temp_for_tuple.size() != 0){
							(bind_exe_ele->queue_var).push( temp_for_tuple.front() );
							temp_for_tuple.pop();
						}
					}
				}

				else{
					if(verbose_on)
						cout<<"\tPopped element from Execution Stack is not TUPPLE. i.e. there is only single bound variable"<<endl;

					//add this element with key from temp_exe_ele's bound var in new environment
					new_env->insert(temp_exe_ele->ctrl_st_var->bound_var->get_node_value(), bind_exe_ele);
					
					if(debug_on || verbose_on) {
						cout<<"\tValue of bound variable: ";
						print_execution_element(bind_exe_ele);
						cout<<endl; 
					}
				}
				
				
				

					
				//change the current environment to new environment
				current_env = new_env;
				execution_element *new_exe_ele = new execution_element();
				new_exe_ele->type = EXE_ENV;
				new_exe_ele->env_var = new_env;

				//push the new environment in both control stack and execution stack
				//push it in execution_stack
				execution_stack.push(new_exe_ele);

				//create new control struct and push it in control stack
				AST_node *new_ast_node = new AST_node;
				new_ast_node->set_node_type(ENV);

				control_st *new_control_st = new control_st;
				new_control_st->node = new_ast_node; //this node will tell the type of control struct in execution
				new_control_st->env = current_env; //this stores the environment
				control_stack.push(new_control_st);


				//load control stack with popped control stack's delta
				if(temp_exe_ele->ctrl_st_var->delta == NULL) {
					cout<<"this lambda doesn't have delta !! "<<endl;
					exit(0);
				}
				load(temp_exe_ele->ctrl_st_var->delta);

				//load(temp_exe_ele->ctrl_st_var->delta);

			}

			//Handle <Ystar> node
			else if( (execution_stack.top())->type == EXE_YSTAR ){
				if(verbose_on)
					cout<<"\t <Ystar> node it is on execution stack.."<<endl;
				//pop <YSTAR> node and discard it
				execution_stack.pop();
				//pop <CS> node and change its type from <CS> to <ITA>
				execution_element *temp_cs = execution_stack.top(); // shouldn't be NULL
				if( temp_cs == NULL || temp_cs->type != EXE_CTRL_STRUCT ){
					cout<<"ERROR -Processing <YSTAR> node. Was expecting <LAMBDA> on execution stack but it is either NULL or some other type !"<<endl;
					exit(0);
				}
				
				/* NOTE - ctrl_st_var in execution_element will store both */
				temp_cs->type = EXE_ITA; //change its type
				
				if(verbose_on || debug_on){
					cout<<"\tChanged type of <EXE_CTRL_STRUCT> node to <EXE_ITA> : ";
					print_execution_element(temp_cs);
					cout<<"\tSize of Execution Stack : "<<execution_stack.size();
				}

				//DEBUGGING
				//cout<<"**************************LOADING************"<<endl;
				//load(temp_cs->ctrl_st_var->delta);


				
			}

			//Handle <EXE_ITA> 
			else if((execution_stack.top())->type == EXE_ITA){
				i++;
				if(verbose_on)
					cout<<"\t<EXE_ITA> node on execution stack..."<<endl;

				//create new execution element of type EXE_CTRL_STRUCT
				execution_element *new_ctrl_struct = new execution_element;
				new_ctrl_struct->type = EXE_CTRL_STRUCT;
				
				//BUG - there couldn't be any top element on stack
				new_ctrl_struct->ctrl_st_var = (execution_stack.top())->ctrl_st_var;
				
				/*if(i == 1){
				cout<<"**************************************"<<endl;
				load(new_ctrl_struct->ctrl_st_var->delta);
				} */
				//push the new element
				execution_stack.push(new_ctrl_struct);

				if(verbose_on || debug_on){
					cout<<"\tPushed <EXE_CTRL_STRUCT> node: ";
					print_execution_element(new_ctrl_struct);
					cout<<"\tSize of Execution Stack : "<<execution_stack.size();
				}



				//create new GAMMA ast node and new control stru and push it in CS
				AST_node *another_gamma_to_push = new AST_node;
				another_gamma_to_push->set_node_type(GAMMA);

				control_st *another_gamma_cs = new control_st;
				another_gamma_cs->node =another_gamma_to_push;

				control_stack.push(another_gamma_cs);



				//push two gamma CS in CS Stack
				//control_stack.push(popped_cntrl_st); //pop back the same gamma

				//create new GAMMA ast node and new control stru and push it in CS
				AST_node *new_gamma_to_push = new AST_node;
				new_gamma_to_push->set_node_type(GAMMA);

				control_st *new_gamma_cs = new control_st;
				new_gamma_cs->node = new_gamma_to_push;

				control_stack.push(new_gamma_cs);

				if(verbose_on)
					cout<<"\tPushed two <GAMMA> node in Control stack.."<<endl;
			}
			//must be some primitive function
			else if( (execution_stack.top())->type == EXE_PRIMITIVE_FUNC) {
				if(verbose_on)
					cout<<"\t PRIMITIVE FUNCTION it is..."<<endl;
				//pop it
				execution_element *popped_primitive_func = execution_stack.top();
				execution_stack.pop();
				
				if(popped_primitive_func->primitive_fun_var == PRINT){
				
				//switch(popped_primitive_func->primitive_fun_var){
					//case PRINT:
					if(verbose_on)
						cout<<"\tGot PRINT Function.."<<endl;
					//pop the node and print it
					execution_element *another_ele = execution_stack.top(); //shouldn't be null
					if(another_ele == NULL){
						cout<<"ERROR - Was expecting an element to Print but found NULL please check PRINT again"<<endl;
						exit(0);
					}
					//pop the element
					execution_stack.pop(); 
					print_execution_element(another_ele); //print it
					//delete another_ele;	

					//store the DUMMY in execution stack
					execution_element *new_exec_ele = new execution_element;
					new_exec_ele->type = EXE_DUMMY;
					execution_stack.push(new_exec_ele);

					if(verbose_on){
						cout<<"Pushed the element: ";
						print_execution_element(new_exec_ele);
						cout<<"\nSize of Execution Stack is: "<<execution_stack.size()<<endl;
					}

					//break;
				}
			
				else if(popped_primitive_func->primitive_fun_var == ITOS){
				
				//switch(popped_primitive_func->primitive_fun_var){
					//case PRINT:
					if(verbose_on)
						cout<<"\tGot ITOS Function.."<<endl;
					//pop the node and print it
					execution_element *another_ele = execution_stack.top(); //shouldn't be null
					if(another_ele == NULL){
						cout<<"ERROR - Was expecting an element to Print but found NULL please check PRINT again"<<endl;
						exit(0);
					}
					//pop the element
					execution_stack.pop(); 
					if( another_ele->type != EXE_INT){
						cout<<"ERROR - ITOS operands invalied"<<endl;
						exit(0);
					}
					stringstream ss;
					ss << (another_ele->int_var);
					string temp_string = ss.str();
					//sprintf((char*)(temp_string.c_str()), "%d", another_ele->int_var);
					//itoa( another_ele->int_var, temp_string.c_str(), 10);

					execution_element *itos_ele = new execution_element;
					itos_ele->type = EXE_STR;
					itos_ele->string_var = temp_string;
					execution_stack.push(itos_ele);
					//another_ele->type = EXE_STR;
					//another_ele->string_var = temp_string;

					if(verbose_on)
						cout<<"\tConverted string: "<<temp_string;
					//print_execution_element(another_ele); //print it
					//delete another_ele;	

					//store the DUMMY in execution stack
					/*execution_element *new_exec_ele = new execution_element;
					new_exec_ele->type = EXE_STR;
					execution_stack.push(new_exec_ele);

					if(verbose_on){
						cout<<"Pushed the element: ";
						print_execution_element(new_exec_ele);
						cout<<"\nSize of Execution Stack is: "<<execution_stack.size()<<endl;
					 */

					//break;
				}
				
					
					
					
					
					
					
					
					
					
					
					
					
					
					else if(popped_primitive_func->primitive_fun_var == ISINTEGER){
					if(verbose_on)
						cout<<"\tGot <ISINTEGER> function on Execution Stack.."<<endl;

					bool isinteger;
					execution_element *check_integer= execution_stack.top();
					if( check_integer== NULL){
						cout<<"ERROR - Please check Istuple again.."<<endl;
						exit(0);
					}
					
					execution_stack.pop();
					execution_element *new_check_int= new execution_element;
					if(check_integer->type == EXE_INT)
						new_check_int->type = EXE_TRUE;
					else
						new_check_int->type = EXE_FALSE;
					
					execution_stack.push(new_check_int);
					if(verbose_on){
						cout<<"\tPushed element: ";
						print_execution_element(new_check_int);
						cout<<"\tSize of execution stack: "<<execution_stack.size();
					}
				}

				else if(popped_primitive_func->primitive_fun_var == ISTRUTHVALUE){
					if(verbose_on)
						cout<<"\tGot <ISTRUTHVALUE> function on Execution Stack.."<<endl;

					bool istuple ;
					execution_element *check_tuple = execution_stack.top();
					if( check_tuple == NULL){
						cout<<"ERROR - Please check Istruthvalue again.."<<endl;
						exit(0);
					}
					
					execution_stack.pop();
					execution_element *new_check_tuple = new execution_element;
					if(check_tuple->type == EXE_FALSE || check_tuple->type == EXE_TRUE)
						new_check_tuple->type = EXE_TRUE;
					else
						new_check_tuple->type = EXE_FALSE;
					
					execution_stack.push(new_check_tuple);
					if(verbose_on){
						cout<<"\tPushed element: ";
						print_execution_element(new_check_tuple);
						cout<<"\tSize of execution stack: "<<execution_stack.size();
					}
				}



				else if(popped_primitive_func->primitive_fun_var == ISSTRING){
					if(verbose_on)
						cout<<"\tGot <ISSTRING> function on Execution Stack.."<<endl;

					bool istuple ;
					execution_element *check_tuple = execution_stack.top();
					if( check_tuple == NULL){
						cout<<"ERROR - Please check Isstring again.."<<endl;
						exit(0);
					}
					
					execution_stack.pop();
					execution_element *new_check_tuple = new execution_element;
					if(check_tuple->type == EXE_STR)
						new_check_tuple->type = EXE_TRUE;
					else
						new_check_tuple->type = EXE_FALSE;
					
					execution_stack.push(new_check_tuple);
					if(verbose_on){
						cout<<"\tPushed element: ";
						print_execution_element(new_check_tuple);
						cout<<"\tSize of execution stack: "<<execution_stack.size();
					}
				}


				else if(popped_primitive_func->primitive_fun_var == ISTUPLE){
					if(verbose_on)
						cout<<"\tGot <ISTUPLE> function on Execution Stack.."<<endl;

					bool istuple ;
					execution_element *check_tuple = execution_stack.top();
					if( check_tuple == NULL){
						cout<<"ERROR - Please check Istuple again.."<<endl;
						exit(0);
					}
					
					execution_stack.pop();
					execution_element *new_check_tuple = new execution_element;
					if(check_tuple->type == EXE_TUPPLE || check_tuple->type == EXE_NILL)
						new_check_tuple->type = EXE_TRUE;
					else
						new_check_tuple->type = EXE_FALSE;
					
					execution_stack.push(new_check_tuple);
					if(verbose_on){
						cout<<"\tPushed element: ";
						print_execution_element(new_check_tuple);
						cout<<"\tSize of execution stack: "<<execution_stack.size();
					}
				}

				//Handle ORDER
				else if(popped_primitive_func->primitive_fun_var == ORDER){
					if(verbose_on)
						cout<<"\tGot <ORDER> Function on Execution Stack.."<<endl;
				
					//pop an element
					execution_element *temp_tupple = execution_stack.top();
					
					//it should be <TUPPLE>
					if( temp_tupple == NULL || (temp_tupple->type != EXE_TUPPLE && temp_tupple->type != EXE_NILL) ){ 
						cout<<"ERROR - Was expecting a tupple next to ORDER. Please check ORDER again"<<endl;
						exit(0);
					}
					execution_stack.pop();

					//count number of elements in queue.
					int num_elements;
					if(temp_tupple->type == EXE_NILL)
						num_elements = 0;
					else
						num_elements = (temp_tupple->queue_var).size();

					if(verbose_on || debug_on)
						cout<<"\tNumber of elements in the Tupple: "<<num_elements<<endl;

					//change the type and value of the execution element 
					execution_element *new_int_element = new execution_element;
					new_int_element->type = EXE_INT;
					new_int_element->int_var = num_elements;
					execution_stack.push(new_int_element);
					
					if(verbose_on){
						cout<<"\tPushed back the integer element: ";
						print_execution_element(new_int_element); //print it
						cout<<"\tSize of execution stack: "<<execution_stack.size();
					}
				}
			
				else if(popped_primitive_func->primitive_fun_var == STEM){
					if(verbose_on)
						cout<<"\tGot <STEM> function..."<<endl;

					execution_element *stem_element = execution_stack.top();
					if( stem_element ==  NULL || stem_element->type != EXE_STR){
						cout<<"ERROR - Stem's operand is invalid.."<<endl;
						exit(0);
					}

					execution_stack.pop();
					string stem_string = stem_element->string_var;
					stem_string = stem_string.substr(0, 1);
					if(verbose_on)
						cout<<"\tResultant string: "<<stem_string<<endl;
					//stem_element->string_var = stem_string;
					execution_element *new_stem_ele = new execution_element;
					new_stem_ele->type = EXE_STR;
					new_stem_ele->string_var = stem_string;
					execution_stack.push(new_stem_ele);

					if(verbose_on){
						cout<<"Pushed the element: ";
						print_execution_element(new_stem_ele);
						cout<<"\nSize of Execution Stack is: "<<execution_stack.size()<<endl;
					}
				}
				

				//CONC needs two gamma
				else if (popped_primitive_func->primitive_fun_var == CONC){
					if(verbose_on)
						cout<<"\tGot <CONC> Function..."<<endl;

					//pop two strings
					execution_element *first_string = execution_stack.top();
					if(first_string == NULL || first_string->type != EXE_STR){
						cout<<"ERROR - CONC's first operand is invalid"<<endl;
						exit(0);
					}
					execution_stack.pop();
				
					//if conc_applied is false means conc hasn't been applied yet so just store the string
					if(popped_primitive_func->conc_applied == false){
						popped_primitive_func->string_var = first_string->string_var;
						popped_primitive_func->conc_applied = true;
						execution_stack.push(popped_primitive_func);
					}

					else{ //conc had been applied. just concetenat the strings (funny spelling)
						if(verbose_on)
							cout<<"\tConc had been applied once..."<<endl;
					
						execution_element *conc_string = new execution_element;
						conc_string->type = EXE_STR;
						conc_string->string_var = popped_primitive_func->string_var + first_string->string_var ;
						execution_stack.push(conc_string);
						if(verbose_on)
							cout<<"\tConc string: "<<conc_string->string_var<<endl; 


					}
					
					/*execution_element *second_string = execution_stack.top();
					if(second_string == NULL || second_string->type != EXE_STR){
						cout<<"ERROR - CONC's second operand is invalid"<<endl;
						exit(0);
					}
					execution_stack.pop(); */

					
				}
				else if(popped_primitive_func->primitive_fun_var == STERN){
					//case STERN:
					if(verbose_on)
						cout<<"\tGot STERN Function.."<<endl;

					//pop the node 
					execution_element *stern_element= execution_stack.top(); //shouldn't be null
					if(stern_element== NULL || stern_element->type != EXE_STR){ //should be string
						cout<<"ERROR - Was expecting an element to Stern but found NULL or not string please check STERN again"<<endl;
						exit(0);
					}
					//pop the element
					execution_stack.pop(); 

					if(verbose_on){
						cout<<"\tPopped element: ";
						print_execution_element(stern_element); //print it
						cout<<"\tSize of execution stack: "<<execution_stack.size();
					}
					//delete stern_element;	
					
					//apply stern
					string stern_string = stern_element->string_var;

					//TODO - THERE ARE NO ERROR CHECKS. Lets see if we will get time
					string resultant_string = stern_string.substr(1, stern_string.size() ); //sterning
					/*//store the DUMMY in execution stack
					execution_element *new_exec_ele = new execution_element;
					new_exec_ele->type = EXE_DUMMY;
					execution_stack.push(new_exec_ele); */
					execution_element *new_stern_exe = new execution_element;
					new_stern_exe->type = EXE_STR;
					new_stern_exe->string_var = resultant_string;
					//stern_element->string_var = resultant_string;
					execution_stack.push(new_stern_exe);

					if(verbose_on){
						cout<<"Pushed the element: ";
						print_execution_element(new_stern_exe);
						cout<<"\nSize of Execution Stack is: "<<execution_stack.size()<<endl;
					}

				//	break;

				}

			}

			else if((execution_stack.top())->type == EXE_TUPPLE){ //if there is TUPPLE on Execution Stack
				if(verbose_on)
					cout<<"\t<TUPPLE> on Execution Stack "<<endl;

				//pop the tupple
				execution_element *tupple_exe_ele = execution_stack.top(); //this contains queue
				execution_stack.pop();

				//pop another element. It should be INTEGER
				execution_element *second_exe_ele = execution_stack.top();
				if(second_exe_ele == NULL || second_exe_ele->type != EXE_INT){ //should be INTEGER only
					cout<<"ERROR - Processing <GAMMA>, <TUPPLE> on Execution Stack - Was expecting an <INTEGER> but either there is nothing or it isn't <INTEGER>"<<endl;
					exit(0);
				}

				execution_stack.pop();
				if(verbose_on){
					cout<<"\tPopped an <INTEGER> from Execution Stack:";
					print_execution_element(second_exe_ele);
					cout<<"\tExecution Stack size: "<<execution_stack.size();
				}
				
				if(verbose_on)
					cout<<"\tSize of popped tupple: "<<(tupple_exe_ele->queue_var).size()<<endl;
				queue<execution_element*> temp_queue; //temp queue
				for(int i=1; i< (second_exe_ele->int_var); i++){
					if((tupple_exe_ele->queue_var).size() == 0){
						cout<<"ERROR - Processing <GAMMA>, <TUPPLE> on Execution Stack - There are less number of elements in TUPPLE than the <INTEGER>"<<endl;
						exit(0);
					}
					//BUG - pop is deleting the element. we need to preserve the queue

					temp_queue.push( (tupple_exe_ele->queue_var).front() ); //pop n-1 elements
					(tupple_exe_ele->queue_var).pop(); 
				}

				execution_element *resultant_ele = new execution_element;
				execution_element *temp_ex_el = (tupple_exe_ele->queue_var).front(); //could be anything
				while( (tupple_exe_ele->queue_var).size() != 0){
					temp_queue.push( (tupple_exe_ele->queue_var).front() );
					(tupple_exe_ele->queue_var).pop();
				}

				//REALLY SH**TY way to do this
				resultant_ele->type = temp_ex_el->type; //same type
				resultant_ele->int_var = temp_ex_el->int_var;
				resultant_ele->string_var= temp_ex_el->string_var;
				resultant_ele->queue_var= temp_ex_el->queue_var;
				resultant_ele->ctrl_st_var = temp_ex_el->ctrl_st_var;
				resultant_ele->env_var = temp_ex_el->env_var;
				resultant_ele->primitive_fun_var= temp_ex_el->primitive_fun_var;

				if(resultant_ele == NULL){
					cout<<"ERROR - Processing <GAMMA>, <TUPPLE> on Execution Stack..Processing last element- There are less number of elements in TUPPLE than the <INTEGER>"<<endl;
					exit(0);
				}

				//now put back the popped elements
				//this will preserve the queue
				while( temp_queue.size() != 0){
					(tupple_exe_ele->queue_var).push( temp_queue.front() );
					temp_queue.pop();
				}
				//(tupple_exe_ele->queue_var).pop(); //pop the element

				//push the resultant element in execution stack
				execution_stack.push(resultant_ele);
					
				if(verbose_on){
					cout<<"\tPushed the element: ";
					print_execution_element(resultant_ele);
					cout<<"\tTupple is: ";
					print_execution_element(tupple_exe_ele);
					cout<<"\n\tSize of Execution Stack: "<<execution_stack.size()<<endl;
				}
			}
			else{
				cout<<"WHILE PROCESSING GAMMA - SOME OTHER TYPE !! "<<endl;	
				print_execution_element( execution_stack.top() );
				exit(0);
			}

			if(verbose_on || debug_on)
				getchar();
		}


		else if( (popped_cntrl_st->node->get_node_type() == BETA) ) { //BETA Node
			if(verbose_on)
				cout<<"\t<BETA> node it is...."<<endl;

			//pop an element from exeuction stack. It should be either TRUE or FALSE
			execution_element *beta_exe_element = execution_stack.top();
			if(beta_exe_element == NULL){
				cout<<"ERROR - While processing BETA node - Execution Stack is empty ! "<<endl;
				exit(0);
			}

			execution_stack.pop();
			if( !(beta_exe_element->type == EXE_TRUE || beta_exe_element->type == EXE_FALSE) ){
				cout<<"ERROR - While processing BETA node - Expected truth value of exectuion stack but got something else! "<<endl;
				exit(0);
			}
			

			control_st *load_delta = NULL;
			//we have truth value in beta_exe_element
			if(beta_exe_element->type == EXE_TRUE){
				if(verbose_on)
					cout<<"\tPopped element is <TRUE>"<<endl;

				//pop an element from CONTROL STACK and discard and load next element
				control_st *temp_delta = control_stack.top(); //it should be delta else
				if( temp_delta == NULL || temp_delta->node->get_node_type() != DELTA_ELSE ){
					cout<<"ERROR - Processing <BETA> was expecting DELTA_ELSE but it some other type (may be NULL)"<<endl;
					exit(0);
				}
				control_stack.pop();

				temp_delta = control_stack.top(); //access delta_then
				if(temp_delta == NULL || temp_delta->node->get_node_type() != DELTA_THEN){
					cout<<"ERROR - Processing <BETA> was expecting DELTA_THEN but it some other type (may be NULL)"<<endl;
					exit(0);
				}
				control_stack.pop();

				load_delta = temp_delta;
			}

			else{
				if(verbose_on)
					cout<<"\tPopped element is <FALSE>"<<endl;
				
				control_st *temp_delta = control_stack.top();
				if( temp_delta == NULL || temp_delta->node->get_node_type() != DELTA_ELSE ){
					cout<<"ERROR - Processing <BETA> was expecting DELTA_ELSE but it some other type (may be NULL)"<<endl;
					exit(0);
				}
				control_stack.pop();

				control_st *temp_delta_discard = control_stack.top(); //access delta_then
				if(temp_delta_discard == NULL || temp_delta_discard->node->get_node_type() != DELTA_THEN){
					cout<<"ERROR - Processing <BETA> was expecting DELTA_THENbut it some other type (may be NULL)"<<endl;
					exit(0);
				}
				control_stack.pop();

				load_delta = temp_delta;
			}

			//load control struct
			load(load_delta->delta);

				
		}
		else if(popped_cntrl_st->node->get_node_type() == EQ
				|| popped_cntrl_st->node->get_node_type() == NE){
			if(verbose_on)
				cout<<"\t<BINARY EQ> or <NE> node it is..."<<endl;

			//pop two element
			execution_element *temp_first = execution_stack.top(); //shouldn't be NULL
			if(temp_first == NULL){
				cout<<"ERROR - Processing EQ operator. But there are less than two elements on Execution Stack ! "<<endl;
				exit(0);
			}
			execution_stack.pop(); //actually pop the element
			if(verbose_on){
				cout<<"\tPopped the element: ";
				print_execution_element(temp_first);
				cout<<"\n\tSize of Execution Stack: "<<execution_stack.size()<<endl;
			}

			execution_element *temp_second = execution_stack.top();
			if(temp_second == NULL){
				cout<<"ERROR - Processing EQ operator. But there are less than two elements on Execution Stack ! "<<endl;
				exit(0);
			}
			execution_stack.pop(); //actually pop the element
			if(verbose_on){
				cout<<"\tPopped the element: ";
				print_execution_element(temp_second);
				cout<<"\n\tSize of Execution Stack: "<<execution_stack.size()<<endl;
			}
			
			bool temp_result;
			//both should be of either STRING, INTEGER or BOOL
			if( temp_first->type == EXE_STR && temp_second->type == EXE_STR){
				if(verbose_on)
					cout<<"\tBoth are of type <STRING>"<<endl;
				if(popped_cntrl_st->node->get_node_type() == EQ)
					temp_result = !((temp_first->string_var).compare(temp_second->string_var));
				else
					temp_result = ((temp_first->string_var).compare(temp_second->string_var));
			}

			else if(temp_first->type == EXE_INT && temp_second->type == EXE_INT){ //both are INTEGERS
				if(verbose_on)
					cout<<"\tBoth are of type <INTEGERS>"<<endl;

				if(popped_cntrl_st->node->get_node_type() == EQ)
					temp_result = (temp_first->int_var == temp_second->int_var) ;
				else
					temp_result = (temp_first->int_var != temp_second->int_var);
			}

			else if( (temp_first->type == EXE_TRUE ||temp_first->type== EXE_FALSE) && (temp_second->type == EXE_FALSE || temp_second->type == EXE_TRUE) ){
				if(verbose_on)
					cout<<"\tBoth are of type bool"<<endl;

				
				if(popped_cntrl_st->node->get_node_type() == EQ){

					if( temp_first->type == temp_second->type)
						temp_result = true;
					else
						temp_result = false;
				}
				else{
					if( temp_first->type != temp_second->type)
						temp_result = true;
					else
						temp_result = false;
				}
			}

			else{
				cout<<"ERROR - EQ operator's operands should have been of same type. "<<endl;
				exit(0);
			}

			if(verbose_on || debug_on)
				cout<<"\tResult is (value of temp_result) : "<<temp_result<<endl;;
			//create new execution element
			execution_element *new_eq_exe_ele = new execution_element;
			if(temp_result)
				new_eq_exe_ele->type = EXE_TRUE;
			else
				new_eq_exe_ele->type = EXE_FALSE;

			//push into execution stack
			execution_stack.push(new_eq_exe_ele);

			if(verbose_on || debug_on){
				cout<<"Pushed element on execution stack : ";
				print_execution_element(new_eq_exe_ele);
				cout<<"\nSize of Execution Stack: "<<execution_stack.size()<<endl;

			}
		}

		//AUG Operator
		else if(popped_cntrl_st->node->get_node_type() == AUG){
			if(verbose_on)
				cout<<"\tGot <AUG> Operator..."<<endl;
		
			//pop first element
			execution_element *temp_first_tupple = execution_stack.top();
			if(temp_first_tupple == NULL){
				cout<<"ERROR - Processing AUG function. Was expecting <TUPPLE> or <NIL> but received NULL !"<<endl;
				exit(0);
			}

			execution_stack.pop();
			if(verbose_on){
				cout<<"Popped element from execution stack: ";
				print_execution_element(temp_first_tupple);
				cout<<"Size of execution Stack: "<<execution_stack.size();
			}

			//pop second element
			execution_element *temp_second_tupple= execution_stack.top();
			if(temp_second_tupple == NULL){
				cout<<"ERROR - Processing AUG function. Was expecting <TUPPLE> or <NIL> but received NULL (second operand) !"<<endl;
				exit(0);
			}

			execution_stack.pop();
			if(verbose_on){
				cout<<"Popped element from execution stack: ";
				print_execution_element(temp_second_tupple);
				cout<<"Size of execution Stack: "<<execution_stack.size();
			}

			//first element has to be either TUPPLE or NIL
			if( !(temp_first_tupple->type == EXE_TUPPLE || temp_first_tupple->type == EXE_NILL) ){
				cout<<"ERROR - CSE_machine - was expecting first operand of AUG to be either TUPPLE or NIL ! Please check AUG again"<<endl;
				exit(0);
			}

			//second element should either be INT, STR, TRUE, FALSE, NIL, DUMMY or TUPPLE
			if(!(temp_second_tupple->type == EXE_TUPPLE || temp_second_tupple->type == EXE_NILL || temp_second_tupple->type == EXE_INT || temp_second_tupple->type == EXE_STR || temp_second_tupple->type == EXE_TRUE || temp_second_tupple->type == EXE_FALSE || temp_second_tupple->type == EXE_DUMMY)) {
				cout<<"ERROR - CSE_machine - was expecting second operand of AUG to be either TUPPLE, DUMMY, TRUE, FALSE, INTEGER, STRING or NIL ! Please check AUG again"<<endl;
				exit(0);
			}

			execution_element *new_aug_tuple = new execution_element;
			new_aug_tuple->type = EXE_TUPPLE;

			queue<execution_element*> preserve_tuple;

			//if first element is TUPPLE
			if( (temp_first_tupple->type != EXE_NILL)){
				while( (temp_first_tupple->queue_var).size() != 0 ){
					(new_aug_tuple->queue_var).push( (temp_first_tupple->queue_var).front() );
					preserve_tuple.push((temp_first_tupple->queue_var).front()); //to preserve tuple
					(temp_first_tupple->queue_var).pop(); //BUG changing tupple
				}

			}
			//preserve tuple
			while( preserve_tuple.size() !=0){
				(temp_first_tupple->queue_var).push( preserve_tuple.front() );
				preserve_tuple.pop();
			}

				//change the type of it
				//temp_first_tupple->type = EXE_TUPPLE;
			(new_aug_tuple->queue_var).push( temp_second_tupple);
			//(temp_first_tupple->queue_var).push(temp_second_tupple); //simply push the second guy in queue
			execution_stack.push(new_aug_tuple); //push back the first tupple

			if(verbose_on){
				cout<<"\tPushed element on execution stack: ";
				print_execution_element(new_aug_tuple);
				cout<<"\tSize of Execution Stack: "<<execution_stack.size()<<endl;
			}
		}

		else if((popped_cntrl_st->node->get_node_type() == AMP)
				||(popped_cntrl_st->node->get_node_type() == OR)){
			if(verbose_on)
				cout<<"\t<Uniary OP> node it is..."<<endl;

			//pop two elements from execution stack
			execution_element *first_op = execution_stack.top();
			if( first_op == NULL){
				cout<<"ERROR - check &, or again...first op is not found"<<endl;
				exit(0);
			}

			execution_stack.pop();
			if(verbose_on){
				cout<<"\tPopped element: "; print_execution_element(first_op);
			}

			

			execution_element *second_op = execution_stack.top();
			if( second_op == NULL){
				cout<<"ERROR - check & again...second op is not found"<<endl;
				exit(0);
			}

			execution_stack.pop();
			if(verbose_on){
				cout<<"\tPopped element: "; print_execution_element(second_op);
			}
			
			if( !(first_op->type == EXE_TRUE || first_op->type == EXE_FALSE) || !(second_op->type == EXE_TRUE || second_op->type == EXE_FALSE)){
				cout<<"ERROR - & must have both operands as boolean"<<endl;
				exit(0);
			}
			
			execution_element *new_amp =  new execution_element;


			if (popped_cntrl_st->node->get_node_type() == OR){
				if(verbose_on)
					cout<<"\t<Apply <OR> operator..."<<endl;
				if( first_op->type == EXE_FALSE && second_op->type == EXE_FALSE)
					new_amp->type = EXE_FALSE;
				else
					new_amp->type = EXE_TRUE;

			}
			else if(popped_cntrl_st->node->get_node_type() == AMP){
				if(verbose_on)
					cout<<"\tApplying <AMP> operator..."<<endl;
				if( first_op->type == EXE_TRUE && second_op->type == EXE_TRUE)
					new_amp->type = EXE_TRUE;
				else
					new_amp->type = EXE_FALSE;
			}
			execution_stack.push(new_amp);
			if(verbose_on){
				cout<<"\tPushed element: ";
				print_execution_element(new_amp);
			}
				
		}
	
		//NEG operator
		else if( popped_cntrl_st->node->get_node_type() == NEG ){
			if(verbose_on)
				cout<<"\t<NEG> node it is..."<<endl;

			//pop an element from execution stack
			execution_element *not_ele = execution_stack.top();
			if(not_ele == NULL){
				cout<<"ERROR - <NEG>'s operand is NULL !"<<endl;
				exit(0);
			}
			execution_stack.pop();
			if( not_ele->type != EXE_INT){
				cout<<"\tERROR - <NEG> operator was expecting <INTEGER> as operand. Please check again.."<<endl;
				exit(0);
			}
			
			execution_element *neg_ele = new execution_element;
			neg_ele->type = EXE_INT;
			neg_ele->int_var = -(not_ele->int_var);
			execution_stack.push(neg_ele);

			if(verbose_on)
				cout<<"\tApplied <NEG> operator. Result : "<<(not_ele->int_var);
		}


		else if( popped_cntrl_st->node->get_node_type() == NOT){
			if(verbose_on)
				cout<<"\t<NOT> node it is..."<<endl;

			//pop an element from execution stack
			execution_element *not_ele = execution_stack.top();
			if(not_ele == NULL){
				cout<<"ERROR - <NOT>'s operand is NULL !"<<endl;
				exit(0);
			}
			execution_stack.pop();
			
			execution_element *not_new_ele = new execution_element;

			if( not_ele->type == EXE_TRUE)
				not_new_ele->type = EXE_FALSE;
			else if( not_ele->type == EXE_FALSE)
				not_new_ele->type = EXE_TRUE;
			else{
				cout<<"ERROR - <NOT>'s operand should be truthvalue"<<endl;
				exit(0);
			}

			execution_stack.push(not_new_ele);

			if(verbose_on){
				cout<<"\tPushed element:";
				print_execution_element(not_new_ele);
			}

		}

		//PLUS, DIV operator
		else if( (popped_cntrl_st->node->get_node_type() == DIV) || 
				popped_cntrl_st->node->get_node_type() == PLUS || 
				(popped_cntrl_st->node->get_node_type() == MINUS) ||
				(popped_cntrl_st->node->get_node_type() == GR) ||
				(popped_cntrl_st->node->get_node_type() == LE) ||
				(popped_cntrl_st->node->get_node_type() == MULTI)
				|| (popped_cntrl_st->node->get_node_type() == LS) )  {
			if(verbose_on)
				cout<<"\t<BINARY OP> node it is..."<<endl;

			execution_element *temp_pop_1 = execution_stack.top();
			//this should not be NULL and should be integer
			if( temp_pop_1 == NULL || temp_pop_1->type != EXE_INT ){
				cout<<"ERROR - Processing DIV/PLUS: First Element on execution stack is either not INTEGER or there is no element !"<<endl;
				exit(0);
			}
			execution_stack.pop();

			execution_element *temp_pop_2 = execution_stack.top(); //pop second element

			//both should not be NULL and should be integers
			if( temp_pop_2==NULL || temp_pop_2->type != EXE_INT ){
				cout<<"ERROR - Processing DIV/PLUS: Second Element on execution stack is either not INTEGER or there is no element !"<<endl;
				//cout<<"ERROR:While Processing DIV operator - Was expecting two integers on execution stack but got less than that "<<endl;
				exit(0);
			}
			execution_stack.pop();
			//execution_stack.pop();

			if(verbose_on){
				cout<<"\tPopped two elements from Execution Stack. Size of Execution Stack: "<<execution_stack.size()<<endl;
				cout<<"\tFirst Element: ";
				print_execution_element(temp_pop_1);
				cout<<endl;
				cout<<"\tSecond Element: ";
				print_execution_element(temp_pop_2);
				cout<<endl;
			}
			
			//perform calculation
			int temp_result;
			if(popped_cntrl_st->node->get_node_type() == DIV)
				temp_result = (temp_pop_1->int_var)/(temp_pop_2->int_var);

			else if (popped_cntrl_st->node->get_node_type() == PLUS)
				temp_result = (temp_pop_1->int_var)+(temp_pop_2->int_var);

			else if (popped_cntrl_st->node->get_node_type() == MINUS)
				temp_result = (temp_pop_1->int_var)-(temp_pop_2->int_var);

			else if (popped_cntrl_st->node->get_node_type() == MULTI)
				temp_result = (temp_pop_1->int_var)*(temp_pop_2->int_var);

			else if (popped_cntrl_st->node->get_node_type() == GR)
				temp_result = (temp_pop_1->int_var)>(temp_pop_2->int_var);

			else if (popped_cntrl_st->node->get_node_type() == LE)
				temp_result = (temp_pop_1->int_var) <= (temp_pop_2->int_var);

			else if (popped_cntrl_st->node->get_node_type() == LS)
				temp_result = (temp_pop_1->int_var) < (temp_pop_2->int_var);

			//create new execution_element. btw we don't need the popped ones anymore
			execution_element *new_exe_ele = new execution_element;

			if (popped_cntrl_st->node->get_node_type() == GR || (popped_cntrl_st->node->get_node_type() == LE)
					||(popped_cntrl_st->node->get_node_type() == LS)){
				if (temp_result)
					new_exe_ele->type = EXE_TRUE;
				else
					new_exe_ele->type = EXE_FALSE;
			}
			else{
				new_exe_ele->type = EXE_INT;
				new_exe_ele->int_var = temp_result;
			}

			execution_stack.push(new_exe_ele);

			if(verbose_on || debug_on){
				cout<<"\tPushed the result: "<<temp_result<<" \nSize of Execution Stack: "<<execution_stack.size()<<endl;
				getchar();
			}
		}

		/*else if(popped_cntrl_st->node->get_node_type() == PLUS){
			if(verbose_on)
				cout<<"\t<PLUS> node it is...."<<endl;

		} */

		else{ 
			cout<<"CSE_machine execution - Some Unexpected type of node in Control Stack !"<<endl; 
			print_ast_node(popped_cntrl_st->node);
		
		exit(0);
		}

	}

	


	//check execution stack's size
	//cout<<"EXECUTION STACKS size: "<<(execution_stack.size() )<<endl;
	//print_execution_element(execution_stack.top() );
	//if(execution_stack.empty() != true)

}

void CSE_machine::print_execution_element(execution_element *in_element){
	if(in_element == NULL){
		cout<<"CSE_machine::print_execution - in_element in NULL "<<endl;
		exit(0);
	}

	switch(in_element->type){
		case EXE_INT: cout<<in_element->int_var;
			  	break;

		case EXE_STR: //printf("%c", (in_element->string_var).c_str());
			      for(int i=0; i< (in_element->string_var).size(); i++){
				      if( (in_element->string_var)[i] == '\\') {
					      i++;
					      if( (in_element->string_var)[i] == 'n') 
						      cout<<endl;
					      else
						      cout<<"\t";
				      }

				      else
					      cout<<(in_element->string_var)[i];
			      }
			      //cout<<in_element->string_var<<endl;
			      break;
		case EXE_ENV: cout<<"<ENVIRONMENT>"<<endl;
			      break;

		case EXE_TUPPLE: //cout<<"<PRINTING TUPPLE>"<<endl;
				 print_tupple(in_element);
			      break;
		case EXE_CTRL_STRUCT: cout<<"[lambda closure: ";
				      cout<<in_element->ctrl_st_var->bound_var->get_node_value()<<" ]";
			      break;
		case EXE_TRUE: cout<<"true";
			       break;

		case EXE_FALSE: cout<<"false";
			       break;

		case EXE_DUMMY: cout<<"dummy";
			       break;
		case EXE_YSTAR: cout<<"<YSTAR>"<<endl;
			       break;

		case EXE_NILL: cout<<"nil";
			       break;
		case EXE_ITA: cout<<"<ITA>"<<endl;
				cout<<"[lambda closure: ";
				      cout<<in_element->ctrl_st_var->bound_var->get_node_value()<<" ]"<<endl;
			      break;

		default:
			      cout<<"print_execution_element - Some other type !! "<<endl;

	}


}
/* this will insert an execution_element with key in environment's hash map*/
void environment::insert(string key, execution_element *element){ //insert into hash table
	if(debug_on || verbose_on)
		cout<<"Inserting key: "<<key<<endl;
	bound_variable[key] = element; //TO-DO check if key already exists
	//bound_variable.insert(key, element);	

}

/* this will push the control structure into control stack */
void CSE_machine::load(control_st *delta){
	if(verbose_on)
		cout<<"CSE_machine::load - Loading new control structure...."<<endl;
	while(delta != NULL){
		if(verbose_on || debug_on){
			cout<<"\tload() - Pushing on control stack...."; print_ast_node(delta->node);
		}
		control_stack.push(delta);
		delta = delta->next;
	}
	
	if(verbose_on)
		cout<<"CSE_machine::load - Loading new control structure.......[done]"<<endl;
}

/* this will lookup execution element corresponding to key in current environment
 * it will look into every environment in its path to root until it found it
 * if not found it returns NULL
 */
execution_element* environment::lookup(string key){ //return the execution_element corresponding to key
	if(verbose_on || debug_on)
		cout<<"Finding key<"<<key<<"> ...."<<endl;

	//TODO - check in parent environment
	if( bound_variable[key] != NULL)
		return bound_variable[key];
	else if( parent != NULL) //look into parents
		return parent->lookup(key);
	else 
		return NULL;
}

//this is to print the tupple
void CSE_machine::print_tupple(execution_element *tupple){
  	queue<execution_element*> temp_tupples;
	cout<<"(";
	while( (tupple->queue_var).size() != 0 ){
		//pop an element from queue
		execution_element *temp_tuple = (tupple->queue_var).front();

		//call print_execution_element
		if( temp_tuple != NULL){
			print_execution_element(temp_tuple);
			if( (tupple->queue_var).size() != 1)
				cout<<", ";
		}

		//save it in another queue
		temp_tupples.push(temp_tuple);
		(tupple->queue_var).pop();
	}
	
	//print )
	cout<<")";
	//push back all the elements
	while( temp_tupples.size() != 0){
		(tupple->queue_var).push( temp_tupples.front() );
		temp_tupples.pop();
	}
} 
