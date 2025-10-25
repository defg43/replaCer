/*
grammar rules:
'literal' parses a literal and dicards the output
key:type parses a type of rule and stores the parsed result in 'key'
key:type | key2:type2 parses either type and stores the result in key 
	or parses type2 and stores the result in key2
key:type? tries to parse type but if parsing fails key is not present
key:type[] parses at least one occurence of type and stores it in key which is 
	an array

raw_string -> #type[] #type2? #'literal' | #'other literal' parsing rule for
	raw strings that arent stored inside keys

potential examples
character -> 'a' | 'b' | 'c' | 'd' | 'e' | 'f'
digit -> '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'

raw_string -> #character[] #digit[] #'!'?

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

const char *convertToAnsiEscape(char *color) {
    match(color) {
        pattern("black")   {return "\033[0;30m";}
        pattern("red")     {return "\033[0;31m";}
        pattern("green")   {return "\033[0;32m";}
        pattern("yellow")  {return "\033[0;33m";}
        pattern("blue")    {return "\033[0;34m";}
        pattern("magenta") {return "\033[0;35m";}
        pattern("cyan")    {return "\033[0;36m";}
        pattern("white")   {return "\033[0;37m";}
        pattern("reset")   {return "\033[0m";}
        pattern(_)         {return color;}        
    }
    return color;
}


void printParsingMessage(FILE *stream, char *msg, string source, const char *const color, size_t color_start, size_t color_stop) {
    puts(msg);
    assert((color_start < color_stop) && 
        (color_start < stringlen(source)) && 
        (color_stop < stringlen(source)));
    for(size_t i = 0; i < stringlen(source); i++) {
        if(i == color_start) {
            fprintf(stream, "\033[0;31m", "%s", convertToAnsiEscape(color));
        }
        putc(source.at[i], stream);
        if(i == color_stop - 1) {
            fprintf(stream, "\033[0m");
        }
    }
    putc('\n', stream);
}

option(grammar_rule_t) parseGrammarRule(iterstring_t *rule) {
    parseWhitespace(rule);
    grammar_rule_t ret;
    option(string) literal = parseLiteral(rule);
    if(literal.valid) {
        type_modifier_t mod = parseTypeModifier(rule);
        ret = (grammar_rule_t) {
            .literal_or_rule = is_literal,
            .literal = literal.value,
            .modifier = mod,
        };
        return (option(grammar_rule_t)) some(ret);
    }

    option(string) key_name = parseGrammarKey(rule);
    if(!key_name.valid) {
        fprintf(stderr, "parsing key failed at:\n %ld");
        printParsingMessage(stderr, "parsing key failed",
            rule->str, "red", rule->index, (rule->str.at[rule->index] == '\0') ? rule->index :
            rule->index + 1);
        return (option(grammar_rule_t)) none;
    } 
    
    parseWhitespace(rule);
    if(!parseSeperator(rule)) {
        printParsingMessage(stderr, "parsing key failed, expected seperator",
            rule->str, "red", rule->index, //(rule->str.at[rule->index] == '\0') ? rule->index :
            rule->index + 1);

        return (option(grammar_rule_t)) none;
    }
    option(string) rule_name = parseGrammarType(rule);
    if(rule_name.valid) {
        type_modifier_t mod = parseTypeModifier(rule);
        ret = (grammar_rule_t) {
            .literal_or_rule = is_rule,
            .key_name = key_name.value,
            .rule_name = rule_name.value,
            .modifier = mod,
        };
        return (option(grammar_rule_t))some(ret);
    }
    return (option(grammar_rule_t)) none;
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

option(string) parseGrammarKey(iterstring_t *rule) {
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

option(string) parseGrammarType(iterstring_t *rule) {
    return parseGrammarKey(rule);
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

grammar_rule_t *compileGrammarRuleChain(iterstring_t *rule) {
    option(grammar_rule_t) result = parseGrammarRule(rule);
    if (!result.valid) return NULL;

    grammar_rule_t *head = malloc(sizeof(grammar_rule_t));
    if (!head) {
        fprintf(stderr, "malloc failed in %s\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    *head = result.value;

    grammar_rule_t *current = head;
    while (parseWhitespace(rule), rule->str.at[rule->index]) {
        bool is_alt = isFollowedByAlternative(rule);
        option(grammar_rule_t) res = parseGrammarRule(rule);
        if (!res.valid) return NULL; // ideally this would do some form of cleanup

        grammar_rule_t *ptr = malloc(sizeof(grammar_rule_t));
        if (!ptr) {
            fprintf(stderr, "malloc failed in %s\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
        *ptr = res.value;

        current->next_or_alternative = is_alt ? is_alternative : is_next;
        if (is_alt) {
            current->alternative = ptr;
        } else {
            current->next = ptr;
        }
        current = ptr;
    }
    return head;
}

string_parse_rule_t *compileStringParseRuleChain(iterstring_t *rule) {
    if (!rule || !rule->str.at) {
        return NULL;
    }

    string_parse_rule_t *head = NULL;
    string_parse_rule_t *tail = NULL;

    while (parseWhitespace(rule), rule->str.at[rule->index] == '#') {
        rule->index++;

        string_parse_rule_t *node = malloc(sizeof(string_parse_rule_t));
        if (!node) {
            fprintf(stderr, "malloc failed in %s\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }

        option(string) gr_type = parseGrammarType(rule);
        if (gr_type.valid) {
            node->literal_or_rule = is_rule;
            node->rule_name = gr_type.value;
            node->grammar = NULL; // optional, might be resolved later
        } else {
            option(string) str_lit = parseLiteral(rule);
            if (!str_lit.valid) {
                printParsingMessage(stderr, "parsing of string rule failed:",
                                    rule->str, "red", rule->index, rule->index + 1);
                free(node);
                return NULL;
            }
            node->literal_or_rule = is_literal;
            node->literal = str_lit.value;
        }

        node->modifier = parseTypeModifier(rule);
        node->next_or_alternative = is_next;
        node->next = NULL;
        node->alternative = NULL;

        if (!head) {
            head = node;
            tail = node;
        } else {
            if (isFollowedByAlternative(rule)) {
                tail->next_or_alternative = is_alternative;
                tail->alternative = node;
            } else {
                tail->next_or_alternative = is_next;
                tail->next = node;
            }
            tail = node;
        }
    }

    dbg("the head being returned is %p", head);
    return head;
}

option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]) {
    grammar_t ret = {
        .at = malloc(sizeof(pair(string, grammar_or_string_rule_t)) * count),
        .count = count,
    };
    if (!ret.at) {
        fprintf(stderr, "malloc failed in %s\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    size_t i = 0;
    foreach (string rl of *rules) {
        iterstring_t rule = { .str = rl };
        option(string) rule_name = parseGrammarType(&rule);
        if (!rule_name.valid) return (option(grammar_t)) none;

        ret.at[i].first = rule_name.value;

        parseWhitespace(&rule);
        if (rule.str.at[rule.index] == '-') {
            rule.index++;
            if (rule.str.at[rule.index] != '>') {
                return (option(grammar_t)) none;
            }
            rule.index++;
            iterstringAdvance(&rule);
        } else {
            return (option(grammar_t)) none;
        }

        parseWhitespace(&rule);

        bool string_rule = rule.str.at[rule.index] == '#';
        if (string_rule) {
            string_parse_rule_t *chain = compileStringParseRuleChain(&rule);
            if (!chain) return (option(grammar_t)) none;

            ret.at[i].second.gram_or_str = is_string_rule;
            ret.at[i].second.str = chain;
        } else {
            grammar_rule_t *chain = compileGrammarRuleChain(&rule);
            if (!chain) return (option(grammar_t)) none;

            ret.at[i].second.gram_or_str = is_grammar_rule;
            ret.at[i].second.gram = chain;
        }
        i++;
    }
    return (option(grammar_t)) some(ret);
}

option(size_t) findGrammarRule(grammar_t *gram, string *name) {
    for(size_t i = 0; i < gram->count; i++) {
        if(stringeql(gram->at[i].first, *name)) {
            return (option(size_t)) some(i);
        }
    }
    return (option(size_t)) none;
}

bool linkGrammar_ruleChain(grammar_rule_t *head, grammar_t *gram) {
	if(head->next == NULL) {
		return true;
	}
}

bool linkGrammar(grammar_t *gram) {
    if(!gram) {
        return false;
    }

    // Iterate through each grammar definition
    for(size_t i = 0; i < gram->count; i++) {
        if(gram->at[i].second.gram_or_str == is_grammar_rule) {
            // Handle grammar_rule_t chain
            grammar_rule_t *current = gram->at[i].second.gram;
            
            while(current) {
                if(current->literal_or_rule == is_rule) {
                    // Find the grammar definition for this rule
                    option(size_t) index = findGrammarRule(gram, &current->rule_name);
                    if(index.valid) {
                        current->grammar = &gram->at[index.value].second;
                    } else {
                        fprintf(stderr, "linking failed, unknown rule '%s'\n", current->rule_name.at);
                        return false;
                    }
                }
                
                // Move to next node in chain (could be next or alternative)
                if(current->next_or_alternative == is_next) {
                    current = current->next;
                } else if(current->next_or_alternative == is_alternative) {
                    current = current->alternative;
                } else {
                    break; // End of chain
                }
            }
            
        } else if(gram->at[i].second.gram_or_str == is_string_rule) {
            // Handle string_parse_rule_t chain
            string_parse_rule_t *current = gram->at[i].second.str;
            
            while(current) {
                if(current->literal_or_rule == is_rule) {
                    // Find the grammar definition for this rule
                    option(size_t) index = findGrammarRule(gram, &current->rule_name);
                    if(index.valid) {
                        current->grammar = &gram->at[index.value].second;
                    } else {
                        fprintf(stderr, "linking failed, unknown rule '%s'\n", current->rule_name.at);
                        return false;
                    }
                }
                
                // Move to next node in chain
                if(current->next_or_alternative == is_next) {
                    current = current->next;
                } else if(current->next_or_alternative == is_alternative) {
                    current = current->alternative;
                } else {
                    break; // End of chain
                }
            }
        }
    }
    
    return true;
}

// both branches assign grammar which is wrong: TODO fix
bool linkGrammar_old(grammar_t *gram) {
	printf("count of grammar: %ld\n", gram->count);
    for(size_t i = 0; i < gram->count; i++) {
    	if(gram->at[i].second.gram_or_str == is_grammar_rule) {
           	grammar_rule_t *head = gram->at[i].second.gram;
           	printf("head->literal_or_rule: %s\n", head->literal_or_rule == is_rule ? "rule" : "literal");
           	do if(head->literal_or_rule == is_rule) {
            	option(size_t) index = findGrammarRule(gram, &head->rule_name);
				printf("here too\n");
            	if(index.valid) {
                    head->grammar = &gram->at[index.value].second;
                    head = head->next;
            	} else {
                    fprintf(stderr, "linking failed, unknown rule %s\n", head->rule_name);
                    return false;
                }
            } else head = head->next; while(head);
        } else if(gram->at[i].second.gram_or_str = is_string_rule) {
            string_parse_rule_t *head = gram->at[i].second.str;
           	printf("head->literal_or_rule: %s\n", head->literal_or_rule == is_rule ? "rule" : "literal");
            do if(head->literal_or_rule == is_rule) {
                option(size_t) index = findGrammarRule(gram, &head->rule_name);
                if(index.valid) {
                    head->grammar = &gram->at[index.value].second;
                    head = head->next;
                } else {
                    fprintf(stderr, "linking failed, unknown rule %s\n", head->rule_name);
                    return false;
                }
            } else head = head->next; while(head);
        }
    }
    return true;
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

object_t parseIntoObject(object_t obj, string input, grammar_t gram) {
	// we assume that we are just appending results into the object that is 
	// already created
	iterstring_t is = { .str = input, .index = 0, .previous = 0 };

	// some form of entry point is required at which we start parsing
	// string recursively
	// if we run into alternaitve rules we try every single alternative
	// at least one must be met, as soon as we have found one rule that works
	// we should stop as soon as one rule is matched
	// for arrays we need to match the rule at least once
	// for options we dont need but can match the rule
	// all of these are stored in json format
	// if we at some point fail at parsing we stop and the given input
	// is not parseable with the provided grammar

	// what should be the root? should it have a specific required name?
	// should the root just be the first entry in the grammar??
	// should any rule be a valid entry point?
	// should the entry point name be provided as a seperate name to this function
	// as an argument?

}

int main__test() {
//    dbg("test\n");
    printf("test");
}
