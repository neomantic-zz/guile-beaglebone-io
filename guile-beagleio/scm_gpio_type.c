#include "event_gpio.h"
#include "guile_beagleio_gpio.h"
#include "scm_gpio_type.h"
#include "scm_gpio_value_type.h"

static scm_t_bits gpio_tag;

struct scm_callback
{
  SCM procedure;
  unsigned long long lastcall;
  unsigned int bouncetime;
  struct scm_callback *next;
  struct Gpio *gpio;
};
static struct scm_callback *scm_gpio_callbacks = NULL;

int
setEdge(const void* self, int new_edge)
{
  Gpio *me;
  unsigned int current_direction;

  me = (Gpio*)self;
  if (me->getDirection(me, &current_direction) == 0 &&
      current_direction != INPUT)
    return -2;

  return gpio_set_edge((unsigned int) me->pin_number, new_edge);
}

static SCM
run_scm_callbacks(unsigned int callback_pin_number)
{

  Gpio *gpio;
  unsigned int pin_number;
  struct scm_callback *scm_callback = scm_gpio_callbacks;
  struct timeval tv_timenow;
  unsigned long long timenow;
  unsigned int gpio_bbio_value;

  while (scm_callback != NULL) {
    gpio = (Gpio *) scm_callback->gpio;
    pin_number = (unsigned int) gpio->pin_number;
    if (scm_callback->pin_number == callback_pin_number) {
      gettimeofday(&tv_timenow, NULL);
      timenow = tv_timenow.tv_sec*1E6 + tv_timenow.tv_usec;
      if (scm_callback->bouncetime == 0 ||
	  timenow - scm_callback->lastcall > scm_callback->bouncetime*1000 ||
	  scm_callback->lastcall == 0 ||
	  scm_callback->lastcall > timenow) {

	// save lastcall before calling func to prevent reentrant bounce
	scm_callback->lastcall = timenow;

	if (gpio->getValue(gpio, gpio_bbio_value) == -1)
	  return scm_gpio_throw("unable to read /sys/class/gpio/*/value");
	return scm_call_1(cp->procedure, scm_new_gpio_value_smob(gpio_bbio_value));
      }
      scm_callback->lastcall = timenow;
    }
    scm_callback = scm_callback->next;
  }
}

int
appendEventCallback(const void* self, SCM procedure, unsigned int bouncetime)
{
  Gpio *me;
  struct scm_callback *new_scm_callback;
  struct scm_callback *current_callback = scm_gpio_callbacks;
  unsigned int current_direction;

  new_scm_callback = malloc(sizeof(struct scm_callback));

  if (new_scm_callback == 0){
    /* TODO:  handle */
    return -1;
  }

  me = (Gpio *) self;

  if (!gpio_is_evented((unsigned int) gpio->pin_number))
    return -3;

  if (gpio->getDirection(gpio, &current_direction) != 0)
    return -1;

  if (current_direction != INPUT)
    return -2;

  new_scm_callback->procedure = procedure;
  new_scm_callback->gpio = me;
  new_scm_callback->lastcall = 0;
  new_scm_callback->bouncetime = bouncetime;
  new_scm_callback->next = NULL;

  if (scm_gpio_callbacks == NULL) {
    scm_gpio_callbacks = new_scm_callback;
  } else {
    while (current_callback->next != NULL)
      current_callback = current_callback->next;
    current_callback->next = new_scm_callback;
  }

  add_edge_callback((unsigned int) me->pin_number, run_scm_callbacks);
  return 0;
}

int
setValue(const void* self, int new_value)
{
  unsigned int current_direction;
  Gpio *me;

  me = (Gpio*)self;
  if (me->getDirection(me, &current_direction) == 0 &&
      current_direction != OUTPUT)
    return -2;

  return gpio_set_value((unsigned int) me->pin_number, new_value);
}

int
getValue(const void* self, unsigned int *current_value)
{
  unsigned int pin_number;
  Gpio *me;

  me = (Gpio*)self;
  pin_number = (unsigned int) me->pin_number;

  return gpio_get_value(pin_number, current_value);
}

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

static void
free_scm_callbacks_by_gpio(Gpio *gpio)
{
  Gpio *current_gpio;
  struct scm_callback *prev_scm_callback;
  unsigned int pin_number = (unsigned int) gpio->pin_number;
  struct scm_callback *current_scm_callback = scm_gpio_callbacks;
  prev_scm_callback = current_scm_callback;

  /* This terrible loop deals with the fact that to get the py c library
     to work, all callback are storted in a global (ugh) linked list of structs. When
     the gc collects the scm gpio object, it should also clean up the callback structs,
     The loop finds them, frees them, and removes the from the global.

     The py c library does not do this, possibily leaving the global struct referencing
     addresses (in ->next) that have been freed.
  */

  while (current_scm_callback != NULL) {
    current_gpio = (Gpio *) current_scm_callback->gpio;
    if ((unsigned int) gpio->pin_number == pin_number) {
      current_scm_callback = current_scm_callback->next;
      prev_scm_callback->next = current_scm_callback;
      free(current_scm_callback);
    }
  }
}

static size_t
scm_gpio_free(SCM gpio_smob)
{
  scm_assert_smob_type(gpio_tag, gpio_smob);
  Gpio *gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);
  free_scm_callbacks_by_gpio(gpio);
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
  gpio->setValue = &setValue;
  gpio->getValue = &getValue;
  gpio->setEdge = &setEdge;
  gpio->appendEventCallback = &appendEventCallback;
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
