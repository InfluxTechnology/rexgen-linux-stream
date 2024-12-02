#!/bin/sh

PWD="/home/root/rexusb/"
COUNT=$("$PWD"rexgen SD log count | tr -dc '0-9')
echo "Found " $COUNT " files"
COUNT=$((COUNT-1))

for i in $(seq 0 "$COUNT"); do
    FILE=$("$PWD"rexgen SD file "$i")
    echo -ne "["$((i + 1))"] "$FILE "... "
    RES=$("$PWD"rexgen SD rxd "$i")
    if [ "$RES" != "" ]; then 
	echo "ERROR"
	continue
    else
	echo "Done " 
    fi

#    mv $FILE "/home/root/"
done

# for optimization, previous SD commands stops this service
systemctl start rexgen_data.service