#include "event_gpio.h"
#include "guile_beagleio_gpio.h"
#include "scm_gpio_type.h"

static scm_t_bits gpio_tag;

int
getDirection(const void* self, unsigned int *direction)
{
  unsigned int current_bbio_direction;
  unsigned int pin_number;
  Gpio *me = (Gpio*)self;
  pin_number = (unsigned int) me->pin_number;

  if (gpio_get_direction(pin_number, &current_bbio_direction) == -1)
    return -1;

  if (current_bbio_direction != me->past_bbio_direction) {
    if (gpio_set_direction(pin_number, me->past_bbio_direction) == -1) {
	return -2;
    }
  }
  *direction = me->past_bbio_direction;
  return 0;
}

int
setDirection(const void *self, int new_direction)
{
  Gpio *me = (Gpio*)self;
  if (new_direction != INPUT && new_direction != OUTPUT)
    return -2;

  if (gpio_set_direction((unsigned int) me->pin_number, new_direction) == -1)
    return -1;

  me->past_bbio_direction = new_direction;

  return 0;
}

static int
scm_gpio_print(SCM gpio_smob, SCM port, scm_print_state *pstate)
{
  Gpio *gpio;
  scm_assert_smob_type(gpio_tag, gpio_smob);
  gpio = (Gpio*) SCM_SMOB_DATA(gpio_smob);
  scm_puts("#<gpio channel: ", port);
  scm_display(gpio->channel, port);
  scm_puts(" pin: ", port);
  scm_display(scm_from_unsigned_integer(gpio->pin_number), port);
  scm_puts(">", port);
  return 1;
}

static size_t
scm_gpio_free(SCM gpio_smob)
{
  scm_assert_smob_type(gpio_tag, gpio_smob);
  Gpio *gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);
  scm_gc_free(gpio, sizeof(Gpio), "gpio");
  return 0;
}

static SCM
scm_gpio_mark(SCM gpio_smob)
{
  Gpio *gpio;
  scm_assert_smob_type(gpio_tag, gpio_smob);
  gpio = (Gpio *) SCM_SMOB_DATA (gpio_smob);
  scm_gc_mark(gpio->channel);
  return (SCM) gpio->update_func;
}

static SCM
scm_gpio_equalp(SCM gpio_smob, SCM other_gpio_smob)
{
  Gpio *gpio, *other;
  scm_assert_smob_type(gpio_tag, gpio_smob);
  scm_assert_smob_type(gpio_tag, other_gpio_smob);
  gpio = (Gpio *) SCM_SMOB_DATA (gpio_smob);
  other = (Gpio *) SCM_SMOB_DATA (other_gpio_smob);
  if (gpio->pin_number == other->pin_number)
      return SCM_BOOL_T;
  return SCM_BOOL_F;
}

SCM
scm_gpio_type_p(SCM smob)
{
  if (!SCM_SMOB_PREDICATE(gpio_tag, smob))
    return SCM_BOOL_F;
  return SCM_BOOL_T;
}

SCM
scm_new_gpio_smob(unsigned int *gpio_number, SCM *s_channel)
{
  SCM smob;
  Gpio *gpio;
  gpio = (Gpio *) scm_gc_malloc(sizeof(Gpio), "gpio");
  gpio->pin_number = *gpio_number;
  gpio->channel = SCM_BOOL_F;
  gpio->update_func = SCM_BOOL_F;
  smob = scm_new_smob(gpio_tag, (scm_t_bits) gpio);
  gpio->channel = *s_channel;
  gpio->getDirection = &getDirection;
  gpio->setDirection = &setDirection;
  return smob;
}

void
scm_assert_gpio_smob_type(SCM *smob)
{
  scm_assert_smob_type(gpio_tag, *smob);
}

void
init_gpio_type(void)
{
  gpio_tag = scm_make_smob_type("gpio", sizeof(Gpio));
  scm_set_smob_print(gpio_tag, scm_gpio_print);
  scm_set_smob_free(gpio_tag, scm_gpio_free);
  scm_set_smob_mark(gpio_tag, scm_gpio_mark);
  scm_set_smob_equalp(gpio_tag, scm_gpio_equalp);
}
