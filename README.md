# AWS IoT Framework for Automotive Grade Linux (AGL)

AWS IoT Framework for AGL is the reference implementation to enable native integration of AWS Greengrass and AWS IoT into the AGL software stack. The AWS IoT Framework is composed of the AWS Greengrass Core and a AWS IoT binding service. The AWS IoT binding service is built using AGL Framework SDK and AWS IoT device SDK to provide APIs for publishing and subscribing to MQTT topics on AWS Greengrass core.

## Build an AGL image for AWS Greengrass Core

Set up the build environment to create an AGL image by following the steps from [AGL Documentation](http://docs.automotivelinux.org/docs/devguides/en/dev/reference/sdk-devkit/docs/part-1/1_0_Abstract.html)

When you click on the link, you will see the steps as outlined below
1.	Setting up your operating system
2.	Install Docker image; start the container
3.	Setup the build environment inside the container
4.  **Perform kernel configurations required by AWS Greengrass, for more information check [AWS Greengrass FAQs](https://aws.amazon.com/greengrass/faqs/) (search for ‘Kernel configuration’)**
```
Key Retention: CONFIG_KEYS
Mqueue: CONFIG_POSIX_MQUEUE
Overlay FS: CONFIG_OVERLAY_FS
Seccomp Arch Filter: CONFIG_HAVE_ARCH_SECCOMP_FILTER
Seccomp Filter: CONFIG_SECCOMP_FILTER
Seccomp: CONFIG_SECCOMP
IPC isolation: CONFIG_IPC_NS
UTS isolation: CONFIG_UTS_NS
User isolation: CONFIG_USER_NS
PID isolation: CONFIG_PID_NS
Enable cgroups: CONFIG_CGROUPS
Enable Memory cgroup: CONFIG_MEMCG
Enable devices cgroup: CONFIG_CGROUP_DEVICE
```
5.	Build the image
6.	Deploy the image on target board

## Environment Setup for AWS Greengrass

*	Connect to the AGL system on target board through SSH
* Add ggc_user and ggc_group to AGL system
```
adduser --system ggc_user
addgroup --system ggc_group
```
* Update /etc/fstab
```
echo 'cgroup               /sys/fs/cgroup       cgroup     defaults              0  0' >> /etc/fstab
```
*	Update SMACK permissions
```
sed -i 's/System _ -----l/System _ -wx--l/g' /etc/smack/accesses.d/default-access-domains
```
* Check if the AGL system is ready to run AWS Greengrass Core, more information can be found [here](https://docs.aws.amazon.com/greengrass/latest/developerguide/module1.html) (scroll down to ‘Setting Up Other Devices’)
```
git clone https://github.com/aws-samples/aws-greengrass-samples.git
cd greengrass-dependency-checker-GGCvx.x.x
sudo ./check_ggc_dependencies
```

## Install AWS Greengrass Core on AGL
* Configure AWS Greengrass on AWS IoT via AWS Console, follow there steps from [AWS Greengrass Documentation](https://docs.aws.amazon.com/greengrass/latest/developerguide/gg-config.html)
* Ensure to download the appropriate version of Greengrass Core for hardware platform. At the end you have downloaded the gzip files for Greengrass Core software and related security resources.
*	Start AWS Greengrass Core on AGL, follow there steps from [AWS Greengrass Documentation](https://docs.aws.amazon.com/greengrass/latest/developerguide/gg-device-start.html)
*	[Optional] Test the Greengrass on AGL by running the sample AWS lambda examples from [here](https://docs.aws.amazon.com/greengrass/latest/developerguide/module3-I.html)

## Install AWS IoT Binding on AGL
*	Create AWS IoT Device in the same AWS Greengrass group, for reference, follow the steps from [here](https://docs.aws.amazon.com/greengrass/latest/developerguide/device-group.html) At the end you have downloaded the gzip file for Device’s security resources i.e. certificate, private key, etc.
*	Login to the Docker container for AGL, and install the AGL SDK environment, follow the steps from [AGL Documentation](http://docs.automotivelinux.org/docs/devguides/en/dev/reference/sdk-devkit/docs/part-1/1_7-SDK-compilation-installation.html)
* Setup
```
# SSH to docker container
source /xdt/sdk/environment-setup-aarch64-agl-linux
cd ~
git clone --recursive https://github.com/awslabs/aws-iot-binding-for-agl
cd aws-iot-binding-for-agl/source/aws-iot-binding
cd data/certs
# Copy AWS IoT Device certificates/keys
# Copy AWS IoT root certificate from Symantec
curl -o ./root-ca-cert.pem http://www.symantec.com/content/en/us/enterprise/verisign/roots/VeriSign-Class%203-Public-Primary-Certification-Authority-G5.pem
cd config
# Update the config.json with the correct filename for certificates, thing_name, client_id and endpoint for Greengrass e.g. greengrass.iot.us-east-1.amazonaws.com
```
* Build
```
# Assuming /home/devel/mirror is shared between the host OS and docker container
~/aws-iot-binding-for-agl/deployment/build-wgt.sh ~/aws-iot-binding-for-agl/source/
```
* Deploy on AGL
```
# Run this from the host OS; outside of docker container
# Assuming ~/mirror on host OS is the shared directory between the host OS and docker container
export YOUR_BOARD_IP=192.168.1.X
export APP_NAME=aws-iot-service
scp ~/mirror/${APP_NAME}.wgt root@${YOUR_BOARD_IP}:/tmp
ssh root@${YOUR_BOARD_IP} afm-util install /tmp/${APP_NAME}.wgt
APP_VERSION=$(ssh root@${YOUR_BOARD_IP} afm-util list | grep ${APP_NAME}@ | cut -d"\"" -f4| cut -d"@" -f2)
ssh root@${YOUR_BOARD_IP} afm-util start ${APP_NAME}@${APP_VERSION}
```

## TEST AWS IoT Framework on AGL
* Configure the Greengrass group subscription to forward the messages from device to the cloud and device to device for topic: 'hello/world', for reference on how to configure subscriptions check [this](https://docs.aws.amazon.com/greengrass/latest/developerguide/config-subs.html) link

```
export YOUR_BOARD_IP=192.168.1.X
export PORT=8000
ssh root@${YOUR_BOARD_IP}
afb-daemon --ws-client=unix:/run/user/0/apis/ws/aws-iot-service --port=${PORT} --token='x' -v

#On an other terminal
ssh root@${YOUR_BOARD_IP}
afb-client-demo -H 127.0.0.1:${PORT}/api?token=x
aws-iot-service subscribe {"topic":"hello/world"}
aws-iot-service publish {"topic":"hello/world","message":"Hello from AGL!!!"}
```

* You should see the messages on topic “hello/world” being published on cloud via AWS IoT console and being forwarded as AFB event on the device console.
***

Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.

Licensed under the Amazon Software License (the "License"). You may not use this file except in compliance with the License. A copy of the License is located at

    http://aws.amazon.com/asl/

or in the "license" file accompanying this file. This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or implied. See the License for the specific language governing permissions and limitations under the License.
