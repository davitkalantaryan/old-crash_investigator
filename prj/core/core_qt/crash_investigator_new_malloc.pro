#
# File crash_investigateor.pro
# File created : 18 Nov 2021
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
# CONFIG += 2test

include("$${PWD}/crash_investigator_libs_common.pri")
TARGET = libcrash_investiator_new_malloc.so


OTHER_FILES +=	\
	$${PWD}/../core_mkfl/crash_investigator_new_malloc.unix.Makefile
