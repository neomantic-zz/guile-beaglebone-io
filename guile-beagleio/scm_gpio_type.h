#ifndef SCM_GPIO_TYPE_H
#define SCM_GPIO_TYPE_H

#include <libguile.h>

struct scm_callback
{
  SCM procedure;
  unsigned long long lastcall;
  unsigned int bouncetime;
  struct scm_callback *next;
};

typedef struct gpio {
  unsigned int pin_number;
  unsigned int past_bbio_direction;
  int (*getDirection)(const void* self, unsigned int *direction);
  int (*setDirection)(const void* self, int new_direction);
  int (*setValue)(const void* self, int new_value);
  int (*getValue)(const void* self, unsigned int *current_value);
  int (*setEdge)(const void* self, unsigned int new_edge);
  int (*getEdge)(const void* self, unsigned int *edge);
  int (*appendEventCallback)(const void* self, SCM procedure);
  void (*clearEventCallbacks)(const void* self);
  int (*close)(const void* self);
  SCM channel;
  SCM update_func;
  scm_i_pthread_mutex_t callbacks_mutex;
  struct scm_callback *scm_gpio_callbacks;
} Gpio;

void scm_assert_gpio_smob_type(SCM *smob);
SCM scm_new_gpio_smob(unsigned int *gpio_number, SCM *s_channel);
SCM scm_gpio_type_p(SCM smob);
void init_gpio_type(void);

#endif
