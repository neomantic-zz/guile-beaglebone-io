(define-module (beagleio)
  #:use-module ((ice-9 match))
  #:export (gpio-setup
	    gpio-cleanup-all
	    gpio-direction-set!
	    gpio-direction
	    gpio-value-set!
	    gpio-value
	    gpio-number-lookup
	    gpio?
	    INPUT
	    OUTPUT
	    HIGH
	    LOW
	    RISING
	    FALLING
	    NO_EDGE
	    BOTH_FALLING_RISING
	    gpio-event-detection-set!))

(dynamic-call "scm_init_beagleio_gpio"
	      (dynamic-link "guile-beagleio/.libs/libguile-beagleio"))
