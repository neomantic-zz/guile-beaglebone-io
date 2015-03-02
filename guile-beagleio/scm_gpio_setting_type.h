#ifndef SCM_GPIO_SETTING_TYPE_H
#define SCM_GPIO_SETTING_TYPE_H
#include <libguile.h>

typedef struct gpio_setting {
  int sysfs_value;
  SCM update_func;
} GpioValue;

#endif
