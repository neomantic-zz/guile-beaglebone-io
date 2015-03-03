#include "event_gpio.h"
#include "scm_gpio_setting_type.h"
#include "scm_gpio_direction_type.h"

static scm_t_bits gpio_direction_tag;

static int
scm_gpio_direction_print(SCM gpio_direction_smob, SCM port, scm_print_state *pstate)
{
  GpioDirection *gpio_direction;
  scm_assert_gpio_direction_smob(&gpio_direction_smob);
  gpio_direction = (GpioDirection*)SCM_SMOB_DATA(gpio_direction_smob);
  scm_puts("#<gpio-direction direction: ", port);
  if (gpio_direction->bbio_value == INPUT) {
    scm_puts("input", port);
  } else {
    scm_puts("output", port);
  }
  scm_puts(" >", port);
  return 1;
}

static size_t
scm_gpio_direction_free(SCM gpio_direction_smob)
{
  GpioDirection *gpio_direction;
  scm_assert_gpio_direction_smob(&gpio_direction_smob);
  gpio_direction = (GpioDirection*) SCM_SMOB_DATA(gpio_direction_smob);
  scm_gc_free(gpio_direction, sizeof(GpioDirection), "gpio-direction");
  return 0;
}

static SCM
scm_gpio_direction_equalp(SCM gpio_direction_smob, SCM other_gpio_direction_smob)
{
  GpioDirection *gpio_direction, *other_direction;
  scm_assert_gpio_direction_smob(&gpio_direction_smob);
  scm_assert_gpio_direction_smob(&other_gpio_direction_smob);
  gpio_direction = (GpioDirection*) SCM_SMOB_DATA(gpio_direction_smob);
  other_direction = (GpioDirection*) SCM_SMOB_DATA(other_gpio_direction_smob);
  if (gpio_direction->bbio_value == other_direction->bbio_value)
    return SCM_BOOL_T;
  return SCM_BOOL_F;
}


SCM
scm_new_gpio_direction_smob(const int value)
{
  GpioDirection *gpio_direction;
  gpio_direction = (GpioDirection*) scm_gc_malloc(sizeof(GpioDirection), "gpio-direction");
  gpio_direction->update_func = SCM_BOOL_F;
  gpio_direction->bbio_value = value;
  return scm_new_smob(gpio_direction_tag, (scm_t_bits) gpio_direction);
}

void scm_assert_gpio_direction_smob(SCM *smob)
{
  scm_assert_smob_type(gpio_direction_tag, *smob);
}

void
init_gpio_direction_type(void)
{
  gpio_direction_tag = scm_make_smob_type("gpio-direction", sizeof(GpioDirection));
  scm_set_smob_print(gpio_direction_tag, scm_gpio_direction_print);
  scm_set_smob_free(gpio_direction_tag, scm_gpio_direction_free);
  scm_set_smob_equalp(gpio_direction_tag, scm_gpio_direction_equalp);
}
