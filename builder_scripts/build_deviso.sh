#!/bin/sh

# $Id$

#set -e -u

. ./pfsense_local.sh

# Suck in script helper functions
. ./builder_common.sh

# Output build flags
print_flags

# Set extra before pfsense_local.sh will do
# Add comconsole to the list
# export EXTRA="comconsole customroot"
export EXTRA="customroot"

if [ $pfSense_version = "6" ]; then
	export MAKE_CONF="${PWD}/conf/make.conf.developer"
	export MAKE_CONF_INSTALL="${PWD}/conf/make.conf.developer"	
fi
if [ $pfSense_version = "7" ]; then
	export MAKE_CONF="${PWD}/conf/make.conf.developer.7"
	export SRC_CONF="${PWD}/conf/make.conf.developer.7"
fi

export PRUNE_LIST=""

# Use pfSense.6 as kernel configuration file
export DEVIMAGE=yo
if [ $pfSense_version = "6" ]; then
	export KERNELCONF=${KERNELCONF:-"${PWD}/conf/pfSense_Dev.6"}
fi
if [ $pfSense_version = "7" ]; then
	export KERNELCONF=${KERNELCONF:-"${PWD}/conf/pfSense_Dev.7"}
fi

# Check if the world and kernel are already built and remove
# lock files accordingly

objdir=${MAKEOBJDIRPREFIX:-/usr/obj}
build_id_w=`basename ${KERNELCONF}`
build_id_k=${build_id_w}

# If PFSENSE_DEBUG is set, remove the ${build_id_k}.DEBUG file
if [ ! -z "${PFSENSE_DEBUG:-}" -a -f ${KERNELCONF}.DEBUG ]; then
    build_id_k=${build_id_w}.DEBUG
fi

if [ -f "${objdir}/${build_id_w}.world.done" ]; then
    #rm -f ${objdir}/${build_id_w}.world.done
    #rm -rf /usr/obj*
fi

# Suck in script helper functions
. ./builder_common.sh

# Make sure cvsup_current has been run first 
check_for_clog

# Allow old CVS_CO_DIR to be deleted later
chflags -R noschg $CVS_CO_DIR

# Use pfSense.6 as kernel configuration file
if [ $pfSense_version = "6" ]; then
	export KERNELCONF=${KERNELCONF:-"${PWD}/conf/pfSense_Dev.6"}
fi
if [ $pfSense_version = "7" ]; then
	export KERNELCONF=${KERNELCONF:-"${PWD}/conf/pfSense_Dev.7"}
fi

# Add etcmfs and rootmfs to the EXTRA plugins used by freesbie2
export EXTRA="${EXTRA:-} rootmfs varmfs etcmfs"

unset NO_UNIONFS
export UNION_DIRS="usr"

# Clean out directories
freesbie_make cleandir

# Checkout a fresh copy from pfsense cvs depot
update_cvs_depot

# Calculate versions
export version_kernel=`cat $CVS_CO_DIR/etc/version_kernel`
export version_base=`cat $CVS_CO_DIR/etc/version_base`
export version=`cat $CVS_CO_DIR/etc/version`

if [ $pfSense_version = "7" ]; then
	recompile_pfPorts
fi

# Build world, kernel and install
echo ">>> Building world and kernels... $pfSense_version  $freebsd_branch ..."
make_world

# Build SMP, Embedded (wrap) and Developers edition kernels
echo ">>> Building all extra kernels... $pfSense_version  $freebsd_branch ..."
build_all_kernels

# Add extra pfSense packages
echo ">>> Phase install_custom_packages"
install_custom_packages
echo ">>> Phase set_image_as_cdrom"
set_image_as_cdrom

# Fixup library changes if needed
fixup_libmap

# Nuke the boot directory
# [ -d "${CVS_CO_DIR}/boot" ] && rm -rf ${CVS_CO_DIR}/boot

rm -f $BASE_DIR/tools/builder_scripts/conf/packages

echo ">>> Searching for packages..."
set +e # grep could fail
(cd /var/db/pkg && ls | grep bsdinstaller) > $BASE_DIR/tools/builder_scripts/conf/packages
(cd /var/db/pkg && ls | grep lighttpd) >> $BASE_DIR/tools/builder_scripts/conf/packages
(cd /var/db/pkg && ls | grep lua) >> $BASE_DIR/tools/builder_scripts/conf/packages
(cd /var/db/pkg && ls | grep grub) >> $BASE_DIR/tools/builder_scripts/conf/packages
set -e

echo ">>> Installing packages: " 
cat $BASE_DIR/tools/builder_scripts/conf/packages

rm -f $MAKEOBJDIRPREFIX/usr/home/pfsense/freesbie2/*pkginstall*

# Install custom packages
echo ">>> Installing custom packageas..."
freesbie_make pkginstall

# Add extra files such as buildtime of version, bsnmpd, etc.
echo ">>> Phase populate_extra..."
populate_extra

# Overlay pfsense checkout on top of FreeSBIE image
# using the customroot plugin
echo ">>> Merging extra items..."
freesbie_make extra

# Overlay host binaries
overlay_host_binaries
check_for_zero_size_files

# Prepare /usr/local/pfsense-clonefs
echo ">>> Cloning filesystem..."
freesbie_make clonefs

# Finalize iso
echo ">>> Finalizing iso..."
freesbie_make iso

report_zero_sized_files
