# AWS IoT Framework for Automotive Grade Linux (AGL)

AWS IoT Framework for AGL is the reference implementation to enable
native integration of AWS IoT Greengrass and AWS IoT into the AGL
software stack. The AWS IoT Framework is composed of the AWS
Greengrass Core and a AWS IoT binding service. The AWS IoT binding
service is built using AGL Framework SDK and AWS IoT device SDK to
provide APIs for publishing and subscribing to MQTT topics on AWS
Greengrass core.

## Build an AGL image for AWS Greengrass Core

Starting in 2019, the [meta-aws
project](https://github.com/aws/meta-aws) provides mechanisms for
installing AWS IoT Greengrass and other AWS software to Embedded
Linux.  Please reference that repository for integrating AWS IoT
Greengrass with Automotive Grade Linux.

## Install AWS IoT Binding on AGL
*	Create AWS IoT Device in the same AWS Greengrass group, for
     reference, follow the steps from
     [here](https://docs.aws.amazon.com/greengrass/latest/developerguide/device-group.html)
     At the end you have downloaded the gzip file for Device’s
     security resources i.e. certificate, private key, etc.
*	Login to the Docker container for AGL, and install the AGL SDK
     environment, follow the steps from [AGL
     Documentation](http://docs.automotivelinux.org/docs/devguides/en/dev/reference/sdk-devkit/docs/part-1/1_7-SDK-compilation-installation.html)
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
