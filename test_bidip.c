#include <stdio.h>
#include <errno.h>
#include <string.h>

int main()
{
    FILE* fp = 0;
    const char* modes[] = {"r+", "rw"};
    int i;

    for (i=0; i < sizeof(modes)/sizeof(*modes); ++i)
    {
        errno = 0;
        popen("grep pattern", modes[i]);
        if (errno!=EINVAL) {
            printf(modes[i]);
            return 0;
        }
    }
    return errno;
}

