CSV_UDP    ?= udp.csv
CSV_NOUDP  ?= noudp.csv
CSV_APSCAN ?= apscan.csv
CSV_NOWIFI ?= nowifi.csv

CSV_ALL := $(CSV_UDP) $(CSV_NOUDP) $(CSV_APSCAN) $(CSV_NOWIFI)
FORMAT := pngcairo
EXT := png
TARGETS := hwtimer_interval.$(EXT) hwtimer_delay.$(EXT)

all: $(TARGETS) intervals.txt delays.txt

clean:
	-@$(RM) $(TARGETS) intervals.txt delays.txt

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

intervals.txt: $(CSV_ALL)
	-$(RM) $@
	python3 ../../tool/loganalyze.py ${CSV_UDP}    2 udp    >> $(abspath $@)
	python3 ../../tool/loganalyze.py ${CSV_NOUDP}  2 noudp  >> $(abspath $@)
	python3 ../../tool/loganalyze.py ${CSV_APSCAN} 2 apscan >> $(abspath $@)
	python3 ../../tool/loganalyze.py ${CSV_NOWIFI} 2 nowifi >> $(abspath $@)

delays.txt: $(CSV_ALL)
	-$(RM) $@
	python3 ../../tool/loganalyze.py ${CSV_UDP}    1 udp    >> $(abspath $@)
	python3 ../../tool/loganalyze.py ${CSV_NOUDP}  1 noudp  >> $(abspath $@)
	python3 ../../tool/loganalyze.py ${CSV_APSCAN} 1 apscan >> $(abspath $@)
	python3 ../../tool/loganalyze.py ${CSV_NOWIFI} 1 nowifi >> $(abspath $@)