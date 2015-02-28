#ifndef SCM_GPIO_TYPE_H
#define SCM_GPIO_TYPE_H

#include <libguile.h>

scm_t_bits gpio_tag;

struct gpio {
  unsigned int pin_number;
  SCM channel;
  SCM update_func;
};

SCM scm_new_gpio_smob(unsigned int *gpio_number, SCM *s_channel);
SCM scm_gpio_type_p(SCM smob);
void init_gpio_type(void);

#endif
