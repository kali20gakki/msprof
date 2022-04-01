#!/bin/sh
INSTALL="/etc/ascend_install.info"
if [ ! -f "$INSTALL" ]; then
    INSTALL="/etc/HiAI_install.info"
fi
cat_install_info(){
    . $INSTALL
    driverInstallPath=$Driver_Install_Path_Param
    if [ -z "$driverInstallPath" ];then
        driverInstallPath="/usr/local/Ascend"
    fi

    if [ -f "$INSTALL" ];then
        cat $INSTALL
    else
        echo "not found install info"
    fi

    if [ -f "$driverInstallPath"/driver/scene.info ];then
        cat "$driverInstallPath"/driver/scene.info
        echo ""
    else
        echo "not found scene info"
    fi

    if [ -f "$driverInstallPath"/driver/version.info ];then
        cat "$driverInstallPath"/driver/version.info
    else
        echo "not found version info"
    fi
}

while true
do
    case "$1" in
    --install_info)
        cat_install_info
        exit 0
        ;;
    --*)
        shift
        continue
        ;;
    *)
        break
        ;;
    esac
done
