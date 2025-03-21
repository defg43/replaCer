#ifndef FORMAT_H
#define FORMAT_H
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <malloc.h>
#include <stdint.h>
#include <ctype.h>
#include "map.h"
#include "../pesticide/include/debug.h"
#include "../ion/include/ion.h"
#include <stdlib.h>

typedef struct {
    char *start;
    char *end;
} substring_t;

typedef struct {
    size_t entry_count;
    char **key;
    char **value;
} dictionary_t;

typedef struct {
	bool success;
	size_t index;
} dictionary_index_search_t;

// string functions prototypes
int printh(char *fmt, dictionary_t dictionary); // better name ?
char *format(char *fmt, dictionary_t dictionary);
char *surroundWithBraces(char *text);
char *replaceSubstrings(char *inputString, dictionary_t dictionary);
substring_t substringTrimWhitespace(substring_t substr);
char *substringStrchr(substring_t substr, char c);
char *strdup(const char *s);
char *strdupSubstring(substring_t substr);
substring_t substring(char *start, char *end);
char *stringAfter(char *in, size_t index);
void printSubstring(substring_t substr);
int getIdentifierIndex(char *in, size_t index);
char *getIdentifier(char *in);
char *positionalInsert(char *buf, dictionary_t dictionary);

int vasprintf(char **str, const char *fmt, va_list args);
int asprintf (char **str, const char *fmt, ...);

// dictionary functions 
dictionary_t createDictionary(size_t count, char *data[count][2]);
void destroyDictionary(dictionary_t to_destroy);
dictionary_t convertKeysToTags(dictionary_t dictionary);
void printDictionary(dictionary_t dictionary);

#define PLEASE_GCC_AND_CLANG_STOP_FIGTHING_OVER_PRAGMAS                 \
        _Pragma("GCC diagnostic ignored \"-Wpragmas\"");                \
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

#define substring(start, end) \
    ({ \
        assert((start) != NULL && (end) != NULL); \
        substring(start, end); \
    })

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

#define dict(...) ({                                					\
    char *dictionary[][2] = __VA_ARGS__;            					\
    size_t size = lengthof(dictionary);             					\
    createDictionary(size, dictionary);             					\
})

#define dictDeepCopy(...) ({                        					\
    char *dictionary[][2] = __VA_ARGS__;            					\
    size_t size = lengthof(dictionary);             					\
    char *copied_dictionary[size][2];               					\
    for(size_t i = 0; i < size; i++) {              					\
        copied_dictionary[i][0] = dictionary[i][0]; 					\
        copied_dictionary[i][1] = dictionary[i][1]; 					\
    }                                               					\
    createDictionary(size, copied_dictionary);      					\
})

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

// this function/macro returns a pointer to malloced memory, the user is
// responsible for freeing that memory with free
#define format(fmt, ...)                                                \
    ({                                                                  \
        _Pragma("GCC diagnostic push"); 					            \
        PLEASE_GCC_AND_CLANG_STOP_FIGTHING_OVER_PRAGMAS                 \
        auto tmp = convertKeysToTags(                                   \
	        dict({ MAP(_createKeyValuePairs, __VA_ARGS__) })); 			\
        char *ret = format(fmt, tmp);									\
        _Pragma("GCC diagnostic pop");			    			        \
        destroyDictionary(tmp);											\
        ret;                                                            \
    })

#define printh(fmt, ...)   										        \
    ({                                                                  \
        _Pragma("GCC diagnostic push"); 					            \
        PLEASE_GCC_AND_CLANG_STOP_FIGTHING_OVER_PRAGMAS                 \
        auto tmp = convertKeysToTags(									\
        	dict({ __VA_OPT__(MAP(_createKeyValuePairs, __VA_ARGS__)) }));			\
        int ret = printh(fmt, tmp); 									\
        _Pragma("GCC diagnostic pop");			    			        \
        destroyDictionary(tmp);											\
        ret;                                                            \
    })

#endif // FORMAT_H
