#sbs-git:slp/pkgs/g/gst-plugins-good0.10 gst-plugins-good 0.10.31 6e8625ba6fe94fb9d09e6c3be220b54ffaa01273
Name:       gst-plugins-good
Summary:    GStreamer plugins from the "good" set
Version:    0.10.43
Release:    1
VCS:        framework/multimedia/gst-plugins-good0.10#gst-plugins-good-0.10.33-86-17-g0afc72a030537b095e6cf8cfff007a376a68736f
Group:      Applications/Multimedia
License:    LGPLv2+
Source0:    %{name}-%{version}.tar.gz
#Patch0 :    gst-plugins-good-divx-drm.patch
#Patch1 :    gst-plugins-good-ebml-read.patch
#Patch2 :    gst-plugins-good-gstrtph264pay.patch
#Patch3 :    gst-plugins-good-mkv-demux.patch
#Patch4 :    gst-plugins-good-parse-frame.patch
#Patch5 :    gst-plugins-good-qtdemux.patch
Patch6 :    gst-plugins-good-disable-gtk-doc.patch
BuildRequires:  gettext
BuildRequires:  which
BuildRequires:  gst-plugins-base-devel  
BuildRequires:  libjpeg-turbo-devel
BuildRequires:  pkgconfig(gstreamer-0.10) 
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(liboil-0.3)
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(libsoup-2.4)
BuildRequires:  pkgconfig(libpulse)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xfixes)
BuildRequires:  pkgconfig(xdamage)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(iniparser)

%description
GStreamer is a streaming media framework, based on graphs of filters
which operate on media data.  Applications using this library can do
anything from real-time sound processing to playing videos, and just
about anything else media-related.  Its plugin-based architecture means
that new data types or processing capabilities can be added simply by
installing new plug-ins.
This package contains the GStreamer plugins from the "good" set, a set
of good-quality plug-ins under the LGPL license.


%prep
%setup -q 
#%patch0 -p1
#%patch1 -p1
#%patch2 -p1
#%patch3 -p1
#%patch4 -p1
#%patch5 -p1
%patch6 -p1

%build
./autogen.sh

export CFLAGS+=" -Wall -g -fPIC\
 -DGST_EXT_SOUP_MODIFICATION \
 -DGST_EXT_RTSPSRC_MODIFICATION \
 -DGST_EXT_AMRPARSER_MODIFICATION \
 -DGST_EXT_AACPARSER_MODIFICATION \
 -DGST_EXT_SS_TYPE \
 -DGST_EXT_MPEGAUDIO_MODIFICATION\
 -DGST_EXT_ENHANCEMENT\
 -D_LITEW_OPT_"

%configure --prefix=%{_prefix}\
 --disable-static\
%ifarch %{arm}
 --enable-divx-drm\
%endif
 --disable-nls\
 --with-html-dir=/tmp/dump\
 --disable-examples\
 --disable-gconftool\
 --disable-alpha\
 --disable-apetag\
 --disable-audiofx\
 --disable-auparse\
 --disable-cutter\
 --disable-debugutils\
 --disable-deinterlace\
 --disable-effectv\
 --disable-equalizer\
 --disable-icydemux\
 --disable-flx\
 --disable-goom\
 --disable-goom2k1\
 --disable-level\
 --disable-monoscope\
 --disable-replaygain\
 --disable-smpte\
 --disable-spectrum\
 --disable-videobox\
 --disable-videomixer\
 --disable-y4m\
 --disable-directsound\
 --disable-oss\
 --disable-sunaudio\
 --disable-osx_audio\
 --disable-osx_video\
 --disable-aalib\
 --disable-aalibtest\
 --disable-annodex\
 --disable-cairo\
 --disable-esd\
 --disable-esdtest\
 --disable-flac\
 --disable-gconf\
 --disable-hal\
 --disable-libcaca\
 --disable-libdv\
 --disable-dv1394\
 --disable-shout2\
 --disable-speex\
 --disable-taglib\
 --disable-avi\
 --disable-flv\
 --disable-imagefreeze\
 --disable-law\
 --disable-matroska\
 --disable-multifile\
 --disable-rtp\
 --disable-rtpmanager\
 --disable-rtsp\
 --disable-shapewipe\
 --disable-udp\
 --disable-videocrop\
 --disable-oss4\
 --disable-x\
 --disable-xshm\
 --disable-xvideo\
 --disable-cairo_gobject\
 --disable-gdk_pixbuf\
 --disable-jack\
 --disable-libpng\
 --disable-taglib\
 --disable-wavpack\
 --disable-zlib\
 --disable-bz2


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}
%make_install




%files
%manifest gst-plugins-good.manifest
%defattr(-,root,root,-)
%{_libdir}/gstreamer-0.10
#%{_libdir}/gstreamer-0.10/libgstavi.so
#%{_libdir}/gstreamer-0.10/libgstrtsp.so
%{_libdir}/gstreamer-0.10/libgstisomp4.so
#%{_libdir}/gstreamer-0.10/libgstvideocrop.so
%{_libdir}/gstreamer-0.10/libgstid3demux.so
%{_libdir}/gstreamer-0.10/libgstpulse.so
#%{_libdir}/gstreamer-0.10/libgstmultifile.so
#%{_libdir}/gstreamer-0.10/libgstpng.so
#%{_libdir}/gstreamer-0.10/libgstflv.so
#%{_libdir}/gstreamer-0.10/libgstudp.so
#%{_libdir}/gstreamer-0.10/libgstximagesrc.so
#%{_libdir}/gstreamer-0.10/libgstalaw.so
#%{_libdir}/gstreamer-0.10/libgstrtpmanager.so
%{_libdir}/gstreamer-0.10/libgstaudioparsers.so
#%{_libdir}/gstreamer-0.10/libgstimagefreeze.so
#%{_libdir}/gstreamer-0.10/libgstjpeg.so
%{_libdir}/gstreamer-0.10/libgstautodetect.so
%{_libdir}/gstreamer-0.10/libgstvideofilter.so
#%{_libdir}/gstreamer-0.10/libgstmatroska.so
#%{_libdir}/gstreamer-0.10/libgstmulaw.so
#%{_libdir}/gstreamer-0.10/libgstrtp.so
%{_libdir}/gstreamer-0.10/libgstwavparse.so
%{_libdir}/gstreamer-0.10/libgstwavenc.so
%{_libdir}/gstreamer-0.10/libgstvideo4linux2.so
#%{_libdir}/gstreamer-0.10/libgstshapewipe.so
/usr/share/license/%{name}
#%{_libdir}/gstreamer-0.10/libgstoss4audio.so
%{_libdir}/gstreamer-0.10/libgstsouphttpsrc.so
