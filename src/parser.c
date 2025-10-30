#include "../include/parser.h"

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
