SUBDIRS = bbio
BBIO_LIB = beagebone-io
lib_LTLIBRARIES = libguile-beagleio.la
AM_CFLAGS = -Wall -O2 -std=c99 -D_POSIX_C_SOURCE=199309L @CFLAGS@  -I$(srcdir)
libguile_beagleio_la_SOURCES = 	guile_beagleio_gpio.h guile_beagleio_gpio.c \
			scm_gpio_setting_type.h \
			scm_gpio_value_type.h scm_gpio_value_type.c \
			scm_gpio_direction_type.h scm_gpio_direction_type.c \
			scm_gpio_type.h scm_gpio_type.c
libguile_beagleio_la_CFLAGS = $(AM_CFLAGS) @guile_CFLAGS@ -I$(SUBDIRS)
libguile_beagleio_la_LIBADD = -L$(SUBDIRS) @guile_LIBS@ -l$(BBIO_LIB)
libguile_beagleio_la_LDFLAGS = -export-dynamic -module -avoid-version
noinst_HEADERS = guile_beagleio_gpio.h scm_gpio_value_type.h scm_gpio_type.h scm_gpio_direction_type.h
