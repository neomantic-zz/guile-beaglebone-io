#ifndef SCM_GPIO_DIRECTION_TYPE_H
#define SCM_GPIO_DIRECTION_TYPE_H

#include <libguile.h>

void init_gpio_direction_type(void);
SCM scm_new_gpio_direction_smob(const int value);
void scm_assert_gpio_direction_smob(SCM *smob);

#endif
