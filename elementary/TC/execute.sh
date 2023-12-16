#export MACHINE=`echo $SBOX_UNAME_MACHINE`
export MACHINE=`echo $DEB_BUILD_ARCH_ABI`
#export MACHINE="gnueabi"

if [ $MACHINE = "gnu" ]
then
	TET_SCEN_FILE=tet_scen_i386
else
	TET_SCEN_FILE=tet_scen_arm
fi

TET_SCEN_NAME=all
RESULT_TO_JOURNAL="false"

if [ $# -eq 0 ]
then
	RESULT_TO_JOURNAL="false"
	TET_SCEN_NAME="tet_scen"	
fi

SCEN_FILE_INPUT="false"
args_count=0

for i in $*
do
	args_count=`expr $args_count + 1`
	if [ $SCEN_FILE_INPUT = "true" ]
	then
		TET_SCEN_FILE=$i
		SCEN_FILE_INPUT="false"
	elif [ $i = "-j" ]
	then
		RESULT_TO_JOURNAL="true"
	elif [ $i = "-s" ]
	then
		SCEN_FILE_INPUT="true"
	elif [ $args_count = $# ]
	then
		TET_SCEN_NAME=$i
	fi
done

#Export the path information
. ./_export_env.sh

echo TET_ROOT=$TET_ROOT
echo TET_SUITE_ROOT=$TET_SUITE_ROOT
echo TET_SCEN_FILE=$TET_SCEN_FILE
echo TET_SCEN_NAME=$TET_SCEN_NAME
echo RESULT_TO_JOURNAL=$RESULT_TO_JOURNAL

RESULT_DIR=result

if [ $MACHINE = "gnu" ]
then
	TEXT_RESULT=$RESULT_DIR/EXE-i386-$TET_SCEN_NAME-$FILE_NAME_EXTENSION.html
	JOURNAL_RESULT=$RESULT_DIR/EXE-i386-$TET_SCEN_NAME-$FILE_NAME_EXTENSION.journal
else
	TEXT_RESULT=$RESULT_DIR/EXE-ARM-$TET_SCEN_NAME-$FILE_NAME_EXTENSION.html
	JOURNAL_RESULT=$RESULT_DIR/EXE-ARM-$TET_SCEN_NAME-$FILE_NAME_EXTENSION.journal
fi

### Make Result output directory
echo
echo "$RESULT_DIR Folder Creat"
if [ -e $RESULT_DIR ]
then
	echo "  -> $RESULT_DIR Folder exist"
else
	mkdir $RESULT_DIR
fi

##execute and mkae html report
if [ $RESULT_TO_JOURNAL = "false" ]
then
	tcc -e -p -j - -s $TET_SCEN_FILE ./ $TET_SCEN_NAME
else
	tcc -e -j $JOURNAL_RESULT -p -s $TET_SCEN_FILE ./ $TET_SCEN_NAME
	grw -c 3 -f chtml -o $TEXT_RESULT -- $JOURNAL_RESULT
	echo RESULT_SUMMARY     = $TEXT_RESULT
	echo RESULT_JOURNAL  = $JOURNAL_RESULT
fi
	rm -rf ./results
