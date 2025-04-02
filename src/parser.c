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

optional(string) parseLiteral(iterstring_t rule) {
    if(rule.str.data == NULL) {
        return (optional(string)) none;
    }

    if(rule.str[rule.index] != '\'') {
        iterstringReset(rule);
        return (optional(string)) none;
    }
    rule.index++;
    while(isalnum(rule.str.at[rule.index]))
}

optional(string) parseGrammarKey(iterstring_t rule) {}
optional(string) parseGrammarType(iterstring_t rule) {}
bool parseWhitespace(iterstring_t rule) {}
bool parseSeperator(iterstring_t rule) {}
bool isFollowedByAlternative(iterstring_t rule) {}

option(grammar_t) compilerGrammar(size_t count, string rules[static count]) {
	if(count == 0 || !rules) {
		return (option(grammar_t))none;
	}
	grammar_t ret;

	if(!(ret.at = malloc(count * sizeof(grammar_rule_t)))) {
		return (option(grammar_t))none;
	}
	ret.count = count;
	
	foreach(string rule in count sized rules) {
		// this is basically just a giant state machine
		pair(string, grammar_rule_t) *gr_rule = malloc(sizeof(pair(string, grammar_rule_t)));
		if(!gr_rule) {
			fprintf(stderr, "failed to allocate memory in %s", __FUNCTION__);
			exit(EXIT_FAILURE);
		}

        grammar_rule_t *head;
	
        // call function that returns linked list of grammar_rules_t

        head = parseRule(rule);
    
    	dynarray(string) tokens = tokenizeString((char_ptr_conv_t){ .into_char_ptr = rule}, ' ');
	   
    	foreach(size_t i of tokens) {
            head = (head = malloc(sizeof(grammar_t))) ? head : fprintf(stderr, "malloc failed"), exit(EXIT_FAILURE), NULL;
			switch(i) {
                case 0: {
                    if(stringIsOnlyAlphNum(tokens.at[i].at)) {
					    gr_rule->first = string(tokens.at[i]);
				    } else {
                        fprintf(stderr, "the name of a rule must be in plain text");
                        return (option(grammar_t)) none;
                    }
                } break;
                case 1: {
                    if(streql(tokens.at[i].at, '->')) {} else {
                        fprintf(stderr, "missing arrow");
                        return (option(grammar_t)) none;
                    }
                } break;
                default: {
                    enum { parsing_key = 1, parsing_type = 2} parsing_state;
                    bool second_literal_marker = false;
                    foreach(char c in stringlen(tokens.at[i].at) sized tokens.at[i]) {
                        match(c, head->literal_or_rule, 
                            parsing_state, second_literal_marker) {
                            pattern('\'', is_literal, _, _) {
                                // in a literal already
                                if(second_literal_marker) {
                                    return (option(grammar_t)) none;
                                } else {
                                    // set marker
                                    second_literal_marker = true;
                                }
                            }

                            pattern('\'', is_rule, _, _) {

                            }

                            pattern('\'', _, _, _) {
                            }

                            pattern(_, is_rule, parsing_key, _ when isalpha(c) || isalnum(c)) {
                                head->key_name = appendString(head->key_name, c);
                            }

                            pattern(_, is_rule, parsing_type, _ when isalpha(c) || isalnum(c)) {

                            }
                        }
                        
                        
                        case '\'': {
                            if(head->literal_or_rule == is_literal) {
                                // we are already in a literal
                                // we need to terminate it
                            } else if(head->literal_or_rule == is_rule) {
                                fprintf(stderr, "encountered ' when parsing rule, "
                                        "which is only permitted in literals");
                                return (option(grammar_t)) none;
                            }
                        } break;
                    }
                } break;
            }
            if(i == 0) {

			} 
    		destroyString(tokens.at[i]);
		}
        destroy_dynarray(tokens);
	}
}

struct parsing_rules_t;

typedef typeof(obj_t_value_t(*)(string, struct parsing_rules_t)) parser_func_t;

typedef struct parsing_rule_t {w
    string type_name;
    bool parser_is_template;
    union {
        string template_string;
        parser_func_t parser_function;
    };
} parsing_rule_t;

typedef struct parsing_rules_t {
    parsing_rule_t *entries;
    size_t count;
} parsing_rules_t;

bool addParserTemplate(parsing_rules_t *rules, string type_name, string template_string);
bool addParserFromDefinition(parsing_rules_t *rules, string defition);

obj_t_value_t parseFromTemplate(string input, string template);

obj_t_value_t parseFromTemplate(string input, string template) {
	if (stringlen(input) == 0) {
		goto error;
	}
	// we assume the template to be a series of at least one parsers
	string parsing_type = {};
	string name = {};
	
	bool has_ident = false;
	bool type_variant = false;
	bool optional = false;
	bool array_type = false;
	bool literal = false;
	
    /*
	iterstring_t inp = {
		.previous = 0,
		.index = 0,
		.str = input,	
	};
    */
	
    dynarray(string) subtokens = tokenizeString(template.at, " ");
    
    foreach(string subtoken of subtokens) {
        printf("the subtoken is %s\n", subtoken);
        destroyString(subtoken);
    }

	error:
		return (obj_t_value_t) {  };
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
    dbg("test\n");

    array(string) tokens = tokenizePairwiseString("{abc} {def} {ghi}", "{", "}");
    
    // array(string) tokens = tokenize("a, b, c, d, e, f", ",");

    printf("the length is %ld\n", tokens.count);

    foreach(string token of tokens) {
        printf("%s\n", token);
        destroyString(token);
    }

    /*
    parserRegistry_t registry = { 
        .entries = NULL, 
        .count = 0 
    };
    addParserFromDefinition(&registry, "my_type -> {int}{':'}{int}");
    */
}
