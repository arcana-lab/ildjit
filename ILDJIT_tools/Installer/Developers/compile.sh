#!/bin/bash

#---------- Configuration: modify this to adapt it to your system -------
INSTALL_DIR=$HOME/ildjit
MAKE_FLAG="-j5"
#------------------------------------------------------------------------

# ----------- Input parameter handling ----------------------
if test $# -eq 1; then
	CONFIG_FILE=$1
else		
	CONFIG_FILE="compile_modules"
fi
echo "Using \"$CONFIG_FILE\" as the configuration file"


#----------- Platform auto-detection ----
if test `uname -m` = "arm"; then
	echo -e "\nARM platform detected"
	#PLATFORM_CONFIG_OPTIONS="CFLAGS=\"-mcpu=arm920 -msoft-float -mapcs-frame \""
	PLATFORM_CONFIG_OPTIONS="CFLAGS=\"-mcpu=arm920 -mapcs-frame -U__SOFTFP__\""
elif test `uname -m` = "x86_64"; then
	echo -e "\nx86_64 platform detected"
	PLATFORM_CONFIG_OPTIONS="CFLAGS=\"-m32\" CXXFLAGS=\"-m32\""
else
	PLATFORM_CONFIG_OPTIONS=""
fi
#----------------------------------------


CONFIGURE_OPTIONS="--prefix=$INSTALL_DIR $PLATFORM_CONFIG_OPTIONS"

#---------- Configuration macros --------
STANDARD="$CONFIGURE_OPTIONS"
# in the debug the STANDARD is expanded in a second step 
DEBUG="STANDARD --enable-debug"
PROFILE="STANDARD --enable-profile"
#----------------------------------------


while read i 
do
	directory=${i%%:*} #Everything up to colon
	option=${i#*:} #Everything after colon

	if test "$option" = "$directory"; then
	  	option="STANDARD"
	fi;

	#Recursively expand macros
	old_option=""
	while [ "$option" != "$old_option" ]
	do
		old_option=$option
		option=${option/DEBUG/"$DEBUG"}
		option=${option/STANDARD/"$STANDARD"}
		option=${option/PROFILING/"$PROFILING"}

	done;

	cd $directory
		
	make distclean

	eval ./configure $option && make $MAKE_FLAG && make install

	
	if test $? -ne 0 ; then
	    echo "ERROR while compiling " "$directory{$option}";
	    exit 1;
	fi;	
	cd ..

	echo "**********"	
	echo "********** $directory done !!!"
	echo "**********"	
done < $CONFIG_FILE;
echo All done
exit

