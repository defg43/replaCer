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

#define printn(fmt, ...) printn_count(fmt, (const char *[]){__VA_ARGS__}, \
	sizeof((const char *[]){__VA_ARGS__}) / sizeof(char *))

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
	({ 	char *temp;	typeof(in) _in; _in = in;				\
		_generic_format(&temp, _in);						\
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

#define format(fmt, ...) 												 \
	format_impl2(fmt, 													 \
	lengthof((replacement_pair[]) 										 \
	{MAP(createReplacementPair, __VA_ARGS__)}), \
	MAP(createReplacementPair, __VA_ARGS__) NULL)

char *strstrTag(char *haystack, char *needle, size_t offset) {
    int needle_len = strlen(needle);
	haystack += offset;
    while (*haystack != '\0') {
        if (*haystack == '{') {
            char *end_brace = strchr(haystack, '}');
            if (end_brace != NULL) {
                int inside_len = (int)(end_brace - haystack - 1);

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
	
char *format_impl2(char *fmt, size_t count, ...) {
    replacement_pair *rep_pairs;
    char *output;
    size_t output_length = 0;
  
    va_list args;
    va_start(args, count);
    rep_pairs = malloc(count * sizeof(replacement_pair));
    for(size_t i = 0; i < count; i++) {
        rep_pairs[i] = va_arg(args, replacement_pair);    
    }
    va_end(args);
    // calculate the length of the final string
    size_t index = 0;
    char *tag_ptr;
    while(fmt[index] != '\0') {
        int found = 0;
        for(size_t i = 0; i < count; i++) {
            if(tag_ptr = strstrTag(fmt, rep_pairs[i].key, index)) {
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
            if(tag_ptr = strstrTag(fmt, rep_pairs[i].key, index)) {
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
    for(int i = 0; i < count; i++) {
    	free(rep_pairs[i].key);
    	free(rep_pairs[i].value);
    }
    free(rep_pairs);
    return output;
}

char *format_impl(char *fmt, size_t count, ...) {
    replacement_pair *rep_pairs;
    rep_pairs = malloc(count * sizeof(replacement_pair));
    char *output;
    size_t output_length = 0;

    va_list args;
    va_start(args, count);
    for (size_t i = 0; i < count; i++) {
        rep_pairs[i] = va_arg(args, replacement_pair);
    }
    va_end(args);

    size_t index = 0;
    while (fmt[index] != '\0') {
        size_t token_len = strcspn(fmt + index, "{");
        output_length += token_len;
        index += token_len;

        int found = 0;
        for (size_t i = 0; i < count; i++) {
            char *tag_ptr = strstrTag(fmt, rep_pairs[i].key, index);
            if (tag_ptr) {
                size_t key_len = strlen(rep_pairs[i].key);
                output_length += strlen(rep_pairs[i].value);
                index = tag_ptr - fmt + 2 + key_len;
                found = 1;
                break;
            }
        }

        if (!found) {
            break;
        }
    }

    output = malloc(output_length + 1);

    size_t output_index = 0;
    index = 0;
    while (fmt[index] != '\0') {
        size_t token_len = strcspn(fmt + index, "{");
        strncpy(output + output_index, fmt + index, token_len);
        output_index += token_len;
        index += token_len;

        int found = 0;
        for (size_t i = 0; i < count; i++) {
            char *tag_ptr = strstrTag(fmt, rep_pairs[i].key, index);
            if (tag_ptr) {
                size_t key_len = strlen(rep_pairs[i].key);
                strncpy(output + output_index, rep_pairs[i].value, strlen(rep_pairs[i].value));
                output_index += strlen(rep_pairs[i].value);
                index = tag_ptr - fmt + 2 + key_len;
                found = 1;
                break;
            }
        }

        if (!found) {
            break;
        }
    }

    for (size_t i = 0; i < count; i++) {
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
	char *my_formated_string = 
		format("a is {a}, b is {b} and c is {c}\n", a, b, c);

	puts(my_formated_string);

	free(my_formated_string);	
    return 0;
}
