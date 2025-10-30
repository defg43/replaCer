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
object_t scanh(string fmt); // this assumes a default parsing rules

static option(obj_t_value_t) parseGrammarRuleChain(iterstring_t *is, grammar_rule_t *rule);
static option(obj_t_value_t) parseStringRuleChain(iterstring_t *is, string_parse_rule_t *rule);
static option(obj_t_value_t) parseGrammarOrStringRule(iterstring_t *is, grammar_or_string_rule_t *rule);

static string flattenToString(obj_t_value_t val);

static bool parseLiteralString(iterstring_t *is, string literal) {
    size_t lit_len = stringlen(literal);
    
    for(size_t i = 0; i < lit_len; i++) {
        if(is->str.at[is->index + i] == '\0' || 
           is->str.at[is->index + i] != literal.at[i]) {
            return false;
        }
    }
    
    is->index += lit_len;
    iterstringAdvance(is);
    return true;
}

static string flattenToString(obj_t_value_t val) {
    switch(val.discriminant) {
        case obj_t_string:
            return stringFromString(val.str);
            
        case obj_t_array: {
            string result = string("");
            for(size_t i = 0; i < val.arr.count; i++) {
                string part = flattenToString(val.arr.array[i]);
                result = appendString(result, part);
                destroyString(part);
            }
            return result;
        }
        
        case obj_t_obj: {
            string result = string("");
            for(size_t i = 0; i < val.obj.count; i++) {
                string part = flattenToString(val.obj.value[i]);
                result = appendString(result, part);
                destroyString(part);
            }
            return result;
        }
        
        default:
            return string("");
    }
}

static option(obj_t_value_t) parseStringRuleChain(iterstring_t *is, string_parse_rule_t *rule) {
    printf("parseStringRuleChain called, rule=%p\n", (void*)rule);
    if(!rule) {
        printf("  rule is NULL!\n");
        return (option(obj_t_value_t)) none;
    }
    printf("  first node: literal_or_rule=%d, modifier=%d\n", 
           rule->literal_or_rule, rule->modifier);
           
    string result = string("");
    string_parse_rule_t *current = rule;
    
    while(current) {
        bool matched = false;
        
        if(current->literal_or_rule == is_literal) {
            if(parseLiteralString(is, current->literal)) {
                result = appendString(result, current->literal);
                matched = true;
            }
        } else if(current->literal_or_rule == is_rule) {
            if(!current->grammar) {
                fprintf(stderr, "Unlinked grammar rule in string parse\n");
                destroyString(result);
                return (option(obj_t_value_t)) none;
            }
            
            if(current->modifier & modifier_array) {
                option(obj_t_value_t) first = parseGrammarOrStringRule(is, current->grammar);
                if(!first.valid) {
                    // Array match failed, don't set matched=true
                } else {
                    string part = flattenToString(first.value);
                    result = appendString(result, part);
                    destroyString(part);
                    
                    while(1) {
                        size_t save_pos = is->index;
                        option(obj_t_value_t) next = parseGrammarOrStringRule(is, current->grammar);
                        if(!next.valid) {
                            is->index = save_pos;
                            break;
                        }
                        string next_part = flattenToString(next.value);
                        result = appendString(result, next_part);
                        destroyString(next_part);
                    }
                    matched = true;
                }
                
            } else if(current->modifier & modifier_optional) {
                size_t save_pos = is->index;
                option(obj_t_value_t) opt = parseGrammarOrStringRule(is, current->grammar);
                if(opt.valid) {
                    string part = flattenToString(opt.value);
                    result = appendString(result, part);
                    destroyString(part);
                } else {
                    is->index = save_pos;
                }
                matched = true;
                
            } else {
                option(obj_t_value_t) val = parseGrammarOrStringRule(is, current->grammar);
                if(val.valid) {
                    string part = flattenToString(val.value);
                    result = appendString(result, part);
                    destroyString(part);
                    matched = true;
                }
            }
        }
        
        if(!matched) {
            if(current->next_or_alternative == is_alternative && current->alternative) {
                iterstringReset(is);
                destroyString(result);
                return parseStringRuleChain(is, current->alternative);
            } else {
                iterstringReset(is);
                destroyString(result);
                return (option(obj_t_value_t)) none;
            }
        }
        
        if(current->next_or_alternative == is_next && current->next) {
            current = current->next;
        } else {
            break;
        }
    }
    
    obj_t_value_t ret = { .discriminant = obj_t_string, .str = result };
    return (option(obj_t_value_t)) some(ret);
}

static option(obj_t_value_t) parseGrammarRuleChain(iterstring_t *is, grammar_rule_t *rule) {
    if(!rule) return (option(obj_t_value_t)) none;

	printf("parseGrammarRuleChain called, rule=%p\n", (void*)rule);
	if(rule) printf("  first node: literal_or_rule=%d\n", rule->literal_or_rule);
    
    object_t result = createEmptyObject();
    grammar_rule_t *current = rule;
    
    while(current) {
	   	printf("  current node: literal_or_rule=%d, modifier=%d\n", 
        	current->literal_or_rule, current->modifier);
        if(current->literal_or_rule == is_literal) {
            if(!parseLiteralString(is, current->literal)) {
                if(current->next_or_alternative == is_alternative && current->alternative) {
                    iterstringReset(is);
                    destroyObject(result);
                    return parseGrammarRuleChain(is, current->alternative);
                }
                iterstringReset(is);
                destroyObject(result);
                return (option(obj_t_value_t)) none;
            }
            
        } else if(current->literal_or_rule == is_rule) {
            if(!current->grammar) {
                fprintf(stderr, "Unlinked grammar rule\n");
                destroyObject(result);
                return (option(obj_t_value_t)) none;
            }
            
            if(current->modifier & modifier_array) {
                array_t arr = createEmptyArray();
                
                option(obj_t_value_t) first = parseGrammarOrStringRule(is, current->grammar);
                if(!first.valid) {
                    if(current->next_or_alternative == is_alternative && current->alternative) {
                        iterstringReset(is);
                        destroyObject(result);
                        destroyArray(arr);
                        return parseGrammarRuleChain(is, current->alternative);
                    }
                    destroyObject(result);
                    destroyArray(arr);
                    return (option(obj_t_value_t)) none;
                }
                
                arr = insertIntoArray(arr, first.value);
                
                while(1) {
                    size_t save_pos = is->index;
                    option(obj_t_value_t) next = parseGrammarOrStringRule(is, current->grammar);
                    if(!next.valid) {
                        is->index = save_pos;
                        break;
                    }
                    arr = insertIntoArray(arr, next.value);
                }
                
                result = insertArrayEntry(result, current->key_name, arr);
                
            } else if(current->modifier & modifier_optional) {
                size_t save_pos = is->index;
                option(obj_t_value_t) opt = parseGrammarOrStringRule(is, current->grammar);
                if(opt.valid) {
                    result = insertObjectEntry(result, current->key_name, opt.value);
                } else {
                    is->index = save_pos;
                }
                
            } else {
                option(obj_t_value_t) val = parseGrammarOrStringRule(is, current->grammar);
                if(!val.valid) {
                    if(current->next_or_alternative == is_alternative && current->alternative) {
                        iterstringReset(is);
                        destroyObject(result);
                        return parseGrammarRuleChain(is, current->alternative);
                    }
                    destroyObject(result);
                    return (option(obj_t_value_t)) none;
                }
                result = insertObjectEntry(result, current->key_name, val.value);
            }
        }
        
        if(current->next_or_alternative == is_next && current->next) {
            current = current->next;
        } else if(current->next_or_alternative == is_alternative) {
            break;
        } else {
            break;
        }
    }
    
    obj_t_value_t ret = {
        .discriminant = obj_t_obj,
        .obj = result
    };
    return (option(obj_t_value_t)) some(ret);
}

static option(obj_t_value_t) parseGrammarOrStringRule(iterstring_t *is, grammar_or_string_rule_t *rule) {
    if(!rule) {
        printf("parseGrammarOrStringRule: rule is NULL!\n");
        return (option(obj_t_value_t)) none;
    }


    printf("parseGrammarOrStringRule: gram_or_str=%d\n", rule->gram_or_str);
    
    if(rule->gram_or_str == is_grammar_rule) {
        return parseGrammarRuleChain(is, rule->gram);
    } else if(rule->gram_or_str == is_string_rule) {
    	printf("  calling parseStringRuleChain with rule->str=%p\n", (void*)rule->str);
        return parseStringRuleChain(is, rule->str);
    }
    
    printf("parseGrammarOrStringRule: unknown rule type!\n");
    return (option(obj_t_value_t)) none;
}

option(obj_t_value_t) genericParserEntry(iterstring_t *is, struct grammar_or_string_rule *rule) {
    if(!rule) {
        return (option(obj_t_value_t)) none;
    }

    if(rule->gram_or_str == is_grammar_rule) {
        grammar_rule_t *r = rule->gram;
        switch (r->modifier) {
            case modifier_none:
                option(obj_t_value_t) result = parseRegularRule(is, r);
            break;
            case modifier_array:
                // we collect the array here
                obj_t_array_t arr = createEmptyArray();
                option(obj_t_value_t) result = parseRegularRule(is, r);

                do {

                }while(1);


            break;
            case modifier_optional:

            break;
            default:

            break;

        }
    } else {
        // do this for string parsing // by default capture all strings
    }

}

option(obj_t_value_t) parseRegularRule(iterstring_t *is, grammar_rule_t *rule) {}
option(obj_t_value_t) parseAlternative(iterstring_t *is, grammar_rule_t *rule) {}
option(obj_t_value_t) parseOptional(iterstring_t *is, grammar_rule_t *rule) {}
option(obj_t_value_t) parseArray(iterstring_t *is, grammar_rule_t *rule) {}

object_t parseIntoObject(object_t obj, string input, grammar_t *gram, string start_rule) {
    iterstring_t is = { .str = input, .index = 0, .previous = 0 };

    option(size_t) start_idx = findGrammarRule(gram, &start_rule);
    if(!start_idx.valid) {
        fprintf(stderr, "Start rule '%s' not found in grammar\n", start_rule.at);
        return obj;
    }

    grammar_or_string_rule_t *entry = &gram->at[start_idx.value].second;

    // go through the entry, it should have at least one key
    if(entry->gram_or_str == is_grammar_rule) {

        dynarray(sr_pair) rules = createDynArray(sr_pair);

    } else { // we cant start with a string parsing rule
        fprintf(stderr, "first rule can not be string rule\n");
    }

    option(obj_t_value_t) result = parseGrammarOrStringRule(&is, entry);
    if(!result.valid) {
        fprintf(stderr, "Failed to parse input\n");
        return obj;
    }

    if(is.str.at[is.index] != '\0') {
        fprintf(stderr, "Warning: parsing succeeded but %zu characters remain\n",
                stringlen(is.str) - is.index);
    }

    if(result.value.discriminant == obj_t_obj) {
        for(size_t i = 0; i < result.value.obj.count; i++) {
            obj = insertObjectEntry(obj, result.value.obj.key[i], result.value.obj.value[i]);
        }
    } else {
        obj = insertObjectEntry(obj, start_rule, result.value);
    }

    return obj;
}

object_t parseIntoObject_naw(object_t obj, string input, grammar_t *gram, string start_rule) {
    iterstring_t is = { .str = input, .index = 0, .previous = 0 };
    
    option(size_t) start_idx = findGrammarRule(gram, &start_rule);
    if(!start_idx.valid) {
        fprintf(stderr, "Start rule '%s' not found in grammar\n", start_rule.at);
        return obj;
    }
    
    grammar_or_string_rule_t *start = &gram->at[start_idx.value].second;
    
    option(obj_t_value_t) result = parseGrammarOrStringRule(&is, start);
    if(!result.valid) {
        fprintf(stderr, "Failed to parse input\n");
        return obj;
    }
    
    if(is.str.at[is.index] != '\0') {
        fprintf(stderr, "Warning: parsing succeeded but %zu characters remain\n",
                stringlen(is.str) - is.index);
    }
    
    if(result.value.discriminant == obj_t_obj) {
        for(size_t i = 0; i < result.value.obj.count; i++) {
            obj = insertObjectEntry(obj, result.value.obj.key[i], result.value.obj.value[i]);
        }
    } else {
        obj = insertObjectEntry(obj, start_rule, result.value);
    }
    
    return obj;
}

object_t _parseIntoObject(object_t obj, string input, grammar_t gram) {
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
    printf("test");
}
