#ifndef GUILE_BBIO_GPIO_LEVEL_H
#define GUILE_BBIO_GPIO_LEVEL_H
#include <libguile.h>

void init_gpio_level_type(void);
int get_level_smob_value(SCM *level_smob, int *level);
SCM high_smob(void);
SCM low_smob(void);

#endif
