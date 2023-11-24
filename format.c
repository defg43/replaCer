#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "map.h"

// todo: reorganize asprintf and make va_list by ref

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


#define lengthof(array) (sizeof(array)/sizeof((array)[0]))

int printfi(char *fmt, size_t count, ...);
char *format(char *fmt, size_t count, ...);
char *vformat(char *fmt, size_t count, va_list *args);

typedef struct { char * key; char *value; } replacement_pair;

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

#define createReplacementPair(in) 							\
	({ 														\
		_Pragma("GCC diagnostic push"); 					\
		_Pragma("GCC diagnostic ignored \"-Wformat=\""); 	\
		_Pragma("GCC diagnostic ignored \"-Wunused-variable\""); \
		char *temp; 										\
		typeof(in) _in = in; 									\
		_generic_format(&temp, _in);     					\
		_Pragma("GCC diagnostic pop");						\
		replacement_pair ret = (replacement_pair) { 		\
			.key = getIdentifier(#in),						\
			.value = temp									\
		};													\
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
    printf("the needle is: %s\n", needle);
    printf("the needle length is %d\n", needle_len);
    haystack += offset + offset == 0;
	char *found_ptr;
	while(*haystack) {
		found_ptr = strstr(haystack, needle);
		printf("the found ptr is: %p\n", found_ptr);
		printf("the string is: %.6s\n", found_ptr);
		if(found_ptr != NULL) {
			if(offset != 0) {
				printf("character at start: %c\n", found_ptr[0]);
				printf("end char: %c -> %d\n", found_ptr[needle_len], found_ptr[needle_len]);
				if(found_ptr[-1] == '{' && found_ptr[needle_len] == '}') {
					printf("%s", found_ptr - 1);
					return found_ptr - 1;
				} else {
					haystack = found_ptr + needle_len;
					continue;
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
	
char *format(char *fmt, size_t count, ...) {
    char *output;
    va_list args;
    va_start(args, count);
   	output = vformat(fmt, count, &args);
    va_end(args);
    return output;
}

#define format(fmt, ...) 												 \
	format(fmt, 														 \
	lengthof((replacement_pair[]) 										 \
	{MAP(createReplacementPair, __VA_ARGS__)}), 						 \
	MAP(createReplacementPair, __VA_ARGS__) 0)

int printfi(char *fmt, size_t count, ...) {
	va_list args;
	va_start(args, count);
	char *output = vformat(fmt, count, &args);
	int output_length = strlen(output);
	fputs(output, stdout);
	va_end(args);
	free(output);
	return output_length;
}

#define printfi(fmt, ...) 												 \
		printfi(fmt, 														 \
		lengthof((replacement_pair[]) 										 \
		{MAP(createReplacementPair, __VA_ARGS__)}), 						 \
		MAP(createReplacementPair, __VA_ARGS__) 0) // avoiding trailing comma

char *vformat(char *fmt, size_t count, va_list *args) {
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
                index = tag_ptr - fmt + 2 + strlen(rep_pairs[i].key);
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
    while(fmt[index] != '\0') {
        int found = 0;
        for(size_t i = 0; i < count; i++) {
            if((tag_ptr = strstrTag(fmt, rep_pairs[i].key, index))) {
                // Copy the part of the original string before the token
                strncpy(output + output_index, fmt + index, tag_ptr - fmt - index);
                output_index += tag_ptr - fmt - index;

                // Copy the replacement value
                strcpy(output + output_index, rep_pairs[i].value);
                output_index += strlen(rep_pairs[i].value);

                // Update the index to continue the search
                index = tag_ptr - fmt + 2 + strlen(rep_pairs[i].key);
                found = 1;
                break;  // Exit the loop after finding the first match
            }
        }
        if (!found) {
            // If no replacement is found, copy the remaining part of the string
            strcpy(output + output_index, fmt + index);
            break;  // Exit the loop since no more replacements are possible
        }
    }
    for(size_t i = 0; i < count; i++) {
    	free(rep_pairs[i].key);
    	free(rep_pairs[i].value);
    }
    free(rep_pairs);
    return output;
}


int main() {
	int a = 5;
	int b = 10;
	int c = 15;
	char *var;
	printfi("Hello {var}\n", var = "World");
	printfi("a is {a}, b is {b} and c is {c}\n", a = a, b = b, c = c);
	
    return 0;
}
