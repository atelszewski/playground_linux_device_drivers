obj-m += exercise.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.RECIPEPREFIX = >

all:
>   $(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
>   $(MAKE) -C $(KERNELDIR) M=$(PWD) clean

.RECIPEPREFIX =
