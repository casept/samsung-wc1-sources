MAINTAINERCLEANFILES = Makefile.in

EET_EET = @eet_eet@

EXTRA_DIST = \
        profile.desktop \
        icon.png \
	base.src \
	theme.src

filesdir = $(datadir)/elementary/config/mobile-2.2
files_DATA = \
        profile.desktop \
        icon.png \
	base.cfg \
	theme.cfg

%.cfg: %.src
	$(EET_EET) -e \
	$(top_builddir)/config/mobile-2.2/$@ config \
	$< 1

clean-local:
	rm -rf *.cfg *~ *.in
	
