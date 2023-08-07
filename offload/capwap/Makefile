# Makefile for the netfn capwap engine

obj-m += qca-nss-netfn-capwap.o
qca-nss-netfn-capwap-objs += netfn_capwap.o
qca-nss-netfn-capwap-objs += netfn_capwap_tun.o
qca-nss-netfn-capwap-objs += netfn_capwap_tunid.o
obj ?= .

ccflags-y += -DNETFN_BUILD_ID="$(BUILD_ID)"
ccflags-y += -Wall -Werror
