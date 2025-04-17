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

type -> key1:type1 key2:type2

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

grammar_t *parseRule(iterstring_t *rule) {

}

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

option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]) {
    grammar_t ret = {
        .at = malloc(sizeof(pair(string, grammar_rule_t)) * count),
        .count = count,
    };
    if (!ret.at) {
        fprintf(stderr, "malloc failed in compileGrammar\n");
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
        option(grammar_rule_t) result = parseGrammarRule(&rule);
        if (!result.valid) return (option(grammar_t)) none;

        ret.at[i].second = result.value;
        grammar_rule_t *head = &ret.at[i].second;

        while (parseWhitespace(&rule), rule.str.at[rule.index]) {
            bool is_alt = isFollowedByAlternative(&rule); 
            option(grammar_rule_t) res = parseGrammarRule(&rule);
            if (!res.valid) return (option(grammar_t)) none;

            grammar_rule_t *ptr = malloc(sizeof(*ptr));
            if (!ptr) {
                fprintf(stderr, "malloc failed\n");
                exit(EXIT_FAILURE);
            }
            *ptr = res.value;

            head->next_or_alternative = is_alt ? is_alternative : is_next;
            if (is_alt) {
                head->alternative = ptr;
            } else {
                head->next = ptr;
            }
            head = ptr; // move forward in chain
        }

        i++;
    }

    return (option(grammar_t)) some(ret);
}

option(grammar_t) compileGrammar_old(size_t count, typeof(string) (*rules)[count]) {
    grammar_t ret = {
        .at = malloc(sizeof(pair(string, grammar_rule_t)) * count),
        .count = count,
    };
    if(!ret.at) {
        fprintf(stderr, "malloc failed in compile grammar\n");
        exit(EXIT_FAILURE);
    }
    size_t i = 0;
    foreach(string rl of *rules) {
        iterstring_t rule = {
            .str = rl,
        };
        option(string) rule_name = parseGrammarType(&rule);
        parseWhitespace(&rule);
        if(rule.str.at[rule.index] == '-') {
            rule.index++;
            if(rule.str.at[rule.index] != '>') {
                return (option(grammar_t)) none;
            }
            rule.index++;
            iterstringAdvance(&rule);
        } else {
            return (option(grammar_t)) none;
            
        }
        parseWhitespace(&rule);
        option(grammar_rule_t) result = parseGrammarRule(&rule);
        grammar_rule_t *head = nullptr;
        if(result.valid) {
            head = &result.value;
            memcpy(&ret.at[i].second, head, sizeof(grammar_rule_t));
        } else {
            return (option(grammar_t)) none;
        }

        while(parseWhitespace(&rule), rule.str.at[rule.index]) {
            bool is_alt = isFollowedByAlternative(&rule); 
            option(grammar_rule_t) res = parseGrammarRule(&rule);
            if(res.valid) {
                grammar_rule_t *ptr = malloc(sizeof res.value);
                if(!ptr) {
                    fprintf(stderr, "malloc failed\n");
                    exit(EXIT_FAILURE);
                }
                memcpy(ptr, &res.value, sizeof res.value);
                head->next_or_alternative = is_alt ? is_alternative : is_next;
                if(is_alt) {
                    head->alternative = ptr;
                } else {
                    head->next = ptr;  
                }
            } else {
                return (option(grammar_t)) none;
            }
        }
        i++;
    }
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
