#ifndef PATTERN_MATCH_H
#define PATTERN_MATCH_H

/* Match one pathname component: * is any string and ? is one character. */
static int matches(const char *pattern, const char *name)
{
    const char *star = NULL;
    const char *retry = NULL;
    while (*name != '\0') {
        if (*pattern == '?' || *pattern == *name) {
            ++pattern;
            ++name;
        } else if (*pattern == '*') {
            star = pattern++;
            retry = name;
        } else if (star != NULL) {
            pattern = star + 1;
            name = ++retry;
        } else {
            return 0;
        }
    }
    while (*pattern == '*')
        ++pattern;
    return *pattern == '\0';
}

#endif
