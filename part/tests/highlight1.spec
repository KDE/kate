# norootforbuild  To this "special" comment, _nothing_ should follow!
  # comments in tag content are now allowd:
Name:                kradioripper-unstable  # will be "kradioripper" for stable and "kradioripper-unstable" for unstable release...  
  
Version:             0.3.9 # This line will be corrected with the actual version number...  
  
%if 0%{?mandriva_version}  
Release:             %mkrel 1.2
%else  
Release:             0  
%endif  
  
License:             GPLv2  
  
%if 0%{?mandriva_version}  
Group:               Sound  
%else  
Group:               Productivity/Multimedia/Sound/Utilities  
%endif  
  
Summary:             Recorder for internet radios (based on Streamripper)  
  
Summary(de.UTF-8):   Aufnahmeprogramm für Internetradios (basiert auf Streamripper)  
  
URL:                 http://kradioripper.sourceforge.net/  
  
Source0:             %{name}-%{version}.tar.bz2  
  
BuildRoot:           %{_tmppath}/%{name}-%{version}-build  
  
%if 0%{?suse_version}  
BuildRequires:       update-desktop-files  
BuildRequires:       libkde4-devel  
%endif  
%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}  
BuildRequires:       desktop-file-utils kdelibs-devel gcc-c++  
%endif  
%if 0%{?mandriva_version}  
# maybeBuildRequires:  kdesdk4-core  
# maybeBuildRequires:  kdesdk4-scripts  
BuildRequires:  kdelibs4-devel  
# The following lines are because the build service needs to know how to satisfy  
# esound needed by lib64esound0: esound-daemon or pulseaudio-esound-compat...  
%if 0%{?opensuse_bs}  
BuildRequires:  pulseaudio-esound-compat  
BuildRequires:  kde4-l10n-en_GB  
BuildRequires:  free-kde4-config  
BuildRequires:  phonon-gstreamer  
BuildRequires:  mandriva-theme-Free  
BuildRequires:  mandriva-theme-Free-screensaver  
BuildRequires:  wget  
BuildRequires:  kernel-desktop-2.6.27-0.rc8.2mnb  
%endif  
%endif  
BuildRequires:       cmake >= 2.4  
  
%if 0%{?suse_version}  
# Maybe you want to use "Requieres" instead of "Recommends" ?  
Recommends:          streamripper >= 1.63  
%endif  
  
# This following "Conflicts" tag will be removed by set-version.sh,  
# if it is a "kradioripper" release (and not a kradioripper-unstable release)...  
Conflicts:           kradioripper  
  
  
%description  
A KDE program for ripping internet radios. Based on StreamRipper.  
  
Authors:  
--------  
    Tim Fechtner  
  

# The %%description macro can have arguments on the same line which determinate the place where to save the information - and not the information itself. So these arguments are blue like %%description.
%description -l de.UTF-8  
Ein KDE-Aufnahmeprogramm für Internetradios. Basiert auf StreamRipper.  
  
Authors:  
--------  
    Tim Fechtner  
  
  
# The following section _must_ be immediatly befor the percent+"prep" section.  
# (see http://lists.opensuse.org/opensuse-packaging/2007-11/msg00000.html)  
#  
# To disable debug packages, we do:  

# Don't use second %%define on the same line!
%define debug_package %{nil}  %define
  
  
%prep  
# in prep section, comments don't need the be the first non-space character!:
%setup -q -n kradioripper  # This is a comment!
# q means quit. n sets the directory.  
  
# sections should always start at the first column. If not-> Error!
 %pre #invalid
do_something  %pre # invalid
do_something %preABC # valid!

# Don't use single % in comments. Using it double is okay: %%

%build  
cmake ./ -DCMAKE_INSTALL_PREFIX=%{_prefix}  
%__make %{?jobs:-j %jobs}  
  
  
%install  
%if 0%{?suse_version}  
%makeinstall  
%suse_update_desktop_file kradioripper  
%else  
%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}  
make install DESTDIR=%{buildroot}  
desktop-file-install --delete-original --vendor fedora --dir=%{buildroot}/%{_datadir}/applications/kde4 %{buildroot}/%{_datadir}/applications/kde4/kradioripper.desktop  
$(test)
%else  
%makeinstall_std  
%endif  
%endif  
  
  
%clean  
rm -rf "%{buildroot}"  
  
  
%files  
%defattr(-,root,root)  
%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}  
%{_datadir}/applications/kde4/fedora-kradioripper.desktop  
%else  
%{_datadir}/applications/kde4/kradioripper.desktop  
%endif  
%{_bindir}/kradioripper  
%{_datadir}/locale/*/LC_MESSAGES/kradioripper.mo  
%if 0%{?suse_version} || 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}  
%doc COPYING LICENSE LICENSE.GPL2 LICENSE.GPL3 NEWS WARRANTY  
%dir %{_datadir}/kde4/apps/kradioripper  
%{_datadir}/kde4/apps/kradioripper/*  
%else  
%{_datadir}/apps/kradioripper/*  
%endif  
  
  
%changelog  
* Thu Dec 23 2008 Tim Fechtner 0.4.28  # no comments allowed here!
- disabling debug packages # no comments allowed here!
* Wed Nov 26 2008 Tim Fechtner 0.4.8  
- support for localization  
- installing the hole _datadir/kde4/apps/kradioripper/* instead of single files  
* Fri Nov 14 2008 Tim Fechtner 0.4.7  
- recommanding streamripper at least in versio 1.63  
* Thu Nov 11 2008 Tim Fechtner 0.4.4  
- revolving ambigiously dependency for Mandriva explicitly when using openSUSE build service  
* Wed Oct 08 2008 Tim Fechtner 0.4.2  
- Integrated Mandriva support. Thanks to Bock & Busse System GbR: http://www.randosweb.de  
* Sun Sep 14 2008 Tim Fechtner 0.3.0-1  
- streamripper no longer requiered but only recommended  
* Sat Sep 13 2008 Tim Fechtner 0.2.1-1  
- ported to openSUSE build service  
- support for Fedora 9  
* Sat May 24 2008 Detlef Reichelt <detlef@links2linux.de> 0.2.1-0.pm.1  
- initial build for packman  
  
