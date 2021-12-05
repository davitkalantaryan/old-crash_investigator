#!/bin/bash

#
# file:			upload_to_supported_hosts.sh
# path:			scripts/upload_to_supported_hosts.sh
# created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
# created on:	2021 Dec 05
#
# the aim:		to uploa library and scripts to supported hosts
#

versionToUpload=0020

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
cd ../..
repositoryRoot=`pwd`
echo repositoryRoot=$repositoryRoot

lsbRelease=`lsb_release -sc`


uploadToSingleHost()
{
	scp ${repositoryRoot}/sys/${lsbRelease}/Debug/lib/libcrash_investigator_new_malloc_${versionToUpload}.so root@$1:/local/lib/.
	scp ${repositoryRoot}/scripts/doocs_under_invest ${repositoryRoot}/scripts/run_under_invest   root@$1:/export/doocs/server/.
}


while [ $# -gt 0 ]
do
	uploadToSingleHost $1
	shift
done
