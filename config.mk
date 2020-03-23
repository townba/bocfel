# Select the compiler to be used.  See compiler.mk for supported compilers.
# Even if your compiler is not officially supported, it might still work; C99
# support is required, however.
CC=		gcc

# Select the optimization level.  Because of how the program is structured, it
# benefits very much from automatically inlined functions, and even more so if
# the compiler is able to do link-time optimization.  On the other hand, most
# interactive fiction does not need a fast interpreter.  For good results from
# gcc, the following is recommended:
# -O3 -flto -fomit-frame-pointer
# -flto works only on gcc 4.5 and newer, and selects link-time optimization.
#
OPT=		-O

# Select the target platform.  Valid values are:
# • unix (for POSIX systems)
# • win32 (for Win32 systems)
#
# Any other value (or none at all) will result in the use of standard C99
# functions only.
#
PLATFORM=	unix

# Set the target directories for the install of the binary and the man page.
PREFIX=		/usr/local
BINDIR=		$(PREFIX)/bin
MANDIR=		$(PREFIX)/man/man6

# If defined, this variable controls which GLK implementation will be used.  The
# GLK library must exist in a directory of the same name (so if GLK is set to
# glktermw, the GLK implementation will be found in glktermw/).
# If this value is empty or not defined, I/O will be done solely in terms of C’s
# standard library.
#
GLK=		gargoyle

# The GLK specification recommends calling glk_tick() every instruction, because
# there may be some platforms that need to do some processing every now and
# again.  However, glk_tick() in the following implementations does not actually
# do anything:
# cheapglk 1.0.0, glkterm(w) 1.0.0, GlkDOS 0.19.1, XGlk 0.4.11, Gargoyle 2010.1
# On the other hand, the following does do something:
# WindowsGlk 1.2.1
# By default, for performance reasons, glk_tick() is not called.  If the
# following variable is defined (with any value), glk_tick() will be called.
#
# GLK_TICK=		1

# By default, many safety checks are performed, such as verifying that a story
# is not overflowing its stack, performing invalid memory accesses, and so on.
# If this variable is defined (with any value), these checks are not performed.
# This speeds up the interpreter, but misbehaving story files could cause
# unpredictable results, including crashing.
#
# NO_SAFETY_CHECKS=	1

# By default, Z-machine versions 1 and 2 are supported.  There are, to my
# knowledge, only four version 1 and 2 story files (early releases of Zork I and
# II).  Because support for versions 1 and 2 requires extra processing for each
# and every character printed to the screen, support for these story files can
# be disabled if the following variable is defined (with any value).
#
# NO_V2=		1

# Rudimentary cheating support is available.  This allows certain parts of
# memory to be “frozen”, meaning no matter what the game does, they will always
# contain specific values.  The intended use for this is to freeze hunger and
# thirst daemons: assuming that the daemons work by using a increasing or
# decreasing value to indicate hunger or lack thereof, freezing this counter can
# forever report that the user is sated.  It is possible to build without
# support for cheating, which might be useful because cheating slows down memory
# access.  If the following variable is defined (with any value), cheating will
# be disabled.
# NO_CHEAT=		1

# Tandy censored Infocom games.  This was accomplished by Tandy interpreters
# setting a special flag to indicate to the game that it should cow to pressure
# from groups who were, apparently, unable to simply not look at things they
# found offensive.  While Appendix B of the Z-machine standard indicates that
# games are not allowed to change this flag themselves, at least The Witness has
# code to do this.  Normally, this causes the interpreter to halt because
# read-only memory is being written.  In the interest of authenticity, I would
# like Infocom games to be able to run properly under any conditions.  However,
# allowing this slows down each memory write.  Thus by default games are not
# allowed to set the “Tandy bit”, but if the following variable is defined (with
# any value), the setting of this bit will be allowed.
#
# TANDY=		1
