.PHONY: all clean

TARGETS := hrtimer hwtimer_app_22 hwtimer_app_24 hwtimer_pro_22 hwtimer_pro_24

all: $(TARGETS)
	for t in $^; do cd $$t; make; cd ..; done

clean: $(TARGETS)
	for t in $^; do cd $$t; make clean; cd ..; done