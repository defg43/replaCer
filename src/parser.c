#include "../include/parser.h"
#include "../CSTL/include/cstl.h"
#include <stdio.h>
#include <ctype.h>

option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]);
bool linkGrammar(grammar_t *gram);
option(size_t) findGrammarEntry(grammar_t *gram, string *name);

type_modifier_t parseTypeModifier(iterstring_t *rule) {
	type_modifier_t ret = modifier_none;
		bool saw_optional = false;
		bool saw_array = false;
		int bracket_depth = 0;

		parseWhitespace(rule);

		while (1) {
		    char c = rule->str.at[rule->index];
		    switch (c) {
		        case '?':
		            if (saw_optional) return modifier_none;
		            saw_optional = true;
		            ret |= modifier_optional;
		            rule->index++;
		            break;

		        case '[':
		            if (bracket_depth > 0) return modifier_none;
		            bracket_depth++;
		            rule->index++;
		            break;

		        case ']':
		            if (bracket_depth != 1 || saw_array) return modifier_none;
		            bracket_depth--;
		            saw_array = true;
		            ret |= modifier_array;
		            rule->index++;
		            break;
		        default:
		            goto end;
		    }
		}
	end:
		if (bracket_depth != 0) return modifier_none;

		iterstringAdvance(rule);
		return ret;	
}

option(string) parseLiteral(iterstring_t *rule) {
	if(rule->str.data == NULL) {
	    return (option(string)) none;
	}

	if(rule->str.at[rule->index] != '\'') {
	    iterstringReset(rule);
	    return (option(string)) none;
	}

	rule->index++;
	string ret = string("");

	while (rule->str.at[rule->index] != '\0' && rule->str.at[rule->index] != '\'') {
	    if (rule->str.at[rule->index] == '\\' && rule->str.at[rule->index + 1] != '\0') {
	        // Handle escape character
	        rule->index++;
	    }
	    ret = appendChar(ret, rule->str.at[rule->index]);
	    rule->index++;
	}

	if(rule->str.at[rule->index] != '\'') {
	    iterstringReset(rule);
	    return (option(string)) none;
	}

	rule->index++;
	iterstringAdvance(rule);
	return (option(string)) some(ret);
}

// both key name and rule
option(string) parseIdentifier(iterstring_t *rule) {
	if(rule->str.data == NULL) {
	    return (option(string)) none;
	}

	string ret = string("");

	if((((rule->str.at[rule->index] >= 'A') && (rule->str.at[rule->index] <= 'Z')) ||
	       ((rule->str.at[rule->index] >= 'a') && (rule->str.at[rule->index] <= 'z')))
	       || (rule->str.at[rule->index] == '_')) {
	    ret = appendChar(ret, rule->str.at[rule->index]);
	    rule->index++;    
	} else {
	    return (option(string)) none;
	}

	while((((rule->str.at[rule->index] >= 'A') && (rule->str.at[rule->index] <= 'Z')) ||
	       ((rule->str.at[rule->index] >= 'a') && (rule->str.at[rule->index] <= 'z'))) 
	    ||((rule->str.at[rule->index] >= '0') && (rule->str.at[rule->index] <= '9'))
	    || (rule->str.at[rule->index] == '_')) {
	    ret = appendChar(ret, rule->str.at[rule->index]);
	    rule->index++;
	}

	if(rule->index == rule->previous) {
	    iterstringReset(rule);
	    return (option(string)) none;
	}

	iterstringAdvance(rule);
	return (option(string)) some(ret);
}

bool parseWhitespace(iterstring_t *rule) {
	if(rule->str.data == NULL) {
		return false;
	}
	bool at_least_one_space_found = false;
	
	while(isspace(rule->str.at[rule->index])) {
		at_least_one_space_found = true;
	    rule->index++;
	}

	if(!at_least_one_space_found) {
		iterstringReset(rule);
	} else {
		iterstringAdvance(rule);
	}
	return at_least_one_space_found;	
}

bool parseSeperator(iterstring_t *rule) {
	parseWhitespace(rule);
	if(rule->str.at[rule->index] == ':') {
	    rule->index++;
	    iterstringAdvance(rule);
	} else {
	    iterstringReset(rule);
	    return false;
	}
	parseWhitespace(rule);
	return true;
}

bool isFollowedByAlternative(iterstring_t *rule) {
	parseWhitespace(rule);
	if(rule->str.at[rule->index] == '|') {
	    rule->index++;
	    iterstringAdvance(rule);
	} else {
	    iterstringReset(rule);
	    return false;
	}
	parseWhitespace(rule);
	return true;
}

option(rule_t) compileRule(iterstring_t *rule) {
	// first we either parse a string literal or 
	option(string) res;
	if((res = parseLiteral(rule)).valid) {
		// literal without storage

		rule_t ret = {
			.storage_key = none,
			.literal_or_rule = is_literal,
			.literal = res.value,
			.type_mod = parseTypeModifier(rule),		
		};
		
		return (option(rule_t)) some(ret);
	}
	return (option(rule_t)) none;
}

option(rule_node_t) compileRuleNode(iterstring_t *rule);
option(grammar_entry_t) compileGrammarEntry(string rule_definition);

object_t parseIntoObject(object_t obj, string input, grammar_t *gram, string start_rule);

void printParsingMessage(FILE *stream, char *msg, string source, const char *const color, size_t color_start, size_t color_stop);
void printGrammar(grammar_t gram);
void destroyGrammar(grammar_t *gram);
