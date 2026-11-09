* Opsiyonel DC acilis testi; ns darbe testi degil
* Sadece yavas bias ve besleme yerlesmesi. Hizli testlerin 10ps adimi degistirilmedi.
.param AMP=30m
.param PW_NS=20
.param RATIO2=1
.param DELAY_NS=0
.param PAIR_NS=500
.param SECOND=0
.param ON1=0
.param ON2=0
.param N_SIG=0
.param N_REF=0
.param N_UP=0
.param REF_COMMON=0
.param NSEED=1
.param RTH1=50
.param RTH2=50
.param VOS1=0
.param VOS2=0
.param RATT_VAL=100
.param RCL_BOT=1k
.param C_AND_LOAD=10p
.param C_HOLD_PAR=4p
.param VIN_NOM=5.6
.param T0=1u
.param TSTOP=80m
.param TMAX=2u
.param K50={(50+49.9)/49.9}
.param RNPORT={50*49.9/(50+49.9)}
.options plotwinsize=0 reltol=0.0002 abstol=1e-12 solver=alt
.temp 25
.include SAVE_NODES.inc
.tran 0 {TSTOP} 0 {TMAX} startup
.meas tran V3A_FINAL FIND V(V3A) AT=80m
.meas tran BIAS1_FINAL FIND V(VBIAS1) AT=80m
.meas tran TH1_FINAL FIND V(TH1_OFF) AT=80m
.meas tran BIAS2_FINAL FIND V(VBIAS2) AT=80m
.meas tran TH2_FINAL FIND V(TH2_OFF) AT=80m
