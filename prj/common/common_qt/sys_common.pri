# File sys_common.pri
# File created : 12 Feb 2017
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
#

PRJ_PWD			= $${PWD}/../../..
repositoryRoot		= $${PWD}/../../..
cpputilsRepoRoot	= $${repositoryRoot}/contrib/cpputils
systemRepoRoot		= $${repositoryRoot}/contrib/system
include ( "$${cpputilsRepoRoot}/prj/common/common_qt/sys_common.pri" )

CONFIG += c++14

OTHER_FILES +=	\
	$${repositoryRoot}/prj/common/common_mkfl/sys_common.unix.Makefile
