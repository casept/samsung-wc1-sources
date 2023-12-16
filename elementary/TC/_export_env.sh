# Customize below path information 
#TET_INSTALL_PATH=/scratchbox/TETware

CURRENT_USER=`echo $HOME`
TET_INSTALL_PATH=$CURRENT_USER/sbs/TETware

TET_SIMUL_PATH=$TET_INSTALL_PATH/tetware-simulator
TET_TARGET_PATH=$TET_INSTALL_PATH/tetware-target
TET_MOUNTED_PATH=/mnt/nfs/sbs/TETware/tetware-target
#TET_MOUNTED_PATH=/opt/home/root/tmp/sbs/TETware/tetware-target

#MACHINE=`echo $SBOX_UNAME_MACHINE`

MACHINE=`echo $DEB_BUILD_ARCH_ABI`

if [ $MACHINE = "gnu" ]		# SBS i386
then			
	export ARCH=simulator
	export TET_ROOT=$TET_SIMUL_PATH
elif [ $MACHINE = "gnueabi" ]	# SBS ARM
then
	export ARCH=target
	export TET_ROOT=$TET_TARGET_PATH
else
	export ARCH=target
	export TET_ROOT=$TET_MOUNTED_PATH
fi

export PATH=$TET_ROOT/bin:$PATH
export LD_LIBRARY_PATH=$TET_ROOT/lib/tet3:$LD_LIBRARY_PATH

set $(pwd)
export TET_SUITE_ROOT=$1

set $(date +%y%m%d_%H%M%S)
FILE_NAME_EXTENSION=$1
