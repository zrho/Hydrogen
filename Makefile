.PHONY: run clean loader image test

PREFIX		:= ./build
EMU			:= bochs
EMUFLAGS	:= -q

all: loader test image

run: image
	@ $(EMU) $(EMUFLAGS)
	
qemu: image
	@ qemu-system-x86_64 -cdrom build/boot.iso -smp 4
	
loader:
	@ $(MAKE) -C loader/
	
image: loader
	@ $(MAKE) -C image/

test:
	@ $(MAKE) -C test/

clean:
	@ rm -R $(PREFIX)/*