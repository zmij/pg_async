/*
PStreams - POSIX Process I/O for C++
Copyright (C) 2002 Jonathan Wakely

This file is part of PStreams.

PStreams is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of
the License, or (at your option) any later version.

PStreams is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with PStreams; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <fcntl.h>
#include <errno.h>

#define REDI_PSTREAMS_POPEN_USES_BIDIRECTIONAL_PIPE 1

//#include "pstream_compat.h"

// TODO test rpstream - this does nothing for the moment
#define RPSTREAM 1

// test eviscerated pstreams
#define REDI_EVISCERATE_PSTREAMS 1

#include "pstream.h"

using namespace std;
using namespace redi;


string
test_type(const istream& s)
{ return "r"; }

string
test_type(const ostream& s)
{ return "w"; }

template <typename T>
string
test_id(const T& s)
{
    static int count = 0;
    ostringstream buf;
    buf << test_type(s) << ++count;
    return buf.str();
}

template <typename T>
void
print_result(const T& s, bool result)
{
    cerr << "Test" << setw(4) << test_id(s) << ": "
        << (result ? "Pass" : "Fail!")
        << endl;
}

template <typename T>
bool
check_pass(const T& s, bool expect_pass = true)
{
    bool res = s.good();
    if (!expect_pass)
        res = !res;
    print_result(s, res);
    return res;
}

template <typename T>
bool
check_fail(const T& s) { return check_pass(s, false); }


int main()
{
    ios_base::sync_with_stdio();

    string str;

    cerr << "# Testing basic I/O\n";

    {
        // test formatted output
        opstream cat("/bin/cat");
        cat << "Hello, world!\n";
        str = "Hello, world!\n";
        cat << str;
        check_pass(cat);
    }

    {
        // test formatted output
        vector<string> argv;
        argv.push_back("cat");
        argv.push_back("/etc/motd");
        argv.push_back("/etc/issue");
        opstream cat("cat", argv);
        check_pass(cat);
    }

    {
        // test unformatted output
        opstream cat("/bin/cat");
        str = "Hello, world!\n";
        for (string::const_iterator i = str.begin(); i!=str.end(); ++i)
            cat.put(*i);
        check_pass(cat);
    }

    {
        // test formatted input
        ipstream host("hostname");
        if (getline(host, str))  // extracts up to newline, eats newline
            cout << "hostname = " << str << endl;
        check_pass(host);
        // check we hit EOF at next read
        char c;
        if (host.get(c))
            cout << "read '" << c << "'" << endl;
        print_result(host, host.eof());
        check_fail(host);
    }

    {
        // test unformatted input
        ipstream host("hostname");
        str.clear();
        char c;
        while (host.get(c))  // extracts up to EOF (including newline)
            str += c;
        cout << "hostname = " << str << flush;
        print_result(host, host.eof());
    }

    {
        // open after construction, then write
        opstream cat2;
        cat2.open("/bin/cat");
        cat2 << "Hello, world!\n";
        check_pass(cat2);
    }

    {
        // open after construction, then write
        ipstream host;
        host.open("hostname");
        if (host >> str)
            cout << "hostname = " << str << endl;
        check_pass(host);
        // chomp newline and try to read past end
        char c;
        host.get(c);
        host.get(c);
        check_fail(host);
    }

#if 0
    {
        cerr << "Testing some more\n";
        opstream grep("grep 'pattern'");
        grep << "This string matches pattern.\n"
            << "This string doesn't.\n"
            << "The first line of this multiline string matches pattern.\n"
               "But the second doesn't.\n";
        check_pass(grep);
    }
#endif

    cerr << "# Testing behaviour with bad commands" << endl;

    //string badcmd = "hgfhdgf";
    string badcmd = "hgfhdgf 2>/dev/null";

    {
        // check eof() works 
        ipstream ifail(badcmd);
        int i = ifail.get();
        print_result(ifail, i==EOF && ifail.eof() );
    }

    {
        // test writing to bad command
        opstream ofail(badcmd);
        if (ofail << "blahblah")
            cout << "Wrote to " << ofail.command() << endl;
        check_fail(ofail);
        // FAIL !!!
        // no way to tell if command fails, 
    }

    {
        // test writing to bad command
        ofstream ofail(string("/root/ngngng/" + badcmd).c_str());
        if (ofail << "blahblah")
            cout << "Wrote to file" << endl;
        check_fail(ofail);
    }

    {
        // writing to bad cmd
        vector<string> argv;
        argv.push_back("cat");
        argv.push_back("/etc/motd");
        argv.push_back("/etc/issue");
        ipstream ifail("hdhdhd", argv);
        if (ifail >> str)
            cout << "Read from bad process" << endl;
        check_fail(ifail);
    }
    
    cerr << "# Testing behaviour with uninit'ed streams" << endl;

    {
        // check eof() works 
        ipstream ifail;
        int i = ifail.get();
        print_result(ifail, i==EOF && ifail.eof() );
    }

    {
        // test writing to no command
        opstream ofail;
        if (ofail << "blahblah")
            cout << "Wrote to " << ofail.command() << endl;
        check_fail(ofail);
    }


    cerr << "# Testing other member functions\n";

    {
        string cmd("grep re");
        opstream s(cmd);
        print_result(s, cmd == s.command());
    }

    {
        string cmd("grep re");
        opstream s;
        s.open(cmd);
        print_result(s, cmd == s.command());
    }

    {
        string cmd("/bin/ls");
        ipstream s(cmd);
        print_result(s, cmd == s.command());
    }

    {
        string cmd("/bin/ls");
        ipstream s;
        s.open(cmd);
        print_result(s, cmd == s.command());
    }

    // TODO more testing of other members?

    cerr << "# Testing writing to closed stream\n";

    {
        opstream os("sed 's/foo/FOO/'");
        os << "foo\n";
        os.close();
        if (os << "bar\n")
            cout << "Wrote to closed stream" << endl;
        print_result(os, !(os << "bar\n"));
    }

#if REDI_EVISCERATE_PSTREAMS
    cerr << "Testing eviscerated pstream\n";

    {
        opstream os("cat");
        FILE *in, *out, *err;
        size_t res = os.fopen(in, out, err);
        print_result(os, res & pstreambuf::pstdin);
        print_result(os, in!=NULL);
    }
#endif
    
#if 0

#ifdef _STREAM_COMPAT

    // check fstreambase doesn't try to close() an attached fd
    FILE* f = fopen("/tmp/poo", "a");
    fprintf(f, "text\n");
    fflush(f);
    {
        fstream fs;
        fs.attach(fileno(f));
        fs << "more text" << endl;
    }
    fprintf(f, "even more text\n");
    fflush(f);
    cout << "retval of fclose(): " << fclose(f) << endl;

    // same check again, using only filedescs (no FILE*s)
    int fd = open("/tmp/oops", O_CREAT|O_WRONLY, 0777);
    cerr << errno << ' ' << strerror(errno) << endl;
    errno=0;
    cerr << write(fd, "TEXT\n", 5) << endl;
    cerr << errno << ' ' << strerror(errno) << endl;
    errno=0;
    fsync(fd);
    {
        ofstream fs(fd);
        fs << "MORE TEXT" << endl;
        cerr << strerror(errno) << endl;
        errno=0;
    }
    cerr << write(fd, "EVEN MORE TEXT\n", 15) << endl;
    cerr << strerror(errno) << endl;
    errno=0;
    fsync(fd);
    close(fd);
#endif  // _STREAM_COMPAT

#endif

    return 0;
}


