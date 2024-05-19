// #define DEBUG
#include "format.h"
#include "debug.h"

#undef substring
#undef printh
#undef format

dictionary_t createDictionary(size_t count, char *data[count][2]) {
    dictionary_t dictionary_to_return;
    dictionary_to_return.key = malloc(count * sizeof(char *));
    dictionary_to_return.value = malloc(count * sizeof(char *));
    dictionary_to_return.entry_count = count;
    for (size_t i = 0; i < count; i++) {
        dictionary_to_return.key[i] = data[i][0];
        dictionary_to_return.value[i] = data[i][1];
    }
    return dictionary_to_return;
}

void destroyDictionary(dictionary_t to_destroy) {
    for (size_t i = 0; i < to_destroy.entry_count; i++) {
        free(to_destroy.key[i]);
        free(to_destroy.value[i]);
    }
    free(to_destroy.key);
    free(to_destroy.value);
}

void printDictionary(dictionary_t dictionary) {    
    for (size_t i = 0; i < dictionary.entry_count; ++i) {
        printf("%s: ", dictionary.key[i]);
        printf("%s\n", dictionary.value[i]);
    }
}

void printSubstring(substring_t substr) {
    if(substr.start == NULL) {
        printf("<start pointer null>");
    } else {
        char *ptr = substr.start;
        while(ptr != substr.end || *ptr == '\0') {
            putchar(*ptr);
            if(substr.end > substr.start) {
                ptr++;
            } else {
                ptr--;
            }
        }
        putchar(*ptr);
    }
}

char * strdup(const char * s) {
  	size_t len = 1 + strlen(s);
  	char *p = malloc(len);

  	return p ? memcpy(p, s, len) : NULL;
}

char *surroundWithBraces_old(char *text) {
	dbg("called btw");
  	if (!text) {
    	return strdup("{}");
  	}
  	size_t len = strlen(text);
  	if (malloc_usable_size(text) < len + 3) {
    	memmove(text + 1, text, len);
  	} else {
    	char *nbuf = malloc(len + 3);
    	memcpy(nbuf + 1, text, len);
    	free(text);
    	text = nbuf;
  	}
  	text[0] = '{';
  	text[len + 1] = '}';
  	text[len + 2] = '\0';
  	return text;
}

char *surroundWithBraces(char *text) {
    if (!text) {
        return strdup("{}");
    }

    size_t len = strlen(text);
    char *new_text = realloc(text, len + 3);
    if (!new_text) {
        return NULL;
    }

    memmove(new_text + 1, new_text, len);
    new_text[0] = '{';
    new_text[len + 1] = '}';
    new_text[len + 2] = '\0';
	dbg("%s", new_text);
    return new_text;
}


dictionary_t convertKeysToTags(dictionary_t dictionary) {
    for(size_t index = 0; index < dictionary.entry_count; index++) {
        dictionary.key[index] = surroundWithBraces(dictionary.key[index]);
    }
    return dictionary;
}

char *replaceSubstrings_old(char *inputString, dictionary_t dictionary) {
    // Calculate the length of the modified string
    size_t inputLength = strlen(inputString);
    size_t outputLength = inputLength;

    for (size_t i = 0; i < dictionary.entry_count; i++) {
        char *substring = dictionary.key[i];
        char *replacement = dictionary.value[i];

        // Count occurrences of substring
        char *pos = inputString;
        while ((pos = strstr(pos, substring)) != NULL) {
            outputLength += strlen(replacement) - strlen(substring);
            pos += strlen(substring);
        }
    }

    // Allocate memory for the modified string
    char *outputString = malloc(outputLength + 1);
    if (outputString == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Copy and replace substrings
    size_t currentIndex = 0;
    for (size_t i = 0; i < inputLength; i++) {
        int match_found = 0;
        for (size_t j = 0; j < dictionary.entry_count; j++) {
            char *substring = dictionary.key[j];
            char *replacement = dictionary.value[j];

            if (strncmp(inputString + i, substring, strlen(substring)) == 0) {
                strcpy(outputString + currentIndex, replacement);
                currentIndex += strlen(replacement);
                i += strlen(substring) - 1;  // Move the index past the matched substring
                match_found = 1;
                break;
            }
        }

        if (!match_found) {
            outputString[currentIndex++] = inputString[i];
        }
    }

    outputString[currentIndex] = '\0'; // Null-terminate the output string

    return outputString;
}

char *replaceSubstrings_new(char *inputString, dictionary_t dictionary) {
    // Calculate the length of the modified string
    size_t inputLength = strlen(inputString);
    size_t outputLength = inputLength;

    for (size_t i = 0; i < dictionary.entry_count; i++) {
        char *substring = dictionary.key[i];
        char *replacement = dictionary.value[i];

        // Count occurrences of substring
        char *pos = inputString;
        while ((pos = strstr(pos, substring)) != NULL) {
            outputLength += strlen(replacement) - strlen(substring);
            pos += strlen(substring);
        }
    }

    // Allocate memory for the modified string
    char *outputString = malloc(outputLength + 1);
    if (outputString == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Copy and replace substrings
    size_t currentIndex = 0;
    for (size_t i = 0; i < inputLength; i++) {
        int match_found = 0;
        for (size_t j = 0; j < dictionary.entry_count; j++) {
            char *substring = dictionary.key[j];
            char *replacement = dictionary.value[j];

            if (strncmp(inputString + i, substring, strlen(substring)) == 0) {
                strcpy(outputString + currentIndex, replacement);
                currentIndex += strlen(replacement);
                i += strlen(substring) - 1;  // Move the index past the matched substring
                match_found = 1;
                break;
            }
        }

        if (!match_found) {
            outputString[currentIndex++] = inputString[i];
        }
    }

    outputString[currentIndex] = '\0'; // Null-terminate the output string
	free(inputString);
    return outputString;
}

size_t countSubstring(const char *str, const char *sub) {
  size_t count = 0;
  size_t len_sub = strlen(sub);
	// Iterate through the main string
	for (size_t i = 0; str[i] != '\0'; i++) {
    	// Check if substring matches at current position
    	if (strncmp(str + i, sub, len_sub) == 0) {
      		count++;
      		// Move i to the end of the substring to avoid counting overlaps
      		i += len_sub - 1;
    	}
  	}

	return count;
}

dictionary_index_search_t sequenceMatchesDictionaryKey(char *str, size_t index, dictionary_t dictionary) {
    dictionary_index_search_t result;
    result.success = false;
    result.index = 0;
    dbgmem(str);

    // Iterate through the dictionary keys to find a match
    for (size_t i = 0; i < dictionary.entry_count; i++) {
        size_t key_len = strlen(dictionary.key[i]);

        // Check if there are enough characters remaining in str to compare with the key
        if (index + key_len <= strlen(str) && strncmp(str + index, dictionary.key[i], key_len) == 0) {
			dbg("str is %p, the string is %s", str, str);
			dbg("str is %p, the string is %s", str, str);
			dbg("hello?");
            result.success = true;
            result.index = i;
            return result;
        }
    }
    return result;
}

char *replaceSubstrings(char *input_string, dictionary_t dictionary) {
    // Calculate the difference in length for all replacements
	dbg("the received string is %s, strlen is %ld, usable memory %ld", 
	input_string, strlen(input_string), malloc_usable_size(input_string));
	size_t curstrlen = strlen(input_string);
    int64_t max_increase = 0;
    int64_t max_decrease = 0;
    int64_t diff = 0; // im sure this wont backfire
    int64_t len_diff = 0;
    for (size_t i = 0; i < dictionary.entry_count; i++) {
        diff = 
			countSubstring(input_string, dictionary.key[i]) * strlen(dictionary.value[i]) - 
			countSubstring(input_string, dictionary.key[i]) * strlen(dictionary.key[i]);
            dbg("diff: %ld", diff);
        if(diff > INT64_MAX || diff < INT64_MIN) {
            fprintf(stderr, "congrats, string difference is so large that it doesnt fit into 64 bit\n");
            fprintf(stderr, "%s @ %d in %s\n", __FUNCTION__, __LINE__, __FILE__);
            exit(EXIT_FAILURE);
        }
        if(diff < 0) {
            max_decrease += diff;
        } else {
            max_increase += diff;
        }
    }
    len_diff = max_increase + max_decrease;
    dbg("max_increase: %ld, max_decrease: %ld, len_diff %ld", max_increase, max_decrease, len_diff);

    // Reallocate memory for the modified string with extra space for null terminator
    size_t new_len = strlen(input_string) + max_increase + 1;
	dbg("the new length is %ld, the old was %ld\n", new_len, strlen(input_string));
    input_string = realloc(input_string, new_len);
	dbg("after realloc the new length is %ld, the old was %ld\n", new_len, strlen(input_string));    
    // input_string[new_len] = 0;
    dbgstr(input_string, new_len);
    dbgmem(input_string);
    if (input_string == NULL) {
        fprintf(stderr, "realloc in replaceSubstrings failed\n");
        exit(EXIT_FAILURE);
    }
    
	size_t iter = 0;
	while(input_string[iter]) {
		dbgmem(input_string);
		dictionary_index_search_t search = sequenceMatchesDictionaryKey(input_string, iter, dictionary);
        dbgstr(input_string, iter);
		if(search.success) {
			
			size_t pushback_len = strlen(dictionary.value[search.index]) 
				- strlen(dictionary.key[search.index]);
            dbg("the pushback_len is: %ld", pushback_len);

            dbgstr(input_string, iter, iter + strlen(dictionary.key[search.index]) + pushback_len,
            input_string + iter + strlen(dictionary.key[search.index]), 
            	iter + strlen(dictionary.key[search.index]) 
            	+ curstrlen - iter - strlen(dictionary.key[search.index]) + 1);

			memmove(input_string + iter + strlen(dictionary.key[search.index]) + pushback_len, 
                input_string + iter + strlen(dictionary.key[search.index]), 
                curstrlen - iter - strlen(dictionary.key[search.index]) + 1);
                
            // input_string[iter + pushback_len] = '\0';
			// curstrlen = strlen(input_string);
			curstrlen += strlen(dictionary.value[search.index]) - strlen(dictionary.key[search.index]);
			strncpy(input_string + iter, dictionary.value[search.index], strlen(dictionary.value[search.index]));
            iter += strlen(dictionary.value[search.index]) - 1;
  		}
		iter++;
	}
    return realloc(input_string, strlen(input_string) + 1);
}

substring_t substring(char *start, char *end) {
    return (substring_t) {
        start, end
    };
}

int asprintf (char **str, const char *fmt, ...) {
  int size = 0;
  va_list args;

  // init variadic argumens
  va_start(args, fmt);

  // format and get size
  size = vasprintf(str, fmt, args);

  // toss args
  va_end(args);

  return size;
}

int vasprintf (char **str, const char *fmt, va_list args) {
  	int size = 0;
  	va_list tmpa;
	
  	// copy
  	va_copy(tmpa, args);
	
  	// apply variadic arguments to
  	// sprintf with format to get size
  	size = vsnprintf(NULL, 0, fmt, tmpa);
	
  	// toss args
  	va_end(tmpa);
	
  	// return -1 to be compliant if
  	// size is less than 0
  	if (size < 0) { return -1; }
	
  	// alloc with size plus 1 for `\0'
  	*str = (char *) malloc(size + 1);
	
  	// return -1 to be compliant
  	// if pointer is `NULL'
  	if (NULL == *str) { return -1; }

	// format string with original
  	// variadic arguments and set new size
  	size = vsprintf(*str, fmt, args);
  	return size;
} // end of asprintf

int getIdentifierIndex(char *in, size_t index) {
	if(strlen(in) < index) {
        return -1;
    }
    int position = -1;
	for(size_t i = index; in[i] != 0; i++) 
        if(in[i] <= ' ' || in[i] == '=') {
	    	position = i;
		    break;
    	}
	return position;	
}

char *stringAfter(char *in, size_t index) {
	if(in == NULL) {
		return NULL;
	}
	size_t length = strlen(in);
	index = (length <= index) ? length: index;
	char *substring;
	(substring = strncpy(malloc(index + 1), in, index))[index] = '\0'; 
	return substring;
}

char *getIdentifier(char *in) {
	return stringAfter(in, getIdentifierIndex(in, 0));
}

// buf is going to be edited by 
char *positionalInsert(char *buf, dictionary_t dictionary) {
	if(buf == NULL) {
		return NULL;
	}

	size_t len = strlen(buf);
	// keeps track of number of {} and indexes
	// into the dictionary correctly	
	size_t dictionary_index = 0; 
	
	size_t index = 0;
	char ch;
	while(ch = buf[index]) {
		dbg("main loop running, the index is %ld\n", index);
		dbgstr(buf, index);
		if(ch == '{') {
			dbg("the charcter ch is now \e[0;31m%c\e[0m (should be '{')", buf[index]);
			size_t run = index + 1;
			size_t temp_number = 0;
			dbgstr(buf, index);
			if(buf[index + 1] == '}') {
				dictionary_index++; // todo insert value from index
				dbg("dictionary index: %ld\n", dictionary_index);
				size_t val_len = strlen(dictionary.value[dictionary_index - 1]);
				size_t new_length = len + val_len + 1; // null terminator :)
				if(val_len > 2) {
					buf = realloc(buf, new_length);
				}
				// 								the + 1 copies the null terminator
				memmove(buf + index + val_len, buf + index + 2, len - index - 2 + 1);
				if(val_len < 2) {
					buf = realloc(buf, new_length);
				}
				len += val_len - 2;
				if(val_len != 0) {
					strncpy(buf + index, dictionary.value[dictionary_index - 1], 
						strlen(dictionary.value[dictionary_index - 1]));
				} 
				dbg("->%s\n", buf);
			} else {
				dbg("entering number construction segment");
				while(('0' <= buf[run] && buf[run] <= '9') || buf[run] == ' ') {
					temp_number *= 10;
					temp_number += buf[run] - '0';
					run++;
					dbg("constructed number is %ld", temp_number);
					dbgstr(buf, index, run);
				} 

				size_t end = run + 1;
				char end_char;
				while(end_char = buf[end]) {
					dbg("end tests");
					if(end_char == '}') {
						// end found

						// set dictionary_index to the number
						dictionary_index = temp_number;
						dbg("dictionary index: %ld\n", dictionary_index);
						break;
					} else if(end_char == ' ') {
						// keep searching for end
						end++;
					} else {
						dbg("no end possible");
						break;
						// end not possible
						// discard results
					}
				}
			}
		} else {
			dbg("skipping to next character");
		}
		index++;
	}
	dbg("->%s\n", buf);
	return buf;	
}
                                                                                          
char *format(char *buf, dictionary_t dictionary) {
    char *output;
	output = positionalInsert(buf, dictionary);
	output = replaceSubstrings(output, dictionary);
    return output;
}

int printh(char *fmt, __attribute_maybe_unused__ dictionary_t dictionary) {
	char *output = format(strdup(fmt), dictionary);
    int output_length = strlen(output);
	fputs(output, stdout);
	free(output);
	return output_length;
}
