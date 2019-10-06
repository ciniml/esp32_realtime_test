.PHONY: all clean

CSV_UDP    := ./log_17545106.csv
CSV_NOUDP  := ./log_51566126.csv
CSV_APSCAN := ./log_93834675.csv
CSV_NOWIFI := ./log_127859786.csv

CSV_ALL := $(CSV_UDP) $(CSV_NOUDP) $(CSV_APSCAN) $(CSV_NOWIFI)
FORMAT := pngcairo
EXT := png
TARGETS := hrtimer.$(EXT) hrtimer_range2000.$(EXT)

all: $(TARGETS)

clean:
	-@$(RM) $(TARGETS)

hrtimer.$(EXT): $(CSV_ALL)
	gnuplot -e "set terminal $(FORMAT) mono size 800,600;set output \"$@\"" -c ../../tool/plot_states.plt \
		$(CSV_UDP)    "hr (udp)" \
		$(CSV_NOUDP)  "hr (noudp)" \
		$(CSV_APSCAN) "hr (apscan)" \
		$(CSV_NOWIFI) "hr (nowifi)"
		

hrtimer_range2000.$(EXT): $(CSV_ALL)
	gnuplot -e "set terminal $(FORMAT) mono size 800,600;set output \"$@\";set yrange [0:2000]" -c ../../tool/plot_states.plt \
		$(CSV_UDP)    "hr (udp)" \
		$(CSV_NOUDP)  "hr (noudp)" \
		$(CSV_APSCAN) "hr (apscan)" \
		$(CSV_NOWIFI) "hr (nowifi)"