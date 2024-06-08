#ifndef DEBUG_H
#define DEBUG_H

#include <stdlib.h>
#include <malloc.h>
#include "map.h"

#ifndef lengthof
#define lengthof(array) (sizeof(array) / sizeof((array)[0]))
#endif

#ifdef DEBUG

void dbgmem(void *ptr);
size_t strlen_probe(char *str);

#define free(x) \
	({ \
		dbg("freeing variable %s with the value %p", #x, x); \
		free(x); \
	})
#define dbg(fmt, ...) \
    ({  \
        printf("[\e[93mdebug\e[0m: %s @ %d in %s from %s] " fmt "\n", \
        __FUNCTION__, __LINE__, __FILE__, getCaller() __VA_OPT__(, __VA_ARGS__)); \
    })
#else
#define dbg(fmt, ...)
#define dbgmem(ptr) 
#endif

#ifdef DEBUG
#define _compareWithAnyofArray(arg, arr) ({											\
	bool matches = false;															\
	for(size_t i = 0; i < lengthof(arr); i++) {										\
		matches |= arg == arr[i];													\
	}																				\
	matches;																		\
})

#define _stringify_comma(x) _stringify_comma_defer(x)
#define _stringify_comma_defer(x) #x,

#define _maxFromArray(arr) ({														\
	size_t largest = 0;																\
	bool found = false;																\
	for(size_t iter = 0; iter < lengthof(arr); iter++) {							\
		if(largest < arr[iter]) {													\
			largest = arr[iter];													\
			found = true;															\
		}																			\
	}																				\
	largest;		 																\
})

#define _dbgstr_convert_ptr_to_diff(x) 												\
		_Generic((x),																\
			char *: (str - x),														\
			default: (x)															\
		),

#define dbgstr(strin, ...) ({														\
		char *str = strin; 															\
		size_t idx[] = { MAP(_dbgstr_convert_ptr_to_diff, __VA_ARGS__) };			\
		const char *varname[] = { MAP(_stringify_comma, __VA_ARGS__) };				\
        size_t len = malloc_usable_size(str); 										\
        size_t usable_size = malloc_usable_size(str);								\
        size_t index = 0;															\
        size_t arrow_index = 0;														\
        size_t arrow_pos[lengthof(idx)];											\
        bool idx_printed[lengthof(idx)] = {};										\
        size_t arrow_count = 0;														\
		size_t temp = 0;															\
																					\
		for(size_t entry = 0; entry < lengthof(idx); entry++) {						\
			size_t compensation = 0;												\
			if(idx[entry] > len) {													\
				arrow_pos[entry] = len + 2; 										\
			} else {																\
																					\
				for(size_t iterator = 0; iterator < len; iterator++) {				\
					if(iterator	== idx[entry]) {									\
						break;														\
					}																\
					if(str[iterator] == '\t' || str[iterator] == '\b' || 			\
					str[iterator] == '\r' || str[iterator] == '\e' || 				\
					str[iterator] == '\\' || str[iterator] == '\0'||				\
					str[iterator] == '\n') {										\
						compensation++;												\
					}																\
				}																	\
				arrow_pos[entry] = idx[entry] + compensation;						\
				arrow_count++;														\
				compensation = 0;													\
			}																		\
		}																			\
       																				\
       	while(index < usable_size) {									\
        	if(_compareWithAnyofArray(index, idx)) {								\
        		if(str[index] == ' ') {												\
        			printf("\e[43m");												\
        		} else {															\
        			printf("\e[33m");												\
        		}																	\
        	}																		\
        	if(str[index] == '\n') { printf("\\n"); }								\
        	else if(str[index] == '\t') { printf("\\t"); }							\
			else if(str[index] == '\b') { printf("\\b"); }							\
			else if(str[index] == '\r') { printf("\\r"); }							\
			else if(str[index] == '\e') { printf("\\e"); }							\
			else if(str[index] == '\\') { printf("\\"); }							\
			else if(str[index] == '\0') { printf("\\0"); }							\
			else if(str[index] >= 128) { printf("??"); } 							\
			else { printf("%c", str[index]); }										\
			if(_compareWithAnyofArray(index, idx)) {								\
				printf("\e[0m");													\
			}																		\
			index++;																\
        }																			\
        printf("\n");																\
        for(int entry = 0; entry < arrow_count; entry++) {							\
       		for(size_t i = 0; i <= _maxFromArray(arrow_pos); i++) {					\
       			bool print_seperator = false;										\
	      		for(size_t check_entry = 0; check_entry < arrow_count;				\
	      		check_entry++) {													\
	      			print_seperator |= i == arrow_pos[check_entry]					\
	      				&& idx_printed[check_entry] == false;						\
	      		}																	\
      			if(print_seperator) {												\
      				printf("|");													\
      			} else {															\
      				printf(" ");													\
      			}																	\
        	}																		\
        	printf("\n");															\
																					\
       		for(size_t i = 0; i <= _maxFromArray(arrow_pos); i++) {					\
	      		if(i == arrow_pos[entry]) {											\
       				i += printf("%s [%ld/%ld]", varname[entry], idx[entry], len);	\
       				idx_printed[entry] = true;										\
	      		}																	\
       			bool print_seperator = false;										\
	      		for(size_t check_entry = 0; check_entry < arrow_count;				\
	      		check_entry++) {													\
	      			print_seperator |= i == arrow_pos[check_entry]					\
	      				&& idx_printed[check_entry] == false;						\
	      		}																	\
      			if(print_seperator) {												\
      				printf("|");													\
      			} else {															\
      				printf(" ");													\
      			}																	\
        	}																		\
        	printf("\n");															\
        }																			\
        																			\
})

#else
#define dbgstr(strin, ...)
#endif

const char *getCaller(void);

#endif // DEBUG_H
