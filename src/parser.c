#include "../include/parser.h"
#include "../CSTL/include/cstl.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

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
    parseWhitespace(rule);
    
    option(string) res;
    
    if((res = parseLiteral(rule)).valid) {
        rule_t ret = {
            .storage_key = none,
            .literal_or_rule = is_literal,
            .literal = res.value,
            .type_mod = parseTypeModifier(rule),		
        };
        return (option(rule_t)) some(ret);
    }
    
    if((res = parseIdentifier(rule)).valid) {
        parseWhitespace(rule);
        
        if(rule->str.at[rule->index] == ':') {
            rule->index++;
            iterstringAdvance(rule);
            parseWhitespace(rule);
            
            option(string) rule_name = parseIdentifier(rule);
            if(!rule_name.valid) {
                destroyString(res.value);
                return (option(rule_t)) none;
            }
            
            rule_t ret = {
                .storage_key = some(res.value),
                .literal_or_rule = is_rule,
                .rule_name = rule_name.value,
                .ge = NULL,
                .type_mod = parseTypeModifier(rule),
            };
            return (option(rule_t)) some(ret);
        } else {
            // Just a rule reference, no storage key
            rule_t ret = {
                .storage_key = none,
                .literal_or_rule = is_rule,
                .rule_name = res.value,
                .ge = NULL,
                .type_mod = parseTypeModifier(rule),
            };
            return (option(rule_t)) some(ret);
        }
    }   
    return (option(rule_t)) none;
}

bool parseStringLiterally(iterstring_t *rule, const char *literal) {
    size_t len = strlen(literal);
    for(size_t i = 0; i < len; i++) {
        if(rule->str.at[rule->index + i] != literal[i]) {
            return false;
        }
    }
    rule->index += len;
    iterstringAdvance(rule);
    return true;
}

option(rule_node_t) compileRuleNode(iterstring_t *rule) {
    parseWhitespace(rule);
    
    // Parse first rule
    option(rule_t) first_rule = compileRule(rule);
    if(!first_rule.valid) {
        return (option(rule_node_t)) none;
    }
    
    parseWhitespace(rule);
    
    // Check if there are alternatives
    if(rule->str.at[rule->index] == '|') {
        // We have alternatives - collect them
        dynarray(rule_t) alternatives = create_dynarray(rule_t);
        dynarray_append(alternatives, first_rule.value);
        
        while(isFollowedByAlternative(rule)) {
            option(rule_t) alt_rule = compileRule(rule);
            if(!alt_rule.valid) {
                destroy_dynarray(alternatives);
                return (option(rule_node_t)) none;
            }
            dynarray_append(alternatives, alt_rule.value);
            parseWhitespace(rule);
        }
        
        rule_node_t node = {
            .alternative_or_regular = has_alternative,
            .alternative = alternatives
        };
        return (option(rule_node_t)) some(node);
    } else {
        // Just a single rule
        rule_node_t node = {
            .alternative_or_regular = is_regular,
            .rule = first_rule.value
        };
        return (option(rule_node_t)) some(node);
    }
}

option(grammar_entry_t) compileGrammarEntry(string rule_definition) {
    iterstring_t it = { .previous = 0, .index = 0, .str = rule_definition };
    
    parseWhitespace(&it);
    option(string) name = parseIdentifier(&it);
    if(!name.valid) {
        fprintf(stderr, "Failed to parse rule name\n");
        return (option(grammar_entry_t)) none;
    }
    
    parseWhitespace(&it);
    
    grammar_entry_t ret = {
        .name = name.value,
        .rule_type = storage_type_not_set,
    };
    
    ret.element = create_dynarray(rule_node_t);
    
    if(!parseStringLiterally(&it, "->")) {
        fprintf(stderr, "Expected '->' after rule name '%s'\n", name.value.at);
        destroyString(ret.name);
        return (option(grammar_entry_t)) none;
    }
    
    parseWhitespace(&it);
    
    bool has_storage_key = false;
    option(rule_node_t) node;
    
    while((node = compileRuleNode(&it)).valid) {
        if(node.value.alternative_or_regular == is_regular) {
            if(node.value.rule.storage_key.valid) {
                has_storage_key = true;
            }
        } else {
            for(size_t i = 0; i < node.value.alternative.count; i++) {
                if(node.value.alternative.at[i].storage_key.valid) {
                    has_storage_key = true;
                    break;
                }
            }
        }
        
        dynarray_append(ret.element, node.value);
        parseWhitespace(&it);
    }
    
    if(ret.element.count == 0) {
        fprintf(stderr, "Rule '%s' has no elements\n", ret.name.at);
        destroyString(ret.name);
        destroy_dynarray(ret.element);
        return (option(grammar_entry_t)) none;
    }
    
    ret.rule_type = has_storage_key ? object_storage : implicit_storage;
    
    return (option(grammar_entry_t)) some(ret);
}

option(grammar_t) compileGrammar(size_t count, typeof(string) (*rules)[count]) {
    grammar_t gram;
    gram.entry = create_dynarray(grammar_entry_t);
    
    for(size_t i = 0; i < count; i++) {
        option(grammar_entry_t) entry = compileGrammarEntry((*rules)[i]);
        if(!entry.valid) {
            fprintf(stderr, "Failed to compile grammar entry %zu: '%s'\n", 
                    i, (*rules)[i].at);
            destroy_dynarray(gram.entry);
            return (option(grammar_t)) none;
        }
        dynarray_append(gram.entry, entry.value);
    }
    
    return (option(grammar_t)) some(gram);
}

option(size_t) findGrammarEntry(grammar_t *gram, string *name) {
    for(size_t i = 0; i < gram->entry.count; i++) {
        if(stringeql(gram->entry.at[i].name, *name)) {
            return (option(size_t)) some(i);
        }
    }
    return (option(size_t)) none;
}

bool linkRule(rule_t *rule, grammar_t *gram) {
    if(rule->literal_or_rule == is_rule) {
        option(size_t) idx = findGrammarEntry(gram, &rule->rule_name);
        if(!idx.valid) {
            fprintf(stderr, "Linking failed: unknown rule '%s'\n", 
                    rule->rule_name.at);
            return false;
        }
        rule->ge = &gram->entry.at[idx.value];
    }
    return true;
}

bool linkRuleNode(rule_node_t *node, grammar_t *gram) {
    if(node->alternative_or_regular == is_regular) {
        return linkRule(&node->rule, gram);
    } else {
        for(size_t i = 0; i < node->alternative.count; i++) {
            if(!linkRule(&node->alternative.at[i], gram)) {
                return false;
            }
        }
        return true;
    }
}

bool linkGrammar(grammar_t *gram) {
    if(!gram) return false;
    
    for(size_t i = 0; i < gram->entry.count; i++) {
        grammar_entry_t *entry = &gram->entry.at[i];
        
        for(size_t j = 0; j < entry->element.count; j++) {
            if(!linkRuleNode(&entry->element.at[j], gram)) {
                return false;
            }
        }
    }
    
    return true;
}

void destroyGrammar(grammar_t *gram) {
    if(!gram) return;
    
    for(size_t i = 0; i < gram->entry.count; i++) {
        grammar_entry_t *entry = &gram->entry.at[i];
        destroyString(entry->name);
        
        for(size_t j = 0; j < entry->element.count; j++) {
            rule_node_t *node = &entry->element.at[j];
            
            if(node->alternative_or_regular == has_alternative) {
                for(size_t k = 0; k < node->alternative.count; k++) {
                    rule_t *rule = &node->alternative.at[k];
                    if(rule->storage_key.valid) {
                        destroyString(rule->storage_key.value);
                    }
                    if(rule->literal_or_rule == is_literal) {
                        destroyString(rule->literal);
                    } else {
                        destroyString(rule->rule_name);
                    }
                }
                destroy_dynarray(node->alternative);
            } else {
                rule_t *rule = &node->rule;
                if(rule->storage_key.valid) {
                    destroyString(rule->storage_key.value);
                }
                if(rule->literal_or_rule == is_literal) {
                    destroyString(rule->literal);
                } else {
                    destroyString(rule->rule_name);
                }
            }
        }
        destroy_dynarray(entry->element);
    }
    destroy_dynarray(gram->entry);
}

static option(obj_t_value_t) executeRule(iterstring_t *is, rule_t *rule, grammar_t *gram);
static option(obj_t_value_t) executeRuleNode(iterstring_t *is, rule_node_t *node, grammar_t *gram);
static option(obj_t_value_t) executeGrammarEntry(iterstring_t *is, grammar_entry_t *entry, grammar_t *gram);

static bool parseLiteralFromInput(iterstring_t *is, string literal) {
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
        
        case obj_t_null:
        case obj_t_true:
        case obj_t_false:
        case obj_t_number:
        default:
            return string("");
    }
}

static option(obj_t_value_t) executeRule(iterstring_t *is, rule_t *rule, grammar_t *gram) {
    if(rule->literal_or_rule == is_literal) {
        if(!parseLiteralFromInput(is, rule->literal)) {
            return (option(obj_t_value_t)) none;
        }
        
        obj_t_value_t ret = {
            .discriminant = obj_t_string,
            .str = stringFromString(rule->literal)
        };
        return (option(obj_t_value_t)) some(ret);
        
    } else {
        if(!rule->ge) {
            fprintf(stderr, "Unlinked rule reference\n");
            return (option(obj_t_value_t)) none;
        }
        
        if(rule->type_mod & modifier_array) {
            array_t arr = createEmptyArray();
            
            option(obj_t_value_t) first = executeGrammarEntry(is, rule->ge, gram);
            if(!first.valid) {
                destroyArray(arr);
                return (option(obj_t_value_t)) none;
            }
            
            arr = insertIntoArray(arr, first.value);
            
            while(1) {
                size_t save_pos = is->index;
                option(obj_t_value_t) next = executeGrammarEntry(is, rule->ge, gram);
                if(!next.valid) {
                    is->index = save_pos;
                    break;
                }
                arr = insertIntoArray(arr, next.value);
            }
            
            obj_t_value_t ret = {
                .discriminant = obj_t_array,
                .arr = arr
            };
            return (option(obj_t_value_t)) some(ret);
            
        } else if(rule->type_mod & modifier_optional) {
            size_t save_pos = is->index;
            option(obj_t_value_t) opt = executeGrammarEntry(is, rule->ge, gram);
            if(!opt.valid) {
                is->index = save_pos;
                obj_t_value_t ret = {
                    .discriminant = obj_t_null
                };
                return (option(obj_t_value_t)) some(ret);
            }
            return opt;
            
        } else {
            return executeGrammarEntry(is, rule->ge, gram);
        }
    }
}

static option(obj_t_value_t) executeRuleNode(iterstring_t *is, rule_node_t *node, grammar_t *gram) {
    if(node->alternative_or_regular == is_regular) {
        return executeRule(is, &node->rule, gram);
    } else {
        for(size_t i = 0; i < node->alternative.count; i++) {
            size_t save_pos = is->index;
            option(obj_t_value_t) result = executeRule(is, &node->alternative.at[i], gram);
            if(result.valid) {
                return result;
            }
            is->index = save_pos;
            iterstringReset(is);
        }
        return (option(obj_t_value_t)) none;
    }
}

static option(obj_t_value_t) executeGrammarEntry(iterstring_t *is, grammar_entry_t *entry, grammar_t *gram) {
    if(entry->rule_type == implicit_storage) {
        string result = string("");
        
        for(size_t i = 0; i < entry->element.count; i++) {
            option(obj_t_value_t) val = executeRuleNode(is, &entry->element.at[i], gram);
            if(!val.valid) {
                destroyString(result);
                return (option(obj_t_value_t)) none;
            }
            
            string part = flattenToString(val.value);
            result = appendString(result, part);
            destroyString(part);
        }
        
        obj_t_value_t ret = {
            .discriminant = obj_t_string,
            .str = result
        };
        return (option(obj_t_value_t)) some(ret);
        
    } else {
        object_t result = createEmptyObject();
        
        for(size_t i = 0; i < entry->element.count; i++) {
            rule_node_t *node = &entry->element.at[i];
            
            if(node->alternative_or_regular == has_alternative) {
                bool matched = false;
                for(size_t j = 0; j < node->alternative.count; j++) {
                    size_t save_pos = is->index;
                    option(obj_t_value_t) val = executeRule(is, &node->alternative.at[j], gram);
                    if(val.valid) {
                        if(node->alternative.at[j].storage_key.valid) {
                            result = insertObjectEntry(result, 
                                node->alternative.at[j].storage_key.value, val.value);
                        }
                        matched = true;
                        break;
                    }
                    is->index = save_pos;
                    iterstringReset(is);
                }
                if(!matched) {
                    destroyObject(result);
                    return (option(obj_t_value_t)) none;
                }
            } else {
                option(obj_t_value_t) val = executeRule(is, &node->rule, gram);
                if(!val.valid) {
                    destroyObject(result);
                    return (option(obj_t_value_t)) none;
                }
                
                if(node->rule.storage_key.valid) {
                    result = insertObjectEntry(result, node->rule.storage_key.value, val.value);
                }
            }
        }
        
        obj_t_value_t ret = {
            .discriminant = obj_t_obj,
            .obj = result
        };
        return (option(obj_t_value_t)) some(ret);
    }
}

object_t parseIntoObject(object_t obj, string input, grammar_t *gram, string start_rule) {
    iterstring_t is = { .str = input, .index = 0, .previous = 0 };
    
    option(size_t) start_idx = findGrammarEntry(gram, &start_rule);
    if(!start_idx.valid) {
        fprintf(stderr, "Start rule '%s' not found in grammar\n", start_rule.at);
        return obj;
    }
    
    grammar_entry_t *start_entry = &gram->entry.at[start_idx.value];
    
    option(obj_t_value_t) result = executeGrammarEntry(&is, start_entry, gram);
    if(!result.valid) {
        fprintf(stderr, "Failed to parse input\n");
        return obj;
    }
    
    if(is.str.at[is.index] != '\0') {
        fprintf(stderr, "Warning: parsing succeeded but %zu characters remain at position %zu\n",
                stringlen(is.str) - is.index, is.index);
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

void printParsingMessage(FILE *stream, char *msg, string source, 
                        const char *const color, size_t color_start, size_t color_stop) {
    fprintf(stream, "%s\n", msg);
    assert((color_start < color_stop) && 
        (color_start < stringlen(source)) && 
        (color_stop < stringlen(source)));
    
    for(size_t i = 0; i < stringlen(source); i++) {
        if(i == color_start) {
            fprintf(stream, "%s", color);
        }
        fputc(source.at[i], stream);
        if(i == color_stop - 1) {
            fprintf(stream, "\033[0m");
        }
    }
    fputc('\n', stream);
}
