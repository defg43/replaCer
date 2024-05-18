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
