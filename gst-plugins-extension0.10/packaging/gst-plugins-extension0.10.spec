Name:       gst-plugins-extension0.10
Summary:    GStreamer extra plugins
Version:    0.2.94
Release:    0
VCS:        magnolia/framework/multimedia/gst-plugins-extension0.10#gst-plugins-extension0.10-0.2.29-0-138-g727bcef71fb0e4da4ec19a589fa090af35e9ab51
Group:      TO_BE/FILLED_IN
License:    TO_BE/FILLED_IN
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  prelink
BuildRequires:  gst-plugins-base-devel
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(libcrypto)

%description
Description: GStreamer extra plugins

%package -n gstreamer0.10-plugins-extension
Summary:    GStreamer extra plugins (common)
Group:      TO_BE/FILLED_IN

%description -n gstreamer0.10-plugins-extension
Description: GStreamer extra plugins (common)

%prep
%setup -q

%build
./autogen.sh
CFLAGS=" %{optflags} -DGST_EXT_XV_ENHANCEMENT -DGST_EXT_TIME_ANALYSIS -DGST_EXT_HLS_PROXY_ROUTINE -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "; export CFLAGS
%configure \
%ifarch %{arm}
    --disable-i386 \
%else
    --enable-i386                      \
%endif


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/gstreamer0.10-plugins-extension

%make_install

mkdir -p %{buildroot}/usr/etc/
install -m 755 Others/hlsdemux2/predefined_frame/* %{buildroot}/usr/etc/



%files -n gstreamer0.10-plugins-extension
%manifest gstreamer0.10-plugins-extension.manifest
%defattr(-,root,root,-)
%{_libdir}/gstreamer-0.10/libgsthlsdemux2.so
/usr/etc/blackframe_QVGA.264
/usr/etc/sec_audio_fixed_qvga.264
/usr/etc/sec_audio_fixed_qvga.jpg
/usr/share/license/gstreamer0.10-plugins-extension
