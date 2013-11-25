%define _confdir /usr/share/giveme3
Summary: some shit 
Name: giveme 
Version: 3.0.6
Release: 1%{?dist}
Epoch: 1
License: GPL2
Group: Amusements/Games
Source0: giveme-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: gcc
Requires: lame >= 3.97
Requires: ffmpeg
Requires: flac >= 1.1.4
Requires: vorbis-tools >= 1.1
Requires: faad2 >= 2.5
Requires: faac >= 1.25
Requires: mac
Requires: wavpack

%description
some shit

%prep
%setup -q 

%build
./configure
make

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_bindir} %{buildroot}%{_confdir}
mkdir -p %{buildroot}%{_confdir}
install -p giveme %{buildroot}%{_bindir}
install -p wavDec.sh %{buildroot}%{_bindir}
install -p giveme3.conf %{buildroot}%{_confdir}


%clean
rm -rf %{buildroot}

%post
echo -e "\nInstalling conf files for the following users: "
for i in `ls /home | grep -v lost.found`; do
	grep "$i" /etc/passwd 1> /dev/null 2> /dev/null
	if [[ $? -eq 0 ]]; then
		echo "-- $i"
		install -o "$i" -g "users" -m 644 %{_confdir}/giveme3.conf /home/${i}/.giveme3.conf
	fi
done	

%files
%defattr(-,root,root,-)
%{_bindir}/giveme
%{_bindir}/wavDec.sh
%{_confdir}/giveme3.conf
