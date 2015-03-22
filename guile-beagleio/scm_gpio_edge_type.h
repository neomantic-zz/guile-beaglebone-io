#ifndef SCM_GPIO_EDGE_TYPE_H
#define SCM_GPIO_EDGE_TYPE_H

void scm_assert_gpio_edge_smob(SCM *smob);
SCM scm_new_gpio_edge_smob(const int edge_value);
void init_gpio_edge_type(void);

#endif
