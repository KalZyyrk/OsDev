# Operating system Dev

## Need to run the project

- GNU Make
- NASM -Assembly
- an Virualization software (qemu)

### Installation

For Linux

> `apt install make nasm qemu`

For MacOs

> `brew install make nasm qemu`

For windows use WSL

## For Running the OS

In the project folder

`make && qemu-system-i386 -fda build/main_floppy.img`
