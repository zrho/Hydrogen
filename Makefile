.PHONY: run clean loader image test

PREFIX    := ./build
EMU       := bochs
EMUFLAGS  := -q

all: loader test image

run: image
	@ $(EMU) $(EMUFLAGS)
	
qemu: image
	@ qemu-system-x86_64 -cdrom $(PREFIX)/boot.iso -smp 4
	
loader:
	@ $(MAKE) -C loader/ install
	
image: loader
	@ $(MAKE) -C image/

test:
	@ $(MAKE) -C test/ install

clean:
	@ rm -R $(PREFIX)/*
