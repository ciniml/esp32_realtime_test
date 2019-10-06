CSV_ALL := $(CSV_UDP) $(CSV_NOUDP) $(CSV_APSCAN) $(CSV_NOWIFI)
FORMAT := pngcairo
EXT := png
TARGETS := hwtimer.$(EXT)

all: $(TARGETS)

clean:
	-@$(RM) $(TARGETS)

hwtimer.$(EXT): $(CSV_ALL)
	gnuplot -e "set terminal $(FORMAT) mono size 800,600;set output \"$@\"" -c ../../tool/plot_states.plt \
		$(CSV_UDP)    "hwtimer (udp)" \
		$(CSV_NOUDP)  "hwtimer (noudp)" \
		$(CSV_APSCAN) "hwtimer (apscan)" \
		$(CSV_NOWIFI) "hwtimer (nowifi)"
