# !/bin/bash

echo "download the example file" 
wget http://t2solve.com/realbugtracker/rec97371911.tar.gz
echo "download the checksum file " 
wget http://t2solve.com/realbugtracker/rec97371911.tar.gz.sha512
echo "validate checksum" 
sha512sum -c rec97371911.tar.gz.sha512
