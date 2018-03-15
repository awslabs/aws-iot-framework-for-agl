#!/bin/bash -i

# This script should be run from inside the docker container for AGL
# It expects the full path to the source directory as an argument
# The script will copy the widget file to /home/devel/mirror/ assuming its the mirror directory shared between the host OS and docker container
# It will download & install the SDK envrionment (v4.0.2) for building the aws-iot-binder service for R-Car M3 reference board

# Check to see if input has been provided:
if [ -z "$1" ]; then
    echo "Please provide the path to source directory.\nFor example: /home/devel/mirror/aws-iot-binding-for-agl/source/"
    exit 1
fi

echo "Download the AGL SDK (v4.0.2 for aarch64)"
mkdir -p /xdt/build/tmp/deploy/sdk
cd /xdt/build/tmp/deploy/sdk
wget https://download.automotivelinux.org/AGL/release/dab/4.0.2/m3ulcb-nogfx/deploy/sdk/poky-agl-glibc-x86_64-agl-image-ivi-crosssdk-aarch64-toolchain-4.0.2.sh
chmod 755 poky-agl-glibc-x86_64-agl-image-ivi-crosssdk-aarch64-toolchain-4.0.2.sh
echo "Install the AGL SDK (v4.0.2 for aarch64)"
install_sdk /xdt/build/tmp/deploy/sdk/poky-agl-glibc-x86_64-agl-image-ivi-crosssdk-aarch64-toolchain-4.0.2.sh
echo "Source the SDK build envrionment"
rm -Rf /tmp/source
cp -r $1 /tmp
source /xdt/sdk/environment-setup-aarch64-agl-linux
cd /tmp/source/aws-iot-binding
mkdir -p build
cd build
echo "Running cmake .."
cmake ..
echo "Running cmake .."
cmake ..
echo "Running make aws-iot-service"
make aws-iot-service
echo "Running make widget"
make widget
cp aws-iot-service.wgt /home/devel/mirror/
