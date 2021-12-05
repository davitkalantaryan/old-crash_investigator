#!/bin/bash

#
# file:			upload_to_supported_hosts.sh
# path:			scripts/upload_to_supported_hosts.sh
# created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
# created on:	2021 Dec 05
#
# the aim:		to uploa library and scripts to supported hosts
#


scriptDirectory=`dirname "${0}"`
scriptFileName=`basename "${0}"`
cd "${scriptDirectory}"
fileOrigin=`readlink "${scriptFileName}"` || :
while [ ! -z "${fileOrigin}" ]
do
	scriptDirectory=`dirname "${fileOrigin}"`
	scriptFileName=`basename "${fileOrigin}"`
	cd "${scriptDirectory}"
	fileOrigin=`readlink "${scriptFileName}"`  || :
done
cd ..
repositoryRoot=`pwd`
echo repositoryRoot=$repositoryRoot


${repositoryRoot}/scripts/raw/_upload_to_supported_hosts.sh	\
	picus10													\
	picus11													\
	picus12													\
	picus13													\
	picus14													
