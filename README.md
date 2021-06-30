Source code for Linux Device Drivers development training.

Cross compiling the kernel modules:

```
$ export ARCH=arm
$ export KERNELDIR=/path/to/kernel/source
$ export CROSS_COMPILER=/path/to/cross-compiler-
$ make
```

TODO:

- Add Buildroot config and instructions for testing (some of) the modules
  using QEMU.
