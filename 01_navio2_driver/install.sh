#!/bin/bash

# Compile the overlay
echo "Compile and publish overlay..."
dtc -@ -I dts -O dtb -o rcio.dtbo rcio-overlay.dts
cp rcio.dtbo overlays/
echo "Done."

# Build driver
echo "Compile driver..."
make
echo "Done."

# Update DMKS
version=`dkms status | head -1 | awk -F, '{print $2;}' | sed 's/ /rcio\//g'`

if [ -n "$version" ]; then
  echo "Remove old kernel module..."
  dkms remove "$version" --all
  echo "Done."
fi

echo "Install new kernel module..."
dkms install . --force
echo "Done."

# Add module to autostart
echo "Add kernel module to autostart..."
mkdir -p /lib/modules/$(uname -r)/kernel/drivers/rcio
cp rcio_core.ko /lib/modules/$(uname -r)/kernel/drivers/rcio/
cp rcio_spi.ko /lib/modules/$(uname -r)/kernel/drivers/rcio/
echo -e "rcio_core\nrcio_spi" > modules-load.d/rcio.conf
echo "Done."

echo "Refresh dependencies..."
depmod -a
echo "Done."

# Re-launch kernel module
echo "Re-launche kernel module..."
modprobe -r rcio_spi
insmod rcio_core.ko
insmod rcio_spi.ko
echo "Done."
echo "Kernel module installed."