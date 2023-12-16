#sbs-git:slp/apps/a/all-fonts-slp all-fonts-slp 0.1.2 e95df0b7bbbace8966a72cd8d1298d1ebf0cbba1
Name: all-fonts-slp
Summary: Font sets for tizen
Version: 2.3.1
Release: 8
Group: TO_BE / FILLED_IN
License: TO_BE / FILLED_IN
Source: %{name}-%{version}.tar.gz
BuildRequires: sec-product-features
Requires:    smack-utils
Requires(post): fontconfig

%description
font packages for TIZEN

%package -n tizen-fonts-chinese
Summary: TIZEN Chinese fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig
Requires: tizen-fonts-conf

%description -n tizen-fonts-chinese
Fallback font set for TIZEN

%prep
%setup -q -n %{name}-%{version}

%build
cp packaging/*.manifest .

%install
rm -rf %{buildroot}

mkdir -p %{buildroot}%{_datadir}/fallback_fonts && cp -a common/fallback_fonts %{buildroot}%{_datadir}

%post -n tizen-fonts-chinese
/usr/bin/fc-cache -f

%files -n tizen-fonts-chinese
%manifest tizen-fonts.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/SECHan*.*tf


