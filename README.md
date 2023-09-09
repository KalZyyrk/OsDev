# Operating system Dev

## Need to run the project

- GNU Make
- NASM -Assembly
- an Virualization software (qemu)

### Installation

For Linux

> `apt install make nasm qemu mtools gcc dosfstools`

For MacOs

> `brew install make nasm qemu mtools gcc dosfstools`

For windows use WSL

## For Running the OS

In the project folder

`make && qemu-system-i386 -fda build/main_floppy.img`


## Tools  

Okteta

`sudo snap install okteta`

bochs

`sudo apt install bochs bochs-sdl bochsbios vgabios`