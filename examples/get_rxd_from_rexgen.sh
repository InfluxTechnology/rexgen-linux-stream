#!/bin/sh

PWD="/home/root/rexusb/"

RXD_DIR=$(cat "$PWD"etc/rexgen.conf | grep 'RXD_FOLDER' | cut -d '=' -f2)

COUNT=$("$PWD"rexgen SD log count | tr -dc '0-9')
echo "Found " $COUNT " rxd files "
COUNT=$((COUNT-1))

rm $RXD_DIR*.rxd

for i in $(seq 0 "$COUNT"); do
        FILE=$("$PWD"rexgen SD file "$i")
        echo  "["$((i + 1))"] "$FILE "... "
        RES=$("$PWD"rexgen SD rxd "$i")
done

# for optimization, previous SD commands stops this service
systemctl start rexgen_data.service
