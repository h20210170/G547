# If called directly from the command line, invoke the kernel build system.
ifeq ($(KERNELRELEASE),)


module:
	make -C/lib/modules/$(shell uname -r)/build M=$(PWD) modules


clean:
	make -C/lib/modules/$(shell uname -r)/build M=$(PWD) modules

# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
else

	obj-m := dor.o
	dor-y := ram_block.o ram_device.o partition.o

endif
