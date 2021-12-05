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

pcUserName=`id -nu`
username1=${pcUserName}
username2=${username1}

if [ $# -gt 0 ]; then
	username1=$1
fi

if [ $# -gt 1 ]; then
	username2=$2
fi

echo username1=${username1}
echo username2=${username2}


workAsUserFromDeplHostDevHost()(
	
	userNameTmp=$1
	deplHostnameTmp=$2
	devHostnameTmp=$3
	lsbRelease=$4
	
	echo 
	
#ssh -T -o "UserKnownHostsFile=/dev/null" -o "StrictHostKeyChecking=no" ${userNameTmp}@pitzdev1 << EOSSH1
ssh -T ${userNameTmp}@pi4-vm2 << EOSSH1

#ssh -T -o "UserKnownHostsFile=/dev/null" -o "StrictHostKeyChecking=no" root@${deplHostnameTmp} << EOSSH2
ssh -T root@${deplHostnameTmp} << EOSSH2

	hostname
	id -nu
	echo lsbRelease is $lsbRelease
	echo pcUserName is ${pcUserName}
	echo devHostnameTmp is ${devHostnameTmp}
	#scp -oPasswordAuthentication=yes ${pcUserName}@${devHostnameTmp}:~/dev/crash_investigator/sys/${lsbRelease}/Debug/lib/libcrash_investigator_new_malloc*.so /local/lib/. 
	scp ${pcUserName}@${devHostnameTmp}:~/dev/crash_investigator/sys/${lsbRelease}/Debug/lib/libcrash_investigator_new_malloc*.so /local/lib/.
	# scp ${pcUserName}@${devHostnameTmp}:~/dev/crash_investigator/scripts/* /export/doocs/server/.

	
EOSSH2
EOSSH1


)

#workAsUserFromDeplHostDevHost ${username1} pitzcpusync1 pitzcpudev1 focal
workAsUserFromDeplHostDevHost ${username2} pitzdev1 picus10 Nitrogen
