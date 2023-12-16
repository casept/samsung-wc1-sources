Name:       mmfw-sysconf
Summary:    Multimedia Framework system configuration package
Version:    0.1.247
Release:    0
VCS:        framework/multimedia/mmfw-sysconf#mmfw-sysconf_0.1.110-0-144-g25720f9f7daad9a897662eb6059d71d958cc3f06
Group:      TO_BE/FILLED_IN
License:    Apache-2.0
Source0:    mmfw-sysconf-%{version}.tar.gz
BuildRequires:  sec-product-features

%description
Multimedia Framework system configuration package

%ifarch %{arm}
%package adonis
Summary: Multimedia Framework system configuration package for adonis
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description adonis
Multimedia Framework system configuration package for adonis

%package c110
Summary: Multimedia Framework system configuration package for c110
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description c110
Multimedia Framework system configuration package for c110

%package c210
Summary: Multimedia Framework system configuration package for c210
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description c210
Multimedia Framework system configuration package for ac210

%package e4x12-kernel-v34
Summary: Multimedia Framework system configuration package for e4x12-kernel-v34
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description e4x12-kernel-v34
Multimedia Framework system configuration package for ex12-kernel-v34

%package redwood45
Summary: Multimedia Framework system configuration package for redwood45
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description redwood45
Multimedia Framework system configuration package for redwood45

%package e4x12
Summary: Multimedia Framework system configuration package for e4x12
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description e4x12
Multimedia Framework system configuration package for e4x12

%package e4212
Summary: Multimedia Framework system configuration package for e4212
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description e4212
Multimedia Framework system configuration package for e4212

%package e3250
Summary: Multimedia Framework system configuration package for e3250
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description e3250
Multimedia Framework system configuration package for e3250

%package e3250-blink
Summary: Multimedia Framework system configuration package for e3250-blink
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description e3250-blink
Multimedia Framework system configuration package for e3250-blink

%package marvell
Summary: Multimedia Framework system configuration package for marvell
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description marvell
Multimedia Framework system configuration package for marvell

%package msm8255
Summary: Multimedia Framework system configuration package for msm8255
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description msm8255
Multimedia Framework system configuration package for msm8255

%package msm8x55
Summary: Multimedia Framework system configuration package for msm8x55
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description msm8x55
Multimedia Framework system configuration package for msm8x55

%package msm8x30
Summary: Multimedia Framework system configuration package for msm8x30
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description msm8x30
Multimedia Framework system configuration package for msm8x30

%package msm8x74
Summary: Multimedia Framework system configuration package for msm8x74
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description msm8x74
Multimedia Framework system configuration package for msm8x74

%package msm8x26
Summary: Multimedia Framework system configuration package for msm8x26
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description msm8x26
Multimedia Framework system configuration package for msm8x26

%package msm8x10
Summary: Multimedia Framework system configuration package for msm8x10
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description msm8x10
Multimedia Framework system configuration package for msm8x10

%package omap4
Summary: Multimedia Framework system configuration package for omap4
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description omap4
Multimedia Framework system configuration package for omap4

%else
%package simulator
Summary: Multimedia Framework system configuration package for simulator
Group: TO_BE/FILLED_IN
License:    Apache-2.0

%description simulator
Multimedia Framework system configuration package for simulator
%endif

%prep
%setup -q -n %{name}-%{version}

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/etc
mkdir -p %{buildroot}/usr
mkdir -p %{buildroot}/usr/share/license
%ifarch %{arm}
cp LICENSE.APLv2.0 %{buildroot}/usr/share/license/%{name}-e3250
cat LICENSE.LGPLv2.1 >> %{buildroot}/usr/share/license/%{name}-e3250
%else
cp LICENSE.APLv2.0 %{buildroot}/usr/share/license/mmfw-sysconf-simulator
cat LICENSE.LGPLv2.1 >> %{buildroot}/usr/share/license/mmfw-sysconf-simulator
%endif


%ifarch %{arm}
mkdir -p %{buildroot}/mmfw-sysconf-adonis
cp -arf mmfw-sysconf-adonis/* %{buildroot}/mmfw-sysconf-adonis
mkdir -p %{buildroot}/mmfw-sysconf-c110
cp -arf mmfw-sysconf-c110/* %{buildroot}/mmfw-sysconf-c110
mkdir -p %{buildroot}/mmfw-sysconf-c210
cp -arf mmfw-sysconf-c210/* %{buildroot}/mmfw-sysconf-c210
mkdir -p %{buildroot}/mmfw-sysconf-e4x12
cp -arf mmfw-sysconf-e4x12/* %{buildroot}/mmfw-sysconf-e4x12
mkdir -p %{buildroot}/mmfw-sysconf-e4212
cp -arf mmfw-sysconf-e4212/* %{buildroot}/mmfw-sysconf-e4212
mkdir -p %{buildroot}/mmfw-sysconf-e4x12-kernel-v34
cp -arf mmfw-sysconf-e4x12-kernel-v34/* %{buildroot}/mmfw-sysconf-e4x12-kernel-v34
mkdir -p %{buildroot}/mmfw-sysconf-redwood45
cp -arf mmfw-sysconf-redwood45/* %{buildroot}/mmfw-sysconf-redwood45

mkdir -p %{buildroot}/mmfw-sysconf-e3250
cp -arf mmfw-sysconf-e3250/* %{buildroot}/mmfw-sysconf-e3250
%if "%{sec_product_feature_audio_tuning_mode}" == "R720"
mv -f %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib_sport.txt %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib.txt
mv -f %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib_sport_vr.txt %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib_vr.txt
%else
%if "%{sec_product_feature_audio_tuning_mode}" == "R732"
mv -f %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib_classic.txt %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib.txt
mv -f %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib_classic_vr.txt %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib_vr.txt
%else
rm -f %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib_classic.txt
rm -f %{buildroot}/mmfw-sysconf-e3250/usr/etc/miccalib_sport.txt
%endif
%endif

mkdir -p %{buildroot}/mmfw-sysconf-e3250-blink
cp -arf mmfw-sysconf-e3250-blink/* %{buildroot}/mmfw-sysconf-e3250-blink
mkdir -p %{buildroot}/mmfw-sysconf-marvell
cp -arf mmfw-sysconf-marvell/* %{buildroot}/mmfw-sysconf-marvell
mkdir -p %{buildroot}/mmfw-sysconf-msm8x55
cp -arf mmfw-sysconf-msm8x55/* %{buildroot}/mmfw-sysconf-msm8x55
mkdir -p %{buildroot}/mmfw-sysconf-msm8255
cp -arf mmfw-sysconf-msm8255/* %{buildroot}/mmfw-sysconf-msm8255
mkdir -p %{buildroot}/mmfw-sysconf-msm8x30
cp -arf mmfw-sysconf-msm8x30/* %{buildroot}/mmfw-sysconf-msm8x30
mkdir -p %{buildroot}/mmfw-sysconf-msm8x74

for f in `find mmfw-sysconf-msm8x74/etc/pulse -name "*.in"`; do \
	echo $f;\
	cat $f > ${f%.in}; \
	# if this packge is built for JPN, we would use the below.
	%if "%{?sec_build_conf_tizen_product_name}" == "redwood8974om"
		sed -i -e "s#@DCM@##g" ${f%.in}; \
		sed -i -e "s#@ORG@#\#\#\# #g" ${f%.in}; \
	%else
		sed -i -e "s#@DCM@#\#\#\# #g" ${f%.in}; \
		sed -i -e "s#@ORG@##g" ${f%.in}; \
	%endif
done

cp -arf mmfw-sysconf-msm8x74/* %{buildroot}/mmfw-sysconf-msm8x74

mkdir -p %{buildroot}/mmfw-sysconf-msm8x26
cp -arf mmfw-sysconf-msm8x26/* %{buildroot}/mmfw-sysconf-msm8x26

mkdir -p %{buildroot}/mmfw-sysconf-msm8x10
cp -arf mmfw-sysconf-msm8x10/* %{buildroot}/mmfw-sysconf-msm8x10

mkdir -p %{buildroot}/mmfw-sysconf-omap4
cp -arf mmfw-sysconf-omap4/* %{buildroot}/mmfw-sysconf-omap4
%else
mkdir -p %{buildroot}/mmfw-sysconf-simulator
cp -arf mmfw-sysconf-simulator/* %{buildroot}/mmfw-sysconf-simulator
%endif

%ifarch %{arm}
%post adonis
cp -arf /mmfw-sysconf-adonis/* /
rm -rf /mmfw-sysconf-adonis
%post c110
cp -arf /mmfw-sysconf-c110/* /
rm -rf /mmfw-sysconf-c110
%post c210
cp -arf /mmfw-sysconf-c210/* /
rm -rf /mmfw-sysconf-c210
%post e4x12
cp -arf /mmfw-sysconf-e4x12/* /
rm -rf /mmfw-sysconf-e4x12
%post e4212
cp -arf /mmfw-sysconf-e4212/* /
rm -rf /mmfw-sysconf-e4212
%post e4x12-kernel-v34
cp -arf /mmfw-sysconf-e4x12-kernel-v34/* /
rm -rf /mmfw-sysconf-e4x12-kernel-v34
%post redwood45
cp -arf /mmfw-sysconf-redwood45/* /
rm -rf /mmfw-sysconf-redwood45
%post e3250
cp -arf /mmfw-sysconf-e3250/* /
rm -rf /mmfw-sysconf-e3250
%post e3250-blink
cp -arf /mmfw-sysconf-e3250-blink/* /
rm -rf /mmfw-sysconf-e3250-blink
%post marvell
cp -arf /mmfw-sysconf-marvell/* /
rm -rf /mmfw-sysconf-marvell
%post msm8x55
cp -arf /mmfw-sysconf-msm8x55/* /
rm -rf /mmfw-sysconf-msm8x55
%post msm8255
cp -arf /mmfw-sysconf-msm8255/* /
rm -rf /mmfw-sysconf-msm8255
%post msm8x30
cp -arf /mmfw-sysconf-msm8x30/* /
rm -rf /mmfw-sysconf-msm8x30
%post msm8x74
cp -arf /mmfw-sysconf-msm8x74/* /
rm -rf /mmfw-sysconf-msm8x74
%post msm8x26
cp -arf /mmfw-sysconf-msm8x26/* /
rm -rf /mmfw-sysconf-msm8x26
%post msm8x10
cp -arf /mmfw-sysconf-msm8x10/* /
rm -rf /mmfw-sysconf-msm8x10
%post omap4
cp -arf /mmfw-sysconf-omap4/* /
rm -rf /mfw-sysconf-omap4
%else
%post simulator
cp -arf /mmfw-sysconf-simulator/* /
rm -rf /mmfw-sysconf-simulator
%endif

%ifarch %{arm}
%files adonis
%manifest mmfw-sysconf-adonis.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-adonis/etc/asound.conf
/mmfw-sysconf-adonis/etc/pulse/*
/mmfw-sysconf-adonis/usr/etc/*.ini
/mmfw-sysconf-adonis/usr/etc/gst-openmax.conf
/mmfw-sysconf-adonis/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-adonis/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-adonis/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-adonis/opt/system/*
/usr/share/license/*

%files c110
%manifest mmfw-sysconf-c110.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-c110/etc/asound.conf
/mmfw-sysconf-c110/etc/pulse/*
/mmfw-sysconf-c110/usr/etc/*.ini
/mmfw-sysconf-c110/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-c110/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-c110/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-c110/opt/system/*
/usr/share/license/*

%files c210
%manifest mmfw-sysconf-c210.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-c210/etc/asound.conf
/mmfw-sysconf-c210/etc/pulse/*
/mmfw-sysconf-c210/usr/etc/*.ini
/mmfw-sysconf-c210/usr/etc/gst-openmax.conf
/mmfw-sysconf-c210/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-c210/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-c210/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-c210/opt/system/*
/usr/share/license/*

%files e4x12
%manifest mmfw-sysconf-e4x12.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-e4x12/etc/asound.conf
/mmfw-sysconf-e4x12/etc/pulse/*
/mmfw-sysconf-e4x12/etc/profile.d/*
/mmfw-sysconf-e4x12/usr/etc/*.ini
/mmfw-sysconf-e4x12/usr/etc/gst-openmax.conf
/mmfw-sysconf-e4x12/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-e4x12/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-e4x12/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-e4x12/opt/system/*
/usr/share/license/*

%files e4212
%manifest mmfw-sysconf-e4212.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-e4212/etc/asound.conf
/mmfw-sysconf-e4212/etc/pulse/*
/mmfw-sysconf-e4212/etc/profile.d/*
/mmfw-sysconf-e4212/usr/etc/*.ini
/mmfw-sysconf-e4212/usr/etc/*.txt
/mmfw-sysconf-e4212/usr/etc/gst-openmax.conf
/mmfw-sysconf-e4212/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-e4212/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-e4212/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-e4212/opt/system/*
/usr/share/license/*

%files e4x12-kernel-v34
%manifest mmfw-sysconf-e4x12-kernel-v34.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-e4x12-kernel-v34/etc/asound.conf
/mmfw-sysconf-e4x12-kernel-v34/etc/pulse/*
/mmfw-sysconf-e4x12-kernel-v34/etc/profile.d/*
/mmfw-sysconf-e4x12-kernel-v34/usr/etc/*.ini
/mmfw-sysconf-e4x12-kernel-v34/usr/etc/gst-openmax.conf
/mmfw-sysconf-e4x12-kernel-v34/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-e4x12-kernel-v34/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-e4x12-kernel-v34/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-e4x12-kernel-v34/opt/system/*
/usr/share/license/*

%files redwood45
%manifest mmfw-sysconf-redwood45.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-redwood45/etc/asound.conf
/mmfw-sysconf-redwood45/etc/pulse/*
/mmfw-sysconf-redwood45/etc/profile.d/*
/mmfw-sysconf-redwood45/usr/etc/*.ini
/mmfw-sysconf-redwood45/usr/etc/gst-openmax.conf
/mmfw-sysconf-redwood45/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-redwood45/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-redwood45/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-redwood45/opt/system/*
/usr/share/license/*

%files e3250
%manifest mmfw-sysconf-e3250.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-e3250/etc/asound.conf
/mmfw-sysconf-e3250/etc/pulse/*
/mmfw-sysconf-e3250/etc/profile.d/*
/mmfw-sysconf-e3250/usr/etc/*.ini
/mmfw-sysconf-e3250/usr/etc/*.txt
/mmfw-sysconf-e3250/usr/etc/gst-openmax.conf
/mmfw-sysconf-e3250/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-e3250/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-e3250/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-e3250/opt/system/*
/usr/share/license/*

%files e3250-blink
%manifest mmfw-sysconf-e3250-blink.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-e3250-blink/etc/asound.conf
/mmfw-sysconf-e3250-blink/etc/pulse/*
/mmfw-sysconf-e3250-blink/etc/profile.d/*
/mmfw-sysconf-e3250-blink/usr/etc/*.ini
/mmfw-sysconf-e3250-blink/usr/etc/*.txt
/mmfw-sysconf-e3250-blink/usr/etc/gst-openmax.conf
/mmfw-sysconf-e3250-blink/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-e3250-blink/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-e3250-blink/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-e3250-blink/opt/system/*
/usr/share/license/*

%files marvell
%manifest mmfw-sysconf-marvell.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-marvell/etc/asound.conf
/mmfw-sysconf-marvell/etc/pulse/*
/mmfw-sysconf-marvell/usr/etc/*.ini
/mmfw-sysconf-marvell/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-marvell/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-marvell/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-marvell/opt/system/*
/usr/share/license/*

%files msm8x55
%manifest mmfw-sysconf-msm8x55.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-msm8x55/etc/asound.conf
/mmfw-sysconf-msm8x55/etc/pulse/*
/mmfw-sysconf-msm8x55/usr/etc/*.ini
/mmfw-sysconf-msm8x55/usr/etc/gst-openmax.conf
/mmfw-sysconf-msm8x55/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-msm8x55/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-msm8x55/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-msm8x55/opt/system/*
/usr/share/license/*

%files msm8255
%manifest mmfw-sysconf-msm8255.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-msm8255/etc/asound.conf
/mmfw-sysconf-msm8255/etc/pulse/*
/mmfw-sysconf-msm8255/usr/etc/*.ini
/mmfw-sysconf-msm8255/usr/etc/gst-openmax.conf
/mmfw-sysconf-msm8255/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-msm8255/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-msm8255/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-msm8255/opt/system/*
/usr/share/license/*

%files msm8x30
%manifest mmfw-sysconf-msm8x30.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-msm8x30/etc/asound.conf
/mmfw-sysconf-msm8x30/etc/pulse/*
/mmfw-sysconf-msm8x30/etc/profile.d/*
/mmfw-sysconf-msm8x30/usr/etc/*.ini
/mmfw-sysconf-msm8x30/usr/etc/gst-openmax.conf
/mmfw-sysconf-msm8x30/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-msm8x30/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-msm8x30/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-msm8x30/opt/system/*
/usr/share/license/*

%files msm8x74
%manifest mmfw-sysconf-msm8x74.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-msm8x74/etc/asound.conf
/mmfw-sysconf-msm8x74/etc/pulse/client.conf
/mmfw-sysconf-msm8x74/etc/pulse/daemon.conf
/mmfw-sysconf-msm8x74/etc/pulse/system.pa
/mmfw-sysconf-msm8x74/etc/pulse/default.pa
%exclude /mmfw-sysconf-msm8x74/etc/pulse/system.pa.in
/mmfw-sysconf-msm8x74/etc/profile.d/*
/mmfw-sysconf-msm8x74/usr/etc/*.ini
/mmfw-sysconf-msm8x74/usr/etc/gst-openmax.conf
/mmfw-sysconf-msm8x74/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-msm8x74/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-msm8x74/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-msm8x74/opt/system/*
/usr/share/license/*

%files msm8x26
%manifest mmfw-sysconf-msm8x26.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-msm8x26/etc/asound.conf
/mmfw-sysconf-msm8x26/etc/pulse/client.conf
/mmfw-sysconf-msm8x26/etc/pulse/daemon.conf
/mmfw-sysconf-msm8x26/etc/pulse/system.pa
/mmfw-sysconf-msm8x26/etc/pulse/default.pa
/mmfw-sysconf-msm8x26/etc/profile.d/*
/mmfw-sysconf-msm8x26/usr/etc/*.ini
/mmfw-sysconf-msm8x26/usr/etc/miccalib.txt
/mmfw-sysconf-msm8x26/usr/etc/gst-openmax.conf
/mmfw-sysconf-msm8x26/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-msm8x26/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-msm8x26/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-msm8x26/opt/system/*
/usr/share/license/*

%files msm8x10
%manifest mmfw-sysconf-msm8x10.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-msm8x10/etc/asound.conf
/mmfw-sysconf-msm8x10/etc/pulse/*
/mmfw-sysconf-msm8x10/etc/profile.d/*
/mmfw-sysconf-msm8x10/usr/etc/*.ini
/mmfw-sysconf-msm8x10/usr/etc/gst-openmax.conf
/mmfw-sysconf-msm8x10/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-msm8x10/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-msm8x10/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-msm8x10/opt/system/*
/usr/share/license/*

%files omap4
%manifest mmfw-sysconf-omap4.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-omap4/etc/asound.conf
/mmfw-sysconf-omap4/etc/pulse/*
/mmfw-sysconf-omap4/usr/etc/*.ini
/mmfw-sysconf-omap4/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-omap4/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-omap4/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-omap4/opt/system/*
/usr/share/license/*
%else

%files simulator
%manifest mmfw-sysconf-simulator.manifest
%defattr(-,root,root,-)
/mmfw-sysconf-simulator/etc/asound.conf
/mmfw-sysconf-simulator/etc/pulse/*
/mmfw-sysconf-simulator/usr/etc/*.ini
/mmfw-sysconf-simulator/usr/etc/gst-openmax.conf
/mmfw-sysconf-simulator/usr/share/pulseaudio/alsa-mixer/paths/*.conf
/mmfw-sysconf-simulator/usr/share/pulseaudio/alsa-mixer/paths/*.common
/mmfw-sysconf-simulator/usr/share/pulseaudio/alsa-mixer/profile-sets/*.conf
/mmfw-sysconf-simulator/opt/system/*
/usr/share/license/mmfw-sysconf-simulator
%endif
