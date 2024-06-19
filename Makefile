obj-m := dev_1.o

comp:
	make -C /lib/modules/$(shell uname -r)/build M=${PWD} modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=${PWD} clean



