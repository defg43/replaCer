/*
1.	{type}  // invokes the subparser for type, but does not print its result, parsing fails if the subparserfails
2.	{ident:type} // invokes subparser for type and prints the parsed text with the prefix "ident : %s", parsing fails if the subparserfails
3.	{ident1:type1 | idnet2:type2 | indent3:type3 | ... } // all subparsers get invoked in a for loop. The first succeeding parser is considered the result, parsing fails if all subparserfails
4.	{ident:type?}  // invokes subparser for type, if the subparser fails parsing resumes, nothing is printed
5.  {ident:type[]} // invokes subparser for type succesively, the subparser must suceed at least once, if the subparser does not suceed at least once parsing fails, the results are accumulated in a temorary string or array of strings and printed separated by comas
				   // this behavior also extends to literals so 'x'[], would describe a sequence of at least on character x 
6.	'x' // character literal, must be present in the input string, otherwise parsing fails
8.  rules 3 and 4 apply here, migth look like this: 'x'? or 'x'|'y'|'z', this also is refered to as a non-merging literal, this may appear outside of a specifier {}
registering a new type with a template string:
	type -> ... // the rigth side migth use rule 1., rule 4. and the special rule 9.
	9.	{'x'} // merging literal, if this literal is encountered it is merged into the string that is constructed in the process of parsing an expression of the newly defined type
	// here | and ? also apply so expressions migth look like this {'x'|'y'|'z'} or {'x'?}
	a definition might thus look like this: my_type -> {int}{':'}{int} // the parsed input is accumulated in a string that represent my_type, an example of this migth look like 2:33
*/

/*
'literal'
key:type
key:type | key2:type2
key:type?
key:type[]
key:type

type -> key1:type1 key2:type2

character -> 'a' | 'b' | 'c' | 'd' | 'e' | 'f'

raw_string -> #character[]

function_declartion -> return_type:C_type functionname:C_identifier '(' argument_list:argument[] ')' ';'
*/

#include <stdio.h>
#include <stddef.h>
#define DEBUG
#include "../include/format.h"
#include "../include/parser.h"
#include "../ion/str/include/str.h"
#include "../ion/include/ion.h"
#include "../ion/witc/foreach.h"
#include "../ion/witc/matching.h"
#include "../pesticide/include/debug.h"
#include "../CSTL/include/cstl.h"

grammar_t *parseRule(iterstring_t rule) {}

optional(string) parseLiteral(iterstring_t *rule) {
    if(rule->str.data == NULL) {
        return (optional(string)) none;
    }

    if(rule->str[rule->index] != '\'') {
        iterstringReset(rule);
        return (optional(string)) none;
    }

    rule->index++;
    string ret = string("");
    while(isalnum(rule->str.at[rule->index]) || isalnum(rule->str.at[rule->index])) {
        ret = appendChar(ret, rule->str.at[rule->index]);
        rule->index++
    }

    if(rule->str[rule->index] != '\'') {
        iterstringReset(rule);
        return (optional(string)) none;
    }

    iterstringAdvance(rule);
    return some(ret);

}

optional(string) parseGrammarKey(iterstring_t *rule) {}
optional(string) parseGrammarType(iterstring_t *rule) {}
bool parseWhitespace(iterstring_t *rule) {}
bool parseSeperator(iterstring_t *rule) {}
bool isFollowedByAlternative(iterstring_t *rule) {}

option(grammar_t) compilerGrammar(size_t count, string rules[static count]) {
	if(count == 0 || !rules) {
		return (option(grammar_t))none;
	}
	grammar_t ret;

	if(!(ret.at = malloc(count * sizeof(grammar_rule_t)))) {
		return (option(grammar_t))none;
	}
	ret.count = count;

}

object_t scanh(string fmt); // this assumes a default parsing rules

#if 0

bool addParserFromDefinition(parserRegistry_t *registry, char *definition) {
    // split the type name from the definition of the subparser
    // find first occurence of '->' in the definition
    size_t arrow_index = 0;
    size_t len = strlen(definition);
    for(size_t i = 0; i < len - 1; i++) {
        if(definition[i] == '-' && definition[i + 1] == '>') {
            arrow_index = i;
            break;
        }
    }
    // create substring for the type and trim whitespace
    substring_t type_name = substringTrimWhitespace(substring(definition, definition + arrow_index));
    dbg("the trimmed types is ");
    printSubstring(type_name);

    substring_t template_string = substring(definition + arrow_index + 2, definition + len);
    dbg("the template string is %s\n", template_string.start);
    dbg("the deinition is %s\n", definition);

    auto type_name_str = strdupSubstring(type_name);
    auto template_string_str = strdupSubstring(template_string);
    dbg("the type name is %s\n", type_name_str);
    dbg("the template string is %s\n", template_string_str);    

    return addParserTemplate(registry, type_name_str, template_string_str);
}

bool addParserTemplate(parserRegistry_t *registry, char *type_name, char *template_string) {
    if(registry->count == 0) {
        registry->entries = malloc(sizeof(parserRegistryEntry));
        if(!registry->entries) {
            printh("failed to allocate memory for parser registry\n");
            return false;
        }
        registry->count = 1;
    } else {
        registry->entries = 
            realloc(registry->entries, (registry->count + 1) * sizeof(parserRegistryEntry));
        registry->count++;
    }

    dbg("the type name is %s\n", type_name);
    registry->entries[registry->count - 1].type_name = type_name;
    registry->entries[registry->count - 1].parser_is_template = true;
    registry->entries[registry->count - 1].template_string = template_string;

    return true;
}

bool addParserFunction(parserRegistry_t *registry, char *type_name, subparser_t parser_function) {
    if(registry->count == 0) {
        registry->entries = malloc(sizeof(parserRegistryEntry));
        if(!registry->entries) {
            printh("failed to allocate memory for parser registry\n");
            return false;
        }
        registry->count = 1;
    } else {
        registry->entries = 
            realloc(registry->entries, (registry->count + 1) * sizeof(parserRegistryEntry));
        registry->count++;
    }

    registry->entries[registry->count - 1].type_name = type_name;
    registry->entries[registry->count - 1].parser_is_template = false;
    registry->entries[registry->count - 1].parser_function = parser_function;

    return true;
}
#endif // 0

size_t parse(char *input, char *template) {

}

int main__test() {
//    dbg("test\n");
    printf("test");
}
