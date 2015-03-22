#ifndef SCM_GPIO_EDGE_TYPE_H
#define SCM_GPIO_EDGE_TYPE_H

/* matches the indexes of event_gpio.c */
#define NONE 0
#define RISING 1
#define FALLING 2
#define BOTH 3


void scm_assert_gpio_edge_smob(SCM *smob);
SCM scm_new_gpio_edge_smob(const int edge_value);
void init_gpio_edge_type(void);

#endif
