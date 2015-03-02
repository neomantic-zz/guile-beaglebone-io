#ifndef SCM_GPIO_VALUE_TYPE_H
#define SCM_GPIO_VALUE_TYPE_H

#include <libguile.h>

void init_gpio_value_type(void);
SCM scm_new_gpio_value_smob(const int value);
void scm_assert_gpio_value_smob(SCM *smob);

#endif
