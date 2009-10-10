#!/bin/sh

PRG="$0"

# need this for relative symlinks
while [ -h "$PRG" ] ; do
    ls=`ls -ld "$PRG"`
    link=`expr "$ls" : '.*-> \(.*\)$'`
    if expr "$link" : '/.*' > /dev/null; then
        PRG="$link"
    else
        PRG=`dirname "$PRG"`"/$link"
    fi
done

DIRNAME=`dirname $PRG`

tmp_DIRNAME1=`cd $DIRNAME/..; pwd`
tmp_DIRNAME2=`cd $DIRNAME; pwd`

if [ -d "$tmp_DIRNAME1/share" ]; then
    # installed to /
    DIRNAME="$tmp_DIRNAME1"
    LIBPREFIX="$DIRNAME/lib/mysql-gui"
elif [ -d "$tmp_DIRNAME2/share" ]; then
    # installed to /opt or something similar
    DIRNAME="$tmp_DIRNAME2"
    LIBPREFIX="$DIRNAME"
else
    echo "Data files not found. Please check your installation."
    exit 1
fi

DIRNAME=/

if [ -f "$LIBPREFIX/lib/pango.modules" ]; then
    bundled_deps=1
else
    bundled_deps=0
fi

export MQB_DIR="$DIRNAME"
export LD_LIBRARY_PATH="$LIBPREFIX/lib:$LD_LIBRARY_PATH"

### begin stuff needed for bundled gtk libraries
pangorc_path=$LIBPREFIX/lib/pangorc
gdkpixbuf_path=$LIBPREFIX/lib/gdk-pixbuf.loaders
pangomodules_path=$LIBPREFIX/lib/pango.modules
desktopfile_path=$DIRNAME/MySQLQueryBrowser.desktop

if [ $bundled_deps -ne 0 ]; then
    args=""
    for arg in $*; do
        if test $arg = "--update-paths"; then
            echo "Updating `basename $0` installation paths..."
            old_prefix=$(grep ModuleFiles $pangorc_path|sed -e 's#.*=.\?\(/.*\)/lib/pango.modules#\1#')
            new_prefix=$LIBPREFIX

            # replace paths in our custom configuration files
            for f in $pangorc_path $gdkpixbuf_path $pangomodules_path $desktopfile_path; do
                sed -e "s:$old_prefix:$new_prefix:g" $f > $f.bak
                if [ $? -ne 0 ]; then
                    echo "Error updating files for new installation path."
                    echo "Please make sure `basename $0` is installed correctly and you have"
                    echo "proper write permissions in the installation directory."
                    exit 1
                fi
                mv $f.bak $f
                if [ $? -ne 0 ]; then
                    echo "Error updating files for new installation path."
                    echo "Please make sure `basename $0` is installed correctly and you have"
                    echo "proper write permissions in the installation directory."
                    exit 1
                fi
            done
            echo "Done."
            exit
        else
            args="$args \"$arg\""
        fi
    done


    # if we're in a bundle, make sure the paths in the pango and gdk-pixbuf
    # loaders are correct
    export GDK_PIXBUF_MODULE_FILE="$gdkpixbuf_path"
    export PANGO_RC_FILE="$pangorc_path"
    export GTK_EXE_PREFIX="$LIBPREFIX"

    prefix=$(grep ModuleFiles $pangorc_path|sed -e 's#.*=.\?\(/.*\)/lib/pango.modules#\1#')
        
    if ! [ -f "$prefix/lib/pango.modules" ]; then
        cat <<EOF
Error starting $0.
The actual installation path of `basename $0` is different from the
expected one. Please run $0 --update-paths (as the root
user, if needed) to have the installation directory updated.
EOF
        exit 1
    fi
fi
### end stuff needed for bundled gtk libraries


$PRG-bin $args

