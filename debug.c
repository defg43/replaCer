#ifdef __GNUC__
#include <execinfo.h>
#define __USE_GNU
#include <dlfcn.h>
#include <stddef.h>
const char *getCaller(void) {
    void *callstack[4];
    const int maxFrames = sizeof(callstack) / sizeof(callstack[0]);

    Dl_info info;

    backtrace(callstack, maxFrames);

    if (dladdr(callstack[3], &info) && info.dli_sname != NULL) {
        // printf("I was called from: %s\n", info.dli_sname);
        return info.dli_sname;
    } else {
        // printf("Unable to determine calling function\n");
        return "<?>";
    }
}
#else
const char *getCaller(void) {
	return "<unimplemented>";
}
#endif // __GNUC__

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

const char *escape_sequence[256] = {
	['\a'] = "\\a",
	['\b'] = "\\b",
	['\f'] = "\\f",
	['\n'] = "\\n",
	['\r'] = "\\r",
	['\t'] = "\\t",
	['\v'] = "\\v",
	['\\'] = "\\\\",
	['\''] = "\\'",
	['\"'] = "\\\"",
	['\?'] = "\\?",
	['\0'] = "\\0",	
	[128 ... 255] = "??",	
};

size_t strlen_probe(char *str) {
	size_t usuable_memory = malloc_usable_size(str);
	printf("the usuable size is %ld\n", usuable_memory);
	size_t iter = 0;
	while(str[iter]) {
		if(str[iter] == '\n') {
			printf("strlen_probe: charcter \\n, index: %ld\n", iter);
		} else {
			printf("strlen_probe: charcter %c (%u), index: %ld\n", str[iter], (unsigned char)str[iter], iter);
		}
		iter++;
	}
	return iter;
}

void dbgmem(void *ptr) {
    if (ptr == NULL) {
        printf("pointer is NULL\n");
        return;
    }

    size_t length = malloc_usable_size(ptr);
    unsigned char *p = (unsigned char *)ptr;
	printf("%p points to a usable region of size %ld \n[%p - %p]\n", 
		ptr, length, ptr, ptr + length - 1);

    printf("---------------------------------------------------------------\n");

    for (size_t i = 0; i < length; i++) {
		if(i % 16 == 0) {
			printf("%p: ", p + i);
		}
        printf("%02X ", p[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
    
    printf("---------------------------------------------------------------\n");

    for (size_t i = 0; i < length; i++) {
		if(i % 16 == 0) {
			printf("%p: ", p + i);
		}
		if(escape_sequence[p[i]] == NULL) {
        	printf("%c  ", p[i]);
		} else {
			printf("%s ", escape_sequence[p[i]]);
		}
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

