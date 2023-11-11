#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "map.h"
#include "asprintf.h"

#define lengthof(array) (sizeof(array)/sizeof((array)[0]))

typedef struct { char * key; char *value; } replacement_pair;

#define _get_descriptor(in) 							\
	({													\
		print_descriptor result = {						\
			.key = analyze(#in),						\
			.value = _Generic((auto in), ...)			\
		}												\
														\
	})



#define printn(fmt, ...) printn_count(fmt, (const char *[]){__VA_ARGS__}, \
	sizeof((const char *[]){__VA_ARGS__}) / sizeof(char *))

#define m(a, b) printf(a, #b)


int getIdentifierIndex(char *in) {
	int position = -1;
	for(int i = 0; in[i] != 0; i++) if(in[i] <= ' ' || in[i] == '=') {
		position = i;
		break;
	}
	return position;	
}

char *substring(char *in, size_t index) {
	size_t length = strlen(in);
	index = (length <= index) ? length: index;
	char *substring;
	(substring = strncpy(malloc(index + 1), in, index))[index] = '\0'; 
	return substring;
}

char *getIdentifier(char *in) {
	return substring(in, getIdentifierIndex(in));
}

#define _generic_format(ptr, x) _Generic((x), \
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

#define createReplacementPair(in) 							\
	({ 	char *temp;	__auto_type in;							\
		_generic_format(&temp, in);							\
		replacement_pair ret = (replacement_pair) { 		\
			.key = getIdentifier(#in),						\
			.value = temp									\
		};													\
		ret; }),


void printn_count(const char *fmt, const char **strings, size_t count) {
    for (size_t i = 0; i < count; i++) {
        printf("%s ", strings[i]);
    }
    printf("\n");
}

const char *custom_strstr(const char *haystack, const char *needle) {
    int needle_len = strlen(needle);

    while (*haystack != '\0') {
        if (*haystack == '{') {
            const char *end_brace = strchr(haystack, '}');
            if (end_brace != NULL) {
                // Calculate the length inside the curly braces
                int inside_len = (int)(end_brace - haystack - 1);

                // Check if the substring is inside the curly braces
                if (inside_len >= needle_len &&
                    strncmp(haystack + 1, needle, needle_len) == 0) {
                    return haystack + 1; // Return the pointer to the start of the substring
                }

                // Move the haystack to the character after the closing curly brace
                haystack = end_brace + 1;
            } else {
                // Malformed input: opening curly brace without a closing one
                break;
            }
        } else {
            ++haystack;
        }
    }

    return NULL;
}

#define format(fmt, ...) 												 \
	format_impl(fmt, 													 \
	lengthof((replacement_pair[]) 										 \
	{MAP(createReplacementPair, __VA_ARGS__)}), \
	MAP(createReplacementPair, __VA_ARGS__) NULL)
	
char *format_impl(char *fmt, size_t count, ...) {
	replacement_pair *rep_pairs;
	char *output;
	va_list args;
	va_start(args, count);
	rep_pairs = malloc(count * sizeof(replacement_pair));
	for(size_t i = 0; i < count; i++) {
		rep_pairs[i] = va_arg(args, replacement_pair);	
	}
	va_end(args);
	// parse fmt and replace from dictionary

	// calculate the length of the final string
	
	
	free(rep_pairs);
	return NULL;
}

int main() {
	int a = 5;
	//replacement_pair hm = createReplacementPair(a = 5);
	//printf("\n%s and %s", hm.key, hm.value);
	//free(hm.key);
	//free(hm.value);
	format("a", a = 10, b = 20, c = 20);	
    return 0;
}
