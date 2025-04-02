#ifndef PARSER_H
#define PARSER_H

#include "../CSTL/include/cstl.h"
#include "../ion/str/include/str.h"

typedef struct {
	enum {
		is_literal = 1,
		is_rule = 2,
	} literal_or_rule;
	union {
		string literal;
		struct {
			string key_name; 
			string grammar_name;
			grammar_rule_t *grammar;
		};
	};
	bool is_optional;
	bool is_array;
	enum {
		is_next = 1, 
		is_alternative = 2,
	} next_or_alternative;
    union {
       	grammar_rule_t *next;
    	grammar_rule_t *alternative;
    };
}  grammar_rule_t;

typedef struct {
	pair(string, grammar_rule_t) *at;
	size_t count; 
} grammar_t;

option(grammar_t) compilerGrammar(size_t count, string rules[static count]);
grammar_t *parseRule(iterstring_t *rule);
optional(string) parseLiteral(iterstring_t *rule);
optional(string) parseGrammarKey(iterstring_t *rule);
optional(string) parseGrammarType(iterstring_t *rule);
bool parseWhitespace(iterstring_t *rule);
bool parseSeperator(iterstring_t *rule);
bool isFollowedByAlternative(iterstring_t *rule);


#endif // PARSER_H