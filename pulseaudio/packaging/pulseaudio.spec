%define _unpackaged_files_terminate_build 0
%define pulseversion  4.0
Name:       pulseaudio
Summary:    Improved Linux sound server
Version:    4.0.196
Release:    0
Group:      Multimedia/PulseAudio
License:    LGPLv2+
URL:        http://pulseaudio.org
Source0:    http://0pointer.de/lennart/projects/pulseaudio/pulseaudio-%{version}.tar.gz
Source1:    pulseaudio.service
Requires:   systemd >= 183
Requires:   dbus
Requires:   bluez
Requires(preun):  /usr/bin/systemctl
Requires(post):   /usr/bin/systemctl
Requires(postun): /usr/bin/systemctl
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Requires(post):  sys-assert
BuildRequires:  pkgconfig(sndfile)
BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(libascenario)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(libudev)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(security-server)
%ifarch %{arm}
BuildRequires:  pkgconfig(lvve-nxp)
BuildRequires:  pkgconfig(audiofilters-sec-src)
%endif
BuildRequires:  m4
BuildRequires:  libtool-ltdl-devel
BuildRequires:  libtool
BuildRequires:  intltool
BuildRequires:  fdupes
BuildRequires:  pkgconfig(json)
BuildRequires:  pkgconfig(sbc)
BuildRequires:  pkgconfig(iniparser)
BuildRequires: sec-product-features

%define conf_option --disable-static --enable-alsa --disable-ipv6 --disable-oss-output --disable-oss-wrapper --enable-dlog --enable-bluez5 --disable-bluez4 --disable-hal-compat --with-udev-rules-dir=/usr/lib/udev/rules.d --without-speex --disable-systemd --disable-esd-compat --disable-pasuspender --disable-esound --disable-glib2 --disable-openssl --disable-ladspa --disable-rygel --disable-position-event-sound
%bcond_with pulseaudio_with_bluez5

%description
PulseAudio is a sound server for Linux and other Unix like operating
systems. It is intended to be an improved drop-in replacement for the
Enlightened Sound Daemon (ESOUND).

%package libs
Summary:    PulseAudio client libraries
Group:      Multimedia/PulseAudio
Requires:   %{name} = %{version}-%{release}
Requires:   /bin/sed
Requires(post): /sbin/syslogd

%description libs
Client libraries used by applications that access a PulseAudio sound server
via PulseAudio's native interface.


%package libs-devel
Summary:    PulseAudio client development headers and libraries
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description libs-devel
Headers and libraries for developing applications that access a PulseAudio
 sound server via PulseAudio's native interface


%package utils
Summary:    Command line tools for the PulseAudio sound server
Group:      Multimedia/PulseAudio
Requires:   %{name} = %{version}-%{release}
Requires:   /bin/sed

%description utils
These tools provide command line access to various features of the
PulseAudio sound server. Included tools are:
   pabrowse - Browse available PulseAudio servers on the local network.
   paplay - Playback a WAV file via a PulseAudio sink.
   pacat - Cat raw audio data to a PulseAudio sink.
   parec - Cat raw audio data from a PulseAudio source.
   pacmd - Connect to PulseAudio's built-in command line control interface.
   pactl - Send a control command to a PulseAudio server.
   padsp - /dev/dsp wrapper to transparently support OSS applications.

%package module-bluetooth
Summary:    Bluetooth module for PulseAudio sound server
Group:      Multimedia/PulseAudio
Requires:   %{name} = %{version}-%{release}
Requires:   /bin/sed

%description module-bluetooth
This module enables PulseAudio to work with bluetooth devices, like headset
 or audio gatewa

%prep
%setup -q


%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%if 0%{?sec_product_feature_profile_wearable}
export CFLAGS+=" -DPROFILE_WEARABLE"
%endif
%if 0%{?sec_product_feature_mmfw_audio_call_noise_reduction}
export CFLAGS+=" -DSUPPORT_NOISE_REDUCTION"
%endif
%if 0%{?sec_product_feature_mmfw_audio_call_extra_volume}
export CFLAGS+=" -DSUPPORT_EXTRA_VOLUME"
%endif
%if 0%{?sec_product_feature_mmfw_audio_dock}
export CFLAGS+=" -DSUPPORT_DOCK"
%endif
%if 0%{?sec_product_feature_mmfw_audio_mono_sound}
export CFLAGS+=" -DPA_ENABLE_MONO_AUDIO"
%endif

%if 0%{?sec_build_binary_sdk}
export CFLAGS+=" -D__SDK__"
%endif

%if 0%{?sec_product_feature_bt_a2dp_sink_enable}
export CFLAGS+=" -DSUPPORT_A2DP_SINK"
%endif

unset LD_AS_NEEDED
%ifarch %{arm}
export CFLAGS+=" -mfloat-abi=softfp -mfpu=neon -fPIE"
%endif
export CFLAGS+=" -D__TIZEN__ -DPA_SMOOTH_VOLUME -D__TIZEN_BT__ -D__TIZEN_LOG__ -DBLUETOOTH_APTX_SUPPORT"
export LDFLAGS+="-Wl,--no-as-needed -pie"
%reconfigure \
%if 0%{?tizen_build_binary_release_type_eng}
 --enable-pcm-dump \
%endif
%ifarch %{arm}
 --enable-lvvefs --enable-secsrc --enable-security \
%if "%{sec_product_feature_mmfw_audio_seamless_voice}" == "wolfson"
        --enable-seamlessvoice=wolfson \
%else
%if "%{sec_product_feature_mmfw_audio_seamless_voice}" == "audience"
        --enable-seamlessvoice=audience \
%else
%if "%{sec_product_feature_mmfw_audio_seamless_voice}" == "dbmd2"
        --enable-seamlessvoice=dbmd2 \
%endif
%endif
%endif
%endif
 %{conf_option}
make %{?_smp_mflags}

%install
%make_install
mkdir -p %{buildroot}/%{_datadir}/license
mkdir -p %{buildroot}/opt/usr/devel/usr/bin
cp LGPL %{buildroot}/%{_datadir}/license/%{name}
cp LGPL %{buildroot}/%{_datadir}/license/pulseaudio-libs
cp LGPL %{buildroot}/%{_datadir}/license/pulseaudio-utils
cp LGPL %{buildroot}/%{_datadir}/license/pulseaudio-module-bluetooth

mkdir -p %{buildroot}%{_libdir}/systemd/system/sound.target.wants
install -m 644 %{SOURCE1} %{buildroot}%{_libdir}/systemd/system/pulseaudio.service
ln -s  ../pulseaudio.service  %{buildroot}%{_libdir}/systemd/system/sound.target.wants/pulseaudio.service
mv %{buildroot}/usr/bin/pa* %{buildroot}/opt/usr/devel/usr/bin
mkdir -p %{buildroot}%{_libdir}/tmpfiles.d
cp pulseaudio.conf %{buildroot}%{_libdir}/tmpfiles.d/pulseaudio.conf
mkdir -p %{buildroot}/opt/var/lib/pulse

pushd %{buildroot}/etc/pulse/filter
ln -sf filter_8000_44100.dat filter_11025_44100.dat
ln -sf filter_8000_44100.dat filter_12000_44100.dat
ln -sf filter_8000_44100.dat filter_16000_44100.dat
ln -sf filter_8000_44100.dat filter_22050_44100.dat
ln -sf filter_8000_44100.dat filter_24000_44100.dat
ln -sf filter_8000_44100.dat filter_32000_44100.dat
popd

%find_lang pulseaudio
%fdupes  %{buildroot}/%{_datadir}
%fdupes  %{buildroot}/%{_includedir}


%preun
if [ $1 == 0 ]; then
    systemctl stop pulseaudio.service
fi

%post
/sbin/ldconfig

/usr/bin/vconftool set -t int memory/Sound/SoundCaptureStatus 0 -g 29 -f -i -s tizen::vconf::public::r::platform::rw
/usr/bin/vconftool set -t int memory/private/sound/pcm_dump 0 -g 29 -f -i -s tizen::vconf::public::r::platform::rw
chsmack -a "pulseaudio" /opt/var/lib/pulse

systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart pulseaudio.service
fi

%postun
/sbin/ldconfig
systemctl daemon-reload

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%post module-bluetooth -p /sbin/ldconfig

%postun module-bluetooth -p /sbin/ldconfig


%docs_package

%lang_package

%files
%manifest pulseaudio.manifest
%doc LICENSE GPL LGPL
%{_sysconfdir}/pulse/filter/*.dat
%dir %{_sysconfdir}/pulse/
%{_bindir}/pulseaudio
%{_libdir}/libpulsecore-%{pulseversion}.so
%{_libdir}/udev/rules.d/90-pulseaudio.rules
%{_sysconfdir}/dbus-1/system.d/pulseaudio-system.conf
%{_libdir}/tmpfiles.d/pulseaudio.conf
%dir /opt/var/lib/pulse/
#list all modules
%{_libdir}/pulse-%{pulseversion}/modules/libalsa-util.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/libcli.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/libprotocol-cli.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/libprotocol-http.so
%{_libdir}/pulse-%{pulseversion}/modules/libprotocol-native.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/libprotocol-simple.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/librtp.so
%{_libdir}/pulse-%{pulseversion}/modules/module-alsa-sink.so
%{_libdir}/pulse-%{pulseversion}/modules/module-alsa-source.so
%{_libdir}/pulse-%{pulseversion}/modules/module-always-sink.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-console-kit.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-device-restore.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-device-manager.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-stream-restore.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-cli-protocol-tcp.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-cli-protocol-unix.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-cli.so
%{_libdir}/pulse-%{pulseversion}/modules/module-combine.so
%{_libdir}/pulse-%{pulseversion}/modules/module-default-device-restore.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-detect.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-http-protocol-tcp.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-http-protocol-unix.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-intended-roles.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-match.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-mmkbd-evdev.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-native-protocol-fd.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-native-protocol-tcp.so
%{_libdir}/pulse-%{pulseversion}/modules/module-native-protocol-unix.so
%{_libdir}/pulse-%{pulseversion}/modules/module-null-sink.so
%{_libdir}/pulse-%{pulseversion}/modules/module-null-source.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-pipe-sink.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-pipe-source.so
%{_libdir}/pulse-%{pulseversion}/modules/module-rescue-streams.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-rtp-recv.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-rtp-send.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-simple-protocol-tcp.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-simple-protocol-unix.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-sine.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-tunnel-sink.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-tunnel-source.so
%{_libdir}/pulse-%{pulseversion}/modules/module-suspend-on-idle.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-volume-restore.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-alsa-card.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-augment-properties.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-card-restore.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-sine-source.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-loopback.so
%{_libdir}/pulse-%{pulseversion}/modules/module-policy.so
%{_libdir}/pulse-%{pulseversion}/modules/module-stream-mgr.so
%{_libdir}/pulse-%{pulseversion}/modules/module-echo-cancel.so
%{_libdir}/pulse-%{pulseversion}/modules/module-udev-detect.so
%{_libdir}/systemd/system/pulseaudio.service
%{_libdir}/systemd/system/sound.target.wants/pulseaudio.service
%{_libdir}/pulse-%{pulseversion}/modules/module-combine-sink.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-dbus-protocol.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-filter-apply.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-filter-heuristics.so
#%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-null-source.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-remap-source.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-role-cork.so
%{_libdir}/pulse-%{pulseversion}/modules/module-role-ducking.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-switch-on-connect.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-switch-on-port-available.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-virtual-sink.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-virtual-source.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-virtual-surround-sink.so
%exclude %{_libdir}/pulse-%{pulseversion}/modules/module-remap-sink.so
%{_datadir}/license/%{name}

%files libs
%manifest pulseaudio_shared.manifest
%{_libdir}/libpulse.so.*
%{_libdir}/libpulse-simple.so.*
%{_libdir}/pulseaudio/libpulsecommon-*.so
%{_datadir}/license/pulseaudio-libs

%files libs-devel
%{_includedir}/pulse/*
#%{_includedir}/pulse-modules-headers/pulsecore/
%{_libdir}/libpulse.so
%{_libdir}/libpulse-simple.so
%{_libdir}/pkgconfig/libpulse-simple.pc
%{_libdir}/pkgconfig/libpulse.pc

%files utils
%manifest pulseaudio_shared.manifest
%doc %{_mandir}/man1/pacat.1.gz
%doc %{_mandir}/man1/pacmd.1.gz
%doc %{_mandir}/man1/pactl.1.gz
%doc %{_mandir}/man1/paplay.1.gz
/opt/usr/devel/usr/bin/pacat
/opt/usr/devel/usr/bin/pacat-simple
/opt/usr/devel/usr/bin/pacmd
/opt/usr/devel/usr/bin/pactl
/opt/usr/devel/usr/bin/paplay
/opt/usr/devel/usr/bin/parec
/opt/usr/devel/usr/bin/pamon
/opt/usr/devel/usr/bin/parecord
%{_datadir}/license/pulseaudio-utils

%files module-bluetooth
%manifest pulseaudio_shared.manifest
%{_libdir}/pulse-%{pulseversion}/modules/module-bluetooth-discover.so
%{_libdir}/pulse-%{pulseversion}/modules/module-bluetooth-policy.so
%{_libdir}/pulse-%{pulseversion}/modules/module-bluez5-discover.so
%{_libdir}/pulse-%{pulseversion}/modules/module-bluez5-device.so
%{_libdir}/pulse-%{pulseversion}/modules/libbluez5-util.so
%{_libdir}/pulse-%{pulseversion}/modules/module-bluetooth-policy.so
%{_datadir}/license/pulseaudio-module-bluetooth

