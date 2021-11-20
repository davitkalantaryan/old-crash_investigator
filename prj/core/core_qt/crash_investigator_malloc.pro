#
# File crash_investigateor.pro
# File created : 18 Nov 2021
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
# CONFIG += 2test


include("$${PWD}/crash_investigator_libs_common.pri")
TARGET = libcrash_investiator_malloc.so
DEFINES += CRASH_INVEST_DO_NOT_USE_MAL_FREE
