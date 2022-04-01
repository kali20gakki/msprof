#!/bin/bash

MDC_DRV_SRC_DEVICE=/opt/third_party/device


if [ ! -d "$MDC_DRV_SRC_DEVICE" ]; then
    echo "$MDC_DRV_SRC_DEVICE not exist!"
        exit 1
fi

cp $MDC_DRV_SRC_DEVICE/adda /var/.
cp $MDC_DRV_SRC_DEVICE/mdc_ide_daemon_start.sh /var/.
cp $MDC_DRV_SRC_DEVICE/mdc_ide_daemon.service /lib/systemd/system/.
chmod +x /var/mdc_ide_daemon_start.sh

for i in {0..7}
do
  ln -s /lib/systemd/system/mdc_ide_daemon.service /etc/systemd/system/mdc_mini_${i}.target.wants/mdc_ide_daemon.service
done
