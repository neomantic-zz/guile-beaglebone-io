#ifndef GUILE_BEAGLEIO_GPIO_H
#define GUILE_BEAGLEIO_GPIO_H

#include <libguile.h>

SCM scm_gpio_throw(char *message);
void scm_init_beagleio_gpio(void);

#endif
