%define packagename pstreams

Name:           pstreams-devel
Version:        0.6.0
Release:        8%{?dist}
Summary:        POSIX Process Control in C++

Group:          Development/Libraries
License:        LGPLv3+
URL:            http://%{packagename}.sourceforge.net/
Source0:        http://downloads.sourceforge.net/%{packagename}/%{packagename}-%{version}.tar.gz
# Submitted patch upstream - Till Mass
# http://sourceforge.net/tracker2/?func=detail&atid=453894&aid=2234202&group_id=48695
Patch:          pstreams-0.6.0-destdir_timestamp.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  doxygen
BuildArch:      noarch

%description
PStreams class is like a C++ wrapper for the POSIX.2 functions
popen(3) and pclose(3), using C++ iostreams instead of C's stdio
library.

%prep
%setup -q -n %{packagename}-%{version}
%patch -p1 -b .destdir_timestamp

%build
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install  INSTALL_PREFIX=$RPM_BUILD_ROOT/usr

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc doc/html COPYING.LIB README AUTHORS ChangeLog
%{_includedir}/pstreams

%changelog
* Sun Jul 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.6.0-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Thu Feb 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.6.0-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Fri Nov 07 2008 Rakesh Pandit <rakesh@fedoraproject.org> 0.6.0-6
- timestamp patch (Till Mass)

* Fri Nov 07 2008 Rakesh Pandit <rakesh@fedoraproject.org> 0.6.0-5
- saving timestamp using "install -p"

* Fri Nov 07 2008 Rakesh Pandit <rakesh@fedoraproject.org> 0.6.0-4
- included docs, license and other missing files.

* Fri Nov 07 2008 Rakesh Pandit <rakesh@fedoraproject.org> 0.6.0-3
- consistent use of macros - replaced %%{buildroot} with $RPM_BUILD_ROOT

* Thu Nov 06 2008 Rakesh Pandit <rakesh@fedoraproject.org> 0.6.0-2
- Cleaned up buildrequire

* Tue Nov 04 2008 Rakesh Pandit <rakesh@fedoraproject.org> 0.6.0-1
- initial package
