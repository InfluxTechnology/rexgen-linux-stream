#!/bin/sh

PWD="/home/root/rexusb/"

RXD_DIR=$(cat "$PWD"etc/rexgen.conf | grep 'RXD_FOLDER' | cut -d '=' -f2)

ACCESS_KEY=$("$PWD"/examples/read_aws_conf.sh ACCESS_KEY)
SECRET_KEY=$("$PWD"/examples/read_aws_conf.sh SECRET_KEY)
BUCKET=$("$PWD"/examples/read_aws_conf.sh BUCKET)

if [[ $ACCESS_KEY == "" ]] || [[ $SECRET_KEY == "" ]] || [[ $BUCKET == "" ]]; then
        echo "Fill your Amazon S3 server settings in" "$PWD"etc/aws.conf
        exit
fi

SN_LONG=$("$PWD"rexgen serial | tr -d '\0' | cut -d " " -f 2 | tr -d 'number' | tr -d '[:space:]')

for i in $RXD_DIR*.rxd; do
        FILE=$(echo $i)
        FILE_BASE=$(basename "$FILE")
        DATE_BASE=$(echo $FILE_BASE | rev | cut -d "_" -f 2 | rev)

        aws upload -file $FILE -key $DATE_BASE/$FILE_BASE -accesskey $ACCESS_KEY -secretkey $SECRET_KEY -region eu-central-1 -bucket $BUCKET/$SN_LONG

done
