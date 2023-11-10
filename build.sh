#!/bin/sh
set -xeo pipefail

! [ -f config.h ] && cp config.def.h config.h;

gcc \
	-Wall -Wextra             \
	-I /usr/include/freetype2 \
	-lX11 -lXrender -lXft -lm \
	-ggdb -o coomer           \
	$CFLAGS                   \
	main.c
