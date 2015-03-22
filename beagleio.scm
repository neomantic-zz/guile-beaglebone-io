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
            gpio-callback-append
            gpio-edge-set!
            NONE
            RISING
            FALLING
            BOTH))

(dynamic-call "scm_init_beagleio_gpio"
	      (dynamic-link "guile-beagleio/.libs/libguile-beagleio"))
