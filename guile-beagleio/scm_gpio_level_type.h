#ifndef SCM_GPIO_LEVEL_TYPE_H
#define SCM_GPIO_LEVEL_TYPE_H
#include <libguile.h>

void init_gpio_level_type(void);
void level_smob_to_bbio_value(SCM *level_smob, int *level);
SCM scm_gpio_level_high_smob(void);
SCM scm_gpio_level_low_smob(void);
SCM scm_new_gpio_level_smob(unsigned int *pin_number);

#endif
