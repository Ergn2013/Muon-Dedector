* MUON ASIMETRIK: calibrated source functions. Time is in seconds; PW_NS is FWHM in ns.
.param TR_ASYM=3e-10
.func TAUF(w) {table(w, 10, 1.25985971511948e-08, 15, 1.96420551235112e-08, 20, 2.67339205506113e-08, 25, 3.38526402000034e-08, 30, 4.09884666662937e-08)}
.func TPEAK(w) {TR_ASYM*TAUF(w)/(TAUF(w)-TR_ASYM)*ln(TAUF(w)/TR_ASYM)}
.func PNORM(w) {exp(-TPEAK(w)/TAUF(w))-exp(-TPEAK(w)/TR_ASYM)}
.func ASYM(t,t0,w) {(exp(-max(t-t0,0)/TAUF(w))-exp(-max(t-t0,0)/TR_ASYM))/PNORM(w)}
.func P1(t) {ON1*AMP*(ASYM(t,T0,PW_NS)+SECOND*ASYM(t,T0+PAIR_NS*1n,PW_NS))}
.func P2(t) {ON2*AMP*RATIO2*(ASYM(t,T0+DELAY_NS*1n,PW_NS)+SECOND*ASYM(t,T0+PAIR_NS*1n+DELAY_NS*1n,PW_NS))}
.func NI1(t) {-N_SIG*V(NS1)/RNPORT}
.func NI2(t) {-N_SIG*V(NS2)/RNPORT}
.func NR2SELECT(t) {(1-REF_COMMON)*V(NR2)+REF_COMMON*V(NR1)}
.model BAS70_REVU D(IS=2e-8 N=1.07 RS=5 CJO=1.5p M=0.33 TT=0.1n BV=70 IBV=1u)
.model BAT54_APPROX D(IS=2e-7 N=1.08 RS=4 CJO=8p M=0.35 TT=0.1n BV=30 IBV=5u)
.func SELS1(t) {table(NSEED,1,V(NS1_1),2,V(NS1_2),3,V(NS1_3),4,V(NS1_4),5,V(NS1_5))}
.func SELS2(t) {table(NSEED,1,V(NS2_1),2,V(NS2_2),3,V(NS2_3),4,V(NS2_4),5,V(NS2_5))}
.func SELR1(t) {table(NSEED,1,V(NR1_1),2,V(NR1_2),3,V(NR1_3),4,V(NR1_4),5,V(NR1_5))}
.func SELR2(t) {table(NSEED,1,V(NR2_1),2,V(NR2_2),3,V(NR2_3),4,V(NR2_4),5,V(NR2_5))}
