Name:       bluez
Summary:    Bluetooth utilities
Version:    5.30
Release:    1
VCS:        framework/connectivity/bluez#bluez_4.101-slp2+22-132-g5d15421a5bd7101225aecd7c25b592b9a9ca218b
Group:      Applications/System
License:    GPLv2+
URL:        http://www.bluez.org/
Source0:    http://www.kernel.org/pub/linux/bluetooth/%{name}-%{version}.tar.gz
Source101:    obex-root-setup
Source102:    create-symlinks
Patch1 :    bluez-ncurses.patch
Patch2 :    disable-eir-unittest.patch
Requires:   dbus >= 0.60
BuildRequires:  pkgconfig(libudev)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(ncurses)
BuildRequires:  flex
BuildRequires:  bison
BuildRequires:  readline-devel
BuildRequires:  openssl-devel
BuildRequires:  sec-product-features
%ifarch %{arm}
#!BuildIgnore: kernel-headers
BuildRequires:  kernel-headers-tizen-dev
%endif

%description
Utilities for use in Bluetooth applications:
	--dfutool
	--hcitool
	--l2ping
	--rfcomm
	--sdptool
	--hciattach
	--hciconfig
	--hid2hci

The BLUETOOTH trademarks are owned by Bluetooth SIG, Inc., U.S.A.

%package -n libbluetooth3
Summary:    Libraries for use in Bluetooth applications
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}
Requires(post): eglibc
Requires(postun): eglibc

%description -n libbluetooth3
Libraries for use in Bluetooth applications.

%package -n libbluetooth-devel
Summary:    Development libraries for Bluetooth applications
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   libbluetooth3 = %{version}

%description -n libbluetooth-devel
bluez-libs-devel contains development libraries and headers for
use in Bluetooth applications.

%package -n obexd
Summary: OBEX Server A basic OBEX server implementation
Group: Applications/System

%description -n obexd
OBEX Server A basic OBEX server implementation.

%package -n bluez-test
Summary:    Test utilities for BlueZ
Group:      Test Utilities

%description -n bluez-test
bluez-test contains test utilities for BlueZ testing.

%prep
%setup -q
%patch1 -p1

%build
%if 0%{?sec_product_feature_profile_wearable}
export CFLAGS="${CFLAGS} -D__TIZEN_PATCH__ -D__BROADCOM_PATCH__ -D__BT_SCMST_FEATURE__ -DSUPPORT_SMS_ONLY -D__BROADCOM_QOS_PATCH__ -DTIZEN_WEARABLE"
%else
export CFLAGS="${CFLAGS} -D__TIZEN_PATCH__ -DSUPPORT_SMS_ONLY -DBLUEZ5_27_GATT_CLIENT"

%if 0%{?sec_product_feature_wlan_board_bcmdhd}
export CFLAGS="${CFLAGS} -D__BROADCOM_PATCH__"
%endif

%if "%{?sec_build_project_name}" == "kirane_swa_open"
export CFLAGS="${CFLAGS} -D__BROADCOM_PATCH__ -D__BROADCOM_QOS_PATCH__"
%endif
%endif

%if 0%{?sec_product_feature_bt_hid_device_enable}
export CFLAGS="${CFLAGS} -DTIZEN_BT_HID_DEVICE_ENABLE"
%endif

%if 0%{?sec_product_feature_bt_avrcp_target_enable}
export CFLAGS+=" -DSUPPORT_AVRCP_TARGET"
%endif

%if 0%{?sec_product_feature_bt_avrcp_control_enable}
export CFLAGS+=" -DSUPPORT_AVRCP_CONTROL"
%endif

%if 0%{?sec_product_feature_bt_a2dp_sink_enable}
export CFLAGS+=" -DSUPPORT_LOCAL_DEVICE_A2DP_SINK"
%endif

%if 0%{?sec_product_feature_bt_avrcp_category1_enable}
export CFLAGS="${CFLAGS} -DENABLE_AVRCP_CATEGORY1"
%endif

%if 0%{?sec_product_feature_bt_avrcp_category2_enable}
export CFLAGS="${CFLAGS} -DENABLE_AVRCP_CATEGORY2"
%endif

%if "%{?sec_product_feature_bt_io_capability}" == "NoInputNoOutput"
export CFLAGS="${CFLAGS} -DTIZEN_BT_IO_CAPA_NO_INPUT_OUTPUT"
%endif

# This features are use for bt qualification
#export CFLAGS="${CFLAGS} -DBT_QUALIFICATION"

export CFLAGS="${CFLAGS} -DGATT_NO_RELAY"

echo Model:%{sec_build_option_product_model}
export LDFLAGS=" -lncurses -Wl,--as-needed "
export CFLAGS+=" -DPBAP_SIM_ENABLE"
%reconfigure --disable-static \
			--sysconfdir=%{_sysconfdir} \
			--localstatedir=%{_localstatedir} \
			--enable-systemd \
			--enable-debug \
			--enable-pie \
%if ! 0%{?sec_product_feature_profile_wearable}
			--enable-network \
%endif
			--enable-serial \
			--enable-input \
			--enable-usb=no \
			--enable-tools \
			--disable-bccmd \
			--enable-pcmcia=no \
			--enable-hid2hci=no \
			--enable-alsa=no \
			--enable-gstreamer=no \
			--disable-dfutool \
			--disable-cups \
			--enable-health \
			--enable-dbusoob \
			--enable-test \
			--with-telephony=tizen \
			--enable-obex \
%if ! 0%{?sec_product_feature_bt_pbap_server_enable}
			--disable-obex \
%endif
			--enable-library \
%if 0%{?sec_product_feature_profile_wearable}
			--enable-gatt \
			--enable-wearable \
%endif
			--enable-experimental \
			--enable-autopair=no \
%if 0%{?sec_product_feature_bt_hid_host_enable}
			--enable-hid=yes \
%else
			--enable-hid=no \
%endif
%if 0%{?sec_product_feature_bt_sap_enable}
			--enable-sap \
%endif
			--enable-tizenunusedplugin=no
# enable-wearable			: Conditional compilation for wearables
# enable-autopair=no		: Conditional compilation of Autopair plugin
# enable-hid=no	: Conditional compilation of HID plugin
# enable-tizenunusedplugin=no	: Conditional compilation of Unused plugin

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%if 0%{?sec_product_feature_profile_wearable}
install -D -m 0644 src/main_w.conf %{buildroot}%{_sysconfdir}/bluetooth/main.conf
%else
%if 0%{?sec_product_feature_bt_a2dp_sink_enable}
install -D -m 0644 src/main_hive.conf %{buildroot}%{_sysconfdir}/bluetooth/main.conf
%else
install -D -m 0644 src/main_m.conf %{buildroot}%{_sysconfdir}/bluetooth/main.conf
%endif
%endif

install -D -m 0644 src/bluetooth.conf %{buildroot}%{_sysconfdir}/dbus-1/system.d/bluetooth.conf
#install -D -m 0644 profiles/audio/audio.conf %{buildroot}%{_sysconfdir}/bluetooth/audio.conf
install -D -m 0644 profiles/network/network.conf %{buildroot}%{_sysconfdir}/bluetooth/network.conf

install -D -m 0644 COPYING %{buildroot}%{_datadir}/license/bluez
install -D -m 0644 COPYING %{buildroot}%{_datadir}/license/libbluetooth3
install -D -m 0644 COPYING %{buildroot}%{_datadir}/license/libbluetooth-devel

install -D -m 0755 %SOURCE101 %{buildroot}%{_bindir}/obex-root-setup
install -D -m 0755 %SOURCE102 %{buildroot}%{_sysconfdir}/obex/root-setup.d/000_create-symlinks

install -D -m 0644 src/bluetooth.service %{buildroot}%{_libdir}/systemd/system/bluetooth.service

%post
ln -sf %{_libdir}/systemd/system/bluetooth.service %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/

%post -n libbluetooth3 -p /sbin/ldconfig

%postun -n libbluetooth3 -p /sbin/ldconfig


%files
%manifest bluez.manifest
%defattr(-,root,root,-)
#%{_sysconfdir}/bluetooth/audio.conf
%{_sysconfdir}/bluetooth/main.conf
%{_sysconfdir}/bluetooth/network.conf
#%{_sysconfdir}/bluetooth/rfcomm.conf
%{_sysconfdir}/dbus-1/system.d/bluetooth.conf
%{_datadir}/man/*/*
%{_libexecdir}/bluetooth/bluetoothd
%{_bindir}/hciconfig
%{_bindir}/hciattach
%exclude %{_bindir}/ciptool
%{_bindir}/l2ping
%{_bindir}/sdptool
#%{_bindir}/gatttool
#%{_bindir}/btgatt-client
#%{_bindir}/mcaptest
%{_bindir}/mpris-proxy
%{_bindir}/rfcomm
%{_bindir}/hcitool
%{_bindir}/btmon
%{_bindir}/btsnoop
%dir %{_libdir}/bluetooth/plugins
%dir %{_localstatedir}/lib/bluetooth
%dir %{_libexecdir}/bluetooth
%{_datadir}/license/bluez
%{_libdir}/udev/hid2hci
%{_libdir}/udev/rules.d/97-hid2hci.rules
%{_libdir}/systemd/system/bluetooth.service

%files -n libbluetooth3
%defattr(-,root,root,-)
%{_libdir}/libbluetooth.so.*
%{_datadir}/license/libbluetooth3


%files -n libbluetooth-devel
%defattr(-, root, root)
%{_includedir}/bluetooth/*
%{_libdir}/libbluetooth.so
%{_libdir}/pkgconfig/bluez.pc
%{_datadir}/license/libbluetooth-devel

%files -n obexd
#%dir %{_libdir}/obex/plugins
%manifest obexd.manifest
%defattr(-,root,root,-)
%{_libexecdir}/bluetooth/obexd
%{_datadir}/dbus-1/services/org.bluez.obex.service
%{_sysconfdir}/obex/root-setup.d/000_create-symlinks
%{_bindir}/obex-root-setup
%{_libdir}/systemd/user/obex.service

%files -n bluez-test
%defattr(-,root,root,-)
%{_libdir}/bluez/test/*
%{_bindir}/l2test
%{_bindir}/rctest
%{_bindir}/bccmd
%{_bindir}/bluetoothctl
%{_bindir}/hcidump
%{_bindir}/bluemoon
%{_bindir}/hex2hcd
