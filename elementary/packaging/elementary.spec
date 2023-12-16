Name:       elementary
Summary:    EFL toolkit for small touchscreens
Version:    1.7.1+svn.77535+build450
Release:    1
Group:      System/Libraries
License:    LGPLv2.1
URL:        http://trac.enlightenment.org/e/wiki/Elementary
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  sec-product-features
BuildRequires:  gettext
BuildRequires:  edje-tools
BuildRequires:  eet-tools
BuildRequires:  eina-devel
BuildRequires:  eet-devel
BuildRequires:  evas-devel
BuildRequires:  ecore-devel
BuildRequires:  edje-devel
BuildRequires:  edbus-devel
BuildRequires:  efreet-devel
BuildRequires:  libx11-devel
BuildRequires:  pkgconfig(icu-i18n)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(cairo)
%if ! 0%{?sec_product_feature_profile_lite}
%if ! 0%{?sec_product_feature_profile_wearable}
BuildRequires:  emotion-devel
BuildRequires:  pkgconfig(appsvc)
%endif
%endif
%if ! 0%{?sec_product_feature_profile_wearable}
BuildRequires:  ethumb-devel
%endif

%description
Elementary - a basic widget set that is easy to use based on EFL for mobile This package contains devel content.

%package devel
Summary:    EFL toolkit (devel)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
EFL toolkit for small touchscreens (devel)

%package tools
Summary:    EFL toolkit (tools)
Group:      Development/Tools
Requires:   %{name} = %{version}-%{release}
Provides:   %{name}-bin
Obsoletes:  %{name}-bin

%description tools
EFL toolkit for small touchscreens (tools)

%prep
%setup -q

%build
export CFLAGS+=" -fPIC -Wall"
export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed"

export CFLAGS+=" -DELM_FOCUSED_UI"
export CFLAGS+=" -DELM_FEATURE_MULTI_WINDOW"
export CFLAGS+=" -DELM_FEATURE_POWER_SAVE"

%if 0%{?sec_product_feature_profile_wearable}
export CFLAGS+=" -DELM_FEATURE_WEARABLE"
%endif

%if 0%{?sec_product_feature_profile_lite}
export CFLAGS+=" -DELM_FEATURE_LITE"
%endif

%if 0%{?sec_product_feature_uifw_efl_b3_theme}
export CFLAGS+=" -DELM_FEATURE_WEARABLE_B3"
%endif

%if 0%{?sec_product_feature_uifw_efl_c1_theme}
export CFLAGS+=" -DELM_FEATURE_WEARABLE_C1"
%endif

%autogen \
%if 0%{?sec_product_feature_profile_lite}
	--disable-emotion \
%endif
%if 0%{?sec_product_feature_profile_wearable}
	--disable-emotion \
%endif
	--disable-static \
	--enable-dependency-tracking

make %{?jobs:-j%jobs}

%install
%make_install

mkdir -p %{buildroot}/%{_datadir}/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}-tools

mkdir -p %{buildroot}/opt/etc/dump.d/module.d
cp -af dump/ui_log_dump.sh %{buildroot}/opt/etc/dump.d/module.d

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libelementary.so.*
%{_libdir}/elementary/modules/*/*/*.so
%{_libdir}/edje/modules/elm/*/module.so
%{_datadir}/elementary/edje_externals
%{_datadir}/locale
%{_datadir}/license/%{name}
%manifest %{name}.manifest
%attr(755,root,root) /opt/etc/dump.d/module.d/*

## theme is installed from efl-theme-tizen
## config is installed from elm-misc
%exclude %{_datadir}/elementary/themes
%exclude %{_datadir}/elementary/config

%files devel
%defattr(-,root,root,-)
/usr/include/*
%{_libdir}/libelementary.so
%{_libdir}/pkgconfig/elementary.pc

%files tools
%defattr(-,root,root,-)
%{_bindir}/elementary_*
%{_libdir}/elementary_testql.so
%{_datadir}/icons
%{_datadir}/applications
%{_datadir}/elementary/objects
%{_datadir}/elementary/images
%{_datadir}/license/%{name}-tools
%manifest %{name}-tools.manifest
