#include "pstream.h"

int main()
{
    using namespace redi;

    char c;
    ipstream who("whoami");
    if (!(who >> c))
        return 1;

    redi::opstream cat("cat");
    if (!(cat << c))
        return 2;

    while (who >> c)
        cat << c;

    cat << '\n';

    pstream fail("ghghghg", pstreambuf::pstderr);
    
    return 0;
}

