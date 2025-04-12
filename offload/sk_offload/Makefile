# Makefile for sk offload module

obj-m += qca-nss-netfn-sk-offload.o
qca-nss-netfn-sk-offload-objs += netfn_sk_offload.o
qca-nss-netfn-sk-offload-objs += netfn_sk_offload_db.o
qca-nss-netfn-sk-offload-objs += netfn_sk_offload_rx.o
qca-nss-netfn-sk-offload-objs += netfn_sk_offload_stats.o

ccflags-y += -DNSS_NETFN_BUILD_ID=\"'Build_ID - $(shell date +'%m/%d/%y, %H:%M:%S') SoC=$(SoC)'\"
ccflags-y += -Wall -Werror
