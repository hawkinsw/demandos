## FuzzOff

An on-demand unikernel.[^riscv]

[^riscv]: RISC-V-based programs, at this point!

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
$ qemu-system-riscv64 -M virt -m 256 -nographic -bios <PATH TO OPENSBI BUILD>/fw_jump.bin -kernel ./build/fuzzoff
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

If you want to add a block device, add:

```console
-device virtio-blk-pci,drive=drive0,id=virtblk0,num-queues=2 -drive file=<DISK IMAGE FILEPATH>,if=none,id=drive0,cache=writethrough
```

where `<DISK IMAGE FILEPATH>` is an appropriate path. See [below](#block-devices) for information on creating and inspecting block devices.

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

### Block Devices

#### To create a qcow2 image:

```console
$ qemu-img create -f qcow2 <NAME> <SIZE>
```
where `<NAME>` is the name of the disk image that is created and `<SIZE>` is the size. You can specify size with `B`, `K`, `M`, etc. See `qemu-img`'s `man` page for more information.

To create a good image for testing, use:
```console
$ qemu-img create -f qcow2 simple.qcow2 512B
```

(or use the make-simple-qcow2 CMake target).

#### Inspecting the contents of a qcow2 image:

To read the raw contents of a qcow2 disk image, use `qemu-img`'s `dd` subcommand (which works much like `dd`). For example,

```console
$ qemu-img dd if=simple.qcow2 of=simple.qcow2.dd
```

will read the entirety of `simple.qcow2` and write it to `simple.qcow2.dd`. _That_ file can be displayed nicely with `hexdump`:

```console
$ hexdump -C simple.qcow2.dd
```

#### Making an image that contains a filesystem for testing:

Using ext2 as an example (on a device with 1MB of raw capacity).

1. Create a raw file:
```console
$ dd if=/dev/zero of=formatted.raw bs=1K count=1024
```
2. Create a loopback device with that raw file as the backing:
```console
$ losetup -f formatted.raw
```
3. Make the filesystem:
```console
$ mkfs.ext2 /dev/loop0
```
4. Detach the loopback device:
```console
$ losetup -d loop0
```
5. Convert it to a qcow2:
```console
$ qemu-img convert -f raw -O qcow2 formatted.raw formatted.qcow2
```