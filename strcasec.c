#include <string.h>
#include <ctype.h>

int strcasecmp(const char *p1, const char *p2)
{
    while (*p1 && tolower(*p1) == tolower(*p2)) {
        p1++;
        p2++;
    }
    return tolower(*(unsigned char *) p1) - tolower(*(unsigned char *) p2);
}
int strncasecmp(const char *p1, const char *p2, size_t n)
{
    if (!n) return 0;

    while (n-- != 0 && tolower(*p1) == tolower(*p2)) {
        if (!n || *p1 == 0 || *p2 == 0)
            break;
        p1++;
        p2++;
    }
    return tolower(*(unsigned char *) p1) - tolower(*(unsigned char *) p2);
}
