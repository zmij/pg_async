#include "pstream.h"

int main()
{
    char c;
    redi::ipstream who("whoami");
    if (!(who >> c))
        return 1;

    redi::opstream cat("cat");
    if (!(cat << c))
        return 1;

    while (who >> c)
        cat << c;

    cat << '\n';

    return 0;
}

