#ifndef PARSER_H
#define PARSER_H

#include "../CSTL/include/cstl.h"
#include "../ion/str/include/str.h"

typedef struct grammar_rule_t grammar_rule_t;

typedef	enum {
		modifier_none     = 0b0000'0000,
		modifier_array 	  = 0b0000'0001,
		modifier_optional = 0b0000'0010,
		modifier_both	  = 0b0000'0011,
} type_modifier_t;
struct grammar_rule_t {
	enum {
		is_literal = 1,
		is_rule = 2,
	} literal_or_rule;
	union {
		string literal;
		struct {
			string key_name; 
			string rule_name;
			grammar_rule_t *grammar;
		};
	};
	type_modifier_t modifier;
	enum {
		is_next = 1, 
		is_alternative = 2,
	} next_or_alternative;
    union {
       	grammar_rule_t *next;
    	grammar_rule_t *alternative;
    };
};

typedef struct {

} string_parse_rule_t; 

typedef struct {
	pair(string, grammar_rule_t) *at;
	size_t count; 
} grammar_t;

type_modifier_t parseTypeModifier(iterstring_t *rule);
option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]);
option(grammar_rule_t) parseGrammarRule(iterstring_t *rule);
grammar_t *parseRule(iterstring_t *rule);
option(string) parseLiteral(iterstring_t *rule);
option(string) parseGrammarKey(iterstring_t *rule);
option(string) parseGrammarType(iterstring_t *rule);
bool parseWhitespace(iterstring_t *rule);
bool parseSeperator(iterstring_t *rule);
bool isFollowedByAlternative(iterstring_t *rule);
void printParsingMessage(FILE *stream, char *msg, string source,const char *const color, size_t color_start, size_t color_stop);


#endif // PARSER_H