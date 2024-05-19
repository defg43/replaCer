#include "format.h"

int main() {

/*	char *str = "test {var}, hihi";
	str = strdup(str);
	str = replaceSubstrings_new(str, dict({{"{var}", "abc123456789"}}));
	puts(str);
	exit(EXIT_SUCCESS);
*/
	// printh("the first test string is {} and the second string is {}\n", "foostring", "barstring");

	char *foo, *bar;
    char *aaaaa;
	printh("the foo is {foo} and bar is {bar}, the first was {foo}\n", foo = "test1", bar = "test2");
	printh("{bar}{bar}{bar}\n", bar = "|----------------|");
    printh("{aaaaa} e {aaaaa}e\n", aaaaa = "a");
    printh("{} {} {}\n", "_", "_", "_");
	return 0;
}
