/* File - parser.h
 * It tokenize the given file and parse the file to create abstract syntax tree
 * @Vineet Garg
 */
#ifndef PARSER_H
#define PARSER_H

#include<iostream>
#include<fstream>
#include<cstdlib>
#include<stack>
#include<tr1/unordered_map>
#include<queue>
#include<stack>
#include "token.h"
#include<cctype>
#include<stdlib.h>
#include<cstdio>
#include<sstream>
//#include "control_st.h"

extern bool debug_on;
extern bool verbose_on;
extern bool ast_on;


//verbose dumps everything
//ast dumps only ast tree
//off do no print anything
enum DEBUG_LEVEL { debug, verbose, ast, off }; //debug dumps only debugging information

enum AST_node_type {IDT, STR, INT, TAU, LAMBDA, WHERE, LET, AUG, COND, AMP, OR, NOT, GR, GE, LS, LE, EQ, NE, PLUS, MINUS, NEG, MULTI, DIV, EXPO, AT, GAMMA, TRUE, FALSE, NIL, DUMMY, WITHIN, AND, REC, EQUAL, FUN_FORM, PARAN, COMMA, YSTAR, DELTA_THEN, DELTA_ELSE, BETA, ENV }; //create the node type; type might be required during control stack execution


/* defines the node type of AST */
class AST_node {

		string node_value; //will contain the actual values 
		AST_node_type node_type; //type of node e.g. TERMINAL, TAU etc
	public:
		AST_node(); //initialize everything
		~AST_node();

		AST_node *left_child; //left pointer will point to childern
		AST_node *right_sibling; //this will point to siblings


		void set_node_value(string node_value);
		string get_node_value();

		void set_node_type(AST_node_type node_type);
		AST_node_type get_node_type();
};

void print_ast_node(AST_node *in_node); //for debugging only - this will print out the AST node in presentable manner
//this is the type of elements EXECUTION STACK can have
//had to use EXE to distinct it from other enums !
enum execution_element_type {EXE_ENV, EXE_INT, EXE_STR, EXE_TUPPLE, EXE_CTRL_STRUCT, EXE_PRIMITIVE_FUNC, EXE_TRUE, EXE_FALSE, EXE_NILL, EXE_DUMMY, EXE_YSTAR, EXE_ITA}; 

//keep track of all primitive function
enum primitive_function {PRINT, CONC, ORDER, STERN, ISTUPLE, ISINTEGER, STEM, ISTRUTHVALUE, ISSTRING, ITOS };

//declarations
class environment;
class control_st;

/* data structure to abstract execution element on execution stack 
 * NOT OPTIMIZED - WASTES TOO MUCH MEMORY  :-(
 * ANOTHER Caveat - it provides direct access to all of the elements which unfortunately
 			will be uninitialized, so take care. May the force be with you :D
 */
class execution_element{ 
public:
	bool conc_applied;
	execution_element_type type; //to keep track of the type
	int int_var;
	string string_var;
	environment *env_var;
	control_st *ctrl_st_var;
	queue<execution_element*> queue_var;
	primitive_function primitive_fun_var;

};

/* abstraction of environment
 * it maintains a hash table of all bound variables
 * it points to its parent */
class environment{
	tr1::unordered_map<string, execution_element*> bound_variable; //this is supposed to be the hash map of all bounded variables in the env

public:
	environment *parent; //initialize it to NULL

	environment();
	~environment(); //environment will presist till the end so no need to do anything here

	/* make it return NULL in case key is not found*/
	execution_element* lookup(string key); //return the execution_element corresponding to key

	/* in case key already exists just bring down the whole system */
	void insert(string key, execution_element *element); //insert into hash table
};

//basically this is a tree structure of control stack
class control_st{
	int num_children; //this is only for TAU node
	public:
		AST_node *node; //actual node
		control_st *next; //points to next control structure
		control_st *delta; //keep the pointer to another control structure where LABMDA is encountered
		AST_node *bound_var; //keeps the linked list of bound variables in case CS is LABMDA
		environment *env; //keeps a pointer to environment; will be filled up during CSE execution
		
		void set_num_children(int num_child); //set the number of children
		int get_num_children(); //return the number of children

		control_st();
		~control_st();

};
/* Define the parser of RPAL */
class Parser {

	AST_node *ast_tree;
	control_st  CS; //this points the control structure(first one)

	ifstream input_file;
	token *next_token;

	char saved_char; //scan sometimes will get a character ahead

public:
	Parser(string filename);
	~Parser();

	token* scan(); //it scans the input file and create a new token
	void parser(DEBUG_LEVEL level); //it will begin the parsing

private:

	void read(string token_value); //compare & read the next token
	void preorder_traversal(AST_node *tree, int num_dots); //print the AST in pre-order
	//Grammar Rule
	void E();
	void D();
	void Vb();
	void Ew();
	void Db();
	void Vl();
	void Da();
	void Dr();
	void T();
	void Ta();
	void Tc();
	void B();
	void Bt();
	void Bs();
	void Bp();
	void A();
	void At();
	void Af();
	void Ap();
	void R();
	void Rn();
	void build_tree(AST_node_type node_type, string node_value, int num_node); //it creates a new node
	void print_AST_node(AST_node *node); //print the AST node

	void read(tokenType token_type);
	
	//these functions are to standardize various nodes
	AST_node* standardize_tree(AST_node *node); //this will traverse the AST in post order standarizing each node
	AST_node* standardize_let(AST_node *& let_node); //this is to standardized let node 
	AST_node* standardize_where(AST_node *& where_node); //this is to standardized where node
	AST_node* standardize_within(AST_node *within_node); //this is to standardized within node
	AST_node* standardize_funform(AST_node *& funform_node); //this is to standardize function form node
	AST_node* standardize_at(AST_node *& at_node); //this is to standardize at node
	AST_node* standardize_lambda(AST_node *& lambda_node); //this is to standardize lambda node
	AST_node* standardize_and(AST_node *& and_node); //this is to standardize the and node 

	void standardize_rec(AST_node *& rec_node); //this is to standardize rec node

	/* To Keep in Mind - this will actually change the content of cntrl_st pointer so make sure to save it before calling this proc */
	void generate_control_struct(AST_node *standard_ast, control_st *cntrl_st); //this will recursively generate the control structure
	
	//FOR DEBUGGING PURPOSE ONLY
	void print_control_struct(control_st *cnt_st);
};


/* Here comes the CSE Machine 
 * it contains two stack - Control & Execution
 * It is initialized by parser and then it starts processing
 */
class CSE_machine{
	int i;
	stack<control_st*> control_stack; //control stack
	stack<execution_element*> execution_stack; //execution stack
	environment *current_env; //current environment

	void print_control_struct(control_st *cnt_st);

public:
	CSE_machine(control_st *cntrl_st); //initialize control stack
	//void initialize(control_st *cntrl_st); //initialize control stack, create primitive env etc
	void load(control_st *delta); //load the control stack with 
	void execute(); //this will being the execution
	void print_tupple(execution_element *tupple); //to print tupples
	void print_execution_element(execution_element *in_element); //print the execution element
};
#endif
