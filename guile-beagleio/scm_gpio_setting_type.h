#ifndef SCM_GPIO_SETTING_TYPE_H
#define SCM_GPIO_SETTING_TYPE_H
#include <libguile.h>

typedef struct gpio_setting {
  int bbio_value;
  SCM update_func;
} GpioValue, GpioDirection;

#endif
