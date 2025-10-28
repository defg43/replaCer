#ifndef PARSER_H
#define PARSER_H

#include "../CSTL/include/cstl.h"
#include "../ion/str/include/str.h"

typedef struct grammar_rule_t grammar_rule_t;
typedef struct grammar_or_string_rule grammar_or_string_rule_t;

typedef	enum {
		modifier_none     = 0b00,
		modifier_array 	  = 0b01,
		modifier_optional = 0b10,
		modifier_both	  = 0b11,
} type_modifier_t;

typedef enum {
	is_literal = 1,
	is_rule = 2,
} literal_or_rule_t;

typedef enum {
	is_next = 1, 
	is_alternative = 2,
} alt_or_next_t;

struct grammar_rule_t {
	literal_or_rule_t literal_or_rule;
	union {
		string literal;
		struct {
			string key_name; 
			string rule_name;
			grammar_or_string_rule_t *grammar;
		};
	};
	type_modifier_t modifier;
	alt_or_next_t next_or_alternative;
    union {
       	grammar_rule_t *next;
    	grammar_rule_t *alternative;
    };
};

typedef struct string_parse_rule_t string_parse_rule_t;

struct grammar_or_string_rule {
	enum {
		is_grammar_rule = 1,
		is_string_rule = 2,
	} gram_or_str;
	union {
		grammar_rule_t *gram;
		string_parse_rule_t *str;
	};
};

struct string_parse_rule_t {
	literal_or_rule_t literal_or_rule;
	union {
		struct {
			string rule_name;
			grammar_or_string_rule_t *grammar;
		};	
		string literal;
	};
	type_modifier_t modifier;
	alt_or_next_t next_or_alternative;
	union {
		string_parse_rule_t *next;
		string_parse_rule_t *alternative;
	};
}; 

typedef struct {
	pair(string, grammar_or_string_rule_t) *at;
	size_t count; 
} grammar_t;

type_modifier_t parseTypeModifier(iterstring_t *rule);
bool linkGrammar(grammar_t *gram);
option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]);
option(grammar_rule_t) parseGrammarRule(iterstring_t *rule);
option(string) parseLiteral(iterstring_t *rule);
option(string) parseGrammarKey(iterstring_t *rule);
option(string) parseGrammarType(iterstring_t *rule);
bool parseWhitespace(iterstring_t *rule);
bool parseSeperator(iterstring_t *rule);
bool isFollowedByAlternative(iterstring_t *rule);
void printParsingMessage(FILE *stream, char *msg, string source,const char *const color, size_t color_start, size_t color_stop);

object_t parseIntoObject(object_t obj, string input, grammar_t *gram, string start_rule);
#endif // PARSER_H
