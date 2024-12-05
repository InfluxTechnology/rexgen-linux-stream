#!/bin/sh

if [[ $1 != "ACCESS_KEY" ]] && [[ $1 != "SECRET_KEY" ]] && [[ $1 != "BUCKET" ]]; then
        echo "Usage: read_aws_conf.sh ACCESS_KEY | SECRET_KEY | BUCKET"
        exit
fi

AWS_CONF="/home/root/rexusb/etc/aws.conf"

RES=$(cat $AWS_CONF | grep "$1" | cut -d '=' -f2)
echo $RES
