#ifndef PARSER_H
#define PARSER_H

/*
// there are two types of rules

// in the first rule elements do not specify their storage type the storage type is therefore implicitly a string
// literals are appended to the a rule-local string that is the result of the rule
// other entities such as arrays, objects and numbers are converted to strings and appended
// for arrays each entry is converted to a string if needed and all strings are concatenated
// objects are flattened and all values are concatenated, the keys are ignored
char -> 'a' | 'b' | 'c' | 'd' ...

// these rules can be nested and combined together; the outputs of subrules being concatenated together
token -> char[] charOrDigit[]

// the second rule specifies storage types for at least one element
// these are entries in a Javascript object
// the storage is specified with a key and an asociated rule in the form of key:rule
// the result of the rule will be stored under the key
// these key value pairs are collected in a rule-local Javascript object and is the result of the rule
// if some elements dont provide a storage specifier they are executed but their output is discarded
rule -> t1:token '(' t2:token ')'

the modifiers are [] which searches for at least one or more occurences, ? 
which looks for one or none occurences

in addition there are alternatives that can be used between elements where only one alternative 
needs to succeed. The first matched alternative has priority; matching will stop after the first
successfull match
*/

#include "../CSTL/include/cstl.h"
#include "../ion/str/include/str.h"
#include "../ion/include/ion.h"
#include <stdio.h>

typedef enum {
	storage_type_not_set = 0, // missed during compilation
	implicit_storage = 1,
	object_storage = 2,	
} rule_type_t;

typedef	enum {
	modifier_none     = 0b00,
	modifier_array 	  = 0b01,
	modifier_optional = 0b10,
	modifier_both	  = 0b11,
} type_modifier_t;

typedef struct grammar_entry grammar_entry_t;

typedef struct {
	type_modifier_t type_mod;
	option(string) storage_key; // if none, then discard output
	enum {
		is_literal,
		is_rule,
	} literal_or_rule;
	union {
		struct {
			string rule_name; // produces
			grammar_entry_t *ge;
		};
		string literal;
	};
	
} rule_t;

typedef struct {
	enum {
		is_regular,
		has_alternative,
	} alternative_or_regular;
	union {
		dynarray(rule_t) alternative;
		rule_t rule;
	};
} rule_node_t;

struct grammar_entry {
	string name;
	rule_type_t rule_type;
	dynarray(rule_node_t) element;
};

typedef struct {
	dynarray(grammar_entry_t) entry;
} grammar_t;

/*
					    grammar_t
				_________________________
				|						|
	      ----- |grammar_entry_t entry[]|
		  |		|_______________________|
		  |
		  |
		  |
		  ---------> grammar_entry_t
				_________________________
				|						|
	 		  	|	  string name;		|
				|_______________________|
				|						|
				| rule_type_t rule_type |  => storage_not_set  |
				|_______________________|     implicit_storage |
				|  						|	  object_storage
		  ----  | rule_node_t element[] |
		  |		|_______________________|
	 	  |
		  |
		  |
		  --------->   rule_node_t
				_________________________
				|						|
				|						|
		  ----	|  rule_t alternative[] |
		  |		|			/			|
		  |---	|	   rule_t rule 		|
		  |		|						|
		  |		|_______________________|
		  |		
		  |
		  |
		  |
		  --------->	 rule_t
				_________________________
				|						|
				|  type_modifier_t mod  |
				|_______________________|
				|						|
				|  string storage_key?	|
				|_______________________|
				| ________rule_________	|
				| |					  | |
				| | string rule_name  | |
				| |___________________| |
   				| |					  | |
				| |grammar_entry_t *ge| |
				| |___________________| |
				| 			/			|
				| _______literal_______ |
				| |					  | |
				| |   string literal  | |
				| |___________________| |
				|_______________________|

*/
/*

example -> key1:rule1[] rule2 'literal' key2:'literal2' | key3:'iteral3'
|----------------------------------------------------------------------| grammar_entry_t
|-----| grammar_entry_t.name
		   |-----------------------------------------------------------| grammar_entry_t.element[]
		   |----------| rule_node_t.rule
		   |--| 	    rule_t.storage_key
		   	   |----|	rule_t.rule_name
		   	   		 || rule_t.mod
		   	   		 					|------------------------------| rule_t.alternative[]
*/


option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]);
bool linkGrammar(grammar_t *gram);
option(size_t) findGrammarEntry(grammar_t *gram, string *name);

type_modifier_t parseTypeModifier(iterstring_t *rule);
option(string) parseLiteral(iterstring_t *rule);
option(string) parseIdentifier(iterstring_t *rule);
bool parseWhitespace(iterstring_t *rule);
bool parseSeperator(iterstring_t *rule);
bool isFollowedByAlternative(iterstring_t *rule);

option(rule_t) compileRule(iterstring_t *rule);
option(rule_node_t) compileRuleNode(iterstring_t *rule);
option(grammar_entry_t) compileGrammarEntry(string rule_definition);

object_t parseIntoObject(object_t obj, string input, grammar_t *gram, string start_rule);

void printParsingMessage(FILE *stream, char *msg, string source, const char *const color, size_t color_start, size_t color_stop);
void printGrammar(grammar_t gram);
void destroyGrammar(grammar_t *gram);

#endif // PARSER_H
