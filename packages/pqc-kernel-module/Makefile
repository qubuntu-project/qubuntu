# SPDX-License-Identifier: GPL-2.0
obj-m += liboqs_wrapper.o pqc_kem.o pqc_kem_test.o

ccflags-y += -I$(src)/include

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a
