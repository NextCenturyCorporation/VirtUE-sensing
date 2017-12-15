#!/bin/sh


# push the get_certificates.py script and requirements where ever we need it
echo "Updating Certificate tools"
for push_dir in "control/api_server/tools"
do
	echo " ... [$push_dir]"
	cp ./tools/certificates/get_certificates.py ./$push_dir/get_certificates.py
	cp ./tools/certificates/requirements.txt ./$push_dir/get_certificates_requirements.txt
done