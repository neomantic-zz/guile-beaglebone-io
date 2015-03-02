#ifndef SCM_GPIO_VALUE_TYPE_H
#define SCM_GPIO_VALUE_TYPE_H

#include <libguile.h>

void init_gpio_value_type(void);
void gpio_value_smob_to_bbio_value(SCM *value_smob, int *value);
SCM scm_new_gpio_value_smob(const int value);
void scm_assert_gpio_value_smob(SCM *smob);

#endif
