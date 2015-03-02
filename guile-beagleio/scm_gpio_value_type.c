#include "event_gpio.h"
#include "guile_beagleio_gpio.h"
#include "scm_gpio_setting_type.h"
#include "scm_gpio_value_type.h"

static scm_t_bits gpio_value_tag;

void
scm_assert_gpio_value_smob(SCM *smob)
{
  scm_assert_smob_type(gpio_value_tag, *smob);
}

static int
scm_gpio_value_print(SCM gpio_value_smob, SCM port, scm_print_state *pstate)
{
  GpioValue *gpio_value;
  scm_assert_gpio_value_smob(&gpio_value_smob);
  gpio_value = (GpioValue*)SCM_SMOB_DATA(gpio_value_smob);
  scm_puts("#<gpio-value level: ", port);
  if (gpio_value->sysfs_value == HIGH) {
    scm_puts("high", port);
  } else {
    scm_puts("low", port);
  }
  scm_puts(" >", port);
  return 1;
}

static size_t
scm_gpio_value_free(SCM gpio_value_smob)
{
  GpioValue *gpio_value;
  scm_assert_gpio_value_smob(&gpio_value_smob);
  gpio_value = (GpioValue*) SCM_SMOB_DATA(gpio_value_smob);
  scm_gc_free(gpio_value, sizeof(GpioValue), "gpio-value");
  return 0;
}

static SCM
scm_gpio_value_equalp(SCM gpio_value_smob, SCM other_gpio_value_smob)
{
  GpioValue *gpio_value, *other_gpio_value;
  scm_assert_gpio_value_smob(&gpio_value_smob);
  scm_assert_gpio_value_smob(&other_gpio_value_smob);
  gpio_value = (GpioValue*) SCM_SMOB_DATA(gpio_value_smob);
  other_gpio_value = (GpioValue*) SCM_SMOB_DATA(other_gpio_value_smob);
  if (gpio_value->sysfs_value == other_gpio_value->sysfs_value)
    return SCM_BOOL_T;
  return SCM_BOOL_F;
}

SCM
scm_new_gpio_value_smob(int value)
{
  GpioValue *gpio_value;
  gpio_value = (GpioValue*) scm_gc_malloc(sizeof(GpioValue), "gpio-value");
  gpio_value->update_func = SCM_BOOL_F;
  gpio_value->sysfs_value = value;
  return scm_new_smob(gpio_value_tag, (scm_t_bits) gpio_value);
}
SCM
scm_gpio_value_high_smob(void)
{
  return scm_new_gpio_value_smob(HIGH);
}

SCM
scm_gpio_value_low_smob(void)
{
  return scm_new_gpio_value_smob(LOW);
}

void
init_gpio_value_type(void)
{
  gpio_value_tag = scm_make_smob_type("gpio-value", sizeof(GpioValue));
  scm_set_smob_print(gpio_value_tag, scm_gpio_value_print);
  scm_set_smob_free(gpio_value_tag, scm_gpio_value_free);
  scm_set_smob_equalp(gpio_value_tag, scm_gpio_value_equalp);
}
