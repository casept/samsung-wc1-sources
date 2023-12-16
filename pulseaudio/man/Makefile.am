# This file is part of PulseAudio.
#
# PulseAudio is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# PulseAudio is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with PulseAudio; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA.

DISTCLEANFILES = \
	$(noinst_DATA)

noinst_DATA = \
	pulseaudio.1.xml \
	pax11publish.1.xml \
	paplay.1.xml \
	pacat.1.xml \
	pacmd.1.xml \
	pactl.1.xml \
	padsp.1.xml \
	pulse-daemon.conf.5.xml \
	pulse-client.conf.5.xml \
	default.pa.5.xml \
	pulse-cli-syntax.5.xml

if HAVE_ESD_COMPAT
noinst_DATA += \
	esdcompat.1.xml
endif

if HAVE_PASUSPENDER
noinst_DATA += \
	pasuspender.1.xml
endif

if HAVE_X11
noinst_DATA += \
	start-pulseaudio-kde.1.xml \
	start-pulseaudio-x11.1.xml
endif

xmllint: $(noinst_DATA)
	for f in $(noinst_DATA) ; do \
			xmllint --noout --valid "$$f" || exit 1 ; \
	done

if BUILD_MANPAGES

dist_man_MANS = \
	pulseaudio.1 \
	pax11publish.1 \
	paplay.1 \
	pacat.1 \
	pacmd.1 \
	pactl.1 \
	padsp.1 \
	pulse-daemon.conf.5 \
	pulse-client.conf.5 \
	default.pa.5 \
	pulse-cli-syntax.5

if HAVE_ESD_COMPAT
dist_man_MANS += \
	esdcompat.1
endif

if HAVE_PASUSPENDER
dist_man_MANS += \
	pasuspender.1
endif

if HAVE_X11
dist_man_MANS += \
	start-pulseaudio-kde.1 \
	start-pulseaudio-x11.1
endif

CLEANFILES = \
	$(dist_man_MANS)

%: %.xml Makefile
	$(AM_V_GEN) perl $(srcdir)/xmltoman $< > $@ || rm -f $@

endif

EXTRA_DIST = \
	pulseaudio.1.xml.in \
	pax11publish.1.xml.in \
	paplay.1.xml.in \
	pacat.1.xml.in \
	pacmd.1.xml.in \
	pactl.1.xml.in \
	padsp.1.xml.in \
	pulse-daemon.conf.5.xml.in \
	pulse-client.conf.5.xml.in \
	default.pa.5.xml.in \
	pulse-cli-syntax.5.xml.in \
	xmltoman \
	xmltoman.css \
	xmltoman.xsl \
	xmltoman.dtd

if HAVE_ESD_COMPAT
EXTRA_DIST += \
	esdcompat.1.xml.in
endif

if HAVE_PASUSPENDER
EXTRA_DIST += \
	pasuspender.1.xml.in
endif

if HAVE_X11
EXTRA_DIST += \
	start-pulseaudio-kde.1.xml.in \
	start-pulseaudio-x11.1.xml.in
endif
