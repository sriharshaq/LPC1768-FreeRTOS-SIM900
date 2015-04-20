
# Toolachain prefix
CC 				= arm-none-eabi-

# CPU
CPU 			= cortex-m3

# C Standard
CSTD			= gnu99

# Linker script
LDSCRIPT 		= scripts/lpc17xx.ld

# Output image name
IMAGE 			= app.elf	

# Include paths
INC = 	-Idevice \
		-Iminilib-c/include \
		-Ikernel/include \
		-Ikernel/portable/GCC/ARM_CM3 \
		-Iconfig \
		-Ilibrary/jsmn \
		-Idrivers \
		-Iapp

# Search paths (For sources and libs)
VPATH = device: \
		minilib-c/ctype: \
		minilib-c/stdio: \
		minilib-c/stdlib: \
		minilib-c/string: \
		minilib-c/syscalls: \
		kernel: \
		kernel/portable/GCC/ARM_CM3: \
		kernel/portable/MemMang: \
		library/jsmn: \
		drivers: \
		app

# Object Files
OBJS	=	main.o \
			uart.o misc.o \
			jsmn.o \
			list.o queue.o tasks.o timers.o port.o heap_2.o \
			lpc17xx.o system_LPC17xx.o \
			_errno.o _exit.o _fclose.o _fopen.o _free.o _kill.o _malloc.o _open.o _write.o \
			ctype_.o isalnum.o isalpha.o isascii.o isblank.o iscntrl.o isdigit.o islower.o \
			isprint.o ispunct.o isspace.o isupper.o isxdigit.o toascii.o  tolower.o toupper.o \
			fclose.o fflush.o fgetc.o fgetlen.o fgets.o fopen.o fputc.o fputs.o getc.o getchar.o \
			gets.o printf.o putc.o putchar.o puts.o rget.o scanf.o setbuffer.o wbuf.o \
			abs.o assert.o atoi.o atol.o calloc.o div.o exit.o ldiv.o malloc.o rand.o strtol.o \
			bcmp.o bcopy.o bzero.o index.o memccpy.o memchr.o memcmp.o memcpy.o memmove.o mempcpy.o \
			memset.o rindex.o strcat.o strchr.o strcmp.o strcoll.o strcpy.o strcspn.o strlcat.o \
			strlcpy.o strlen.o strlwr.o strncat.o strncmp.o strncpy.o strnlen.o strrchr.o strspn.o \
			strstr.o strupr.o
			

# Compiling File Details
# DEVICE SPECIFIC FILES :-
#################### Device Files #############################################################
# lpc17xx.S 		
# system_LPC17xx.c	
###############################################################################################

# REAL TIME KERNEL FILES :-
# For more details: http://www.freertos.org/
#################### Device Files #############################################################
# list.c		
# queue.c
# tasks.c
# timers.c
# port.c
# heap_2.c
###############################################################################################

# DRIVERS :-
#################### Peropheral Driver ########################################################
# uart.c
###############################################################################################

# LIBRARY :-
#################### Peropheral Driver ########################################################
# jsmn.c
###############################################################################################

# SYSCALLS :-
#################### Peropheral Driver ########################################################
# _errno.c
# _exit.c
# _fclose.c
# _fopen.c
# _free.c
# _kill.c
# _malloc.c
# _open.c
# _write.c
###############################################################################################


# C STANDARD LIBRARY :-
# For more details : https://code.google.com/p/minilib-c/
#################### minlinb-c ctype files Files ##############################################
# ctype_.c
# isalnum.c 
# isalpha.c
# isascii.c
# isblank.c
# iscntrl.c
# isdigit.c
# islower.c
# isprint.c
# ispunct.c
# isspace.c
# isupper.c
# isxdigit.c
# toascii.c
# tolower.c
# toupper.c
###############################################################################################

#################### minlinb-c stdio files  ###################################################
# fclose.c
# fflush.c
# fgetc.c
# fgetlen.c
# fgets.c
# fopen.c
# fputc.c
# fputs.c
# getc.c
# getchar.c
# gets.c
# printf.c
# putc.c
# putchar.c
# puts.c
# rget.c
# scanf.c
# setbuffer.c
# wbuf.c
###############################################################################################

#################### minlinb-c stdlib files  ##################################################
# abs.c
# assert.c
# atoi.c
# atol.c
# calloc.c
# div.c
# exit.c
# ldiv.c
# malloc.c
# rand.c
# strtol.c
###############################################################################################

#################### minlinb-c string files  ##################################################
# bcmp.c
# bcopy.c
# bzero.c
# index.c
# memccpy.c
# memchr.c
# memcmp.c
# memcpy.c
# memmove.c
# mempcpy.c
# memset.c
# rindex.c
# strcat.c
# strchr.c 
# strcmp.c
# strcoll.c
# strcpy.c
# strcspn.c
# strlcat.c
# strlcpy.c
# strlen.c
# strlwr.c
# strncat.c
# strncmp.c
# strncpy.c
# strnlen.c
# strrchr.c
# strspn.c
# strstr.c
# strupr.c
###############################################################################################

# Compiler flags
# -c 				: Compile only
# -Wall				: All warnings
# -Os				: Optimize for size
# -mthumb 			: Thumb mode
# -mcpu				: Target CPU
# -std 				: c standard (ANSI-C,c99,gnu99 etc)
CFLAGS 				= -g -c -Wall -Os -mthumb -mcpu=$(CPU) $(INC) -std=$(CSTD)

# Linker flags
# -Wall				: All warnings
# -Os				: Optimize for size
# -mthumb 			: Thumb mode
# -mcpu				: Target CPU
# -nostartfiles 	: No default startfiles (i.e we are giving lpc17xx.S startup file)
# -nodefaultlibs 	: No default libraries (arm gcc will include newlib while linking, but we are using minilib-c so tell linker to exclude newlib)
LFLAGS 				= -g -Wall -Os -mthumb -mcpu=$(CPU) -nostartfiles -nodefaultlibs -T$(LDSCRIPT) 


# Target (make all will invoke)
all: 	$(OBJS)
		$(CC)gcc $(LFLAGS) $(OBJS) -o $(IMAGE) 
		$(CC)size $(IMAGE)

clean: 	$(OBJS)
		-rm $(OBJS)
		-rm $(IMAGE)

# APP FILES:
##################### App Files ########################################
main.o: main.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

# KERNEL FILES:
##################### Kernel Files #####################################
list.o: list.c
	$(CC)gcc $(CFLAGS) $^ -o $@

queue.o: queue.c
	$(CC)gcc $(CFLAGS) $^ -o $@

tasks.o: tasks.c
	$(CC)gcc $(CFLAGS) $^ -o $@

timers.o: timers.c
	$(CC)gcc $(CFLAGS) $^ -o $@

port.o: port.c
	$(CC)gcc $(CFLAGS) $^ -o $@

heap_2.o: heap_2.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

# DRIVER FILES:
##################### Driver Files #####################################
uart.o: uart.c
	$(CC)gcc $(CFLAGS) $^ -o $@

misc.o: misc.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

# LIBRARY FILES:
##################### Driver Files #####################################
jsmn.o: jsmn.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

# SYSCALLS:
##################### syscall Files ####################################
_errno.o: _errno.c
	$(CC)gcc $(CFLAGS) $^ -o $@

_exit.o: _exit.c
	$(CC)gcc $(CFLAGS) $^ -o $@

_fclose.o: _fclose.c
	$(CC)gcc $(CFLAGS) $^ -o $@

_fopen.o: _fopen.c
	$(CC)gcc $(CFLAGS) $^ -o $@

_free.o: _free.c
	$(CC)gcc $(CFLAGS) $^ -o $@

_kill.o: _kill.c
	$(CC)gcc $(CFLAGS) $^ -o $@

_malloc.o: _malloc.c
	$(CC)gcc $(CFLAGS) $^ -o $@

_open.o: _open.c
	$(CC)gcc $(CFLAGS) $^ -o $@

_write.o: _write.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

# DEVICE FILES:
##################### Device Files ####################################
lpc17xx.o: lpc17xx.S
	$(CC)gcc $(CFLAGS) $^ -o $@

system_LPC17xx.o: system_LPC17xx.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

# C STANDARD LIBRARY:
##################### minilib-c ctype ##################################
ctype_.o: ctype_.c
	$(CC)gcc $(CFLAGS) $^ -o $@

isalnum.o: isalnum.c
	$(CC)gcc $(CFLAGS) $^ -o $@

isalpha.o: isalpha.c
	$(CC)gcc $(CFLAGS) $^ -o $@

isascii.o: isascii.c
	$(CC)gcc $(CFLAGS) $^ -o $@

isblank.o: isblank.c
	$(CC)gcc $(CFLAGS) $^ -o $@

iscntrl.o: iscntrl.c
	$(CC)gcc $(CFLAGS) $^ -o $@

isdigit.o: isdigit.c
	$(CC)gcc $(CFLAGS) $^ -o $@

islower.o: islower.c
	$(CC)gcc $(CFLAGS) $^ -o $@

isprint.o: isprint.c
	$(CC)gcc $(CFLAGS) $^ -o $@

ispunct.o: ispunct.c
	$(CC)gcc $(CFLAGS) $^ -o $@

isspace.o: isspace.c
	$(CC)gcc $(CFLAGS) $^ -o $@

isupper.o: isupper.c
	$(CC)gcc $(CFLAGS) $^ -o $@

isxdigit.o: isxdigit.c
	$(CC)gcc $(CFLAGS) $^ -o $@

toascii.o: toascii.c
	$(CC)gcc $(CFLAGS) $^ -o $@

tolower.o: tolower.c
	$(CC)gcc $(CFLAGS) $^ -o $@

toupper.o: toupper.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

##################### minilib-c stdio ##################################
fclose.o: fclose.c
	$(CC)gcc $(CFLAGS) $^ -o $@

fflush.o: fflush.c
	$(CC)gcc $(CFLAGS) $^ -o $@

fgetc.o: fgetc.c
	$(CC)gcc $(CFLAGS) $^ -o $@

fgetlen.o: fgetlen.c
	$(CC)gcc $(CFLAGS) $^ -o $@

fgets.o: fgets.c
	$(CC)gcc $(CFLAGS) $^ -o $@

fopen.o: fopen.c
	$(CC)gcc $(CFLAGS) $^ -o $@

fputc.o: fputc.c
	$(CC)gcc $(CFLAGS) $^ -o $@

fputs.o: fputs.c
	$(CC)gcc $(CFLAGS) $^ -o $@

getc.o: getc.c
	$(CC)gcc $(CFLAGS) $^ -o $@

getchar.o: getchar.c
	$(CC)gcc $(CFLAGS) $^ -o $@

gets.o: gets.c
	$(CC)gcc $(CFLAGS) $^ -o $@

printf.o: printf.c
	$(CC)gcc $(CFLAGS) $^ -o $@

putc.o: putc.c
	$(CC)gcc $(CFLAGS) $^ -o $@

putchar.o: putchar.c
	$(CC)gcc $(CFLAGS) $^ -o $@

puts.o: puts.c
	$(CC)gcc $(CFLAGS) $^ -o $@

rget.o: rget.c
	$(CC)gcc $(CFLAGS) $^ -o $@

scanf.o: scanf.c
	$(CC)gcc $(CFLAGS) $^ -o $@

setbuffer.o: setbuffer.c
	$(CC)gcc $(CFLAGS) $^ -o $@

wbuf.o: wbuf.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

##################### minilib-c stdlib ##################################
abs.o: abs.c
	$(CC)gcc $(CFLAGS) $^ -o $@

assert.o: assert.c
	$(CC)gcc $(CFLAGS) $^ -o $@

atoi.o: atoi.c
	$(CC)gcc $(CFLAGS) $^ -o $@

atol.o: atol.c
	$(CC)gcc $(CFLAGS) $^ -o $@

calloc.o: calloc.c
	$(CC)gcc $(CFLAGS) $^ -o $@

div.o: div.c
	$(CC)gcc $(CFLAGS) $^ -o $@

exit.o: exit.c
	$(CC)gcc $(CFLAGS) $^ -o $@

ldiv.o: ldiv.c
	$(CC)gcc $(CFLAGS) $^ -o $@

malloc.o: malloc.c
	$(CC)gcc $(CFLAGS) $^ -o $@

rand.o: rand.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strtol.o: strtol.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

##################### minilib-c string #################################
bcmp.o: bcmp.c
	$(CC)gcc $(CFLAGS) $^ -o $@

bcopy.o: bcopy.c
	$(CC)gcc $(CFLAGS) $^ -o $@

bzero.o: bzero.c
	$(CC)gcc $(CFLAGS) $^ -o $@

index.o: index.c
	$(CC)gcc $(CFLAGS) $^ -o $@

memccpy.o: memccpy.c
	$(CC)gcc $(CFLAGS) $^ -o $@

memchr.o: memchr.c
	$(CC)gcc $(CFLAGS) $^ -o $@

memcmp.o: memcmp.c
	$(CC)gcc $(CFLAGS) $^ -o $@

memcpy.o: memcpy.c
	$(CC)gcc $(CFLAGS) $^ -o $@

memmove.o: memmove.c
	$(CC)gcc $(CFLAGS) $^ -o $@

mempcpy.o: mempcpy.c
	$(CC)gcc $(CFLAGS) $^ -o $@

memset.o: memset.c
	$(CC)gcc $(CFLAGS) $^ -o $@

rindex.o: rindex.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strcat.o: strcat.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strchr.o: strchr.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strcmp.o: strcmp.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strcoll.o: strcoll.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strcpy.o: strcpy.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strcspn.o: strcspn.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strlcat.o: strlcat.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strlcpy.o: strlcpy.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strlen.o: strlen.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strlwr.o: strlwr.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strncat.o: strncat.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strncmp.o: strncmp.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strncpy.o: strncpy.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strnlen.o: strnlen.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strrchr.o: strrchr.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strspn.o: strspn.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strstr.o: strstr.c
	$(CC)gcc $(CFLAGS) $^ -o $@

strupr.o: strupr.c
	$(CC)gcc $(CFLAGS) $^ -o $@
########################################################################

