Summary: A midi controlled audio sampler
Name: petri-foo
Version: 0.0.1
Release: 1
License: GPL
Group: Applications/Multimedia
URL: http://www.gazuga.net
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Requires: libsamplerate libsndfile gtk2 libxml2 alsa-lib jack-audio-connection-kit
BuildPrereq: libsamplerate-devel libsndfile-devel gtk2-devel libxml2-devel alsa-lib-devel jack-audio-connection-kit-devel

%description
Petri-Foo is a midi controlled audio sampler for GNU/Linux systems. It
allows you to create music using various sound files, or "samples", in
tandem with a midi sequencer. It is a fork of the Specimen project.

%prep
%setup -q

%build
%configure
make

%makeinstall

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/petri-foo
%dir %{_datadir}/petri-foo
%{_datadir}/petri-foo/pixmaps/play.png
%{_datadir}/petri-foo/pixmaps/stop.png
%{_datadir}/petri-foo/pixmaps/open.png
%{_datadir}/petri-foo/pixmaps/panic.png
%doc AUTHORS ChangeLog COPYING NEWS README TODO


%changelog
* 1st February 2011 James Morris <james@jwm-art.net>
- forked from Specimen project

* Sat Jun 12 2004 Pete Bessman <ninjadroid@gazuga.net>
- added jack and alsa-lib dependencies

* Thu Mar 11 2004 <ninjadroid@gazuga.net>
- rebuilt for v0.2.4

* Mon Feb 23 2004  <ninjadroid@gazuga.net> 
- modified and rebuilt for v0.2.2-1

* Thu Feb 19 2004 Florin Andrei <florin@andrei.myip.org>
- initial package, v0.2.0-1

