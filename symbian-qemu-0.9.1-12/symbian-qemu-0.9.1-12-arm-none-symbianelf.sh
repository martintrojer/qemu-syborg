#! /usr/bin/env bash
# This file contains the complete sequence of commands
# CodeSourcery used to build this version of Sourcery G++.
# 
# For each free or open-source component of Sourcery G++, the
# source code provided includes all of the configuration
# scripts and makefiles for that component, including any and
# all modifications made by CodeSourcery.  From this list of
# commands, you can see every configuration option used by
# CodeSourcery during the build process.
# 
# This file is provided as a guideline for users who wish to
# modify and rebuild a free or open-source component of
# Sourcery G++ from source. For a number of reasons, though,
# you may not be able to successfully run this script directly
# on your system. Certain aspects of the CodeSourcery build
# environment (such as directory names) are included in these
# commands. CodeSourcery uses Canadian cross compilers so you
# may need to modify various configuration options and paths
# if you are building natively. This sequence of commands
# includes those used to build proprietary components of
# Sourcery G++ for which source code is not provided.
# 
# Please note that Sourcery G++ support covers only your use
# of the original, validated binaries provided as part of
# Sourcery G++ -- and specifically does not cover either the
# process of rebuilding a component or the use of any binaries
# you may build.  In addition, if you rebuild any component,
# you must not use the --with-pkgversion and --with-bugurl
# configuration options that embed CodeSourcery trademarks in
# the resulting binary; see the "CodeSourcery Trademarks"
# section in the Sourcery G++ Software License Agreement.
set -e
inform_fd=2 
umask 022
exec < /dev/null

error_handler ()
{
    exit 1
}

check_status() {
    local status="$?"
    if [ "$status" -ne 0 ]; then
	error_handler
    fi
}

check_pipe() {
    local -a status=("${PIPESTATUS[@]}")
    local limit=$1
    local ix
    
    if [ -z "$limit" ] ; then
	limit="${#status[@]}"
    fi
    for ((ix=0; ix != $limit ; ix++)); do
	if [ "${status[$ix]}" != "0" ] ; then
	    error_handler
	fi
    done
}

error () {
    echo "$script: error: $@" >& $inform_fd
    exit 1
}

warning () {
    echo "$script: warning: $@" >& $inform_fd
}

verbose () {
    if $gnu_verbose; then
	echo "$script: $@" >& $inform_fd
    fi
}

copy_dir() {
    mkdir -p "$2"

    (cd "$1" && tar cf - .) | (cd "$2" && tar xf -)
    check_pipe
}

copy_dir_clean() {
    mkdir -p "$2"
    (cd "$1" && tar cf - \
	--exclude=CVS --exclude=.svn --exclude=.pc \
	--exclude="*~" --exclude=".#*" \
	--exclude="*.orig" --exclude="*.rej" \
	.) | (cd "$2" && tar xf -)
    check_pipe
}

update_dir_clean() {
    mkdir -p "$2"


    (cd "$1" && tar cf - \
	--exclude=CVS --exclude=.svn --exclude=.pc \
	--exclude="*~" --exclude=".#*" \
	--exclude="*.orig" --exclude="*.rej" \
	--after-date="$3" \
	. 2> /dev/null) | (cd "$2" && tar xf -)
    check_pipe
}

copy_dir_exclude() {
    local source="$1"
    local dest="$2"
    local excl="$3"
    shift 3
    mkdir -p "$dest"
    (cd "$source" && tar cfX - "$excl" "$@") | (cd "$dest" && tar xf -)
    check_pipe
}

copy_dir_only() {
    local source="$1"
    local dest="$2"
    shift 2
    mkdir -p "$dest"
    (cd "$source" && tar cf - "$@") | (cd "$dest" && tar xf -)
    check_pipe
}

clean_environment() {
    local env_var_list
    local var




    unset BASH_ENV CDPATH POSIXLY_CORRECT TMOUT

    env_var_list=$(export | \
	grep '^declare -x ' | \
	sed -e 's/^declare -x //' -e 's/=.*//')

    for var in $env_var_list; do
	case $var in
	    CSL_SCRIPTDIR|HOME|HOSTNAME|LOGNAME|PWD|SHELL|SHLVL|SSH_*|TERM|USER)


		;;
	    LD_LIBRARY_PATH|PATH| \
		QMTEST_CLASS_PATH| \
		FLEXLM_NO_CKOUT_INSTALL_LIC|LM_APP_DISABLE_CACHE_READ)


		;;
	    MAKEINFO)

		;;
	    *_LICENSE_FILE)












		if [ "" ]; then
		    local license_file_envvar
		    license_file_envvar=

		    if [ "$var" != "$license_file_envvar" ]; then
			export -n "$var" || true
		    fi
		else
		    export -n "$var" || true
		fi
		;;
	    *)

		export -n "$var" || true
		;;
	esac
    done


    export LANG=C
    export LC_ALL=C


    export CVS_RSH=ssh



    export SHELL=$BASH
    export CONFIG_SHELL=$BASH
}

pushenv() {
    pushenv_level=$(($pushenv_level + 1))
    eval pushenv_vars_${pushenv_level}=
}


pushenv_level=0
pushenv_vars_0=



pushenvvar() {
    local pushenv_var="$1"
    local pushenv_newval="$2"
    eval local pushenv_oldval=\"\$$pushenv_var\"
    eval local pushenv_oldset=\"\${$pushenv_var+set}\"
    local pushenv_save_var=saved_${pushenv_level}_${pushenv_var}
    local pushenv_savep_var=savedp_${pushenv_level}_${pushenv_var}
    eval local pushenv_save_set=\"\${$pushenv_savep_var+set}\"
    if [ "$pushenv_save_set" = "set" ]; then
	error "Pushing $pushenv_var more than once at level $pushenv_level"
    fi
    if [ "$pushenv_oldset" = "set" ]; then
	eval $pushenv_save_var=\"\$pushenv_oldval\"
    else
	unset $pushenv_save_var
    fi
    eval $pushenv_savep_var=1
    eval export $pushenv_var=\"\$pushenv_newval\"
    local pushenv_list_var=pushenv_vars_${pushenv_level}
    eval $pushenv_list_var=\"\$$pushenv_list_var \$pushenv_var\"
}

prependenvvar() {
    local pushenv_var="$1"
    local pushenv_newval="$2"
    eval local pushenv_oldval=\"\$$pushenv_var\"
    pushenvvar "$pushenv_var" "$pushenv_newval$pushenv_oldval"
}

popenv() {
    local pushenv_var=
    eval local pushenv_vars=\"\$pushenv_vars_${pushenv_level}\"
    for pushenv_var in $pushenv_vars; do
	local pushenv_save_var=saved_${pushenv_level}_${pushenv_var}
	local pushenv_savep_var=savedp_${pushenv_level}_${pushenv_var}
	eval local pushenv_save_val=\"\$$pushenv_save_var\"
	eval local pushenv_save_set=\"\${$pushenv_save_var+set}\"
	unset $pushenv_save_var
	unset $pushenv_savep_var
	if [ "$pushenv_save_set" = "set" ]; then
	    eval export $pushenv_var=\"\$pushenv_save_val\"
	else
	    unset $pushenv_var
	fi
    done
    unset pushenv_vars_${pushenv_level}
    if [ "$pushenv_level" = "0" ]; then
	error "Popping environment level 0"
    else
	pushenv_level=$(($pushenv_level - 1))
    fi
}

prepend_path() {
    if $(eval "test -n \"\$$1\""); then
	prependenvvar "$1" "$2:"
    else
	prependenvvar "$1" "$2"
    fi
}
pushenvvar CSL_SCRIPTDIR /scratch/paul/qemu/src/scripts-trunk
pushenvvar PATH /usr/local/tools/gcc-3.4.0/bin:/bin:/usr/bin
pushenvvar LD_LIBRARY_PATH /usr/local/tools/gcc-3.4.0/lib64:/usr/local/tools/gcc-3.4.0/lib
pushenvvar QMTEST_CLASS_PATH QMTEST_CLASS_PATH
pushenvvar MAKEINFO makeinfo
clean_environment
# task [001/090] /init/dirs
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
mkdir -p /scratch/paul/qemu/obj
mkdir -p /scratch/paul/qemu/install
mkdir -p /scratch/paul/qemu/pkg
mkdir -p /scratch/paul/qemu/logs/data
mkdir -p /scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/html
mkdir -p /scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/pdf
popenv

# task [002/090] /init/cleanup
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/pkg/symbian-qemu-0.9.1-12-arm-none-symbianelf.src.tar.bz2 /scratch/paul/qemu/pkg/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup.tar.bz2
popenv

# task [003/090] /init/source_package/libsdl
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/libsdl-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/libsdl-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' libsdl-trunk
popd
popenv

# task [004/090] /init/source_package/qemu
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/qemu-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/qemu-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' qemu-symbian-svp
popd
popenv

# task [005/090] /init/source_package/libpng
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/libpng-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/libpng-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' libpng-1.2.32
popd
popenv

# task [006/090] /init/source_package/expat
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/expat-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/expat-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' expat-2.0.0
popd
popenv

# task [007/090] /init/source_package/dtc
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/dtc-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/dtc-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' dtc-trunk
popd
popenv

# task [008/090] /init/source_package/svp_docs
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/svp_docs-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/svp_docs-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' svp-docs-trunk
popd
popenv

# task [009/090] /init/source_package/python
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/python-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/python-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' python-2.6.1
popd
popenv

# task [010/090] /init/source_package/python_win32
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/python_win32-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/python_win32-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' python-win32-2.6.1
popd
popenv

# task [011/090] /init/source_package/csl_tests
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/csl_tests-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/csl_tests-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' csl-tests-trunk
popd
popenv

# task [012/090] /init/source_package/dejagnu_boards
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/dejagnu_boards-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/dejagnu_boards-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' dejagnu-boards-trunk
popd
popenv

# task [013/090] /init/source_package/scripts
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/scripts-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/scripts-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' scripts-trunk
popd
popenv

# task [014/090] /init/source_package/xfails
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/xfails-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/xfails-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' xfails-trunk
popd
popenv

# task [015/090] /init/source_package/portal
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/portal-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/portal-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' portal-trunk
popd
popenv

# task [016/090] /init/source_package/zlib
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/zlib-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf/zlib-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' zlib-1.2.3
popd
popenv

# task [017/090] /init/source_package/csl_docbook
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
rm -f /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/csl_docbook-0.9.1-12.tar.bz2
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup
pushd /scratch/paul/qemu/src
tar cf /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup/csl_docbook-0.9.1-12.tar.bz2 --bzip2 --owner=0 --group=0 --exclude=CVS --exclude=.svn --exclude=.pc '--exclude=*~' '--exclude=.#*' '--exclude=*.orig' '--exclude=*.rej' csl-docbook-trunk
popd
popenv

# task [018/090] /i686-pc-linux-gnu/host_cleanup
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
popenv

# task [019/090] /i686-pc-linux-gnu/zlib_first/copy
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
rm -rf /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
copy_dir_clean /scratch/paul/qemu/src/zlib-1.2.3 /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
chmod -R u+w /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
popenv

# task [020/090] /i686-pc-linux-gnu/zlib_first/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushd /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
pushenv
pushenvvar CC 'i686-pc-linux-gnu-gcc '
pushenvvar AR 'i686-pc-linux-gnu-ar rc'
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
./configure --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr
popenv
popd
popenv

# task [021/090] /i686-pc-linux-gnu/zlib_first/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushd /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make -j4
popd
popenv

# task [022/090] /i686-pc-linux-gnu/zlib_first/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushd /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make install
popd
popenv

# task [023/090] /i686-pc-linux-gnu/toolchain/zlib/0/copy
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
rm -rf /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
copy_dir_clean /scratch/paul/qemu/src/zlib-1.2.3 /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
chmod -R u+w /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
popenv

# task [024/090] /i686-pc-linux-gnu/toolchain/zlib/0/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushd /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
pushenv
pushenvvar CC 'i686-pc-linux-gnu-gcc '
pushenvvar AR 'i686-pc-linux-gnu-ar rc'
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
./configure --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr
popenv
popd
popenv

# task [025/090] /i686-pc-linux-gnu/toolchain/zlib/0/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushd /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make -j4
popd
popenv

# task [026/090] /i686-pc-linux-gnu/toolchain/zlib/0/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushd /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make install
popd
popenv

# task [027/090] /i686-pc-linux-gnu/toolchain/expat/0/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
mkdir -p /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
pushd /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
/scratch/paul/qemu/src/expat-2.0.0/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr --disable-shared --host=i686-pc-linux-gnu --disable-nls
popd
popenv
popenv
popenv

# task [028/090] /i686-pc-linux-gnu/toolchain/expat/0/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make -j4
popd
popenv
popenv
popenv

# task [029/090] /i686-pc-linux-gnu/toolchain/expat/0/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make install
popd
popenv
popenv
popenv

# task [030/090] /i686-pc-linux-gnu/svp_docs/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/svp_docs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
mkdir -p /scratch/paul/qemu/obj/svp_docs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
pushd /scratch/paul/qemu/obj/svp_docs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
/scratch/paul/qemu/src/svp-docs-trunk/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/opt/codesourcery --host=i686-pc-linux-gnu --disable-nls --with-release-config=symbian-qemu --with-target-arch-name=ARM --with-target-os-name=SymbianOS --with-csl-docbook=/scratch/paul/qemu/src/csl-docbook-trunk --with-version-string=0.9.1-12 '--with-pkgversion=Symbian QEMU 0.9.1-12' '--with-brand=Symbian QEMU' '--with-xml-catalog-files=/usr/local/tools/gcc-3.4.0/share/sgml/docbook/docbook-xsl/catalog.xml /etc/xml/catalog'
popd
popenv
popenv
popenv

# task [031/090] /i686-pc-linux-gnu/svp_docs/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/svp_docs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make -j4
popd
popenv
popenv
popenv

# task [032/090] /i686-pc-linux-gnu/svp_docs/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/svp_docs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make install prefix=/scratch/paul/qemu/install exec_prefix=/scratch/paul/qemu/install libdir=/scratch/paul/qemu/install/lib htmldir=/scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/html pdfdir=/scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/pdf infodir=/scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/info mandir=/scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/man datadir=/scratch/paul/qemu/install/share docdir=/scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf
popd
popenv
popenv
popenv

# task [033/090] /i686-pc-linux-gnu/libsdl/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
mkdir -p /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
pushd /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
/scratch/paul/qemu/src/libsdl-trunk/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr --disable-shared --host=i686-pc-linux-gnu --disable-nls --x-includes=/usr/local/tools/gcc-3.4.0/i686-pc-linux-gnu/sysroot/usr/lib/../X11R6/include --x-libraries=/usr/local/tools/gcc-3.4.0/i686-pc-linux-gnu/sysroot/usr/lib/../X11R6/lib --disable-dga --disable-video-dga --disable-video-dgamouse --disable-video-fbcon
popd
popenv
popenv
popenv

# task [034/090] /i686-pc-linux-gnu/libsdl/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make -j4
popd
popenv
popenv
popenv

# task [035/090] /i686-pc-linux-gnu/libsdl/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make install
popd
popenv
popenv
popenv

# task [036/090] /i686-pc-linux-gnu/libpng/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/lib
rm -rf /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
mkdir -p /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
pushd /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
/scratch/paul/qemu/src/libpng-1.2.32/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr --disable-shared --host=i686-pc-linux-gnu --disable-nls
popd
popenv
popenv
popenv

# task [037/090] /i686-pc-linux-gnu/libpng/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/lib
pushd /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make -j4
popd
popenv
popenv
popenv

# task [038/090] /i686-pc-linux-gnu/libpng/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/lib
pushd /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make install
popd
popenv
popenv
popenv

# task [039/090] /i686-pc-linux-gnu/dtc/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
mkdir -p /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
pushd /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
/scratch/paul/qemu/src/dtc-trunk/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/opt/codesourcery --host=i686-pc-linux-gnu '--with-pkgversion=Symbian QEMU 0.9.1-12' --with-bugurl=https://support.codesourcery.com/GNUToolchain/ --disable-nls '--program-transform-name=s,^,arm-none-symbianelf-,'
popd
popenv
popenv
popenv

# task [040/090] /i686-pc-linux-gnu/dtc/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make -j4
popd
popenv
popenv
popenv

# task [041/090] /i686-pc-linux-gnu/dtc/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make install prefix=/scratch/paul/qemu/install exec_prefix=/scratch/paul/qemu/install libdir=/scratch/paul/qemu/install/lib htmldir=/scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/html pdfdir=/scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/pdf infodir=/scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/info mandir=/scratch/paul/qemu/install/share/doc/symbian-qemu-arm-none-symbianelf/man datadir=/scratch/paul/qemu/install/share
popd
popenv
popenv
popenv

# task [042/090] /i686-pc-linux-gnu/python/copy
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/python-src-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
copy_dir_clean /scratch/paul/qemu/src/python-2.6.1 /scratch/paul/qemu/obj/python-src-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
chmod -R u+w /scratch/paul/qemu/obj/python-src-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
touch /scratch/paul/qemu/obj/python-src-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/.gnu-stamp
popenv
popenv
popenv

# task [043/090] /i686-pc-linux-gnu/python/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/python-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
mkdir -p /scratch/paul/qemu/obj/python-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
pushd /scratch/paul/qemu/obj/python-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
/scratch/paul/qemu/obj/python-src-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr --disable-shared --host=i686-pc-linux-gnu --disable-nls
popd
popenv
popenv
popenv

# task [044/090] /i686-pc-linux-gnu/python/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/python-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make -j4
popd
popenv
popenv
popenv

# task [045/090] /i686-pc-linux-gnu/python/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/python-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make install
popd
popenv
popenv
popenv

# task [046/090] /i686-pc-linux-gnu/python/postinstall
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
mkdir -p /scratch/paul/qemu/install/lib
rsync -a /scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/lib/python2.6 /scratch/paul/qemu/install/lib
rsync -a /scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/include/python2.6 /scratch/paul/qemu/install/include
popenv
popenv
popenv

# task [047/090] /i686-pc-linux-gnu/qemu/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
mkdir -p /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
pushd /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
/scratch/paul/qemu/src/qemu-symbian-svp/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/opt/codesourcery --host=i686-pc-linux-gnu '--with-pkgversion=Symbian QEMU 0.9.1-12' --with-bugurl=https://support.codesourcery.com/GNUToolchain/ --disable-nls --sdl-config=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/bin/sdl-config --png-config=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/bin/libpng-config --python-config=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/bin/python-config --cpu=i386 --cross-prefix=i686-pc-linux-gnu- --host-cc=i686-pc-linux-gnu-gcc --target-list=,arm-softmmu --extra-cflags=-I/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/include --extra-ldflags=-L/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu/usr/lib
popd
popenv
popenv
popenv

# task [048/090] /i686-pc-linux-gnu/qemu/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
make -j4
popd
popenv
popenv
popenv

# task [049/090] /i686-pc-linux-gnu/qemu/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu
mkdir -p /scratch/paul/qemu/install/bin
install -m 755 arm-softmmu/qemu-system-arm /scratch/paul/qemu/install/bin/arm-none-symbianelf-qemu-system
popd
pushd /scratch/paul/qemu/src/qemu-symbian-svp
mkdir -p /scratch/paul/qemu/install/share/qemu/plugins
install -m 644 plugins/qemu_arm_plugins.py /scratch/paul/qemu/install/share/qemu/plugins
install -m 644 plugins/syborg_fb.py /scratch/paul/qemu/install/share/qemu/plugins
install -m 644 plugins/syborg_interrupt.py /scratch/paul/qemu/install/share/qemu/plugins
install -m 644 plugins/syborg_keyboard.py /scratch/paul/qemu/install/share/qemu/plugins
install -m 644 plugins/syborg_pointer.py /scratch/paul/qemu/install/share/qemu/plugins
install -m 644 plugins/syborg_rtc.py /scratch/paul/qemu/install/share/qemu/plugins
install -m 644 plugins/syborg_serial.py /scratch/paul/qemu/install/share/qemu/plugins
install -m 644 plugins/syborg_timer.py /scratch/paul/qemu/install/share/qemu/plugins
popd
popenv
popenv
popenv

# task [050/090] /i686-pc-linux-gnu/finalize_libc_installation
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
popenv

# task [051/090] /i686-pc-linux-gnu/pretidy_installation
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
pushd /scratch/paul/qemu/install
popd
popenv

# task [052/090] /i686-pc-linux-gnu/remove_libtool_archives
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
find /scratch/paul/qemu/install -name '*.la' -exec rm '{}' ';'
popenv

# task [053/090] /i686-pc-linux-gnu/strip_host_objects
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
/usr/local/tools/gcc-3.4.0/bin/i686-pc-linux-gnu-strip /scratch/paul/qemu/install/bin/arm-none-symbianelf-dtc
/usr/local/tools/gcc-3.4.0/bin/i686-pc-linux-gnu-strip /scratch/paul/qemu/install/bin/arm-none-symbianelf-ftdump
/usr/local/tools/gcc-3.4.0/bin/i686-pc-linux-gnu-strip /scratch/paul/qemu/install/bin/arm-none-symbianelf-qemu-system
popenv

# task [054/090] /i686-pc-linux-gnu/package_tbz2
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-pc-linux-gnu-gcc
pushenvvar AR i686-pc-linux-gnu-ar
pushenvvar RANLIB i686-pc-linux-gnu-ranlib
prepend_path PATH /scratch/paul/qemu/install/bin
rm -f /scratch/paul/qemu/pkg/symbian-qemu-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu.tar.bz2
pushd /scratch/paul/qemu/obj
rm -f symbian-qemu-0.9.1
ln -s /scratch/paul/qemu/install symbian-qemu-0.9.1
tar cjf /scratch/paul/qemu/pkg/symbian-qemu-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu.tar.bz2 --owner=0 --group=0 --exclude=host-i686-pc-linux-gnu --exclude=host-i686-mingw32 symbian-qemu-0.9.1/bin symbian-qemu-0.9.1/include symbian-qemu-0.9.1/lib symbian-qemu-0.9.1/share
rm -f symbian-qemu-0.9.1
popd
popenv

# task [055/090] /i686-mingw32/host_cleanup
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
popenv

# task [056/090] /i686-mingw32/host_unpack
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
rm -rf /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32
mkdir /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32
pushd /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32
ln -s . symbian-qemu-0.9.1
tar xf /scratch/paul/qemu/pkg/symbian-qemu-0.9.1-12-arm-none-symbianelf-i686-pc-linux-gnu.tar.bz2 --bzip2
rm symbian-qemu-0.9.1
popd
popenv

# task [057/090] /i686-mingw32/zlib_first/copy
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
rm -rf /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-mingw32
copy_dir_clean /scratch/paul/qemu/src/zlib-1.2.3 /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-mingw32
chmod -R u+w /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-mingw32
popenv

# task [058/090] /i686-mingw32/zlib_first/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushd /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-mingw32
pushenv
pushenvvar CC 'i686-mingw32-gcc '
pushenvvar AR 'i686-mingw32-ar rc'
pushenvvar RANLIB i686-mingw32-ranlib
./configure --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr
popenv
popd
popenv

# task [059/090] /i686-mingw32/zlib_first/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushd /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-mingw32
make -j4
popd
popenv

# task [060/090] /i686-mingw32/zlib_first/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushd /scratch/paul/qemu/obj/zlib-first-0.9.1-12-arm-none-symbianelf-i686-mingw32
make install
popd
popenv

# task [061/090] /i686-mingw32/toolchain/copy_libs
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
copy_dir /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/share/doc/symbian-qemu-arm-none-symbianelf/html /scratch/paul/qemu/install/host-i686-mingw32/share/doc/symbian-qemu-arm-none-symbianelf/html
copy_dir /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/share/doc/symbian-qemu-arm-none-symbianelf/pdf /scratch/paul/qemu/install/host-i686-mingw32/share/doc/symbian-qemu-arm-none-symbianelf/pdf
popenv

# task [062/090] /i686-mingw32/toolchain/zlib/0/copy
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
rm -rf /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-mingw32
copy_dir_clean /scratch/paul/qemu/src/zlib-1.2.3 /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-mingw32
chmod -R u+w /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-mingw32
popenv

# task [063/090] /i686-mingw32/toolchain/zlib/0/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushd /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-mingw32
pushenv
pushenvvar CC 'i686-mingw32-gcc '
pushenvvar AR 'i686-mingw32-ar rc'
pushenvvar RANLIB i686-mingw32-ranlib
./configure --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr
popenv
popd
popenv

# task [064/090] /i686-mingw32/toolchain/zlib/0/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushd /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-mingw32
make -j4
popd
popenv

# task [065/090] /i686-mingw32/toolchain/zlib/0/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushd /scratch/paul/qemu/obj/zlib-0.9.1-12-arm-none-symbianelf-i686-mingw32
make install
popd
popenv

# task [066/090] /i686-mingw32/toolchain/expat/0/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-mingw32
mkdir -p /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-mingw32
pushd /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-mingw32
/scratch/paul/qemu/src/expat-2.0.0/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr --disable-shared --host=i686-mingw32 --disable-nls
popd
popenv
popenv
popenv

# task [067/090] /i686-mingw32/toolchain/expat/0/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-mingw32
make -j4
popd
popenv
popenv
popenv

# task [068/090] /i686-mingw32/toolchain/expat/0/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/expat-0.9.1-12-arm-none-symbianelf-i686-mingw32
make install
popd
popenv
popenv
popenv

# task [069/090] /i686-mingw32/libsdl/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-mingw32
mkdir -p /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-mingw32
pushd /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-mingw32
/scratch/paul/qemu/src/libsdl-trunk/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr --disable-shared --host=i686-mingw32 --disable-nls
popd
popenv
popenv
popenv

# task [070/090] /i686-mingw32/libsdl/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-mingw32
make -j4
popd
popenv
popenv
popenv

# task [071/090] /i686-mingw32/libsdl/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/libsdl-0.9.1-12-arm-none-symbianelf-i686-mingw32
make install
popd
popenv
popenv
popenv

# task [072/090] /i686-mingw32/libpng/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/include
pushenvvar LDFLAGS -L/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/lib
rm -rf /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-mingw32
mkdir -p /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-mingw32
pushd /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-mingw32
/scratch/paul/qemu/src/libpng-1.2.32/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr --disable-shared --host=i686-mingw32 --disable-nls
popd
popenv
popenv
popenv

# task [073/090] /i686-mingw32/libpng/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/include
pushenvvar LDFLAGS -L/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/lib
pushd /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-mingw32
make -j4
popd
popenv
popenv
popenv

# task [074/090] /i686-mingw32/libpng/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/include
pushenvvar LDFLAGS -L/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/lib
pushd /scratch/paul/qemu/obj/libpng-0.9.1-12-arm-none-symbianelf-i686-mingw32
make install
popd
popenv
popenv
popenv

# task [075/090] /i686-mingw32/dtc/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-mingw32
mkdir -p /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-mingw32
pushd /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-mingw32
/scratch/paul/qemu/src/dtc-trunk/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/opt/codesourcery --host=i686-mingw32 '--with-pkgversion=Symbian QEMU 0.9.1-12' --with-bugurl=https://support.codesourcery.com/GNUToolchain/ --disable-nls '--program-transform-name=s,^,arm-none-symbianelf-,'
popd
popenv
popenv
popenv

# task [076/090] /i686-mingw32/dtc/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-mingw32
make -j4
popd
popenv
popenv
popenv

# task [077/090] /i686-mingw32/dtc/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/dtc-0.9.1-12-arm-none-symbianelf-i686-mingw32
make install prefix=/scratch/paul/qemu/install/host-i686-mingw32 exec_prefix=/scratch/paul/qemu/install/host-i686-mingw32 libdir=/scratch/paul/qemu/install/host-i686-mingw32/lib htmldir=/scratch/paul/qemu/install/host-i686-mingw32/share/doc/symbian-qemu-arm-none-symbianelf/html pdfdir=/scratch/paul/qemu/install/host-i686-mingw32/share/doc/symbian-qemu-arm-none-symbianelf/pdf infodir=/scratch/paul/qemu/install/host-i686-mingw32/share/doc/symbian-qemu-arm-none-symbianelf/info mandir=/scratch/paul/qemu/install/host-i686-mingw32/share/doc/symbian-qemu-arm-none-symbianelf/man datadir=/scratch/paul/qemu/install/host-i686-mingw32/share
popd
popenv
popenv
popenv

# task [078/090] /i686-mingw32/python_win32/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushd /scratch/paul/qemu/src/python-win32-2.6.1
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/bin
install -m 775 python26.dll /scratch/paul/qemu/install/host-i686-mingw32/bin
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/curses
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/hotshot
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/myfixes
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/json
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/site-packages
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/email
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/test
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/test/leakers
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/logging
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/msilib
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes/macholib
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/bsddb
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/dummy
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/wsgiref
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/xml
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/dom
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/etree
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/parsers
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/sax
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/lib/sqlite3
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/include/
install -m 664 lib/types.py /scratch/paul/qemu/install/host-i686-mingw32/lib/types.py
install -m 664 lib/contextlib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/contextlib.py
install -m 664 lib/distutils/command/bdist_msi.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/bdist_msi.py
install -m 664 lib/distutils/command/bdist_wininst.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/bdist_wininst.py
install -m 664 lib/distutils/command/wininst-8_d.exe /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/wininst-8_d.exe
install -m 664 lib/distutils/command/build_clib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/build_clib.py
install -m 664 lib/distutils/command/bdist_rpm.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/bdist_rpm.py
install -m 664 lib/distutils/command/sdist.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/sdist.py
install -m 664 lib/distutils/command/build_scripts.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/build_scripts.py
install -m 664 lib/distutils/command/wininst-8.0.exe /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/wininst-8.0.exe
install -m 664 lib/distutils/command/clean.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/clean.py
install -m 664 lib/distutils/command/upload.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/upload.py
install -m 664 lib/distutils/command/wininst-7.1.exe /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/wininst-7.1.exe
install -m 664 lib/distutils/command/bdist_dumb.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/bdist_dumb.py
install -m 664 lib/distutils/command/install_data.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/install_data.py
install -m 664 lib/distutils/command/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/__init__.py
install -m 664 lib/distutils/command/install_lib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/install_lib.py
install -m 664 lib/distutils/command/wininst-9.0-amd64.exe /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/wininst-9.0-amd64.exe
install -m 664 lib/distutils/command/register.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/register.py
install -m 664 lib/distutils/command/build_py.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/build_py.py
install -m 664 lib/distutils/command/install_scripts.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/install_scripts.py
install -m 664 lib/distutils/command/build.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/build.py
install -m 664 lib/distutils/command/install_egg_info.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/install_egg_info.py
install -m 664 lib/distutils/command/build_ext.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/build_ext.py
install -m 664 lib/distutils/command/wininst-9.0.exe /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/wininst-9.0.exe
install -m 664 lib/distutils/command/wininst-6.0.exe /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/wininst-6.0.exe
install -m 664 lib/distutils/command/install_headers.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/install_headers.py
install -m 664 lib/distutils/command/bdist.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/bdist.py
install -m 664 lib/distutils/command/config.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/config.py
install -m 664 lib/distutils/command/install.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/command/install.py
install -m 664 lib/distutils/core.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/core.py
install -m 664 lib/distutils/text_file.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/text_file.py
install -m 664 lib/distutils/fancy_getopt.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/fancy_getopt.py
install -m 664 lib/distutils/ccompiler.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/ccompiler.py
install -m 664 lib/distutils/file_util.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/file_util.py
install -m 664 lib/distutils/versionpredicate.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/versionpredicate.py
install -m 664 lib/distutils/unixccompiler.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/unixccompiler.py
install -m 664 lib/distutils/dep_util.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/dep_util.py
install -m 664 lib/distutils/extension.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/extension.py
install -m 664 lib/distutils/msvccompiler.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/msvccompiler.py
install -m 664 lib/distutils/debug.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/debug.py
install -m 664 lib/distutils/errors.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/errors.py
install -m 664 lib/distutils/dist.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/dist.py
install -m 664 lib/distutils/archive_util.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/archive_util.py
install -m 664 lib/distutils/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/__init__.py
install -m 664 lib/distutils/dir_util.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/dir_util.py
install -m 664 lib/distutils/cmd.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/cmd.py
install -m 664 lib/distutils/msvc9compiler.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/msvc9compiler.py
install -m 664 lib/distutils/util.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/util.py
install -m 664 lib/distutils/spawn.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/spawn.py
install -m 664 lib/distutils/log.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/log.py
install -m 664 lib/distutils/emxccompiler.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/emxccompiler.py
install -m 664 lib/distutils/cygwinccompiler.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/cygwinccompiler.py
install -m 664 lib/distutils/version.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/version.py
install -m 664 lib/distutils/sysconfig.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/sysconfig.py
install -m 664 lib/distutils/config.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/config.py
install -m 664 lib/distutils/mwerkscompiler.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/mwerkscompiler.py
install -m 664 lib/distutils/bcppcompiler.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/bcppcompiler.py
install -m 664 lib/distutils/filelist.py /scratch/paul/qemu/install/host-i686-mingw32/lib/distutils/filelist.py
install -m 664 lib/gettext.py /scratch/paul/qemu/install/host-i686-mingw32/lib/gettext.py
install -m 664 lib/chunk.py /scratch/paul/qemu/install/host-i686-mingw32/lib/chunk.py
install -m 664 lib/tty.py /scratch/paul/qemu/install/host-i686-mingw32/lib/tty.py
install -m 664 lib/sndhdr.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sndhdr.py
install -m 664 lib/inspect.py /scratch/paul/qemu/install/host-i686-mingw32/lib/inspect.py
install -m 664 lib/locale.py /scratch/paul/qemu/install/host-i686-mingw32/lib/locale.py
install -m 664 lib/ConfigParser.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ConfigParser.py
install -m 664 lib/curses/wrapper.py /scratch/paul/qemu/install/host-i686-mingw32/lib/curses/wrapper.py
install -m 664 lib/curses/has_key.py /scratch/paul/qemu/install/host-i686-mingw32/lib/curses/has_key.py
install -m 664 lib/curses/textpad.py /scratch/paul/qemu/install/host-i686-mingw32/lib/curses/textpad.py
install -m 664 lib/curses/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/curses/__init__.py
install -m 664 lib/curses/panel.py /scratch/paul/qemu/install/host-i686-mingw32/lib/curses/panel.py
install -m 664 lib/curses/ascii.py /scratch/paul/qemu/install/host-i686-mingw32/lib/curses/ascii.py
install -m 664 lib/token.py /scratch/paul/qemu/install/host-i686-mingw32/lib/token.py
install -m 664 lib/audiodev.py /scratch/paul/qemu/install/host-i686-mingw32/lib/audiodev.py
install -m 664 lib/md5.py /scratch/paul/qemu/install/host-i686-mingw32/lib/md5.py
install -m 664 lib/rfc822.py /scratch/paul/qemu/install/host-i686-mingw32/lib/rfc822.py
install -m 664 lib/functools.py /scratch/paul/qemu/install/host-i686-mingw32/lib/functools.py
install -m 664 lib/hotshot/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/hotshot/__init__.py
install -m 664 lib/hotshot/stones.py /scratch/paul/qemu/install/host-i686-mingw32/lib/hotshot/stones.py
install -m 664 lib/hotshot/log.py /scratch/paul/qemu/install/host-i686-mingw32/lib/hotshot/log.py
install -m 664 lib/hotshot/stats.py /scratch/paul/qemu/install/host-i686-mingw32/lib/hotshot/stats.py
install -m 664 lib/random.py /scratch/paul/qemu/install/host-i686-mingw32/lib/random.py
install -m 664 lib/nntplib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/nntplib.py
install -m 664 lib/telnetlib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/telnetlib.py
install -m 664 lib/_LWPCookieJar.py /scratch/paul/qemu/install/host-i686-mingw32/lib/_LWPCookieJar.py
install -m 664 lib/heapq.py /scratch/paul/qemu/install/host-i686-mingw32/lib/heapq.py
install -m 664 lib/shelve.py /scratch/paul/qemu/install/host-i686-mingw32/lib/shelve.py
install -m 664 lib/lib2to3/fixes/fix_intern.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_intern.py
install -m 664 lib/lib2to3/fixes/fix_urllib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_urllib.py
install -m 664 lib/lib2to3/fixes/fix_execfile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_execfile.py
install -m 664 lib/lib2to3/fixes/fix_imports2.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_imports2.py
install -m 664 lib/lib2to3/fixes/fix_xrange.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_xrange.py
install -m 664 lib/lib2to3/fixes/fix_types.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_types.py
install -m 664 lib/lib2to3/fixes/fix_print.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_print.py
install -m 664 lib/lib2to3/fixes/fix_numliterals.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_numliterals.py
install -m 664 lib/lib2to3/fixes/fix_buffer.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_buffer.py
install -m 664 lib/lib2to3/fixes/fix_throw.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_throw.py
install -m 664 lib/lib2to3/fixes/fix_funcattrs.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_funcattrs.py
install -m 664 lib/lib2to3/fixes/fix_raise.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_raise.py
install -m 664 lib/lib2to3/fixes/fix_filter.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_filter.py
install -m 664 lib/lib2to3/fixes/fix_repr.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_repr.py
install -m 664 lib/lib2to3/fixes/fix_itertools_imports.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_itertools_imports.py
install -m 664 lib/lib2to3/fixes/fix_unicode.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_unicode.py
install -m 664 lib/lib2to3/fixes/fix_set_literal.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_set_literal.py
install -m 664 lib/lib2to3/fixes/fix_getcwdu.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_getcwdu.py
install -m 664 lib/lib2to3/fixes/fix_paren.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_paren.py
install -m 664 lib/lib2to3/fixes/fix_map.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_map.py
install -m 664 lib/lib2to3/fixes/fix_future.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_future.py
install -m 664 lib/lib2to3/fixes/fix_input.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_input.py
install -m 664 lib/lib2to3/fixes/fix_long.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_long.py
install -m 664 lib/lib2to3/fixes/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/__init__.py
install -m 664 lib/lib2to3/fixes/fix_xreadlines.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_xreadlines.py
install -m 664 lib/lib2to3/fixes/fix_next.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_next.py
install -m 664 lib/lib2to3/fixes/fix_raw_input.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_raw_input.py
install -m 664 lib/lib2to3/fixes/fix_tuple_params.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_tuple_params.py
install -m 664 lib/lib2to3/fixes/fix_renames.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_renames.py
install -m 664 lib/lib2to3/fixes/fix_nonzero.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_nonzero.py
install -m 664 lib/lib2to3/fixes/fix_zip.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_zip.py
install -m 664 lib/lib2to3/fixes/fix_ws_comma.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_ws_comma.py
install -m 664 lib/lib2to3/fixes/fix_idioms.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_idioms.py
install -m 664 lib/lib2to3/fixes/fix_ne.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_ne.py
install -m 664 lib/lib2to3/fixes/fix_imports.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_imports.py
install -m 664 lib/lib2to3/fixes/fix_basestring.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_basestring.py
install -m 664 lib/lib2to3/fixes/fix_sys_exc.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_sys_exc.py
install -m 664 lib/lib2to3/fixes/fix_import.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_import.py
install -m 664 lib/lib2to3/fixes/fix_except.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_except.py
install -m 664 lib/lib2to3/fixes/fix_itertools.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_itertools.py
install -m 664 lib/lib2to3/fixes/fix_callable.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_callable.py
install -m 664 lib/lib2to3/fixes/fix_metaclass.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_metaclass.py
install -m 664 lib/lib2to3/fixes/fix_apply.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_apply.py
install -m 664 lib/lib2to3/fixes/fix_dict.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_dict.py
install -m 664 lib/lib2to3/fixes/fix_standarderror.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_standarderror.py
install -m 664 lib/lib2to3/fixes/fix_exec.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_exec.py
install -m 664 lib/lib2to3/fixes/fix_methodattrs.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_methodattrs.py
install -m 664 lib/lib2to3/fixes/fix_has_key.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixes/fix_has_key.py
install -m 664 lib/lib2to3/fixer_base.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixer_base.py
install -m 664 lib/lib2to3/pytree.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pytree.py
install -m 664 lib/lib2to3/Grammar.txt /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/Grammar.txt
install -m 664 lib/lib2to3/fixer_util.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/fixer_util.py
install -m 664 lib/lib2to3/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/__init__.py
install -m 664 lib/lib2to3/tests/data/fixers/bad_order.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/bad_order.py
install -m 664 lib/lib2to3/tests/data/fixers/myfixes/fix_first.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/myfixes/fix_first.py
install -m 664 lib/lib2to3/tests/data/fixers/myfixes/fix_last.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/myfixes/fix_last.py
install -m 664 lib/lib2to3/tests/data/fixers/myfixes/fix_parrot.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/myfixes/fix_parrot.py
install -m 664 lib/lib2to3/tests/data/fixers/myfixes/fix_preorder.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/myfixes/fix_preorder.py
install -m 664 lib/lib2to3/tests/data/fixers/myfixes/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/myfixes/__init__.py
install -m 664 lib/lib2to3/tests/data/fixers/myfixes/fix_explicit.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/myfixes/fix_explicit.py
install -m 664 lib/lib2to3/tests/data/fixers/no_fixer_cls.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/no_fixer_cls.py
install -m 664 lib/lib2to3/tests/data/fixers/parrot_example.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/tests/data/fixers/parrot_example.py
install -m 664 lib/lib2to3/pgen2/grammar.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2/grammar.py
install -m 664 lib/lib2to3/pgen2/token.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2/token.py
install -m 664 lib/lib2to3/pgen2/tokenize.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2/tokenize.py
install -m 664 lib/lib2to3/pgen2/driver.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2/driver.py
install -m 664 lib/lib2to3/pgen2/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2/__init__.py
install -m 664 lib/lib2to3/pgen2/pgen.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2/pgen.py
install -m 664 lib/lib2to3/pgen2/parse.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2/parse.py
install -m 664 lib/lib2to3/pgen2/literals.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2/literals.py
install -m 664 lib/lib2to3/pgen2/conv.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pgen2/conv.py
install -m 664 lib/lib2to3/pygram.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/pygram.py
install -m 664 lib/lib2to3/refactor.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/refactor.py
install -m 664 lib/lib2to3/PatternGrammar.txt /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/PatternGrammar.txt
install -m 664 lib/lib2to3/patcomp.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/patcomp.py
install -m 664 lib/lib2to3/main.py /scratch/paul/qemu/install/host-i686-mingw32/lib/lib2to3/main.py
install -m 664 lib/UserString.py /scratch/paul/qemu/install/host-i686-mingw32/lib/UserString.py
install -m 664 lib/cgitb.py /scratch/paul/qemu/install/host-i686-mingw32/lib/cgitb.py
install -m 664 lib/poplib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/poplib.py
install -m 664 lib/json/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/json/__init__.py
install -m 664 lib/json/decoder.py /scratch/paul/qemu/install/host-i686-mingw32/lib/json/decoder.py
install -m 664 lib/json/tool.py /scratch/paul/qemu/install/host-i686-mingw32/lib/json/tool.py
install -m 664 lib/json/scanner.py /scratch/paul/qemu/install/host-i686-mingw32/lib/json/scanner.py
install -m 664 lib/json/encoder.py /scratch/paul/qemu/install/host-i686-mingw32/lib/json/encoder.py
install -m 664 lib/trace.py /scratch/paul/qemu/install/host-i686-mingw32/lib/trace.py
install -m 664 lib/abc.py /scratch/paul/qemu/install/host-i686-mingw32/lib/abc.py
install -m 664 lib/unittest.py /scratch/paul/qemu/install/host-i686-mingw32/lib/unittest.py
install -m 664 lib/tokenize.py /scratch/paul/qemu/install/host-i686-mingw32/lib/tokenize.py
install -m 664 lib/hmac.py /scratch/paul/qemu/install/host-i686-mingw32/lib/hmac.py
install -m 664 lib/_strptime.py /scratch/paul/qemu/install/host-i686-mingw32/lib/_strptime.py
install -m 664 lib/io.py /scratch/paul/qemu/install/host-i686-mingw32/lib/io.py
install -m 664 lib/dummy_threading.py /scratch/paul/qemu/install/host-i686-mingw32/lib/dummy_threading.py
install -m 664 lib/py_compile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/py_compile.py
install -m 664 lib/genericpath.py /scratch/paul/qemu/install/host-i686-mingw32/lib/genericpath.py
install -m 664 lib/tabnanny.py /scratch/paul/qemu/install/host-i686-mingw32/lib/tabnanny.py
install -m 664 lib/site-packages/README.txt /scratch/paul/qemu/install/host-i686-mingw32/lib/site-packages/README.txt
install -m 664 lib/SocketServer.py /scratch/paul/qemu/install/host-i686-mingw32/lib/SocketServer.py
install -m 664 lib/asyncore.py /scratch/paul/qemu/install/host-i686-mingw32/lib/asyncore.py
install -m 664 lib/dis.py /scratch/paul/qemu/install/host-i686-mingw32/lib/dis.py
install -m 664 lib/sre.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sre.py
install -m 664 lib/ssl.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ssl.py
install -m 664 lib/uu.py /scratch/paul/qemu/install/host-i686-mingw32/lib/uu.py
install -m 664 lib/cookielib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/cookielib.py
install -m 664 lib/email/header.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/header.py
install -m 664 lib/email/parser.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/parser.py
install -m 664 lib/email/_parseaddr.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/_parseaddr.py
install -m 664 lib/email/feedparser.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/feedparser.py
install -m 664 lib/email/message.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/message.py
install -m 664 lib/email/mime/image.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime/image.py
install -m 664 lib/email/mime/message.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime/message.py
install -m 664 lib/email/mime/application.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime/application.py
install -m 664 lib/email/mime/multipart.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime/multipart.py
install -m 664 lib/email/mime/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime/__init__.py
install -m 664 lib/email/mime/text.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime/text.py
install -m 664 lib/email/mime/nonmultipart.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime/nonmultipart.py
install -m 664 lib/email/mime/audio.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime/audio.py
install -m 664 lib/email/mime/base.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/mime/base.py
install -m 664 lib/email/encoders.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/encoders.py
install -m 664 lib/email/errors.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/errors.py
install -m 664 lib/email/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/__init__.py
install -m 664 lib/email/base64mime.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/base64mime.py
install -m 664 lib/email/generator.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/generator.py
install -m 664 lib/email/quoprimime.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/quoprimime.py
install -m 664 lib/email/charset.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/charset.py
install -m 664 lib/email/utils.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/utils.py
install -m 664 lib/email/iterators.py /scratch/paul/qemu/install/host-i686-mingw32/lib/email/iterators.py
install -m 664 lib/plistlib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/plistlib.py
install -m 664 lib/bdb.py /scratch/paul/qemu/install/host-i686-mingw32/lib/bdb.py
install -m 664 lib/string.py /scratch/paul/qemu/install/host-i686-mingw32/lib/string.py
install -m 664 lib/symtable.py /scratch/paul/qemu/install/host-i686-mingw32/lib/symtable.py
install -m 664 lib/statvfs.py /scratch/paul/qemu/install/host-i686-mingw32/lib/statvfs.py
install -m 664 lib/compileall.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compileall.py
install -m 664 lib/StringIO.py /scratch/paul/qemu/install/host-i686-mingw32/lib/StringIO.py
install -m 664 lib/compiler/symbols.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/symbols.py
install -m 664 lib/compiler/visitor.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/visitor.py
install -m 664 lib/compiler/pyassem.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/pyassem.py
install -m 664 lib/compiler/future.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/future.py
install -m 664 lib/compiler/transformer.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/transformer.py
install -m 664 lib/compiler/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/__init__.py
install -m 664 lib/compiler/ast.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/ast.py
install -m 664 lib/compiler/consts.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/consts.py
install -m 664 lib/compiler/syntax.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/syntax.py
install -m 664 lib/compiler/misc.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/misc.py
install -m 664 lib/compiler/pycodegen.py /scratch/paul/qemu/install/host-i686-mingw32/lib/compiler/pycodegen.py
install -m 664 lib/pydoc.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pydoc.py
install -m 664 lib/struct.py /scratch/paul/qemu/install/host-i686-mingw32/lib/struct.py
install -m 664 lib/doctest.py /scratch/paul/qemu/install/host-i686-mingw32/lib/doctest.py
install -m 664 lib/linecache.py /scratch/paul/qemu/install/host-i686-mingw32/lib/linecache.py
install -m 664 lib/posixpath.py /scratch/paul/qemu/install/host-i686-mingw32/lib/posixpath.py
install -m 664 lib/glob.py /scratch/paul/qemu/install/host-i686-mingw32/lib/glob.py
install -m 664 lib/test/leakers/test_selftype.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/leakers/test_selftype.py
install -m 664 lib/test/leakers/test_ctypes.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/leakers/test_ctypes.py
install -m 664 lib/test/leakers/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/leakers/__init__.py
install -m 664 lib/test/leakers/README.txt /scratch/paul/qemu/install/host-i686-mingw32/lib/test/leakers/README.txt
install -m 664 lib/test/leakers/test_gestalt.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/leakers/test_gestalt.py
install -m 664 lib/test/crashers/gc_inspection.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/gc_inspection.py
install -m 664 lib/test/crashers/infinite_loop_re.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/infinite_loop_re.py
install -m 664 lib/test/crashers/recursive_call.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/recursive_call.py
install -m 664 lib/test/crashers/mutation_inside_cyclegc.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/mutation_inside_cyclegc.py
install -m 664 lib/test/crashers/iter.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/iter.py
install -m 664 lib/test/crashers/nasty_eq_vs_dict.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/nasty_eq_vs_dict.py
install -m 664 lib/test/crashers/borrowed_ref_1.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/borrowed_ref_1.py
install -m 664 lib/test/crashers/bogus_sre_bytecode.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/bogus_sre_bytecode.py
install -m 664 lib/test/crashers/loosing_mro_ref.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/loosing_mro_ref.py
install -m 664 lib/test/crashers/multithreaded_close.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/multithreaded_close.py
install -m 664 lib/test/crashers/bogus_code_obj.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/bogus_code_obj.py
install -m 664 lib/test/crashers/recursion_limit_too_high.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/recursion_limit_too_high.py
install -m 664 lib/test/crashers/borrowed_ref_2.py /scratch/paul/qemu/install/host-i686-mingw32/lib/test/crashers/borrowed_ref_2.py
install -m 664 lib/test/decimaltestdata/dqAbs.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqAbs.decTest
install -m 664 lib/test/decimaltestdata/ddCopyAbs.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddCopyAbs.decTest
install -m 664 lib/test/decimaltestdata/ddMultiply.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddMultiply.decTest
install -m 664 lib/test/decimaltestdata/dqDivideInt.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqDivideInt.decTest
install -m 664 lib/test/decimaltestdata/rotate.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/rotate.decTest
install -m 664 lib/test/decimaltestdata/dqScaleB.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqScaleB.decTest
install -m 664 lib/test/decimaltestdata/ddAdd.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddAdd.decTest
install -m 664 lib/test/decimaltestdata/rounding.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/rounding.decTest
install -m 664 lib/test/decimaltestdata/copyabs.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/copyabs.decTest
install -m 664 lib/test/decimaltestdata/dqNextPlus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqNextPlus.decTest
install -m 664 lib/test/decimaltestdata/multiply.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/multiply.decTest
install -m 664 lib/test/decimaltestdata/tointegral.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/tointegral.decTest
install -m 664 lib/test/decimaltestdata/scaleb.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/scaleb.decTest
install -m 664 lib/test/decimaltestdata/ddRotate.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddRotate.decTest
install -m 664 lib/test/decimaltestdata/class.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/class.decTest
install -m 664 lib/test/decimaltestdata/dqRemainderNear.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqRemainderNear.decTest
install -m 664 lib/test/decimaltestdata/ddMin.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddMin.decTest
install -m 664 lib/test/decimaltestdata/dqBase.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqBase.decTest
install -m 664 lib/test/decimaltestdata/copynegate.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/copynegate.decTest
install -m 664 lib/test/decimaltestdata/dqCompareTotalMag.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqCompareTotalMag.decTest
install -m 664 lib/test/decimaltestdata/dqRotate.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqRotate.decTest
install -m 664 lib/test/decimaltestdata/fma.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/fma.decTest
install -m 664 lib/test/decimaltestdata/dqDivide.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqDivide.decTest
install -m 664 lib/test/decimaltestdata/ddFMA.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddFMA.decTest
install -m 664 lib/test/decimaltestdata/ddMaxMag.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddMaxMag.decTest
install -m 664 lib/test/decimaltestdata/ddShift.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddShift.decTest
install -m 664 lib/test/decimaltestdata/max.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/max.decTest
install -m 664 lib/test/decimaltestdata/dqFMA.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqFMA.decTest
install -m 664 lib/test/decimaltestdata/logb.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/logb.decTest
install -m 664 lib/test/decimaltestdata/tointegralx.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/tointegralx.decTest
install -m 664 lib/test/decimaltestdata/xor.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/xor.decTest
install -m 664 lib/test/decimaltestdata/ddNextMinus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddNextMinus.decTest
install -m 664 lib/test/decimaltestdata/dqCompare.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqCompare.decTest
install -m 664 lib/test/decimaltestdata/comparetotal.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/comparetotal.decTest
install -m 664 lib/test/decimaltestdata/remainderNear.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/remainderNear.decTest
install -m 664 lib/test/decimaltestdata/ddBase.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddBase.decTest
install -m 664 lib/test/decimaltestdata/dqMinMag.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqMinMag.decTest
install -m 664 lib/test/decimaltestdata/divideint.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/divideint.decTest
install -m 664 lib/test/decimaltestdata/dqSubtract.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqSubtract.decTest
install -m 664 lib/test/decimaltestdata/dqCopyAbs.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqCopyAbs.decTest
install -m 664 lib/test/decimaltestdata/ddCopy.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddCopy.decTest
install -m 664 lib/test/decimaltestdata/dqXor.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqXor.decTest
install -m 664 lib/test/decimaltestdata/remainder.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/remainder.decTest
install -m 664 lib/test/decimaltestdata/dqCopySign.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqCopySign.decTest
install -m 664 lib/test/decimaltestdata/dqAdd.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqAdd.decTest
install -m 664 lib/test/decimaltestdata/ddEncode.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddEncode.decTest
install -m 664 lib/test/decimaltestdata/ddMinMag.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddMinMag.decTest
install -m 664 lib/test/decimaltestdata/rescale.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/rescale.decTest
install -m 664 lib/test/decimaltestdata/randoms.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/randoms.decTest
install -m 664 lib/test/decimaltestdata/and.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/and.decTest
install -m 664 lib/test/decimaltestdata/dqShift.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqShift.decTest
install -m 664 lib/test/decimaltestdata/abs.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/abs.decTest
install -m 664 lib/test/decimaltestdata/copysign.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/copysign.decTest
install -m 664 lib/test/decimaltestdata/dqRemainder.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqRemainder.decTest
install -m 664 lib/test/decimaltestdata/squareroot.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/squareroot.decTest
install -m 664 lib/test/decimaltestdata/dqMultiply.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqMultiply.decTest
install -m 664 lib/test/decimaltestdata/decQuad.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/decQuad.decTest
install -m 664 lib/test/decimaltestdata/ddNextPlus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddNextPlus.decTest
install -m 664 lib/test/decimaltestdata/ddClass.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddClass.decTest
install -m 664 lib/test/decimaltestdata/samequantum.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/samequantum.decTest
install -m 664 lib/test/decimaltestdata/minmag.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/minmag.decTest
install -m 664 lib/test/decimaltestdata/randomBound32.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/randomBound32.decTest
install -m 664 lib/test/decimaltestdata/ddSubtract.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddSubtract.decTest
install -m 664 lib/test/decimaltestdata/dqLogB.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqLogB.decTest
install -m 664 lib/test/decimaltestdata/ln.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ln.decTest
install -m 664 lib/test/decimaltestdata/ddQuantize.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddQuantize.decTest
install -m 664 lib/test/decimaltestdata/dqNextToward.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqNextToward.decTest
install -m 664 lib/test/decimaltestdata/dqMinus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqMinus.decTest
install -m 664 lib/test/decimaltestdata/ddCanonical.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddCanonical.decTest
install -m 664 lib/test/decimaltestdata/dqQuantize.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqQuantize.decTest
install -m 664 lib/test/decimaltestdata/shift.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/shift.decTest
install -m 664 lib/test/decimaltestdata/dqClass.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqClass.decTest
install -m 664 lib/test/decimaltestdata/maxmag.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/maxmag.decTest
install -m 664 lib/test/decimaltestdata/dqCompareTotal.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqCompareTotal.decTest
install -m 664 lib/test/decimaltestdata/ddRemainderNear.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddRemainderNear.decTest
install -m 664 lib/test/decimaltestdata/ddSameQuantum.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddSameQuantum.decTest
install -m 664 lib/test/decimaltestdata/testall.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/testall.decTest
install -m 664 lib/test/decimaltestdata/or.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/or.decTest
install -m 664 lib/test/decimaltestdata/ddDivideInt.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddDivideInt.decTest
install -m 664 lib/test/decimaltestdata/reduce.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/reduce.decTest
install -m 664 lib/test/decimaltestdata/ddAbs.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddAbs.decTest
install -m 664 lib/test/decimaltestdata/plus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/plus.decTest
install -m 664 lib/test/decimaltestdata/ddCompareSig.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddCompareSig.decTest
install -m 664 lib/test/decimaltestdata/ddNextToward.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddNextToward.decTest
install -m 664 lib/test/decimaltestdata/dqCopy.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqCopy.decTest
install -m 664 lib/test/decimaltestdata/powersqrt.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/powersqrt.decTest
install -m 664 lib/test/decimaltestdata/nextplus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/nextplus.decTest
install -m 664 lib/test/decimaltestdata/ddReduce.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddReduce.decTest
install -m 664 lib/test/decimaltestdata/log10.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/log10.decTest
install -m 664 lib/test/decimaltestdata/copy.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/copy.decTest
install -m 664 lib/test/decimaltestdata/decSingle.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/decSingle.decTest
install -m 664 lib/test/decimaltestdata/dsBase.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dsBase.decTest
install -m 664 lib/test/decimaltestdata/ddToIntegral.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddToIntegral.decTest
install -m 664 lib/test/decimaltestdata/dqReduce.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqReduce.decTest
install -m 664 lib/test/decimaltestdata/comparetotmag.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/comparetotmag.decTest
install -m 664 lib/test/decimaltestdata/invert.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/invert.decTest
install -m 664 lib/test/decimaltestdata/ddOr.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddOr.decTest
install -m 664 lib/test/decimaltestdata/dqSameQuantum.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqSameQuantum.decTest
install -m 664 lib/test/decimaltestdata/decDouble.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/decDouble.decTest
install -m 664 lib/test/decimaltestdata/dqMin.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqMin.decTest
install -m 664 lib/test/decimaltestdata/ddCompareTotal.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddCompareTotal.decTest
install -m 664 lib/test/decimaltestdata/dqOr.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqOr.decTest
install -m 664 lib/test/decimaltestdata/dqCanonical.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqCanonical.decTest
install -m 664 lib/test/decimaltestdata/ddMinus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddMinus.decTest
install -m 664 lib/test/decimaltestdata/base.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/base.decTest
install -m 664 lib/test/decimaltestdata/subtract.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/subtract.decTest
install -m 664 lib/test/decimaltestdata/dqNextMinus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqNextMinus.decTest
install -m 664 lib/test/decimaltestdata/power.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/power.decTest
install -m 664 lib/test/decimaltestdata/ddDivide.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddDivide.decTest
install -m 664 lib/test/decimaltestdata/min.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/min.decTest
install -m 664 lib/test/decimaltestdata/dqAnd.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqAnd.decTest
install -m 664 lib/test/decimaltestdata/compare.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/compare.decTest
install -m 664 lib/test/decimaltestdata/ddLogB.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddLogB.decTest
install -m 664 lib/test/decimaltestdata/divide.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/divide.decTest
install -m 664 lib/test/decimaltestdata/ddCopyNegate.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddCopyNegate.decTest
install -m 664 lib/test/decimaltestdata/dqPlus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqPlus.decTest
install -m 664 lib/test/decimaltestdata/ddPlus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddPlus.decTest
install -m 664 lib/test/decimaltestdata/ddCompareTotalMag.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddCompareTotalMag.decTest
install -m 664 lib/test/decimaltestdata/dqInvert.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqInvert.decTest
install -m 664 lib/test/decimaltestdata/ddRemainder.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddRemainder.decTest
install -m 664 lib/test/decimaltestdata/exp.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/exp.decTest
install -m 664 lib/test/decimaltestdata/dsEncode.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dsEncode.decTest
install -m 664 lib/test/decimaltestdata/dqToIntegral.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqToIntegral.decTest
install -m 664 lib/test/decimaltestdata/nextminus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/nextminus.decTest
install -m 664 lib/test/decimaltestdata/dqMax.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqMax.decTest
install -m 664 lib/test/decimaltestdata/ddScaleB.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddScaleB.decTest
install -m 664 lib/test/decimaltestdata/dqCompareSig.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqCompareSig.decTest
install -m 664 lib/test/decimaltestdata/inexact.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/inexact.decTest
install -m 664 lib/test/decimaltestdata/dqMaxMag.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqMaxMag.decTest
install -m 664 lib/test/decimaltestdata/ddInvert.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddInvert.decTest
install -m 664 lib/test/decimaltestdata/ddXor.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddXor.decTest
install -m 664 lib/test/decimaltestdata/ddCopySign.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddCopySign.decTest
install -m 664 lib/test/decimaltestdata/ddAnd.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddAnd.decTest
install -m 664 lib/test/decimaltestdata/extra.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/extra.decTest
install -m 664 lib/test/decimaltestdata/quantize.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/quantize.decTest
install -m 664 lib/test/decimaltestdata/nexttoward.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/nexttoward.decTest
install -m 664 lib/test/decimaltestdata/minus.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/minus.decTest
install -m 664 lib/test/decimaltestdata/ddCompare.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddCompare.decTest
install -m 664 lib/test/decimaltestdata/add.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/add.decTest
install -m 664 lib/test/decimaltestdata/ddMax.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/ddMax.decTest
install -m 664 lib/test/decimaltestdata/dqEncode.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqEncode.decTest
install -m 664 lib/test/decimaltestdata/clamp.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/clamp.decTest
install -m 664 lib/test/decimaltestdata/dqCopyNegate.decTest /scratch/paul/qemu/install/host-i686-mingw32/lib/test/decimaltestdata/dqCopyNegate.decTest
install -m 664 lib/sha.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sha.py
install -m 664 lib/csv.py /scratch/paul/qemu/install/host-i686-mingw32/lib/csv.py
install -m 664 lib/xdrlib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xdrlib.py
install -m 664 lib/numbers.py /scratch/paul/qemu/install/host-i686-mingw32/lib/numbers.py
install -m 664 lib/symbol.py /scratch/paul/qemu/install/host-i686-mingw32/lib/symbol.py
install -m 664 lib/this.py /scratch/paul/qemu/install/host-i686-mingw32/lib/this.py
install -m 664 lib/formatter.py /scratch/paul/qemu/install/host-i686-mingw32/lib/formatter.py
install -m 664 lib/getpass.py /scratch/paul/qemu/install/host-i686-mingw32/lib/getpass.py
install -m 664 lib/stringold.py /scratch/paul/qemu/install/host-i686-mingw32/lib/stringold.py
install -m 664 lib/httplib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/httplib.py
install -m 664 lib/runpy.py /scratch/paul/qemu/install/host-i686-mingw32/lib/runpy.py
install -m 664 lib/traceback.py /scratch/paul/qemu/install/host-i686-mingw32/lib/traceback.py
install -m 664 lib/dumbdbm.py /scratch/paul/qemu/install/host-i686-mingw32/lib/dumbdbm.py
install -m 664 lib/codecs.py /scratch/paul/qemu/install/host-i686-mingw32/lib/codecs.py
install -m 664 lib/mimetools.py /scratch/paul/qemu/install/host-i686-mingw32/lib/mimetools.py
install -m 664 lib/ihooks.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ihooks.py
install -m 664 lib/user.py /scratch/paul/qemu/install/host-i686-mingw32/lib/user.py
install -m 664 lib/pprint.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pprint.py
install -m 664 lib/bisect.py /scratch/paul/qemu/install/host-i686-mingw32/lib/bisect.py
install -m 664 lib/sgmllib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sgmllib.py
install -m 664 lib/fractions.py /scratch/paul/qemu/install/host-i686-mingw32/lib/fractions.py
install -m 664 lib/timeit.py /scratch/paul/qemu/install/host-i686-mingw32/lib/timeit.py
install -m 664 lib/hashlib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/hashlib.py
install -m 664 lib/platform.py /scratch/paul/qemu/install/host-i686-mingw32/lib/platform.py
install -m 664 lib/xmlrpclib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xmlrpclib.py
install -m 664 lib/mutex.py /scratch/paul/qemu/install/host-i686-mingw32/lib/mutex.py
install -m 664 lib/gzip.py /scratch/paul/qemu/install/host-i686-mingw32/lib/gzip.py
install -m 664 lib/BaseHTTPServer.py /scratch/paul/qemu/install/host-i686-mingw32/lib/BaseHTTPServer.py
install -m 664 lib/macurl2path.py /scratch/paul/qemu/install/host-i686-mingw32/lib/macurl2path.py
install -m 664 lib/sre_parse.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sre_parse.py
install -m 664 lib/shutil.py /scratch/paul/qemu/install/host-i686-mingw32/lib/shutil.py
install -m 664 lib/optparse.py /scratch/paul/qemu/install/host-i686-mingw32/lib/optparse.py
install -m 664 lib/smtplib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/smtplib.py
install -m 664 lib/pipes.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pipes.py
install -m 664 lib/sre_compile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sre_compile.py
install -m 664 lib/netrc.py /scratch/paul/qemu/install/host-i686-mingw32/lib/netrc.py
install -m 664 lib/fpformat.py /scratch/paul/qemu/install/host-i686-mingw32/lib/fpformat.py
install -m 664 lib/opcode.py /scratch/paul/qemu/install/host-i686-mingw32/lib/opcode.py
install -m 664 lib/keyword.py /scratch/paul/qemu/install/host-i686-mingw32/lib/keyword.py
install -m 664 lib/_abcoll.py /scratch/paul/qemu/install/host-i686-mingw32/lib/_abcoll.py
install -m 664 lib/repr.py /scratch/paul/qemu/install/host-i686-mingw32/lib/repr.py
install -m 664 lib/pyclbr.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pyclbr.py
install -m 664 lib/collections.py /scratch/paul/qemu/install/host-i686-mingw32/lib/collections.py
install -m 664 lib/sets.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sets.py
install -m 664 lib/pty.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pty.py
install -m 664 lib/fileinput.py /scratch/paul/qemu/install/host-i686-mingw32/lib/fileinput.py
install -m 664 lib/subprocess.py /scratch/paul/qemu/install/host-i686-mingw32/lib/subprocess.py
install -m 664 lib/dbhash.py /scratch/paul/qemu/install/host-i686-mingw32/lib/dbhash.py
install -m 664 lib/rexec.py /scratch/paul/qemu/install/host-i686-mingw32/lib/rexec.py
install -m 664 lib/logging/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/logging/__init__.py
install -m 664 lib/logging/handlers.py /scratch/paul/qemu/install/host-i686-mingw32/lib/logging/handlers.py
install -m 664 lib/logging/config.py /scratch/paul/qemu/install/host-i686-mingw32/lib/logging/config.py
install -m 664 lib/xmllib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xmllib.py
install -m 664 lib/zipfile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/zipfile.py
install -m 664 lib/smtpd.py /scratch/paul/qemu/install/host-i686-mingw32/lib/smtpd.py
install -m 664 lib/ftplib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ftplib.py
install -m 664 lib/mailbox.py /scratch/paul/qemu/install/host-i686-mingw32/lib/mailbox.py
install -m 664 lib/robotparser.py /scratch/paul/qemu/install/host-i686-mingw32/lib/robotparser.py
install -m 664 lib/filecmp.py /scratch/paul/qemu/install/host-i686-mingw32/lib/filecmp.py
install -m 664 lib/ast.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ast.py
install -m 664 lib/ntpath.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ntpath.py
install -m 664 lib/MimeWriter.py /scratch/paul/qemu/install/host-i686-mingw32/lib/MimeWriter.py
install -m 664 lib/decimal.py /scratch/paul/qemu/install/host-i686-mingw32/lib/decimal.py
install -m 664 lib/encodings/cp863.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp863.py
install -m 664 lib/encodings/cp874.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp874.py
install -m 664 lib/encodings/cp1255.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1255.py
install -m 664 lib/encodings/iso2022_jp_2004.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso2022_jp_2004.py
install -m 664 lib/encodings/shift_jisx0213.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/shift_jisx0213.py
install -m 664 lib/encodings/string_escape.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/string_escape.py
install -m 664 lib/encodings/mac_farsi.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_farsi.py
install -m 664 lib/encodings/utf_16_be.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/utf_16_be.py
install -m 664 lib/encodings/euc_jp.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/euc_jp.py
install -m 664 lib/encodings/gb18030.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/gb18030.py
install -m 664 lib/encodings/mbcs.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mbcs.py
install -m 664 lib/encodings/cp500.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp500.py
install -m 664 lib/encodings/mac_croatian.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_croatian.py
install -m 664 lib/encodings/mac_cyrillic.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_cyrillic.py
install -m 664 lib/encodings/koi8_r.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/koi8_r.py
install -m 664 lib/encodings/koi8_u.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/koi8_u.py
install -m 664 lib/encodings/ptcp154.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/ptcp154.py
install -m 664 lib/encodings/cp037.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp037.py
install -m 664 lib/encodings/cp1252.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1252.py
install -m 664 lib/encodings/iso2022_jp_ext.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso2022_jp_ext.py
install -m 664 lib/encodings/iso2022_kr.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso2022_kr.py
install -m 664 lib/encodings/uu_codec.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/uu_codec.py
install -m 664 lib/encodings/raw_unicode_escape.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/raw_unicode_escape.py
install -m 664 lib/encodings/cp852.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp852.py
install -m 664 lib/encodings/iso8859_4.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_4.py
install -m 664 lib/encodings/cp869.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp869.py
install -m 664 lib/encodings/iso8859_7.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_7.py
install -m 664 lib/encodings/utf_32.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/utf_32.py
install -m 664 lib/encodings/cp865.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp865.py
install -m 664 lib/encodings/unicode_internal.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/unicode_internal.py
install -m 664 lib/encodings/big5hkscs.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/big5hkscs.py
install -m 664 lib/encodings/cp875.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp875.py
install -m 664 lib/encodings/mac_arabic.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_arabic.py
install -m 664 lib/encodings/iso8859_11.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_11.py
install -m 664 lib/encodings/iso8859_13.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_13.py
install -m 664 lib/encodings/tis_620.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/tis_620.py
install -m 664 lib/encodings/zlib_codec.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/zlib_codec.py
install -m 664 lib/encodings/cp855.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp855.py
install -m 664 lib/encodings/iso8859_1.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_1.py
install -m 664 lib/encodings/iso8859_8.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_8.py
install -m 664 lib/encodings/cp932.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp932.py
install -m 664 lib/encodings/cp950.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp950.py
install -m 664 lib/encodings/cp864.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp864.py
install -m 664 lib/encodings/mac_iceland.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_iceland.py
install -m 664 lib/encodings/iso8859_2.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_2.py
install -m 664 lib/encodings/euc_kr.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/euc_kr.py
install -m 664 lib/encodings/gb2312.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/gb2312.py
install -m 664 lib/encodings/iso8859_5.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_5.py
install -m 664 lib/encodings/mac_romanian.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_romanian.py
install -m 664 lib/encodings/rot_13.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/rot_13.py
install -m 664 lib/encodings/cp1026.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1026.py
install -m 664 lib/encodings/charmap.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/charmap.py
install -m 664 lib/encodings/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/__init__.py
install -m 664 lib/encodings/hz.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/hz.py
install -m 664 lib/encodings/cp737.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp737.py
install -m 664 lib/encodings/iso8859_14.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_14.py
install -m 664 lib/encodings/mac_greek.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_greek.py
install -m 664 lib/encodings/cp1257.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1257.py
install -m 664 lib/encodings/cp860.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp860.py
install -m 664 lib/encodings/iso2022_jp_1.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso2022_jp_1.py
install -m 664 lib/encodings/hex_codec.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/hex_codec.py
install -m 664 lib/encodings/euc_jisx0213.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/euc_jisx0213.py
install -m 664 lib/encodings/iso2022_jp_2.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso2022_jp_2.py
install -m 664 lib/encodings/cp866.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp866.py
install -m 664 lib/encodings/iso8859_6.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_6.py
install -m 664 lib/encodings/punycode.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/punycode.py
install -m 664 lib/encodings/johab.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/johab.py
install -m 664 lib/encodings/cp1250.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1250.py
install -m 664 lib/encodings/cp856.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp856.py
install -m 664 lib/encodings/utf_8_sig.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/utf_8_sig.py
install -m 664 lib/encodings/cp1253.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1253.py
install -m 664 lib/encodings/cp1258.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1258.py
install -m 664 lib/encodings/ascii.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/ascii.py
install -m 664 lib/encodings/palmos.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/palmos.py
install -m 664 lib/encodings/mac_latin2.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_latin2.py
install -m 664 lib/encodings/shift_jis.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/shift_jis.py
install -m 664 lib/encodings/utf_32_le.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/utf_32_le.py
install -m 664 lib/encodings/iso8859_10.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_10.py
install -m 664 lib/encodings/cp861.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp861.py
install -m 664 lib/encodings/iso8859_9.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_9.py
install -m 664 lib/encodings/shift_jis_2004.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/shift_jis_2004.py
install -m 664 lib/encodings/latin_1.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/latin_1.py
install -m 664 lib/encodings/cp775.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp775.py
install -m 664 lib/encodings/iso8859_15.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_15.py
install -m 664 lib/encodings/iso8859_3.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_3.py
install -m 664 lib/encodings/bz2_codec.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/bz2_codec.py
install -m 664 lib/encodings/hp_roman8.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/hp_roman8.py
install -m 664 lib/encodings/cp1006.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1006.py
install -m 664 lib/encodings/mac_roman.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_roman.py
install -m 664 lib/encodings/mac_turkish.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_turkish.py
install -m 664 lib/encodings/iso8859_16.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso8859_16.py
install -m 664 lib/encodings/cp850.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp850.py
install -m 664 lib/encodings/cp424.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp424.py
install -m 664 lib/encodings/undefined.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/undefined.py
install -m 664 lib/encodings/quopri_codec.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/quopri_codec.py
install -m 664 lib/encodings/idna.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/idna.py
install -m 664 lib/encodings/cp1254.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1254.py
install -m 664 lib/encodings/aliases.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/aliases.py
install -m 664 lib/encodings/cp857.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp857.py
install -m 664 lib/encodings/cp1140.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1140.py
install -m 664 lib/encodings/cp949.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp949.py
install -m 664 lib/encodings/iso2022_jp_3.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso2022_jp_3.py
install -m 664 lib/encodings/utf_16.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/utf_16.py
install -m 664 lib/encodings/gbk.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/gbk.py
install -m 664 lib/encodings/base64_codec.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/base64_codec.py
install -m 664 lib/encodings/cp437.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp437.py
install -m 664 lib/encodings/iso2022_jp.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/iso2022_jp.py
install -m 664 lib/encodings/utf_32_be.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/utf_32_be.py
install -m 664 lib/encodings/utf_7.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/utf_7.py
install -m 664 lib/encodings/cp1251.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1251.py
install -m 664 lib/encodings/big5.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/big5.py
install -m 664 lib/encodings/utf_16_le.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/utf_16_le.py
install -m 664 lib/encodings/cp1256.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp1256.py
install -m 664 lib/encodings/mac_centeuro.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/mac_centeuro.py
install -m 664 lib/encodings/unicode_escape.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/unicode_escape.py
install -m 664 lib/encodings/euc_jis_2004.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/euc_jis_2004.py
install -m 664 lib/encodings/utf_8.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/utf_8.py
install -m 664 lib/encodings/cp862.py /scratch/paul/qemu/install/host-i686-mingw32/lib/encodings/cp862.py
install -m 664 lib/wave.py /scratch/paul/qemu/install/host-i686-mingw32/lib/wave.py
install -m 664 lib/msilib/sequence.py /scratch/paul/qemu/install/host-i686-mingw32/lib/msilib/sequence.py
install -m 664 lib/msilib/schema.py /scratch/paul/qemu/install/host-i686-mingw32/lib/msilib/schema.py
install -m 664 lib/msilib/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/msilib/__init__.py
install -m 664 lib/msilib/text.py /scratch/paul/qemu/install/host-i686-mingw32/lib/msilib/text.py
install -m 664 lib/cmd.py /scratch/paul/qemu/install/host-i686-mingw32/lib/cmd.py
install -m 664 lib/pkgutil.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pkgutil.py
install -m 664 lib/stat.py /scratch/paul/qemu/install/host-i686-mingw32/lib/stat.py
install -m 664 lib/posixfile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/posixfile.py
install -m 664 lib/ctypes/wintypes.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes/wintypes.py
install -m 664 lib/ctypes/macholib/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes/macholib/__init__.py
install -m 664 lib/ctypes/macholib/dylib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes/macholib/dylib.py
install -m 664 lib/ctypes/macholib/framework.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes/macholib/framework.py
install -m 664 lib/ctypes/macholib/dyld.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes/macholib/dyld.py
install -m 664 lib/ctypes/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes/__init__.py
install -m 664 lib/ctypes/_endian.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes/_endian.py
install -m 664 lib/ctypes/util.py /scratch/paul/qemu/install/host-i686-mingw32/lib/ctypes/util.py
install -m 664 lib/copy.py /scratch/paul/qemu/install/host-i686-mingw32/lib/copy.py
install -m 664 lib/imaplib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/imaplib.py
install -m 664 lib/mimetypes.py /scratch/paul/qemu/install/host-i686-mingw32/lib/mimetypes.py
install -m 664 lib/bsddb/dbshelve.py /scratch/paul/qemu/install/host-i686-mingw32/lib/bsddb/dbshelve.py
install -m 664 lib/bsddb/dbobj.py /scratch/paul/qemu/install/host-i686-mingw32/lib/bsddb/dbobj.py
install -m 664 lib/bsddb/dbtables.py /scratch/paul/qemu/install/host-i686-mingw32/lib/bsddb/dbtables.py
install -m 664 lib/bsddb/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/bsddb/__init__.py
install -m 664 lib/bsddb/dbutils.py /scratch/paul/qemu/install/host-i686-mingw32/lib/bsddb/dbutils.py
install -m 664 lib/bsddb/db.py /scratch/paul/qemu/install/host-i686-mingw32/lib/bsddb/db.py
install -m 664 lib/bsddb/dbrecio.py /scratch/paul/qemu/install/host-i686-mingw32/lib/bsddb/dbrecio.py
install -m 664 lib/pstats.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pstats.py
install -m 664 lib/textwrap.py /scratch/paul/qemu/install/host-i686-mingw32/lib/textwrap.py
install -m 664 lib/Queue.py /scratch/paul/qemu/install/host-i686-mingw32/lib/Queue.py
install -m 664 lib/Cookie.py /scratch/paul/qemu/install/host-i686-mingw32/lib/Cookie.py
install -m 664 lib/multiprocessing/sharedctypes.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/sharedctypes.py
install -m 664 lib/multiprocessing/reduction.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/reduction.py
install -m 664 lib/multiprocessing/connection.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/connection.py
install -m 664 lib/multiprocessing/process.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/process.py
install -m 664 lib/multiprocessing/heap.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/heap.py
install -m 664 lib/multiprocessing/synchronize.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/synchronize.py
install -m 664 lib/multiprocessing/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/__init__.py
install -m 664 lib/multiprocessing/dummy/connection.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/dummy/connection.py
install -m 664 lib/multiprocessing/dummy/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/dummy/__init__.py
install -m 664 lib/multiprocessing/managers.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/managers.py
install -m 664 lib/multiprocessing/forking.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/forking.py
install -m 664 lib/multiprocessing/util.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/util.py
install -m 664 lib/multiprocessing/pool.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/pool.py
install -m 664 lib/multiprocessing/queues.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multiprocessing/queues.py
install -m 664 lib/profile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/profile.py
install -m 664 lib/calendar.py /scratch/paul/qemu/install/host-i686-mingw32/lib/calendar.py
install -m 664 lib/markupbase.py /scratch/paul/qemu/install/host-i686-mingw32/lib/markupbase.py
install -m 664 lib/dummy_thread.py /scratch/paul/qemu/install/host-i686-mingw32/lib/dummy_thread.py
install -m 664 lib/_threading_local.py /scratch/paul/qemu/install/host-i686-mingw32/lib/_threading_local.py
install -m 664 lib/re.py /scratch/paul/qemu/install/host-i686-mingw32/lib/re.py
install -m 664 lib/code.py /scratch/paul/qemu/install/host-i686-mingw32/lib/code.py
install -m 664 lib/getopt.py /scratch/paul/qemu/install/host-i686-mingw32/lib/getopt.py
install -m 664 lib/pickle.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pickle.py
install -m 664 lib/os.py /scratch/paul/qemu/install/host-i686-mingw32/lib/os.py
install -m 664 lib/urlparse.py /scratch/paul/qemu/install/host-i686-mingw32/lib/urlparse.py
install -m 664 lib/Bastion.py /scratch/paul/qemu/install/host-i686-mingw32/lib/Bastion.py
install -m 664 lib/os2emxpath.py /scratch/paul/qemu/install/host-i686-mingw32/lib/os2emxpath.py
install -m 664 lib/sre_constants.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sre_constants.py
install -m 664 lib/SimpleHTTPServer.py /scratch/paul/qemu/install/host-i686-mingw32/lib/SimpleHTTPServer.py
install -m 664 lib/imghdr.py /scratch/paul/qemu/install/host-i686-mingw32/lib/imghdr.py
install -m 664 lib/anydbm.py /scratch/paul/qemu/install/host-i686-mingw32/lib/anydbm.py
install -m 664 lib/webbrowser.py /scratch/paul/qemu/install/host-i686-mingw32/lib/webbrowser.py
install -m 664 lib/codeop.py /scratch/paul/qemu/install/host-i686-mingw32/lib/codeop.py
install -m 664 lib/pickletools.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pickletools.py
install -m 664 lib/rlcompleter.py /scratch/paul/qemu/install/host-i686-mingw32/lib/rlcompleter.py
install -m 664 lib/quopri.py /scratch/paul/qemu/install/host-i686-mingw32/lib/quopri.py
install -m 664 lib/urllib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/urllib.py
install -m 664 lib/htmllib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/htmllib.py
install -m 664 lib/mimify.py /scratch/paul/qemu/install/host-i686-mingw32/lib/mimify.py
install -m 664 lib/socket.py /scratch/paul/qemu/install/host-i686-mingw32/lib/socket.py
install -m 664 lib/UserDict.py /scratch/paul/qemu/install/host-i686-mingw32/lib/UserDict.py
install -m 664 lib/wsgiref/headers.py /scratch/paul/qemu/install/host-i686-mingw32/lib/wsgiref/headers.py
install -m 664 lib/wsgiref/simple_server.py /scratch/paul/qemu/install/host-i686-mingw32/lib/wsgiref/simple_server.py
install -m 664 lib/wsgiref/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/wsgiref/__init__.py
install -m 664 lib/wsgiref/handlers.py /scratch/paul/qemu/install/host-i686-mingw32/lib/wsgiref/handlers.py
install -m 664 lib/wsgiref/validate.py /scratch/paul/qemu/install/host-i686-mingw32/lib/wsgiref/validate.py
install -m 664 lib/wsgiref/util.py /scratch/paul/qemu/install/host-i686-mingw32/lib/wsgiref/util.py
install -m 664 lib/uuid.py /scratch/paul/qemu/install/host-i686-mingw32/lib/uuid.py
install -m 664 lib/cgi.py /scratch/paul/qemu/install/host-i686-mingw32/lib/cgi.py
install -m 664 lib/multifile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/multifile.py
install -m 664 lib/fnmatch.py /scratch/paul/qemu/install/host-i686-mingw32/lib/fnmatch.py
install -m 664 lib/colorsys.py /scratch/paul/qemu/install/host-i686-mingw32/lib/colorsys.py
install -m 664 lib/nturl2path.py /scratch/paul/qemu/install/host-i686-mingw32/lib/nturl2path.py
install -m 664 lib/xml/dom/xmlbuilder.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/dom/xmlbuilder.py
install -m 664 lib/xml/dom/domreg.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/dom/domreg.py
install -m 664 lib/xml/dom/NodeFilter.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/dom/NodeFilter.py
install -m 664 lib/xml/dom/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/dom/__init__.py
install -m 664 lib/xml/dom/expatbuilder.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/dom/expatbuilder.py
install -m 664 lib/xml/dom/pulldom.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/dom/pulldom.py
install -m 664 lib/xml/dom/minicompat.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/dom/minicompat.py
install -m 664 lib/xml/dom/minidom.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/dom/minidom.py
install -m 664 lib/xml/etree/ElementInclude.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/etree/ElementInclude.py
install -m 664 lib/xml/etree/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/etree/__init__.py
install -m 664 lib/xml/etree/cElementTree.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/etree/cElementTree.py
install -m 664 lib/xml/etree/ElementPath.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/etree/ElementPath.py
install -m 664 lib/xml/etree/ElementTree.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/etree/ElementTree.py
install -m 664 lib/xml/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/__init__.py
install -m 664 lib/xml/parsers/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/parsers/__init__.py
install -m 664 lib/xml/parsers/expat.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/parsers/expat.py
install -m 664 lib/xml/sax/saxutils.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/sax/saxutils.py
install -m 664 lib/xml/sax/expatreader.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/sax/expatreader.py
install -m 664 lib/xml/sax/_exceptions.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/sax/_exceptions.py
install -m 664 lib/xml/sax/xmlreader.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/sax/xmlreader.py
install -m 664 lib/xml/sax/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/sax/__init__.py
install -m 664 lib/xml/sax/handler.py /scratch/paul/qemu/install/host-i686-mingw32/lib/xml/sax/handler.py
install -m 664 lib/sched.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sched.py
install -m 664 lib/_MozillaCookieJar.py /scratch/paul/qemu/install/host-i686-mingw32/lib/_MozillaCookieJar.py
install -m 664 lib/aifc.py /scratch/paul/qemu/install/host-i686-mingw32/lib/aifc.py
install -m 664 lib/pdb.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pdb.py
install -m 664 lib/__future__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/__future__.py
install -m 664 lib/sqlite3/__init__.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sqlite3/__init__.py
install -m 664 lib/sqlite3/dump.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sqlite3/dump.py
install -m 664 lib/sqlite3/dbapi2.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sqlite3/dbapi2.py
install -m 664 lib/mailcap.py /scratch/paul/qemu/install/host-i686-mingw32/lib/mailcap.py
install -m 664 lib/htmlentitydefs.py /scratch/paul/qemu/install/host-i686-mingw32/lib/htmlentitydefs.py
install -m 664 lib/asynchat.py /scratch/paul/qemu/install/host-i686-mingw32/lib/asynchat.py
install -m 664 lib/warnings.py /scratch/paul/qemu/install/host-i686-mingw32/lib/warnings.py
install -m 664 lib/urllib2.py /scratch/paul/qemu/install/host-i686-mingw32/lib/urllib2.py
install -m 664 lib/popen2.py /scratch/paul/qemu/install/host-i686-mingw32/lib/popen2.py
install -m 664 lib/imputil.py /scratch/paul/qemu/install/host-i686-mingw32/lib/imputil.py
install -m 664 lib/cProfile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/cProfile.py
install -m 664 lib/atexit.py /scratch/paul/qemu/install/host-i686-mingw32/lib/atexit.py
install -m 664 lib/dircache.py /scratch/paul/qemu/install/host-i686-mingw32/lib/dircache.py
install -m 664 lib/weakref.py /scratch/paul/qemu/install/host-i686-mingw32/lib/weakref.py
install -m 664 lib/sunau.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sunau.py
install -m 664 lib/commands.py /scratch/paul/qemu/install/host-i686-mingw32/lib/commands.py
install -m 664 lib/threading.py /scratch/paul/qemu/install/host-i686-mingw32/lib/threading.py
install -m 664 lib/binhex.py /scratch/paul/qemu/install/host-i686-mingw32/lib/binhex.py
install -m 664 lib/CGIHTTPServer.py /scratch/paul/qemu/install/host-i686-mingw32/lib/CGIHTTPServer.py
install -m 664 lib/copy_reg.py /scratch/paul/qemu/install/host-i686-mingw32/lib/copy_reg.py
install -m 664 lib/new.py /scratch/paul/qemu/install/host-i686-mingw32/lib/new.py
install -m 664 lib/macpath.py /scratch/paul/qemu/install/host-i686-mingw32/lib/macpath.py
install -m 664 lib/SimpleXMLRPCServer.py /scratch/paul/qemu/install/host-i686-mingw32/lib/SimpleXMLRPCServer.py
install -m 664 lib/tempfile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/tempfile.py
install -m 664 lib/DocXMLRPCServer.py /scratch/paul/qemu/install/host-i686-mingw32/lib/DocXMLRPCServer.py
install -m 664 lib/UserList.py /scratch/paul/qemu/install/host-i686-mingw32/lib/UserList.py
install -m 664 lib/sunaudio.py /scratch/paul/qemu/install/host-i686-mingw32/lib/sunaudio.py
install -m 664 lib/site.py /scratch/paul/qemu/install/host-i686-mingw32/lib/site.py
install -m 664 lib/stringprep.py /scratch/paul/qemu/install/host-i686-mingw32/lib/stringprep.py
install -m 664 lib/pydoc_topics.py /scratch/paul/qemu/install/host-i686-mingw32/lib/pydoc_topics.py
install -m 664 lib/modulefinder.py /scratch/paul/qemu/install/host-i686-mingw32/lib/modulefinder.py
install -m 664 lib/mhlib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/mhlib.py
install -m 664 lib/__phello__.foo.py /scratch/paul/qemu/install/host-i686-mingw32/lib/__phello__.foo.py
install -m 664 lib/whichdb.py /scratch/paul/qemu/install/host-i686-mingw32/lib/whichdb.py
install -m 664 lib/difflib.py /scratch/paul/qemu/install/host-i686-mingw32/lib/difflib.py
install -m 664 lib/shlex.py /scratch/paul/qemu/install/host-i686-mingw32/lib/shlex.py
install -m 664 lib/HTMLParser.py /scratch/paul/qemu/install/host-i686-mingw32/lib/HTMLParser.py
install -m 664 lib/toaiff.py /scratch/paul/qemu/install/host-i686-mingw32/lib/toaiff.py
install -m 664 lib/base64.py /scratch/paul/qemu/install/host-i686-mingw32/lib/base64.py
install -m 664 lib/tarfile.py /scratch/paul/qemu/install/host-i686-mingw32/lib/tarfile.py
install -m 664 include/pygetopt.h /scratch/paul/qemu/install/host-i686-mingw32/include/pygetopt.h
install -m 664 include/asdl.h /scratch/paul/qemu/install/host-i686-mingw32/include/asdl.h
install -m 664 include/longobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/longobject.h
install -m 664 include/complexobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/complexobject.h
install -m 664 include/bitset.h /scratch/paul/qemu/install/host-i686-mingw32/include/bitset.h
install -m 664 include/parsetok.h /scratch/paul/qemu/install/host-i686-mingw32/include/parsetok.h
install -m 664 include/codecs.h /scratch/paul/qemu/install/host-i686-mingw32/include/codecs.h
install -m 664 include/cobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/cobject.h
install -m 664 include/rangeobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/rangeobject.h
install -m 664 include/pgen.h /scratch/paul/qemu/install/host-i686-mingw32/include/pgen.h
install -m 664 include/bytearrayobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/bytearrayobject.h
install -m 664 include/sysmodule.h /scratch/paul/qemu/install/host-i686-mingw32/include/sysmodule.h
install -m 664 include/listobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/listobject.h
install -m 664 include/code.h /scratch/paul/qemu/install/host-i686-mingw32/include/code.h
install -m 664 include/import.h /scratch/paul/qemu/install/host-i686-mingw32/include/import.h
install -m 664 include/pyexpat.h /scratch/paul/qemu/install/host-i686-mingw32/include/pyexpat.h
install -m 664 include/descrobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/descrobject.h
install -m 664 include/pymacconfig.h /scratch/paul/qemu/install/host-i686-mingw32/include/pymacconfig.h
install -m 664 include/cStringIO.h /scratch/paul/qemu/install/host-i686-mingw32/include/cStringIO.h
install -m 664 include/longintrepr.h /scratch/paul/qemu/install/host-i686-mingw32/include/longintrepr.h
install -m 664 include/token.h /scratch/paul/qemu/install/host-i686-mingw32/include/token.h
install -m 664 include/stringobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/stringobject.h
install -m 664 include/opcode.h /scratch/paul/qemu/install/host-i686-mingw32/include/opcode.h
install -m 664 include/pyarena.h /scratch/paul/qemu/install/host-i686-mingw32/include/pyarena.h
install -m 664 include/symtable.h /scratch/paul/qemu/install/host-i686-mingw32/include/symtable.h
install -m 664 include/node.h /scratch/paul/qemu/install/host-i686-mingw32/include/node.h
install -m 664 include/py_curses.h /scratch/paul/qemu/install/host-i686-mingw32/include/py_curses.h
install -m 664 include/pydebug.h /scratch/paul/qemu/install/host-i686-mingw32/include/pydebug.h
install -m 664 include/metagrammar.h /scratch/paul/qemu/install/host-i686-mingw32/include/metagrammar.h
install -m 664 include/intobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/intobject.h
install -m 664 include/pyfpe.h /scratch/paul/qemu/install/host-i686-mingw32/include/pyfpe.h
install -m 664 include/patchlevel.h /scratch/paul/qemu/install/host-i686-mingw32/include/patchlevel.h
install -m 664 include/enumobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/enumobject.h
install -m 664 include/pyport.h /scratch/paul/qemu/install/host-i686-mingw32/include/pyport.h
install -m 664 include/iterobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/iterobject.h
install -m 664 include/pythread.h /scratch/paul/qemu/install/host-i686-mingw32/include/pythread.h
install -m 664 include/pystrtod.h /scratch/paul/qemu/install/host-i686-mingw32/include/pystrtod.h
install -m 664 include/objimpl.h /scratch/paul/qemu/install/host-i686-mingw32/include/objimpl.h
install -m 664 include/pgenheaders.h /scratch/paul/qemu/install/host-i686-mingw32/include/pgenheaders.h
install -m 664 include/object.h /scratch/paul/qemu/install/host-i686-mingw32/include/object.h
install -m 664 include/pyerrors.h /scratch/paul/qemu/install/host-i686-mingw32/include/pyerrors.h
install -m 664 include/funcobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/funcobject.h
install -m 664 include/grammar.h /scratch/paul/qemu/install/host-i686-mingw32/include/grammar.h
install -m 664 include/intrcheck.h /scratch/paul/qemu/install/host-i686-mingw32/include/intrcheck.h
install -m 664 include/sliceobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/sliceobject.h
install -m 664 include/ast.h /scratch/paul/qemu/install/host-i686-mingw32/include/ast.h
install -m 664 include/pystate.h /scratch/paul/qemu/install/host-i686-mingw32/include/pystate.h
install -m 664 include/traceback.h /scratch/paul/qemu/install/host-i686-mingw32/include/traceback.h
install -m 664 include/pymem.h /scratch/paul/qemu/install/host-i686-mingw32/include/pymem.h
install -m 664 include/errcode.h /scratch/paul/qemu/install/host-i686-mingw32/include/errcode.h
install -m 664 include/setobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/setobject.h
install -m 664 include/compile.h /scratch/paul/qemu/install/host-i686-mingw32/include/compile.h
install -m 664 include/moduleobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/moduleobject.h
install -m 664 include/unicodeobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/unicodeobject.h
install -m 664 include/structseq.h /scratch/paul/qemu/install/host-i686-mingw32/include/structseq.h
install -m 664 include/classobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/classobject.h
install -m 664 include/weakrefobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/weakrefobject.h
install -m 664 include/bytesobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/bytesobject.h
install -m 664 include/structmember.h /scratch/paul/qemu/install/host-i686-mingw32/include/structmember.h
install -m 664 include/tupleobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/tupleobject.h
install -m 664 include/warnings.h /scratch/paul/qemu/install/host-i686-mingw32/include/warnings.h
install -m 664 include/frameobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/frameobject.h
install -m 664 include/graminit.h /scratch/paul/qemu/install/host-i686-mingw32/include/graminit.h
install -m 664 include/Python.h /scratch/paul/qemu/install/host-i686-mingw32/include/Python.h
install -m 664 include/pythonrun.h /scratch/paul/qemu/install/host-i686-mingw32/include/pythonrun.h
install -m 664 include/datetime.h /scratch/paul/qemu/install/host-i686-mingw32/include/datetime.h
install -m 664 include/dictobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/dictobject.h
install -m 664 include/floatobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/floatobject.h
install -m 664 include/ucnhash.h /scratch/paul/qemu/install/host-i686-mingw32/include/ucnhash.h
install -m 664 include/eval.h /scratch/paul/qemu/install/host-i686-mingw32/include/eval.h
install -m 664 include/fileobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/fileobject.h
install -m 664 include/ceval.h /scratch/paul/qemu/install/host-i686-mingw32/include/ceval.h
install -m 664 include/pyconfig.h /scratch/paul/qemu/install/host-i686-mingw32/include/pyconfig.h
install -m 664 include/marshal.h /scratch/paul/qemu/install/host-i686-mingw32/include/marshal.h
install -m 664 include/boolobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/boolobject.h
install -m 664 include/pystrcmp.h /scratch/paul/qemu/install/host-i686-mingw32/include/pystrcmp.h
install -m 664 include/pymactoolbox.h /scratch/paul/qemu/install/host-i686-mingw32/include/pymactoolbox.h
install -m 664 include/methodobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/methodobject.h
install -m 664 include/osdefs.h /scratch/paul/qemu/install/host-i686-mingw32/include/osdefs.h
install -m 664 include/bytes_methods.h /scratch/paul/qemu/install/host-i686-mingw32/include/bytes_methods.h
install -m 664 include/abstract.h /scratch/paul/qemu/install/host-i686-mingw32/include/abstract.h
install -m 664 include/timefuncs.h /scratch/paul/qemu/install/host-i686-mingw32/include/timefuncs.h
install -m 664 include/bufferobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/bufferobject.h
install -m 664 include/Python-ast.h /scratch/paul/qemu/install/host-i686-mingw32/include/Python-ast.h
install -m 664 include/genobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/genobject.h
install -m 664 include/modsupport.h /scratch/paul/qemu/install/host-i686-mingw32/include/modsupport.h
install -m 664 include/pymath.h /scratch/paul/qemu/install/host-i686-mingw32/include/pymath.h
install -m 664 include/cellobject.h /scratch/paul/qemu/install/host-i686-mingw32/include/cellobject.h
ln -s ../bin/python26.dll /scratch/paul/qemu/install/host-i686-mingw32/lib/python26.dll
popd
popenv

# task [079/090] /i686-mingw32/qemu/configure
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
rm -rf /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-mingw32
mkdir -p /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-mingw32
pushd /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-mingw32
/scratch/paul/qemu/src/qemu-symbian-svp/configure --build=i686-pc-linux-gnu --target=arm-none-symbianelf --prefix=/opt/codesourcery --host=i686-mingw32 '--with-pkgversion=Symbian QEMU 0.9.1-12' --with-bugurl=https://support.codesourcery.com/GNUToolchain/ --disable-nls --enable-mingw32 --audio-drv-list= --sdl-config=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/bin/sdl-config --png-config=/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/bin/libpng-config --with-python=/scratch/paul/qemu/install/host-i686-mingw32 --cpu=i386 --cross-prefix=i686-mingw32- --host-cc=i686-pc-linux-gnu-gcc --target-list=,arm-softmmu --extra-cflags=-I/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/include --extra-ldflags=-L/scratch/paul/qemu/obj/host-libs-0.9.1-12-arm-none-symbianelf-i686-mingw32/usr/lib
popd
popenv
popenv
popenv

# task [080/090] /i686-mingw32/qemu/build
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-mingw32
make -j4
popd
popenv
popenv
popenv

# task [081/090] /i686-mingw32/qemu/install
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushenv
pushenv
pushd /scratch/paul/qemu/obj/qemu-0.9.1-12-arm-none-symbianelf-i686-mingw32
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/bin
install -m 755 arm-softmmu/qemu-system-arm.exe /scratch/paul/qemu/install/host-i686-mingw32/bin/arm-none-symbianelf-qemu-system.exe
popd
pushd /scratch/paul/qemu/src/qemu-symbian-svp
mkdir -p /scratch/paul/qemu/install/host-i686-mingw32/share/qemu/plugins
install -m 644 plugins/qemu_arm_plugins.py /scratch/paul/qemu/install/host-i686-mingw32/share/qemu/plugins
install -m 644 plugins/syborg_fb.py /scratch/paul/qemu/install/host-i686-mingw32/share/qemu/plugins
install -m 644 plugins/syborg_interrupt.py /scratch/paul/qemu/install/host-i686-mingw32/share/qemu/plugins
install -m 644 plugins/syborg_keyboard.py /scratch/paul/qemu/install/host-i686-mingw32/share/qemu/plugins
install -m 644 plugins/syborg_pointer.py /scratch/paul/qemu/install/host-i686-mingw32/share/qemu/plugins
install -m 644 plugins/syborg_rtc.py /scratch/paul/qemu/install/host-i686-mingw32/share/qemu/plugins
install -m 644 plugins/syborg_serial.py /scratch/paul/qemu/install/host-i686-mingw32/share/qemu/plugins
install -m 644 plugins/syborg_timer.py /scratch/paul/qemu/install/host-i686-mingw32/share/qemu/plugins
popd
popenv
popenv
popenv

# task [082/090] /i686-mingw32/pretidy_installation
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
pushd /scratch/paul/qemu/install/host-i686-mingw32
popd
popenv

# task [083/090] /i686-mingw32/remove_libtool_archives
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
find /scratch/paul/qemu/install/host-i686-mingw32 -name '*.la' -exec rm '{}' ';'
popenv

# task [084/090] /i686-mingw32/strip_host_objects
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
i686-mingw32-strip /scratch/paul/qemu/install/host-i686-mingw32/bin/arm-none-symbianelf-dtc.exe
i686-mingw32-strip /scratch/paul/qemu/install/host-i686-mingw32/bin/arm-none-symbianelf-ftdump.exe
i686-mingw32-strip /scratch/paul/qemu/install/host-i686-mingw32/bin/arm-none-symbianelf-qemu-system.exe
popenv

# task [085/090] /i686-mingw32/package_tbz2
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
pushenvvar CC i686-mingw32-gcc
pushenvvar AR i686-mingw32-ar
pushenvvar RANLIB i686-mingw32-ranlib
prepend_path PATH /scratch/paul/qemu/obj/tools-i686-pc-linux-gnu-0.9.1-12-arm-none-symbianelf-i686-mingw32/bin
rm -f /scratch/paul/qemu/pkg/symbian-qemu-0.9.1-12-arm-none-symbianelf-i686-mingw32.tar.bz2
pushd /scratch/paul/qemu/install/host-i686-mingw32
rm ./lib/python26.dll
ln /scratch/paul/qemu/install/host-i686-mingw32/bin/python26.dll ./lib/python26.dll
popd
pushd /scratch/paul/qemu/obj
rm -f symbian-qemu-0.9.1
ln -s /scratch/paul/qemu/install/host-i686-mingw32 symbian-qemu-0.9.1
tar cjf /scratch/paul/qemu/pkg/symbian-qemu-0.9.1-12-arm-none-symbianelf-i686-mingw32.tar.bz2 --owner=0 --group=0 --exclude=host-i686-pc-linux-gnu --exclude=host-i686-mingw32 symbian-qemu-0.9.1/bin symbian-qemu-0.9.1/include symbian-qemu-0.9.1/lib symbian-qemu-0.9.1/share
rm -f symbian-qemu-0.9.1
popd
popenv

# task [086/090] /fini/build_summary
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
cat > /scratch/paul/qemu/obj/gnu-0.9.1-12-arm-none-symbianelf.txt <<'EOF0'
Version Information
===================

Version:           0.9.1-12
Host spec(s):      i686-pc-linux-gnu i686-mingw32
Target:            arm-none-symbianelf

Build Information
=================

Build date:             20090402
Build machine:          henry2
Build operating system: lenny/sid
Build uname:            Linux henry2 2.6.24-19-server #1 SMP Wed Aug 20 18:43:06 UTC 2008 x86_64 unknown unknown GNU/Linux
Build user:             paul

EOF0
popenv

# task [087/090] /fini/backups_package
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup
pushd /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf
tar cjf /scratch/paul/qemu/pkg/symbian-qemu-0.9.1-12-arm-none-symbianelf.backup.tar.bz2 --owner=0 --group=0 symbian-qemu-0.9.1-12-arm-none-symbianelf.backup
popd
popenv

# task [088/090] /fini/sources_package
pushenv
pushenvvar CC_FOR_BUILD i686-pc-linux-gnu-gcc
mkdir -p /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
cp /scratch/paul/qemu/obj/gnu-0.9.1-12-arm-none-symbianelf.txt /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
cp /scratch/paul/qemu/logs/symbian-qemu-0.9.1-12-arm-none-symbianelf.sh /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf/symbian-qemu-0.9.1-12-arm-none-symbianelf
pushd /scratch/paul/qemu/obj/pkg-0.9.1-12-arm-none-symbianelf
tar cjf /scratch/paul/qemu/pkg/symbian-qemu-0.9.1-12-arm-none-symbianelf.src.tar.bz2 --owner=0 --group=0 symbian-qemu-0.9.1-12-arm-none-symbianelf
popd
/scratch/paul/qemu/src/scripts-trunk/gnu-test -i /scratch/paul/qemu/install -l /scratch/paul/qemu/logs -o /scratch/paul/qemu/obj -p /scratch/paul/qemu/pkg -s /scratch/paul/qemu/src -T /scratch/paul/qemu/testlogs -T /scratch/paul/qemu/obj/testlogs-0.9.1-12-arm-none-symbianelf symbian-qemu
copy_dir /scratch/paul/qemu/obj/testlogs-0.9.1-12-arm-none-symbianelf /scratch/paul/qemu/testlogs
/scratch/paul/qemu/src/scripts-trunk/gnu-test-package -i /scratch/paul/qemu/install -l /scratch/paul/qemu/logs -o /scratch/paul/qemu/obj -p /scratch/paul/qemu/pkg -s /scratch/paul/qemu/src -T /scratch/paul/qemu/testlogs -T /scratch/paul/qemu/obj/testlogs-0.9.1-12-arm-none-symbianelf symbian-qemu
