#include "pstream.h"
#include "rpstream.h"

#include <iostream>


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
    if (!(std::cerr << fail.rdbuf()))
        return 3;
    
    rpstream who2("whoami");
    if (!(who2.out() >> c))
        return 4;

    return 0;
}

