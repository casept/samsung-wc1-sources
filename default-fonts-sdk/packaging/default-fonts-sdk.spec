#default-fonts-sdk
Name:       default-fonts-sdk
Summary:    fonts for Tizen SDK
Version:    2.3.1
Release:    6
Group:      TO_BE/FILLED_IN
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/default-fonts-sdk.manifest
BuildRequires: sec-product-features
Requires(post): fontconfig

%description
fonts for Tizen SDK
This package is maintained by SDK team

%package -n tizen-fonts-latin
Summary: TIZEN Latin fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-latin
Latin font set for TIZEN

%package -n tizen-fonts-japanese
Summary: TIZEN Japanese fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-japanese
Japanese font set for TIZEN

%package -n tizen-fonts-arabic
Summary: TIZEN Arabic fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-arabic
Arabic font set for TIZEN


%package -n tizen-fonts-hebrew
Summary: TIZEN Hebrew fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-hebrew
Hebrew font set for TIZEN


%package -n tizen-fonts-india
Summary: TIZEN India fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-india
India font set for TIZEN


%package -n tizen-fonts-thai
Summary: TIZEN Thai fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-thai
Thai font set for TIZEN


%package -n tizen-fonts-armenian
Summary: TIZEN Armenian fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-armenian
Armenian font set for TIZEN

%package -n tizen-fonts-georgian
Summary: TIZEN Georgian fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-georgian
Georgian font set for TIZEN

%package -n tizen-fonts-ethiopic
Summary: TIZEN Ethiopic fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-ethiopic
Ethiopic font set for TIZEN

%package -n tizen-fonts-khmer
Summary: TIZEN khmer fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-khmer
Khmer font set for TIZEN

%package -n tizen-fonts-laos
Summary: TIZEN laos fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-laos
Laos font set for TIZEN

%package -n tizen-fonts-myanmar
Summary: TIZEN myanmar fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-myanmar
Myanmar font set for TIZEN

%package -n tizen-fonts-korean
Summary: TIZEN Korean fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-korean
Korean font set for TIZEN

%package -n tizen-fonts-fallback
Summary: TIZEN Fallback fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-fallback
Fallback font set for TIZEN

%package -n tizen-fonts-bitmap-emoji
Summary: TIZEN bitmap emoji fonts
Group: TO_BE / FILLED_IN
Requires(post): fontconfig

%description -n tizen-fonts-bitmap-emoji
bitmap emoji font set for TIZEN

%prep
%setup -q

%build
cp %{SOURCE1001} .

%install
rm -rf %{buildroot}
%if 0%{?sec_product_feature_profile_wearable}
    export TARGET=wearable
%else
    export TARGET=mobile
%endif


mkdir -p %{buildroot}/usr/share/license && cp LICENSE %{buildroot}/usr/share/license/%{name}
mkdir -p %{buildroot}%{_datadir}/fonts && cp -a common/fonts %{buildroot}%{_datadir}
##&& cp -a $TARGET/fonts %{buildroot}%{_datadir}
mkdir -p %{buildroot}%{_datadir}/fallback_fonts && cp -a common/fallback_fonts %{buildroot}%{_datadir}
##&& cp -a $TARGET/fallback_fonts %{buildroot}%{_datadir}

%post
/usr/bin/fc-cache -f

%post -n tizen-fonts-latin
/usr/bin/fc-cache -f

%post -n tizen-fonts-japanese
/usr/bin/fc-cache -f

%post -n tizen-fonts-arabic
/usr/bin/fc-cache -f

%post -n tizen-fonts-hebrew
/usr/bin/fc-cache -f

%post -n tizen-fonts-india
/usr/bin/fc-cache -f

%post -n tizen-fonts-thai
/usr/bin/fc-cache -f

%post -n tizen-fonts-armenian
/usr/bin/fc-cache -f

%post -n tizen-fonts-georgian
/usr/bin/fc-cache -f

%post -n tizen-fonts-ethiopic
/usr/bin/fc-cache -f

%post -n tizen-fonts-khmer
/usr/bin/fc-cache -f

%post -n tizen-fonts-laos
/usr/bin/fc-cache -f

%post -n tizen-fonts-myanmar
/usr/bin/fc-cache -f

%post -n tizen-fonts-korean
/usr/bin/fc-cache -f

%post -n tizen-fonts-fallback
/usr/bin/fc-cache -f

%post -n tizen-fonts-bitmap-emoji
/usr/bin/fc-cache -f

%files
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fonts/*
%{_datadir}/fallback_fonts/*
/usr/share/license/%{name}


%files -n tizen-fonts-latin
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%if 0%{?sec_product_feature_profile_wearable}
%{_datadir}/fonts/*Medium*Condensed*.ttf
%{_datadir}/fonts/*Regular*Condensed*.ttf
%else
%{_datadir}/fonts/*Medium*.ttf
%{_datadir}/fonts/*Light*.ttf
%{_datadir}/fonts/*Regular*.ttf
%endif

%files -n tizen-fonts-japanese
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Japanese*.ttf

%files -n tizen-fonts-arabic
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Arabic*.ttf

%files -n tizen-fonts-hebrew
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Hebrew*.ttf

%files -n tizen-fonts-india
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Bengali*.ttf
%{_datadir}/fallback_fonts/*Gujarathi*.ttf
%{_datadir}/fallback_fonts/*Hindi*.ttf
%{_datadir}/fallback_fonts/*Kannada*.ttf
%{_datadir}/fallback_fonts/*Malayalam*.ttf
%{_datadir}/fallback_fonts/*Oriya*.ttf
%{_datadir}/fallback_fonts/*Punjabi*.ttf
%{_datadir}/fallback_fonts/*Sinhala*.ttf
%{_datadir}/fallback_fonts/*Tamil*.ttf
%{_datadir}/fallback_fonts/*Telugu*.ttf

%files -n tizen-fonts-thai
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Thai*.ttf

%files -n tizen-fonts-armenian
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Armenian*.ttf

%files -n tizen-fonts-georgian
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Georgian*.ttf

%files -n tizen-fonts-ethiopic
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Ethiopic*.ttf

%files -n tizen-fonts-khmer
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Khmer*.ttf

%files -n tizen-fonts-laos
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Lao*.ttf

%files -n tizen-fonts-myanmar
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/*Myanmar*.ttf

%files -n tizen-fonts-korean
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%if 0%{?sec_product_feature_profile_wearable}
%{_datadir}/fallback_fonts/*Korean*Condensed*.ttf
%else
%{_datadir}/fallback_fonts/*Korean*.ttf
%endif

%files -n tizen-fonts-fallback
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%if 0%{?sec_product_feature_profile_wearable}
%{_datadir}/fallback_fonts/*Fallback*Condensed*.ttf
%else
%{_datadir}/fallback_fonts/*Fallback*.ttf
%endif

%files -n tizen-fonts-bitmap-emoji
%manifest default-fonts-sdk.manifest
%defattr(-,root,root,-)
%{_datadir}/fallback_fonts/BreezeColorEmoji.ttf
