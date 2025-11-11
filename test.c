#include "include/format.h"
#include "include/parser.h"
#undef lengthof
#include "ion/witc/foreach.h"
#define __USE_GNU
#include <dlfcn.h>
#undef __USE_GNU

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
        case modifier_none: 
        default: break;
    }
}

void printRule_t(rule_t rule) {
    if(rule.storage_key.valid) {
        printf("%s:", rule.storage_key.value.at);
    }
    
    if(rule.literal_or_rule == is_literal) {
        printf("'%s'", rule.literal.at);
    } else {
        printf("%s", rule.rule_name.at);
    }
    
    printTypeModifier(rule.type_mod);
}

void printRuleNode_t(rule_node_t node, int level) {
    printIndent(level);
    
    if(node.alternative_or_regular == is_regular) {
        printRule_t(node.rule);
        printf("\n");
    } else {
        printf("alternatives:\n");
        for(size_t i = 0; i < node.alternative.count; i++) {
            printIndent(level + 1);
            printRule_t(node.alternative.at[i]);
            if(i < node.alternative.count - 1) {
                printf(" |");
            }
            printf("\n");
        }
    }
}

void printGrammarEntry_t(grammar_entry_t entry) {
    printf("%s -> ", entry.name.at);
    printf("(%s) ", entry.rule_type == implicit_storage ? "string" : 
                     entry.rule_type == object_storage ? "object" : "unset");
    printf("%zu elements:\n", entry.element.count);
    
    for(size_t i = 0; i < entry.element.count; i++) {
        printRuleNode_t(entry.element.at[i], 1);
    }
}

void printGrammar(grammar_t gram) {
    printf("Grammar with %zu entries:\n", gram.entry.count);
    for(size_t i = 0; i < gram.entry.count; i++) {
        printGrammarEntry_t(gram.entry.at[i]);
        printf("\n");
    }
}

void printOptionString(option(string) to_print) {
    if(to_print.valid) {
        printf("some(\"%s\")", to_print.value.at);
    } else {
        printf("none");
    }
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

typedef struct {
    typeof(option(string) (*)(iterstring_t *)) testfn;
    option(string) expected;
    iterstring_t *arg;
} parserTest_t;

void runTests(size_t count, parserTest_t (*tests)[count]) {
    foreach(parserTest_t test of *tests) {
        Dl_info info;
        if (!dladdr((void *)test.testfn, &info)) {
            info.dli_sname = "unknown_function";
        }

        option(string) actual_result = test.testfn(test.arg);
        printf("%s(\"%s\") == ", info.dli_sname, test.arg->str.at);
       
        bool passed = false;
        if(test.expected.valid == actual_result.valid) {
            if(actual_result.valid == true) {
                if(stringeql(actual_result.value, test.expected.value)) {
                    passed = true;
                }
            } else {
                passed = true;
            }
        }

        if(passed) {
            printf("\033[32m");
            printOptionString(actual_result);
            printf(" ✓\033[0m\n");
        } else {
            printf("\033[31m");
            printOptionString(actual_result);
            printf(" ✗\033[0m expected: \033[38;5;214m");
            printOptionString(test.expected);
            printf("\033[0m\n");
        }
        
        if(actual_result.valid) {
            destroyString(actual_result.value);
        }
    }
}

int main() {
    parserTest_t tests[] = {
        // parseLiteral tests
        {
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
        
        // parseIdentifier tests
        {
            .testfn = parseIdentifier,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string("'literal'"),
                .index = 0,
                .previous = 0,
            },
        },
        {
            .testfn = parseIdentifier,
            .expected = some(string("key")), 
            .arg = &(iterstring_t) {
                .str = string("key"),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = parseIdentifier,
            .expected = none,
            .arg = &(iterstring_t) {
                .str = string(""),
                .index = 0,
                .previous = 0,
            }
        },
        {
            .testfn = parseIdentifier,
            .expected = none, 
            .arg = &(iterstring_t) {
                .str = string("8"),
                .index = 0,
                .previous = 0,
            }
        },
        
        // parseWhitespace tests
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
        
        // parseSeperator tests
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
        
        // isFollowedByAlternative tests
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
        
        // parseTypeModifier tests
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

    printf("Running %zu tests...\n\n", lengthof(tests));
    runTests(lengthof(tests), &tests);
    printf("\n");

    printf("=== Integration Test ===\n");
    
    string grammar[] = {
        string("char -> 'a' | 'b' | 'c'"),
        string("digit -> '0' | '1' | '2'"),
        string("token -> char[] | digit[]"),
        string("entry -> value:token")
    };


    option(grammar_t) testg = compileGrammar(lengthof(grammar), &grammar);
    if(testg.valid) {
        printf("✓ Grammar compilation successful\n");
        printf("  Grammar has %zu entries\n\n", testg.value.entry.count);
        
        printGrammar(testg.value);

        bool success = linkGrammar(&testg.value);
        if(success) {
            printf("✓ Linking passed\n\n");
        } else {
            printf("✗ Linking failed\n\n");
        }

        object_t obj = parseIntoObject(createEmptyObject(), string("abc"), 
            &testg.value, string("entry"));

        string result = objectToJson(obj);
        printf("Parse result: %s\n", result.at);

        destroyString(result);
        destroyObject(obj);
        destroyGrammar(&testg.value);
    } else {
        printf("✗ Failed to compile grammar\n");
    }

    foreach(string to_free of grammar) {
        destroyString(to_free);
    }
    
    return 0;
}
