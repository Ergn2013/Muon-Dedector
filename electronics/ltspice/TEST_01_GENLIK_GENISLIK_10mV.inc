* Ana tarama: 30mV...1V / 10...30ns / iki gurultu yolu
* 50 kosul; iki kanal ayni genlik/genislik, ilk olay ayni anda.
.param AMP=30m
.param PW_NS=20
.param RATIO2=1
.param DELAY_NS=0
.param PAIR_NS=500
.param SECOND=0
.param ON1=1
.param ON2=1
.param N_SIG=10m
.param N_REF=10m
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
.param TSTOP=3u
.param TMAX=10p
.param K50={(50+49.9)/49.9}
.param RNPORT={50*49.9/(50+49.9)}
.step param AMP list 30m 50m 75m 100m 150m 200m 300m 500m 750m 1
.step param PW_NS list 10 15 20 25 30
.options plotwinsize=0 reltol=0.0002 abstol=1e-12 solver=alt
.temp 25
.include SAVE_NODES.inc
.tran 0 {TSTOP} 0 {TMAX}
.include MEASUREMENTS.inc
