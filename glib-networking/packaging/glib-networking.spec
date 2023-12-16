Name:       glib-networking
Summary:    Network extensions for GLib
Version:    2.32.3_1.5
Release:    1
Group:      System/Libraries
License:    LGPLv2
URL:        http://git.gnome.org/browse/glib-networking/
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gnutls)
BuildRequires:  intltool

%description
Networking extensions for GLib



%prep
%setup -q -n %{name}-%{version}

%build

%configure --disable-static --without-ca-certificates
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}

%make_install

%find_lang glib-networking

%files -f glib-networking.lang
%defattr(-,root,root,-)
%{_libdir}/gio/modules/libgiognutls.so
/usr/share/license/%{name}
%manifest glib-networking.manifest

%changelog
* Wed May 22 2013 Keunsoon Lee <keunsoon.lee@samsung.com>
- [Release] Update changelog for glib-networking-2.32.3_1.5

* Mon Apr 1 2013 Keunsoon Lee <keunsoon.lee@samsung.com>
- [Release] Update changelog for glib-networking-2.32.3_1.4

* Thu Nov 22 2012 praveen.ks <praveen.ks@samsung.com>
- [Release] Update changelog for glib-networking-2.32.3_1.3

* Fri Oct 12 2012 Kwangtae Ko <kwangtae.ko@samsung.com>
- [Release] Update changelog for glib-networking-2.32.3_1.2

* Fri Oct 12 2012 Kwangtae Ko <kwangtae.ko@samsung.com>
- [Title] Add License Information

* Fri Sep 21 2012 Kwangtae Ko <kwangtae.ko@samsung.com>
- [Release] Update changelog for glib-networking-2.32.3_1.1
