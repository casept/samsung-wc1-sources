#!/bin/sh

PACKAGE=scim
SRCROOT=../..
POTFILES=POTFILES.in

ALL_LINGUAS="hy az eu bg ca zh_CN zh_HK zh_TW hr cs da nl_NL en en_US et fi fr_FR gl ka de_DE el_GR hu is ga it_IT ja_JP kk ko_KR lv lt mk nb pl pt_PT pt_BR ro ru_RU sr sk sl es_ES es_US es_MX sv tr_TR uk uz ar zh_SG hi en_PH fr_CA fa th ur vi mn_MN as bn gu kn ml mr ne or pa si ta te tl id km lo ms my"

XGETTEXT=/usr/bin/xgettext
MSGMERGE=/usr/bin/msgmerge

echo -n "Make ${PACKAGE}.pot  "
if [ ! -e $POTFILES ] ; then
	echo "$POTFILES not found"
	exit 1
fi

$XGETTEXT --default-domain=${PACKAGE} --directory=${SRCROOT} \
		--add-comments --keyword=_ --keyword=N_ --files-from=$POTFILES \
&& test ! -f ${PACKAGE}.po \
	|| (rm -f ${PACKAGE}.pot && mv ${PACKAGE}.po ${PACKAGE}.pot)

if [ $? -ne 0 ]; then
	echo "error"
	exit 1
else
	echo "done"
fi

for LANG in $ALL_LINGUAS; do 
	echo "$LANG : "

	if [ ! -e $LANG.po ] ; then
		cp ${PACKAGE}.pot ${LANG}.po
		echo "${LANG}.po created"
	else
		if $MSGMERGE ${LANG}.po ${PACKAGE}.pot -o ${LANG}.new.po ; then
			if cmp ${LANG}.po ${LANG}.new.po > /dev/null 2>&1; then
				rm -f ${LANG}.new.po
			else
				if mv -f ${LANG}.new.po ${LANG}.po; then
					echo ""	
				else
					echo "msgmerge for $LANG.po failed: cannot move $LANG.new.po to $LANG.po" 1>&2
					rm -f ${LANG}.new.po
					exit 1
				fi
			fi
		else
			echo "msgmerge for $LANG failed!"
			rm -f ${LANG}.new.po
		fi
	fi
	echo ""
done

