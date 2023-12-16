#!/bin/sh

### WARNING: DO NOT CHANGE CODES from HERE !!! ###
#import setup
cd `dirname $0`
_PWD=`pwd`
pushd ./ > /dev/null
while [ ! -f "./xo-setup.conf" ]
do
		cd ../
		SRCROOT=`pwd`
		if [ "$SRCROOT" == "/" ]; then
				echo "Cannot find xo-setup.conf !!"
				exit 1
		fi
done
popd > /dev/null
. ${SRCROOT}/xo-setup.conf
cd ${_PWD}
### WARNING: DO NOT CHANGE CODES until HERE!!! ###

export VERSION=1.0
CFLAGS="${CFLAGS}"


if [ "$ARCH" == "arm" ]; then
	CFLAGS="${CFLAGS} 			\
	-DGST_EXT_TIME_ANALYSIS 		\
	-DGST_EXT_TA_UNIT_MEXT 			\
	-DGST_EXT_ASYNC_DEV		\
	-DGST_EXT_USE_PDP_NETWORK		\
	-DGST_EXT_SWITCH_CAMERA			\
	-DGST_EXT_OVERLAYSINK_SQVGA		\
	-DGST_EXT_OVERLAYSINK_MONAHANS		\
	-DGST_EXT_USE_EXT_ARICENT		\
	-DGST_EXT_USE_EXT_AVSYSAUDIO		\
    -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\"	\
	-D_MMCAMCORDER_MERGE_TEMP"



   	##############################
   	#  Select one for camera type
   	##############################
   	# -DUSE_CAMERA_NEC_5M
   	# -DUSE_CAMERA_FUJITSU_5M
   	# -DUSE_CAMERA_SAMSUNG_5M
   	##############################
	if [[ "$CAMERA_TYPE" == "NEC_5M_CAM" ]];
	then
		CFLAGS="${CFLAGS} \
		-DUSE_CAMERA_NEC_5M"
	elif [[ "$CAMERA_TYPE" == "FUJITSU_5M_CAM" ]];
	then
		CFLAGS="${CFLAGS} \
		-DUSE_CAMERA_FUJITSU_5M"
	elif [[ "$CAMERA_TYPE" == "SAMSUNG_3M_CAM" ]];
	then
		CFLAGS="${CFLAGS} \
		-DUSE_CAMERA_SAMSUNG_3M"
	fi

	### ARM_PROTECTOR_VODA ###
	if [[ "x$MACHINE" == "xprotector" && "x$DISTRO" == "xvodafone" ]];
	then
	        CFLAGS="${CFLAGS} \
	        -D_MM_PROJECT_PROTECTOR_DNSE "

	### ARM_VOLANS_VODA ###
	elif [[ "x$MACHINE" == "xvolans" && "x$DISTRO" == "xvodafone" ]];
	then
	        CFLAGS="${CFLAGS} \
	        -D_MM_PROJECT_VOLANS_DNSE "
												        ### ELSE ###
        fi
																							

else
	CFLAGS="${CFLAGS} 			\
	-DGST_EXT_SWITCH_CAMERA			\
    -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\"	\
	-D_MMCAMCORDER_MERGE_TEMP		\
	-D_MMFW_I386_ALL_SIMULATOR"
fi

if [ $1 ];
then
	run make $1 || exit 1
else
	if [ -z "$USE_AUTOGEN" ]; then
		run ./autogen.sh || exit 1
	
		if [ "$ARCH" == "arm" ];
		then
		BUILDOPTION="\
		--disable-extpar-amr			\
		--disable-ext-mp3			\
		--disable-ext-overlay			\
		--disable-extdec-aac 			\
		--disable-extdec-mp3 			\
		--disable-extdec-ogg 			\
		--disable-extenc-amr 			\
		--disable-extdec-amr 			\
		--disable-extdec-wma 			\
		--disable-extdec-h263-hw 		\
		--disable-extdec-h264-hw 		\
		--disable-extdec-mpeg4-hw 		\
		--disable-extenc-h263-hw 		\
		--disable-extenc-mpeg4-hw 		\
		--disable-ext-httpsrc 			\
		--disable-ext-overlay 			\
		--disable-ext-ippjpegenc 		\
		--disable-ext-exifwriter 		\
		--disable-ext-soundeq 			\
		--disable-ext-soundreverb 		\
		--disable-ext-sound3d 			\
		--disable-ext-ippjpegenc 		\
		--disable-ext-ippvideoscale 		\
		--disable-ext-ppvideoscale		\
		--enable-ext-secrtspsrc                 \
		--enable-ext-asfdemux"	
		else
		BUILDOPTION="\
		--disable-extdec-aac 			\
		--disable-extdec-mp3 			\
		--disable-extdec-ogg 			\
		--disable-extenc-amr 			\
		--disable-extdec-amr 			\
		--disable-ext-soundreverb		\
		--disable-ext-dnse 			\
		--disable-ext-plugin_ns	 		\
		--disable-ext-sound3d 			\
		--disable-ext-soundeq 			\
		--disable-extdec-wma 			\
		--disable-extdec-h263-hw 		\
		--disable-extdec-h264-hw 		\
		--disable-extdec-mpeg4-hw 		\
		--disable-extenc-h263-hw 		\
		--disable-extenc-mpeg4-hw 		\
		--disable-ext-httpsrc 			\
		--disable-ext-overlay 			\
		--disable-ext-ippjpegenc 		\
		--disable-ext-exifwriter 		\
		--disable-ext-soundreverb 		\
		--disable-ext-ippjpegenc 		\
		--disable-ext-ippvideoscale 		\
		--disable-ext-aricent 			\
		--disable-ext-ppvideoscale		\
		--disable-ext-imagereader		\
		--disable-ext-queuereader		\
		--disable-ext-queuewriter		\
		--disable-ext-toggle			\
		--disable-ext-videomute			\
		--disable-ext-gif			\
		--disable-ext-jpeg			\
		--disable-ext-mp3			\
		--disable-ext-png			\
		--disable-ext-secrtspsrc"	
	
		fi
	
		# if PROTECTOR_VODA_SDK, disable all plugin except "avsystem" and "mpegaudioparse"
		if [[ "x$MACHINE" == "xprotector" && "x$DISTRO" == "xvodafone-sdk" ]];
		then
			BUILDOPTION=" \
			  --disable-extdec-aac    --disable-extdec-mp3       --disable-extdec-ogg \
			  --disable-extdec-wma	    --disable-extenc-amr     --disable-extdec-amr      --disable-extpar-amr \
			  --disable-extdec-h263-hw  --disable-extdec-h264-hw  --disable-extdec-mpeg4-hw  --disable-extenc-h263-hw \
			  --disable-extenc-mpeg4-hw --disable-ext-httpsrc     --disable-ext-overlay      --disable-ext-ippjpegenc \
			  --disable-ext-dnse    --disable-ext-sound3d     --disable-ext-soundeq      --disable-ext-soundreverb \
			  --disable-ext-exifwriter   --disable-ext-ippvideoscale     --disable-ext-ppvideoscale \
		        --disable-ext-aricent	  --disable-ext-imagereader    --disable-ext-midi     --disable-ext-queuereader \
		        --disable-ext-queuewriter     --disable-ext-gif   --disable-ext-jpeg    --disable-ext-mp3 \
		        --disable-ext-png  --disable-ext-toggle	    --disable-ext-videomute \
			--disable-ext-drmsrc --disable-ext-secrtspsrc"
		fi
		
		if [[ "x$MACHINE" == "xvolans" && "x$DISTRO" == "xvodafone-sdk" ]];
		then
			BUILDOPTION=" \
			  --disable-extdec-aac    --disable-extdec-mp3       --disable-extdec-ogg \
			  --disable-extdec-wma	    --disable-extenc-amr     --disable-extdec-amr      --disable-extpar-amr \
			  --disable-extdec-h263-hw  --disable-extdec-h264-hw  --disable-extdec-mpeg4-hw  --disable-extenc-h263-hw \
			  --disable-extenc-mpeg4-hw --disable-ext-httpsrc     --disable-ext-overlay      --disable-ext-ippjpegenc \
			  --disable-ext-dnse    --disable-ext-sound3d     --disable-ext-soundeq      --disable-ext-soundreverb \
			  --disable-ext-exifwriter   --disable-ext-ippvideoscale     --disable-ext-ppvideoscale \
		        --disable-ext-aricent	  --disable-ext-imagereader    --disable-ext-midi     --disable-ext-queuereader \
		        --disable-ext-queuewriter     --disable-ext-gif   --disable-ext-jpeg    --disable-ext-mp3 \
		        --disable-ext-png  --disable-ext-toggle	    --disable-ext-videomute \
			--disable-ext-drmsrc --disable-ext-secrtspsrc"
		fi
	
	
		run ./configure --prefix=$PREFIX $BUILDOPTION || exit 1
	fi
	run make || exit 1
	run make install || exit 1
	run make_pkg.sh || exit 1
fi


