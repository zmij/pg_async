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


// TODO test rpstream more


// test eviscerated pstreams
#define REDI_EVISCERATE_PSTREAMS 1

#include "pstream.h"

// include these after pstream.h to ensure it #includes everything it needs
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

using namespace std;
using namespace redi;

// explicit instantiations of template classes
template class redi::basic_pstreambuf<char>;
template class redi::pstream_common<char>;
template class redi::basic_ipstream<char>;
template class redi::basic_opstream<char>;
template class redi::basic_pstream<char>;
template class redi::basic_rpstream<char>;

namespace  // anon
{
    // helper functions for printing test results

    char
    test_type(istream const&)
    { return 'r'; }

    char
    test_type(ostream const&)
    { return 'w'; }

    char
    test_type(iostream const&)
    { return 'b'; }

    char
    test_type(rpstream const&)
    { return 'x'; }

    template <typename T>
    string
    test_id(T const& s)
    {
        static int count = 0;
        ostringstream buf;
        buf << test_type(s) << ++count;
        return buf.str();
    }

    template <typename T>
    void
    print_result(T const& s, bool result)
    {
        clog << "Test " << setw(4) << test_id(s) << ": "
            << (result ? "Pass" : "Fail!")
            << endl;
    }

    template <typename T>
    bool
    check_pass(T const& s, bool expected = true)
    {
        bool res = s.good() == expected;
        print_result(s, res);
        return res;
    }

    template <typename T>
    bool
    check_fail(T const& s) { return check_pass(s, false); }

    // exit status of shell when command not found
#if defined(__sun)
    int sh_cmd_not_found = 1;
#else
    int sh_cmd_not_found = 127;
#endif
}


int main()
{
    ios_base::sync_with_stdio();

    string str;

    clog << "# Testing basic I/O\n";

    {
        // test formatted output
        //
        // This should read the strings on stdin and print them on stdout
        // prefixed by "STDOUT: "

        opstream os("sed 's/^/STDIN: /' /dev/stdin /etc/resolv.conf");
        os << ".fnord.\n";
        str = "..fnord..\n";
        os << str;
        check_pass(os);
    }

    {
        // test execve() style construction
        //
        // This should read the strings on stdin and print them on stdout
        // prefixed by "STDIN: "

        vector<string> argv;
        argv.push_back("sed");
        argv.push_back("s/^/STDIN: /");
        opstream os("sed", argv);

        check_pass(os << "Magic Monkey\n");
    }

    {
        // test unformatted output
        //
        // This should read the strings on stdin and print them on stdout
        // prefixed by "STDIN: "

        opstream sed("sed 's/^/STDIN: /'");
        str = "Monkey Magic\n";
        for (string::const_iterator i = str.begin(); i!=str.end(); ++i)
            sed.put(*i);
        check_pass(sed);
    }

    {
        // test formatted input
        // should print hostname on stdout, prefixed by "STDOUT: "
        ipstream host("hostname");
        if (getline(host, str))  // extracts up to newline, eats newline
            cout << "STDOUT: " << str << endl;
        check_pass(host);
        // check we hit EOF at next read
        char c;
        print_result(host, !host.get(c));
        print_result(host, host.eof());
        check_fail(host);
    }

    {
        // test unformatted input
        // should print hostname on stdout, prefixed by "STDOUT: "
        ipstream host("date");
        str.clear();
        char c;
        while (host.get(c))  // extracts up to EOF (including newline)
            str += c;
        cout << "STDOUT:  " << str << flush;
        print_result(host, host.eof());
    }

    {
        // open after construction, then write
        opstream os;
        os.open("sed 's/^/STDIN: /'");
        os << "Hello, world!\n";
        check_pass(os);
    }

    {
        // open after construction, then read
        ipstream is;
        is.open("hostname");
        string s;
        is >> s;
        cout << "STDOUT: " << s << endl;
        check_pass(is);
    }

    {
        // open after construction, then write
        ipstream host;
        host.open("hostname");
        if (host >> str)
            cout << "STDOUT: " << str << endl;
        check_pass(host);
        // chomp newline and try to read past end
        char c;
        host.get(c);
        host.get(c);
        check_fail(host);
    }

    clog << "# Testing bidirectional PStreams\n";

    pstreambuf::pmode all3streams =
        pstreambuf::pstdin|pstreambuf::pstdout|pstreambuf::pstderr;

    {
        // test output on bidirectional pstream
        string cmd = "grep ^127 -- /etc/hosts /no/such/file";
        pstream ps(cmd, all3streams);

        print_result(ps, ps.is_open());
        check_pass(ps.out());
        check_pass(ps.err());

        string buf;
        while (getline(ps.out(), buf))
            cout << "STDOUT: " << buf << endl;
        check_fail(ps);
        while (getline(ps.err(), buf))
            cout << "STDERR: " << buf << endl;
        check_fail(ps);
    }

    {
        // test input on bidirectional pstream and test peof manip
        string cmd = "grep fnord -- /dev/stdin";
        pstream ps(cmd, all3streams);

        print_result(ps, ps.is_open());
        check_pass(ps);

        ps << "12345\nfnord\n0000" << peof;
        // manip calls ps.rdbuf()->peof();

        string buf;
        getline(ps.out(), buf);

        do
        {
            print_result(ps, buf == "fnord");
            cout << "STDOUT: " << buf << endl;
        } while (getline(ps.out(), buf));

        check_fail(ps << "pipe closed, no fnord now");
    }
    // TODO - test child moves onto next file after peof on stdin

    {
        // test signals
        string cmd = "grep 127 -- -";
        pstream ps(cmd, all3streams);

        pstreambuf* pbuf = ps.rdbuf();

        int e1 = pbuf->error();
        print_result(ps, e1 == 0);
        pbuf->kill(SIGTERM);
        int e2 = pbuf->error();
        print_result(ps, e1 == e2);

        pbuf->close();
        int e3 = pbuf->error();
        check_fail(ps << "127 fail 127\n");
        print_result(ps, e1 == e3);
    }


    clog << "# Testing behaviour with bad commands" << endl;

    //string badcmd = "hgfhdgf";
    string badcmd = "hgfhdgf 2>/dev/null";

    {
        // check eof() works 
        ipstream is(badcmd);
        print_result(is, is.get()==EOF);
        print_result(is, is.eof() );
    }

    {
        // test writing to bad command
        opstream ofail(badcmd);
        sleep(1);  // give shell time to try command and exit
        // this would cause SIGPIPE: ofail<<"blahblah";
        // does not show failure: print_result(ofail, !ofail.is_open());
        pstreambuf* buf = ofail.rdbuf();
        print_result(ofail, buf->exited());
        int status = buf->status();
        print_result( ofail,
                WIFEXITED(status) && WEXITSTATUS(status) == sh_cmd_not_found );
    }

    {
        // reading from bad cmd
        vector<string> argv;
        argv.push_back("hdhdhd");
        argv.push_back("arg1");
        argv.push_back("arg2");
        ipstream ifail("hdhdhd", argv);
        check_fail(ifail>>str);
    }
    
    clog << "# Testing behaviour with uninit'ed streams" << endl;

    {
        // check eof() works 
        ipstream is;
        print_result(is, is.get()==EOF);
        print_result(is, is.eof() );
    }

    {
        // test writing to no command
        opstream ofail;
        check_fail(ofail<<"blahblah");
    }


    clog << "# Testing other member functions\n";

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

    // TODO more testing of other members

    clog << "# Testing writing to closed stream\n";

    {
        opstream os("tr '[:lower:]' '[:upper:]' | sed 's/^/STDIN: /'");
        os << "foo\n";
        os.close();
        if (os << "bar\n")
            cout << "Wrote to closed stream" << endl;
        check_fail(os << "bar\n");
    }

    clog << "# Testing EOF detected correctly\n";
    {
        pstream p("tr '[:lower:]' '[:upper:]'");
        p << "newline\neof" << peof;
        string s;
        check_pass(std::getline(p.out(),s));
        print_result(p, s.size()>0);
        cout << "STDOUT: " << s << endl;
        s.clear();
        std::getline(p.out(),s);           // sets eofbit
        print_result(p, p.eof());
        print_result(p, s.size()>0);
        cout << "STDOUT: " << s << endl;
    }

    clog << "# Testing restricted pstream\n";
    {
        rpstream rs("tr '[:lower:]' '[:upper:]'");
        rs << "foo\n" << peof;
        string s;
        check_pass(std::getline(rs.out(),s));
        print_result(rs, s.size()>0);
        cout << "STDOUT: " << s << endl;
    }

#if REDI_EVISCERATE_PSTREAMS
    clog << "# Testing eviscerated pstream\n";

    {
        opstream os("tr '[:lower:]' '[:upper:]' | sed 's/^/STDIN: /'");
        FILE *in, *out, *err;
        size_t res = os.fopen(in, out, err);
        print_result(os, res & pstreambuf::pstdin);
        print_result(os, in!=NULL);
        int i = fputs("flax\n", in);
        fflush(in);
        print_result(os, i>=0 && i!=EOF);
    }

    {
        string cmd = "ls /etc/motd /no/such/file";
        ipstream is(cmd, pstreambuf::pstdout|pstreambuf::pstderr);
        FILE *in, *out, *err;
        size_t res = is.fopen(in, out, err);
        print_result(is, res & pstreambuf::pstdout);
        print_result(is, res & pstreambuf::pstderr);
        print_result(is, out!=NULL);
        print_result(is, err!=NULL);

        const size_t len = 256;
        char buf[len];
        char* p = fgets(buf, len, out);
        cout << "STDOUT: " << buf;
        print_result(is, p!=NULL);

        p = fgets(buf, len, err);
        cout << "STDERR: " << buf;
        print_result(is, p!=NULL);
    }

    {
        string cmd = "grep 127 -- - /etc/hosts /no/such/file";
        pstream ps(cmd, all3streams);
        FILE *in, *out, *err;
        size_t res = ps.fopen(in, out, err);
        print_result(ps, res & pstreambuf::pstdin);
        print_result(ps, res & pstreambuf::pstdout);
        print_result(ps, res & pstreambuf::pstderr);
        print_result(ps, in!=NULL);
        print_result(ps, out!=NULL);
        print_result(ps, err!=NULL);

        // ps << "12345\n1112777\n0000" << EOF;
#if 0
        size_t len = 256;
        char buf[len];
        char* p = fgets(buf, len, out);
        cout << "STDOUT: " << buf;
        print_result(ps, p!=NULL);

        p = fgets(buf, len, err);
        cout << "STDERR: " << buf;
        print_result(ps, p!=NULL);
#endif
    }

#endif
    

    return 0;
}

