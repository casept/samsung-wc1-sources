Name:       isf
Summary:    Input Service Framework
Version:    2.4.8513
Release:    1
Group:      System Environment/Libraries
License:    LGPL-2.1+
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(tts)
BuildRequires:  pkgconfig(security-server)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(feedback)
BuildRequires:  efl-extension-devel
BuildRequires:  pkgconfig(pkgmgr-info)
BuildRequires:  capi-appfw-package-manager-devel
Requires(post): /sbin/ldconfig /usr/bin/vconftool
Requires(postun): /sbin/ldconfig

%define _optexecdir /opt/usr/devel/usr/bin/

%description
Input Service Framewok (ISF) is an input method (IM) platform, and it has been derived from SCIM.


%package devel
Summary:    ISF header files
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains ISF header files for ISE development.

%prep
%setup -q

%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

export GC_SECTIONS_FLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections"
CFLAGS+=" -fvisibility=hidden ${GC_SECTIONS_FLAGS} "; export CFLAGS
CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden ${GC_SECTIONS_FLAGS} ";export CXXFLAGS

%autogen
%configure --disable-static \
		--disable-tray-icon --disable-filter-sctc \
		--disable-frontend-x11
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}%{_datadir}/license
mkdir -p %{buildroot}/opt/etc/dump.d/module.d
install -m0644 %{_builddir}/%{buildsubdir}/COPYING %{buildroot}%{_datadir}/license/%{name}
cp -af ism/dump/isf_log_dump.sh %{buildroot}/opt/etc/dump.d/module.d

%post
/sbin/ldconfig
mkdir -p /etc/scim/conf
mkdir -p /opt/apps/scim/lib/scim-1.0/1.4.0/Helper
mkdir -p /opt/apps/scim/lib/scim-1.0/1.4.0/SetupUI
mkdir -p /opt/apps/scim/lib/scim-1.0/1.4.0/IMEngine


/usr/bin/vconftool set -t bool file/private/isf/autocapital_allow 1 -s tizen::vconf::public::rw -g 6514 || :
/usr/bin/vconftool set -t bool file/private/isf/autoperiod_allow 0 -s tizen::vconf::public::rw -g 6514 || :
/usr/bin/vconftool set -t string db/isf/input_language "en_US" -s tizen::vconf::public::rw -g 5000 || :
/usr/bin/vconftool set -t string db/isf/csc_initial_uuid "" -s tizen::vconf::setting::admin -g 6514 || :
/usr/bin/vconftool set -t string db/isf/input_keyboard_uuid "12aa3425-f88d-45f4-a509-cee8dfe904e3" -s tizen::vconf::setting::admin -g 6514 || :
/usr/bin/vconftool set -t int memory/isf/input_panel_state 0 -s tizen::vconf::public::rw -i -g 5000 || :

%if ! 0%{?sec_product_feature_profile_wearable}
mkdir -p %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
ln -sf %{_libdir}/systemd/system/scim.service %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
%endif

%postun -p /sbin/ldconfig


%files
%manifest %{name}.manifest
/etc/smack/accesses2.d/%{name}.rule
%defattr(-,root,root,-)
%{_libdir}/systemd/system/multi-user.target.wants/scim.service
%{_libdir}/systemd/system/scim.service
%attr(755,root,root) %{_sysconfdir}/profile.d/isf.sh
%{_sysconfdir}/scim/global
%{_sysconfdir}/scim/config
%{_datadir}/scim/isf_candidate_theme1.edj
%{_datadir}/scim/icons/*
%{_datadir}/locale/*
%{_optexecdir}/isf-demo-efl
%{_bindir}/scim
%{_bindir}/isf-log
%{_bindir}/isf-panel-efl
%{_bindir}/isf-query-engines
%{_libdir}/*/immodules/*.so
%{_libdir}/scim-1.0/1.4.0/IMEngine/socket.so
%{_libdir}/scim-1.0/1.4.0/Config/simple.so
%{_libdir}/scim-1.0/1.4.0/Config/socket.so
%{_libdir}/scim-1.0/1.4.0/FrontEnd/*.so
%{_libdir}/scim-1.0/scim-launcher
%{_libdir}/scim-1.0/scim-helper-launcher
%{_libdir}/libscim-*.so*
%{_datadir}/license/%{name}
/opt/etc/dump.d/module.d/*

%files devel
%defattr(-,root,root,-)
%{_includedir}/scim-1.0/*
%{_libdir}/libscim-*.so
%{_libdir}/pkgconfig/isf.pc
%{_libdir}/pkgconfig/scim.pc
