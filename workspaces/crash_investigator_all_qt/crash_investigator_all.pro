#
# file:			sps_all.pro
# path:			workspaces/sps_all_qt/sps_all.pro
# created on:		2021 Nov 18
# creatd by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
# purpose:		Qt Project file
#

include ( "$${PWD}/../../prj/common/common_qt/sys_common.pri" )

message("repositoryRoot: " $$repositoryRoot)

TEMPLATE = subdirs
CONFIG += ordered

wcsServer {
	SUBDIRS		+=	"$${repositoryRoot}/prj/crash_investigations/wcs_server_qt/wcs_server.pro"
}

#crashInvestigatorLibs {
        #SUBDIRS		+=	"$${repositoryRoot}/prj/core/core_qt/crash_investigator_new.pro"
        #SUBDIRS		+=	"$${repositoryRoot}/prj/core/core_qt/crash_investigator_malloc.pro"
	SUBDIRS		+=	"$${repositoryRoot}/prj/core/core_qt/crash_investigator_new_malloc.pro"
#}

includeCppUtils {
	SUBDIRS		+=	"$${repositoryRoot}/contrib/cpputils/workspaces/cpputils_qt/cpputils.pro"
}

#testsFromWorkspace {
	SUBDIRS		+=	"$${repositoryRoot}/prj/tests/double_free01_test_qt/double_free01_test.pro"
#}



#UNIX_SCRIPTS	= $$cpputilsFindFilesRecursive($${repositoryRoot}/scripts, .sh)
#WINDOWS_SCRIPTS	= $$cpputilsFindFilesRecursive($${repositoryRoot}/scripts, .bat)
#MDS_IN_DOCS	= $$cpputilsFindFilesRecursive($${repositoryRoot}/docs, .md)
#TXT_IN_DOCS	= $$cpputilsFindFilesRecursive($${repositoryRoot}/docs, .txt)

OTHER_FILES	=
#OTHER_FILES += $${UNIX_SCRIPTS}
#OTHER_FILES += $${WINDOWS_SCRIPTS}
#OTHER_FILES += $${MDS_IN_DOCS}
#OTHER_FILES += $${TXT_IN_DOCS}

OTHER_FILES	+=	\
	"$${repositoryRoot}/.gitattributes"								\
	"$${repositoryRoot}/.gitignore"									\
	"$${repositoryRoot}/.gitmodules"								\
	"$${repositoryRoot}/LICENSE"									\
	"$${repositoryRoot}/README.md"									\
	"$${repositoryRoot}/Makefile"	
