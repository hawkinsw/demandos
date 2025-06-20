#!/usr/bin/bash

kill -9 `ps -ef | grep qemu-system-riscv64 | awk '{print $2;}' | head -n1`
