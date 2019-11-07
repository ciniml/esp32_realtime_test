set datafile separator ","

set xrange [0.5:4.4]
set xtics (ARG2 1, ARG4 2, ARG6 3, ARG8 4)
set xtics nomirror scale 0
set ytics mirror
set parametric

plot   ARG1 using (1):3 pt 1 title ARG2, \
       ARG3 using (2):3 pt 2 title ARG4, \
       ARG5 using (3):3 pt 3 title ARG6, \
       ARG7 using (4):3 pt 4 title ARG8, \
       [t=-0:5] t,500 notitle