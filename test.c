#include "include/format.h"
#include "include/parser.h"
#undef lengthof
#include "ion/witc/foreach.h"
#define __USE_GNU
#include <dlfcn.h>
#undef __USE_GNU

#define unittest(R, I) struct unittest_##R##_##I {  \
    typeof(R (*)(I)) testfn;                        \
    R expected;                                     \
    I arg;                                          \
}

typedef struct {
    typeof(option(string) (*)(iterstring_t *)) testfn;
    option(string) expected;
    iterstring_t *arg;
} parserTest_t;

void printGrammarRule_t(grammar_rule_t to_print);

void printIndent(int level) {
    for (int i = 0; i < level; ++i) {
        printf("  ");
    }
}

void printTypeModifier(type_modifier_t mod) {
    switch (mod) {
        case modifier_both:     printf("[]?"); break;
        case modifier_array:    printf("[]");  break;
        case modifier_optional: printf("?");   break;
        default: break;
    }
}

void printGrammarRule_t(grammar_rule_t to_print) {
    if (to_print.literal_or_rule == is_literal) {
        printf("literal: '%s'", to_print.literal);
    } else if (to_print.literal_or_rule == is_rule) {
        printf("rule: %s : %s", to_print.key_name, to_print.rule_name);
    } else {
        printf("invalid");
    }
    printTypeModifier(to_print.modifier);
}

void printStringParseRule(string_parse_rule_t *rule, int level) {
    while (rule) {
        printIndent(level);
        if (rule->literal_or_rule == is_literal) {
            printf("literal: '%s'", rule->literal);
        } else if (rule->literal_or_rule == is_rule) {
            printf("rule: %s", rule->rule_name);
        } else {
            printf("invalid");
        }

        printTypeModifier(rule->modifier);
        printf("\n");

        if (rule->next_or_alternative == is_next) {
            rule = rule->next;
        } else if (rule->next_or_alternative == is_alternative) {
            printIndent(level);
            printf("| ");
            rule = rule->alternative;
        } else {
            break;
        }
    }
}


void printGrammarRuleTree(grammar_rule_t *rule, int level) {
    while (rule) {
        printIndent(level);
        printGrammarRule_t(*rule);
        printf("\n");

        if (rule->next_or_alternative == is_next) {
            rule = rule->next;
        } else if (rule->next_or_alternative == is_alternative) {
            printIndent(level);
            printf("| ");
            rule = rule->alternative;
        } else {
            break;
        }
    }
}

void printGrammar(grammar_t grammar) {
    for (size_t i = 0; i < grammar.count; ++i) {
        printf("%s:\n", grammar.at[i].first);

        grammar_or_string_rule_t *entry = &grammar.at[i].second;

        if (entry->gram_or_str == is_grammar_rule) {
            printGrammarRuleTree(entry->gram, 1);
        } else if (entry->gram_or_str == is_string_rule) {
            printStringParseRule(entry->str, 1);
        } else {
            printIndent(1);
            printf("invalid rule type\n");
        }

        printf("\n");
    }
}

void printOptionString(option(string) to_print) {
    if(to_print.valid) {
        printf("some(\"%s\")", to_print.value);
    } else {
        printf("none");
    }
    return;
}

void printOptionGrammarRule_t(option(grammar_rule_t) to_print) {
    if(to_print.valid) {
        printf("some(");
        printGrammarRule_t(to_print.value);
        printf(")");
    } else {
        printf("none");
    }
}

void testparserGrammarRule_t() {
    string test = string("test_key : test_type");
    iterstring_t t = {
        .str = test,
        .index = 0, 
        .previous = 0,
    };

    option(grammar_rule_t) result = parseGrammarRule(&t);

        printOptionGrammarRule_t(result);
    destroyString(test);
}

option(string) testparseTypeModifier(iterstring_t *arg) {
    switch (parseTypeModifier(arg)) {
        case modifier_array:
            return (option(string)) some(string("array"));
        case modifier_optional:
            return (option(string)) some(string("optional"));
        case modifier_both:
            return (option(string)) some(string("both"));
        default:
            return (option(string)) none;
    }
}

option(string) testAdapter(typeof(bool (*)(iterstring_t *)) testfn, iterstring_t *arg) {
    bool result = testfn(arg);
    return result ? (option(string)) some(string("")) : (option(string)) none;
}

option(string) testparseWhitespace(iterstring_t *arg) {
    return testAdapter(parseWhitespace, arg);
}

option(string) testparseSeperator(iterstring_t *arg) {
    return testAdapter(parseSeperator, arg);
}

option(string) testisFollowedByAlternative(iterstring_t *arg) {
    return testAdapter(isFollowedByAlternative, arg);
}

void runTests(size_t count, parserTest_t (*tests)[count]) {
    //static typeof(*tests[0]->testfn) prevfn = 0;
    foreach(parserTest_t test of *tests) {
        Dl_info info;
        if (!dladdr((void *)test.testfn, &info)) {
            info.dli_sname = "unknown_function";
        }

        option(string) actual_result = test.testfn(test.arg);
        printf("%s(\"%s\") == ", info.dli_sname, test.arg->str);
       
        if(test.expected.valid == actual_result.valid) {
            if(actual_result.valid == true) {
                if(stringeql(actual_result.value, test.expected.value)) {
                    goto test_passed;
                } goto test_failed;
            }
        } else {
            goto test_failed;
        }

        test_passed:
            printf("\033[32m");
            printOptionString(actual_result);
            printf(" \u2714\033[0m\n");
            continue;
        test_failed:
            printf("\033[31m");
            printOptionString(actual_result);
            printf(" \u2717\033[0m expected: \033[38;5;214m");
            printOptionString(test.expected);
            printf("\033[0m\n");            
            continue;
    }
}

int main() {
#if 0
	char *str = "test {var}, hihi";
    dictionary_t test = dict({{"{var}", "abc123456789"}});
	char *result = replaceSubstrings(str, test);
	puts(result);

    char *var;
    destroyDictionary(test);
    free(result);
#else
    parserTest_t tests[] = {
        [0] = {
            .testfn = parseLiteral,
            .expected = some(string("just a literal")), 
            .arg = &(iterstring_t) {
                .str = string("'just a literal'"),
                .index = 0,
                .previous = 0,
            },
        },
        {
            .testfn = parseLiteral,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string("'incomplete literal"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = parseLiteral,
            .expected = some(string("new line character\n")), 
            .arg = &(iterstring_t) {
                .str = string("'new line character\n'"),
                .index = 0,
                .previous = 0,
            }
        },

        {
            .testfn = parseLiteral,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string("'"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = parseLiteral,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string(""),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = parseGrammarKey,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string("'literal'"),
                .index = 0,
                .previous = 0,
            },
        },
        {
            .testfn = parseGrammarKey,
            .expected = some(string("key")), 
            .arg = &(iterstring_t) {
                .str = string("key"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = parseGrammarKey,
            .expected = none,
            .arg = &(iterstring_t) {
                .str = string(""),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = parseGrammarKey,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string("8"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseWhitespace,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string("this is not whitespace"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseWhitespace,
            .expected = some(string("")), 
            .arg = &(iterstring_t) {
                .str = string(" it is started by whitespace so this should be fine"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseWhitespace,
            .expected = some(string("")), 
            .arg = &(iterstring_t) {
                .str = string("\t   \tit is started by whitespace so this should be fine"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseWhitespace,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string(""),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseSeperator,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string(""),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseSeperator,
            .expected = some(string("")), 
            .arg = &(iterstring_t) {
                .str = string(":"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseSeperator,
            .expected = some(string("")), 
            .arg = &(iterstring_t) {
                .str = string("  :     "),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseSeperator,
            .expected = some(string("")), 
            .arg = &(iterstring_t) {
                .str = string("  :     some tokens"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testisFollowedByAlternative,
            .expected = none,
            .arg = &(iterstring_t) {
                .str = string(""),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testisFollowedByAlternative,
            .expected = some(string("")),
            .arg = &(iterstring_t) {
                .str = string("|"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testisFollowedByAlternative,
            .expected = some(string("")),
            .arg = &(iterstring_t) {
                .str = string(" |"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testisFollowedByAlternative,
            .expected = some(string("")),
            .arg = &(iterstring_t) {
                .str = string("| "),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testisFollowedByAlternative,
            .expected = some(string("")),
            .arg = &(iterstring_t) {
                .str = string("| some tokens"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testisFollowedByAlternative,
            .expected = none,
            .arg = &(iterstring_t) {
                .str = string("this should failed|"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = some(string("array")),
            .arg = &(iterstring_t) {
                .str = string("[]"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = some(string("optional")),
            .arg = &(iterstring_t) {
                .str = string("?"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = some(string("both")),
            .arg = &(iterstring_t) {
                .str = string("[]?"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = some(string("both")),
            .arg = &(iterstring_t) {
                .str = string("?[]"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = some(string("optional")),
            .arg = &(iterstring_t) {
                .str = string("?   []"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = none,
            .arg = &(iterstring_t) {
                .str = string(""),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = none,
            .arg = &(iterstring_t) {
                .str = string("test"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = some(string("array")),
            .arg = &(iterstring_t) {
                .str = string("[]     test"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = some(string("optional")),
            .arg = &(iterstring_t) {
                .str = string("?     test"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = none,
            .arg = &(iterstring_t) {
                .str = string("?????     test"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = none,
            .arg = &(iterstring_t) {
                .str = string("[][]     test"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = testparseTypeModifier,
            .expected = none,
            .arg = &(iterstring_t) {
                .str = string("[[]]     test"),
                .index = 0,
                .previous = 0,
            }
        },
    };

    runTests(lengthof(tests), &tests);
    // testparserGrammarRule_t();

string grammar[] = {
    string("char -> ' ' | '!' | '\"' | '#' | '$' | '%' | '&' | '\\'' | '(' | ')' | '*' | '+' | ',' | '-' | '.' | '/'"
           // " | '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'"
           " | ':' | ';' | '<' | '=' | '>' | '?' | '@'"
           " | 'A' | 'B' | 'C' | 'D' | 'E' | 'F' | 'G' | 'H' | 'I' | 'J' | 'K' | 'L' | 'M'"
           " | 'N' | 'O' | 'P' | 'Q' | 'R' | 'S' | 'T' | 'U' | 'V' | 'W' | 'X' | 'Y' | 'Z'"
           " | '[' | '\\\\' | ']' | '^' | '_' | '`'"
           " | 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' | 'i' | 'j' | 'k' | 'l' | 'm'"
           " | 'n' | 'o' | 'p' | 'q' | 'r' | 's' | 't' | 'u' | 'v' | 'w' | 'x' | 'y' | 'z'"
           " | '{' | '|' | '}' | '~'"),
    string("digit -> '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'"),
    string("operator -> '+' | '-' | '*' | '/' | '%' | '++' | '--' | '==' | '!=' | '<' | '<=' | '>' | '>='"
       " | '=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&&' | '||' | '!' | '&' | '|' | '^' | '~'"),
    string("token -> #char? #char[] | #digit[]")
};

    option(grammar_t) testg = compileGrammar(lengthof(grammar), &grammar);
    if(testg.valid) {
        printf("grammar compilation successfull\n");
        printf("count of grammars: %ld\n", testg.value.count);
        printf("pointer is %p\n", testg.value.at[0].second.gram);
        printGrammar(testg.value);
        printf("\n");
    } else {
        printf("failed to compile grammar\n");
    }
#endif

/*    
	printh("the first test string is {} and the second string is {}\n", "foostring", "barstring");
	char *foo, *bar;
    char *aaaaa;
	printh("the foo is {foo} and bar is {bar}, the first was {foo}\n", foo = "test1", bar = "test2");
	printh("{bar}{bar}{bar}\n", bar = "|----------------|");
    printh("{aaaaa} e {aaaaa}e\n", aaaaa = "a");
    printh("{} {} {}\n", "_", "_", "_");
*/  
    return 0;
}