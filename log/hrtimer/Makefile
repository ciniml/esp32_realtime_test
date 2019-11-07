.PHONY: all clean

CSV_UDP    := udp.csv
CSV_NOUDP  := noudp.csv
CSV_APSCAN := apscan.csv
CSV_NOWIFI := nowifi.csv

CSV_ALL := $(CSV_UDP) $(CSV_NOUDP) $(CSV_APSCAN) $(CSV_NOWIFI)
FORMAT := pngcairo
EXT := png
TARGETS := hrtimer.$(EXT) hrtimer_range2000.$(EXT)

all: $(TARGETS) intervals.txt

clean:
	-@$(RM) $(TARGETS) intervals.txt

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


intervals.txt: $(CSV_ALL)
	-$(RM) $@
	python3 ../../tool/loganalyze.py ${CSV_UDP}    2 udp    >> $(abspath $@)
	python3 ../../tool/loganalyze.py ${CSV_NOUDP}  2 noudp  >> $(abspath $@)
	python3 ../../tool/loganalyze.py ${CSV_APSCAN} 2 apscan >> $(abspath $@)
	python3 ../../tool/loganalyze.py ${CSV_NOWIFI} 2 nowifi >> $(abspath $@)
