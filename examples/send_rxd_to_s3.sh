#!/bin/sh

# Check if enough arguments are passed
if [ "$#" -ne 6 ]; then
    echo "Usage: $0 <WATCH_DIR> <AWS_ACCESS_KEY> <AWS_SECRET_KEY> <BUCKET_NAME> <SERIAL>"
    exit 1
fi

# Read parameters
WATCH_DIR="$1"
AWS_ACCESS_KEY="$2"
AWS_SECRET_KEY="$3"
BUCKET="$4"
REGION="$5"
SERIAL="$6"

# Export AWS credentials for this session
export AWS_ACCESS_KEY_ID="$AWS_ACCESS_KEY"
export AWS_SECRET_ACCESS_KEY="$AWS_SECRET_KEY"

while true; do
	AWS_ACCESS_KEY=$("$PWD"/examples/read_aws_conf.sh ACCESS_KEY)
	AWS_SECRET_KEY=$("$PWD"/examples/read_aws_conf.sh SECRET_KEY)
	BUCKET=$("$PWD"/examples/read_aws_conf.sh BUCKET)
	REGION=$("$PWD"/examples/read_aws_conf.sh REGION)

    # Get the first file in the folder (sorted by name)
    FILE=$(ls -1 "$WATCH_DIR"/*.rxd 2>/dev/null | head -n 1)

    # If no file is found, wait and check again
    if [ -z "$FILE" ]; then
        sleep 5  # Wait 5 seconds before checking again
        continue
    fi

    FILE_BASE=$(basename "$FILE")
    DATE_BASE=$(echo "$FILE_BASE" | rev | cut -d "_" -f 2 | rev)

    echo "Uploading: $FILE"

    # Run the AWS upload command
	/home/root/rexusb/aws upload -file $FILE -key $DATE_BASE/$FILE_BASE -accesskey $AWS_ACCESS_KEY -secretkey $AWS_SECRET_KEY -region $REGION -bucket $BUCKET/$SERIAL

    # Check if the upload was successful
    if [ $? -eq 0 ]; then
        echo "Upload successful, deleting: $FILE"
        rm -f "$FILE"
    else
        echo "Upload failed, keeping file: $FILE"
    fi

    sleep 2  # Small delay before checking the next file
done
