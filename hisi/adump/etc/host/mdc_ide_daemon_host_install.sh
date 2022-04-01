#!/bin/bash

MDC_DRV_SRC_HOST=/opt/third_party/host
TOOLS_DIR=/usr/lib/mdc/driver/tools

if [ ! -d "$MDC_DRV_SRC_HOST" ]; then
    echo "$MDC_DRV_SRC_HOST not exist!"
        exit 1
fi

if [ ! -d "$TOOLS_DIR" ]; then
    mkdir -p $TOOLS_DIR
fi

cp $MDC_DRV_SRC_HOST/ada $TOOLS_DIR/.
cp $MDC_DRV_SRC_HOST/mdc_ide_daemon_start.sh $TOOLS_DIR/.
cp $MDC_DRV_SRC_HOST/mdc_ide_daemon.service /lib/systemd/system/.
chmod +x $TOOLS_DIR/mdc_ide_daemon_start.sh

ln -s /lib/systemd/system/mdc_ide_daemon.service /etc/systemd/system/mdc_host.target.wants/mdc_ide_daemon.service
