CSV_ALL := $(CSV_UDP) $(CSV_NOUDP) $(CSV_APSCAN) $(CSV_NOWIFI)
FORMAT := pngcairo
EXT := png
TARGETS := hwtimer_interval.$(EXT) hwtimer_delay.$(EXT)

all: $(TARGETS)

clean:
	-@$(RM) $(TARGETS)

hwtimer_interval.$(EXT): $(CSV_ALL)
	gnuplot -e "set terminal $(FORMAT) mono size 800,600;set output \"$@\";set yrange [0:1000]" -c ../../tool/plot_states.plt \
		$(CSV_UDP)    "hwtimer (udp)" \
		$(CSV_NOUDP)  "hwtimer (noudp)" \
		$(CSV_APSCAN) "hwtimer (apscan)" \
		$(CSV_NOWIFI) "hwtimer (nowifi)"

hwtimer_delay.$(EXT): $(CSV_ALL)
	gnuplot -e "set terminal $(FORMAT) mono size 800,600;set output \"$@\"" -c ../../tool/plot_delays.plt \
		$(CSV_UDP)    "hwtimer (udp)" \
		$(CSV_NOUDP)  "hwtimer (noudp)" \
		$(CSV_APSCAN) "hwtimer (apscan)" \
		$(CSV_NOWIFI) "hwtimer (nowifi)"
