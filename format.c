#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include "map.h"
#include <malloc.h>

#define lengthof(array) (sizeof(array) / sizeof((array)[0]))

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

    backtrace(callstack, maxFrames);

    if (dladdr(callstack[2], &info) && info.dli_sname != NULL) {
        // printf("I was called from: %s\n", info.dli_sname);
        return info.dli_sname;
    } else {
        // printf("Unable to determine calling function\n");
        return "<?>";
    }
}

typedef struct {
    char *start;
    char *end;
} substring_t;

typedef struct {
    size_t entry_count;
    char **key;
    char **value;
} dictionary_t;

// string functions prototypes
int printh(char *fmt, dictionary_t dictionary); // better name ?
char *format(char *fmt, dictionary_t dictionary);
char *surroundWithBraces(char *text);
char *replaceSubstrings(char *inputString, dictionary_t dictionary);
char *strdup(const char *s);
substring_t substring(char *start, char *end);
char *stringAfter(char *in, size_t index);
void printSubstring(substring_t substr);
int getIdentifierIndex(char *in, size_t index);
char *getIdentifier(char *in);

int vasprintf(char **str, const char *fmt, va_list args);
int asprintf (char **str, const char *fmt, ...);

// dictionary functions 
dictionary_t createDictionary(size_t count, char *data[count][2]);
void destroyDictionary(dictionary_t to_destroy);
dictionary_t convertKeysToTags(dictionary_t dictionary);
void printDictionary(dictionary_t dictionary);

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

#define dict(...) ({                                \
    char *dictionary[][2] = __VA_ARGS__;            \
    size_t size = lengthof(dictionary);             \
    createDictionary(size, dictionary);             \
})

#define dictDeepCopy(...) ({                        \
    char *dictionary[][2] = __VA_ARGS__;            \
    size_t size = lengthof(dictionary);             \
    char *copied_dictionary[size][2];               \
    for(size_t i = 0; i < size; i++) {              \
        copied_dictionary[i][0] = dictionary[i][0]; \
        copied_dictionary[i][1] = dictionary[i][1]; \
    }                                               \
    createDictionary(size, copied_dictionary);      \
})


char * strdup(const char * s) {
  size_t len = 1 + strlen(s);
  char *p = malloc(len);

  return p ? memcpy(p, s, len) : NULL;
}

char *surroundWithBraces(char *text) {
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
    char *outputString = (char *)malloc(outputLength + 1);
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
	size_t length = strlen(in);
	index = (length <= index) ? length: index;
	char *substring;
	(substring = strncpy(malloc(index + 1), in, index))[index] = '\0'; 
	return substring;
}

char *getIdentifier(char *in) {
	return stringAfter(in, getIdentifierIndex(in, 0));
}
                                                                                          
char *format(char *fmt, dictionary_t dictionary) {
    char *output;
   	output = replaceSubstrings(fmt, dictionary);
    return output;
}

int printh(char *fmt, __attribute_maybe_unused__ dictionary_t dictionary) {
	char *output = format(fmt, dictionary);
    dbg("the is %s", output);
    int output_length = strlen(output);
	fputs(output, stdout);
	free(output);
	return output_length;
}

#define PLEASE_GCC_AND_CLANG_STOP_FIGTHING_OVER_PRAGMAS                 \
        _Pragma("GCC diagnostic ignored \"-Wpragmas\"");                 \
        _Pragma("GCC diagnostic ignored \"-Wunknown-warning-option\""); \
    	_Pragma("GCC diagnostic ignored \"-Wformat=\""); 	            \
        _Pragma("GCC diagnostic ignored \"-Wformat\""); 	            \
        _Pragma("GCC diagnostic ignored \"-Wunused-variable\"");        \


#define _generic_format(ptr, x)                                         \
	_Generic((x),                                                       \
		char: asprintf(ptr, "%c", x),                                   \
	    unsigned char: asprintf(ptr, "%u", x),                          \
	    short: asprintf(ptr, "%hd", x),                                 \
	    unsigned short: asprintf(ptr, "%hu", x),                        \
	    int: asprintf(ptr, "%d", x),                                    \
	    unsigned int: asprintf(ptr, "%u", x),                           \
	    long: asprintf(ptr, "%ld", x),                                  \
	    unsigned long: asprintf(ptr, "%lu", x),                         \
	    long long: asprintf(ptr, "%lld", x),                            \
	    unsigned long long: asprintf(ptr, "%llu", x),                   \
	    float: asprintf(ptr, "%f", x),                                  \
	    double: asprintf(ptr, "%lf", x),                                \
	    long double: asprintf(ptr, "%Lf", x),                           \
	    char *: asprintf(ptr, "%s", x),                                 \
	    default: (NULL)                                                 \
	)	

#define createReplacementPair(in) 							            \
	({ 														            \
		_Pragma("GCC diagnostic push"); 					            \
        PLEASE_GCC_AND_CLANG_STOP_FIGTHING_OVER_PRAGMAS                 \
		char *temp; 										            \
		typeof(in) _in = in; 									        \
		_generic_format(&temp, _in);     					            \
		_Pragma("GCC diagnostic pop");						            \
		replacement_pair ret = (replacement_pair) { 		            \
			.key = getIdentifier(#in),						            \
			.value = temp									            \
		};													            \
		ret; }),

#define _createKey(in)                                                  \
    ({                                                                  \
        getIdentifier(#in);                                             \
    })
	

#define _createValue(in)                                                \
    ({                                                                  \
        char *temp;      										        \
	    typeof(in) _in = in;     								        \
	    _generic_format(&temp, _in);         					        \
        temp;                                                           \
    })	

#define _createKeyValuePairs(in)                                        \
    { _createKey(in), _createValue(in) },

#define format(fmt, ...)                                                \
    ({                                                                  \
        _Pragma("GCC diagnostic push"); 					            \
        PLEASE_GCC_AND_CLANG_STOP_FIGTHING_OVER_PRAGMAS                 \
        char *ret =                                                     \
            formatd(fmt,                                                \
                convertKeysToTags(                                      \
                    dict({ MAP(_createKeyValuePairs, __VA_ARGS__) }))); \
        _Pragma("GCC diagnostic pop");			    			        \
        ret;                                                            \
    })

#define printh(fmt, ...)   										        \
    ({                                                                  \
        _Pragma("GCC diagnostic push"); 					            \
        PLEASE_GCC_AND_CLANG_STOP_FIGTHING_OVER_PRAGMAS                 \
        int ret =                                                       \
            printh(fmt,                                                 \
                convertKeysToTags(                                      \
                    dict({ MAP(_createKeyValuePairs, __VA_ARGS__) }))); \
        _Pragma("GCC diagnostic pop");			    			        \
        ret;                                                            \
    })

int main() {
    int num = 10;
    char *test_string;
    printh("num is {num}\ntest_string is \"{test_string}\"\n", num = 5, test_string = "hello world");
    printh("my number is {}\n", 3);
    return 0;
}
 
