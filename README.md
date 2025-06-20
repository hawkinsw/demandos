## unibpform

### Compiling

#### Requirements

1. opensbi

#### Cmake

Use an architecture file

```console
$ cmake -DCMAKE_TOOLCHAIN_FILE=architecture.cmake -B build -S .
```

### Booting[^boot]

```console
$ qemu-system-riscv64 -M virt -m 256 -nographic -bios /home/hawkinsw/code/riscv/opensbi/build/platform/generic/firmware/fw_jump.bin -kernel ./build/unibpform
```

If you want an [_entropy source_](), add 

```console
-object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0
```

If you want a [_console device_](), add[^1]

```console
-chardev stdio,id=stdio,mux=on,signal=off -device virtio-serial-pci -device virtconsole,chardev=stdio -serial chardev:stdio
```

[^1]: That creates a character device redirected to the hosts' `stdio` and _whose name is stdio_ (not be confused with the _actual_ `stdio`); adds a VirtIO-based serial device to the bus; creates a console that uses the character device (previously created) named stdio for its IO; and redirects the emulated device's (default) serial output to the character device named stdio. 

If you want to redirect the output that would otherwise go to the console to a TCP socket (which can be read using, for example, telnet), add[^2]

```console
-device virtconsole,chardev=vcon, -chardev socket,id=vcon,ipv4=on,host=localhost,port=2222,server=on,telnet=on,wait=on -serial stdio
```

[^2]: See [^1] for a basic description of these options.

**Other Options**
- `-S -gdb tcp::5001`: Listen for remote debugging connection over TCP on port 5001 (localhost only) _and_ wait to start the emulator until there is a connection.
- `-machine dumpdtb=qemu.dtb`: Dump the device tree to the file `qemu.dtb`.

[^boot]: See [https://github.com/riscv-software-src/opensbi/blob/master/docs/platform/qemu_virt.md](https://github.com/riscv-software-src/opensbi/blob/master/docs/platform/qemu_virt.md) for more information.

### Tools

`dtc` for parsing qemu-generated device tree files:

```console
$ dtc qemu.dtb
```

will pretty print the device tree in the file `qemu.dtb`. See above for generating that file.
