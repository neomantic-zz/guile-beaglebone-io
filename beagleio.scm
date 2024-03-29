(define-module (beagleio)
  #:use-module ((ice-9 match))
  #:export (gpio-setup
            gpio-close
	    gpio-direction-set!
	    gpio-direction
	    gpio-value-set!
	    gpio-value
	    gpio-number-lookup
	    gpio?
	    INPUT
	    OUTPUT
	    HIGH
	    LOW))

(dynamic-call "scm_init_beagleio_gpio"
	      (dynamic-link "guile-beagleio/.libs/libguile-beagleio"))
