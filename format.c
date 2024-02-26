#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include "map.h"

// todo: reorganize asprintf and make va_list by ref
#define DEBUG 1

#if DEBUG
#define free(x) ({dbg("freeing variable %s with the value %p", #x, x); free(x);})
#define dbg(fmt, ...) \
    ({  \
        printf("[debug: %s @ %d in %s from %s] " fmt "\n", \
        __FUNCTION__, __LINE__, __FILE__, getCaller() __VA_OPT__(, __VA_ARGS__)); \
        sleep(0); \
    })
#else
#define dbg(fmt, ...)
#endif

#define __USE_GNU
#include <dlfcn.h>
#include <execinfo.h>

const char *getCaller(void) {
    void *callstack[3];
    const int maxFrames = sizeof(callstack) / sizeof(callstack[0]);
    Dl_info info;

    int frames = backtrace(callstack, maxFrames);

    if (dladdr(callstack[2], &info) && info.dli_sname != NULL) {
        // printf("I was called from: %s\n", info.dli_sname);
        return info.dli_sname;
    } else {
        // printf("Unable to determine calling function\n");
        return "<unknown or symbol table not availible>";
    }
}

typedef struct {
    char *start;
    char *end;
} substring_t;

typedef struct {
    size_t count;
    substring_t *results;
} substring_search_result_t; 

typedef struct {
    substring_t to_be_replaced;
    char *to_insert;
} substring_replacement_t;

typedef struct {
    char *string_to_edit;
    size_t count;
    substring_replacement_t *replacements;
} replacement_mapping_t;

typedef struct {
    size_t entry_count;
    char **key;
    char **value;
} dictionary_t;

char * strdup(const char * s) {
  size_t len = 1 + strlen(s);
  char *p = malloc(len);

  return p ? memcpy(p, s, len) : NULL;
}

dictionary_t createDictionary(size_t count, char *data[count][2]) {
    dictionary_t dictionary_to_return;
    dictionary_to_return.key = malloc(count * sizeof(char *));
    dictionary_to_return.value = malloc(count * sizeof(char *));
    dictionary_to_return.entry_count = count;
    for (size_t i = 0; i < count; i++) {
        dictionary_to_return.key[i] = strdup(data[i][0]);
        dictionary_to_return.value[i] = strdup(data[i][1]);
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

#define dict(...) ({                                \
    char *dictionary[][2] = __VA_ARGS__;            \
    size_t size = lengthof(dictionary);             \
    createDictionary(size, dictionary);             \
})

substring_t substring(char *start, char *end);

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

void printSubstringSearchResult(substring_search_result_t substr_srh_res) {
    putchar('{');
    for(size_t index = 0; index < substr_srh_res.count - 1; index++) {
        printSubstring(substr_srh_res.results[index]);
        putchar(',');
        putchar(' ');
    }
    printSubstring(substr_srh_res.results[substr_srh_res.count - 1]);
    putchar('}');
}

substring_search_result_t createSubstringSearchResult(size_t count) {
    return (substring_search_result_t) { 
        .count = count, 
        .results = malloc(count * sizeof(substring_t)),
    };
}

void destroySubstringSearchResult(substring_search_result_t result_to_destroy) {
    result_to_destroy.count = 0;
    free(result_to_destroy.results);
}

char *surroundWithBraces(char text[static 1]) {
  if (!text) return strdup("{}");
  int len = strlen(text);
  if (malloc_usable_size(text) < len + 3) {
    memmove(text + 1, text, len);
  } else {
    char* nbuf = malloc(len + 3);
    memcpy(nbuf + 1, text, len);
    free(text);
    text = nbuf;
  }
  text[0] = '{';
  text[len] = '}';
  text[len + 1] = 0;
  return text;
}

char *surroundWithBraces_old(char text[static 1]) {
    if(!text || !(text = realloc(text, strlen(text) + 2 + 1))) {
        return NULL;
    }
    size_t index = 0;
    char temp = '{';
    while(temp ^= text[index++] ^= temp ^= text[index]);
    text[index++] = '}';
    text[index] = '\0';
    return text;
}

dictionary_t convertKeysToTags(dictionary_t dictionary) {
    for(size_t index = 0; index < dictionary.entry_count; index++) {
        dictionary.key[index] = surroundWithBraces(dictionary.key[index]);
    }
    return dictionary;
}

char *replaceSubstrings(char *inputString, dictionary_t dictionary) {
    // Calculate the length of the modified string
    size_t inputLength = strlen(inputString);
    size_t outputLength = inputLength;

    for (int i = 0; i < dictionary.entry_count; i++) {
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
    char *outputString = (char *)malloc(outputLength + 1);
    if (outputString == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Copy and replace substrings
    size_t currentIndex = 0;
    for (size_t i = 0; i < inputLength; i++) {
        int match_found = 0;
        for (int j = 0; j < dictionary.entry_count; j++) {
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

substring_search_result_t searchSubstring(char *string_a, char *string_b) {
    // determine which is the substring and which is the search string
    struct { size_t a; size_t b; } length_ab = { 
        .a = strlen(string_a),
        .b = strlen(string_b),
    };

    char *needle;
    char *haystack;
    struct { size_t needle; size_t haystack; } length = {
        .needle = (length_ab.a > length_ab.b) ? length_ab.b : length_ab.a,
        .haystack = (length_ab.a > length_ab.b) ? length_ab.a : length_ab.b,
    };

    needle = (length_ab.a > length_ab.b) ? string_b : string_a;
    haystack = (length_ab.a > length_ab.b) ? string_a : string_b;

    substring_search_result_t result;
    char *found_ptr; // pointer to the needle found by strstr
    size_t ratio; // upper limit of how many needles fit in the haystack
    size_t actual_count = 0;

    ratio = (size_t)(length.haystack / length.needle);
    result = createSubstringSearchResult(ratio);
    found_ptr = haystack; 

    for(size_t iteration = 0; iteration < ratio; iteration++) {
        dbg("found_ptr: %p, haystack: %p", found_ptr, haystack);

        assert(found_ptr != NULL);
        assert(haystack != NULL);
        
        found_ptr = strstr(found_ptr, needle);
        if(found_ptr != NULL) {  // Check if found_ptr is not NULL before proceeding
            result.results[actual_count].start = found_ptr;
            result.results[actual_count].end = found_ptr + length.needle - 1;
            dbg("start: %p end: %p", result.results[actual_count].start, result.results[actual_count].end);
            actual_count++;
            found_ptr += length.needle;
        } else {
            break;
        }
    }
    // downsize the result if needed
    if(actual_count != 0) {
        result.count = actual_count;
        result.results = realloc(result.results, actual_count * sizeof(substring_t));
        dbg("reallocating as only %d substrings were found", actual_count);
    } else {
        result.count = 0;
        free(result.results);
        result.results = NULL;
        dbg("no result was found");
    }
    return result;
}

substring_search_result_t OLDsearchSubstring(char *string_a, char *string_b) {
    // determine which is the substring and which is the search string
    struct { size_t a; size_t b; } length_ab = { 
        .a = strlen(string_a),
        .b = strlen(string_b),
    };

    char *needle;
    char *haystack;
    struct { size_t needle; size_t haystack; } length = {
        .needle = (length_ab.a > length_ab.b) ? length_ab.b : length_ab.a,
        .haystack = (length_ab.a > length_ab.b) ? length_ab.a : length_ab.b,
    };

    needle = (length_ab.a > length_ab.b) ? string_b : string_a;
    haystack = (length_ab.a > length_ab.b) ? string_a : string_b;

    substring_search_result_t result;
    char *found_ptr; // pointer to the needle found by strstr
    size_t ratio; // upper limit of how many needles fit in the haystack
    size_t actual_count = 0;

    ratio = (size_t)(length.haystack / length.needle);
    result = createSubstringSearchResult(ratio);
    found_ptr = needle; 
    for(size_t iteration = 0; iteration < ratio; iteration++) {
        dbg("found_ptr: %p, haystack: %p", found_ptr, haystack);

        assert(found_ptr != NULL);
        assert(haystack != NULL);
        
        if(found_ptr = strstr(found_ptr, haystack)) {
            actual_count++;
            result.results[actual_count] = substring(found_ptr, found_ptr + length.needle); 
            found_ptr += length.needle;
        } else {
            break;
        }
    }
    // downsize the result if needed
    result.count = actual_count;
    result.results = realloc(result.results, actual_count * sizeof(substring_t));
    return result;
}

substring_t substring(char *start, char *end) {
    return (substring_t) {
        start, end
    };
}

#define substring(start, end) \
    ({ \
        assert((start) != NULL && (end) != NULL); \
        substring(start, end); \
    })

int vasprintf(char **str, const char *fmt, va_list args);

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

#define lengthof(array) (sizeof(array) / sizeof((array)[0]))

int printfi(char *fmt, dictionary_t dictionary);
char *format(char *fmt, dictionary_t dictionary);

typedef struct { char * key; char *value; } replacement_pair;

int getIdentifierIndex(char *in, size_t index) { // maybe change interface
	int position = -1;
	for(int i = 0; in[i] != 0; i++) if(in[i] <= ' ' || in[i] == '=') {
		position = i;
		break;
	}
	return position;	
}

char *stringAfter(char *in, size_t index) {
	size_t length = strlen(in);
	index = (length <= index) ? length: index;
	char *substring;
	(substring = strncpy(malloc(index + 1), in, index))[index] = '\0'; 
	return substring;
}

char *getIdentifier(char *in) {
	return stringAfter(in, getIdentifierIndex(in, 0));
}

#define _generic_format(ptr, x) \
		_Generic((x), \
		char: asprintf(ptr, "%c", x), \
	    unsigned char: asprintf(ptr, "%u", x), \
	    short: asprintf(ptr, "%hd", x), \
	    unsigned short: asprintf(ptr, "%hu", x), \
	    int: asprintf(ptr, "%d", x), \
	    unsigned int: asprintf(ptr, "%u", x), \
	    long: asprintf(ptr, "%ld", x), \
	    unsigned long: asprintf(ptr, "%lu", x), \
	    long long: asprintf(ptr, "%lld", x), \
	    unsigned long long: asprintf(ptr, "%llu", x), \
	    float: asprintf(ptr, "%f", x), \
	    double: asprintf(ptr, "%lf", x), \
	    long double: asprintf(ptr, "%Lf", x), \
	    char *: asprintf(ptr, "%s", x), \
	    default: (-1) \
	)	

#define createReplacementPair(in) 							    \
	({ 														    \
		_Pragma("GCC diagnostic push"); 					    \
		_Pragma("GCC diagnostic ignored \"-Wformat=\""); 	    \
		_Pragma("GCC diagnostic ignored \"-Wunused-variable\"");\
		char *temp; 										    \
		typeof(in) _in = in; 									\
		_generic_format(&temp, _in);     					    \
		_Pragma("GCC diagnostic pop");						    \
		replacement_pair ret = (replacement_pair) { 		    \
			.key = getIdentifier(#in),						    \
			.value = temp									    \
		};													    \
		ret; }),

char *strstrTag2(char *haystack, char *needle, size_t offset) {
    int needle_len = strlen(needle);
    haystack += offset;
    while (*haystack != '\0') {
        if (*haystack == '{') {
            char *end_brace = strchr(haystack, '}');
            if (end_brace != NULL) {
                int inside_len = (int)(end_brace - haystack + 1);

                if (inside_len >= needle_len &&
                    strncmp(haystack + 1, needle, needle_len) == 0) {
                    return haystack;
                }
                haystack = end_brace + 1;
            } else {
                break;
            }
        } else {
            ++haystack;
        }
    }
    return NULL;
}

char *strstrTag(char *haystack, char *needle, size_t offset) {
	int needle_len = strlen(needle);
    dbg("the needle is: %s\n", needle);
    dbg("the needle length is %d\n", needle_len);
    haystack += offset + offset == 0;
	char *found_ptr;
	while(*haystack) {
		found_ptr = strstr(haystack, needle);
		dbg("the found ptr is: %p\n", found_ptr);
		dbg("the string is: %.6s\n", found_ptr);
		if(found_ptr != NULL) {
			if(true) {
				dbg("character at start: %c\n", found_ptr[0]);
				dbg("end char: %c -> %d\n", found_ptr[needle_len], found_ptr[needle_len]);
				if(found_ptr[-1] == '{' && found_ptr[needle_len] == '}') {
					dbg("%s", found_ptr - 1);
					return found_ptr - 1;
				} else {
					haystack = found_ptr + needle_len;
				}
			} else {
				haystack++;
			}
		} else {
			break;
		}
	}
	return NULL;
}

char *format(char *fmt, dictionary_t dictionary) {
    char *output;
   	output = replaceSubstrings(fmt, dictionary);
    return output;
}

#define format(fmt, ...) 												 \
	format(fmt, 														 \
	lengthof((replacement_pair[]) 										 \
	{MAP(createReplacementPair, __VA_ARGS__)}), 						 \
	MAP(createReplacementPair, __VA_ARGS__) 0)

int printfi(char *fmt, dictionary_t dictionary) {
	char *output = vformat(fmt, dictionary);
	int output_length = strlen(output);
	fputs(output, stdout);
	free(output);
	return output_length;
}

#define printfi(fmt, ...)   												 \
		printfi(fmt, 														 \
		lengthof((replacement_pair[]) 										 \
		{MAP(createReplacementPair, __VA_ARGS__)}), 						 \
		MAP(createReplacementPair, __VA_ARGS__) 0) // avoiding trailing comma

bool isThisATag(char *main_string, substring_t potential_tag) {
    dbg("the main string is %s, the substring is [%p - %p]", main_string, potential_tag.start, potential_tag.end);
    if(main_string == potential_tag.start) { // avoid segfault
        dbg("returning true for ");
        printSubstring(potential_tag);
        return false;
    }

    dbg("the char is %c", potential_tag.end[1]);
    if(potential_tag.start[-1] == '{' && potential_tag.end[1] == '}') {
        dbg("returning true for ");
        printSubstring(potential_tag);
        return true;
    }
    dbg("returning false for ");
    printSubstring(potential_tag);
    return false;
}

int compare_substring_start(void *a, void *b) {
    substring_t *_a = a;
    substring_t *_b = b;
    if(_a == NULL && _b != NULL) {
        return -1;
    }
    if(_b == NULL && _a != NULL) {
        return 1;
    }
    if(_a == NULL && _b == NULL) {
        return 0;
    }
    if(_a->start > _b->start) {
        return 1;
    }
    if(_a->start == _b->start) {
        return 0;
    }
    if(_a->start < _b->start) {
        return -1;
    }
}

char *vformat(char fmt[static 1], dictionary_t dictionary) {
    char *output;
    output = replaceSubstrings(fmt, dictionary);
    return output;
}


char *vformat_very_old(char *fmt, dictionary_t dictionary) {
    replacement_pair *rep_pairs;
    char *output;
    size_t output_length = strlen(fmt);
  	va_list args_copy;
    va_copy(args_copy, *args);
    rep_pairs = malloc(count * sizeof(replacement_pair));
    for(size_t i = 0; i < count; i++) {
        rep_pairs[i] = va_arg(args_copy, replacement_pair);
    }
    va_end(args_copy);

    // create a dictionary and fill it in

    for(size_t i = 0; i < count; i++) {
    	free(rep_pairs[i].key);
    	free(rep_pairs[i].value);
    }
    free(rep_pairs);
    return output;
}

char *vformatOld(char *fmt, size_t count, va_list *args) {
    replacement_pair *rep_pairs;
    char *output;
    size_t output_length = 0;
  	va_list args_copy;
    va_copy(args_copy, *args);
    rep_pairs = malloc(count * sizeof(replacement_pair));
    for(size_t i = 0; i < count; i++) {
        rep_pairs[i] = va_arg(args_copy, replacement_pair);    
    }
    va_end(args_copy);
    // calculate the length of the final string
    size_t index = 0;
    char *tag_ptr;
    while(fmt[index] != '\0') {
        int found = 0;
        for(size_t i = 0; i < count; i++) {
            if((tag_ptr = strstrTag(fmt, rep_pairs[i].key, index))) {
                // Calculate the length of the token found and add it to the output length
                output_length += tag_ptr - fmt + strlen(rep_pairs[i].value);
                // Update the index to continue the search
                index = tag_ptr - fmt + 3 + strlen(rep_pairs[i].key);
                found = 1;
                break;  // Exit the loop after finding the first match
            }
        }
        if (!found) {
            // If no replacement is found, add the remaining part of the string to the output length
            output_length += strlen(fmt + index);
            break;  // Exit the loop since no more replacements are possible
        }
    }
    
    // Allocate memory for the final output string
    output = malloc(output_length + 1);  // +1 for the null terminator

    // Build the final string
    size_t output_index = 0;
    index = 0;
    size_t braces_stack = 0;
    char *brace_start_ptr;
    char *brace_end_ptr;
    substring_t potential_key;
    while(fmt[index] != '\0') { // opening an closing braces should be counted
        if(fmt[index] == '{') {
            braces_stack ++;
            brace_start_ptr = fmt + index;
			int run_ahead_index = 0;               
                while(fmt[++run_ahead_index]) {
                    braces_stack += fmt[run_ahead_index] == '{';
                    braces_stack -= (fmt[run_ahead_index] == '}') && braces_stack;
                    if(!braces_stack) {
                        brace_end_ptr = fmt + run_ahead_index;
                        potential_key = substring(brace_start_ptr + 1, brace_end_ptr - 1);
                    }
            }
		}			
        output[output_index] = fmt[index];
    }
    for(size_t i = 0; i < count; i++) {
    	free(rep_pairs[i].key);
    	free(rep_pairs[i].value);
    }
    free(rep_pairs);
    return output;
}

char *format(char *fmt, dictionary_t replacments) {}

int main() {
    char *test = strdup("this {var} a test {var2}\n");
    dictionary_t replacments = dict({
        { "var", "is"},
        { "var2", "string" },
    });
    replacments = convertKeysToTags(replacments);
    test = replaceSubstrings(test, replacments);
    printf("%s", test);
    destroyDictionary(replacments);
    free(test);
    return 0;

	int a = 5;
	int b = 10;
	int c = 15;
	char *var = "world";
    char *e = "this";
    char *f = "is based";
    
    // printf("%s\n", strstrTag(e, f, 0));

    // printfi("Hello {var}\n", var = "World", a);
    // printfi("hello {var}, {e} {f}\n", var, e, f);
	// printfi("a is {a}, b is {b} and c is {c}\n", a = a, b = b, c = c);
    auto my_string = replaceSubstrings("Hello {var}, is a {var2}", 
    dict({
            {"var", "world"},
            {"var2", "test"}
        })
    );
    
    printf("%s", my_string);
    return 0;
}
 
