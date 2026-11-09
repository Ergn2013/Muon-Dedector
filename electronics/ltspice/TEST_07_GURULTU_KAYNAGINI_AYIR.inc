* Gurultunun geldigi yeri ayir
* 0 temiz; 1 sinyal; 2 ref pini; 3 ikisi; 4 iki ref ortak gurultu; 5 sinyal+ref FILTRE GIRISI gurultusu.
.param AMP=30m
.param PW_NS=10
.param RATIO2=1
.param DELAY_NS=0
.param PAIR_NS=500
.param SECOND=0
.param ON1=1
.param ON2=1
.param N_SIG={if(MODE==0,0,if(MODE==2,0,10m))}
.param N_REF={if(MODE==2,10m,if(MODE==3,10m,if(MODE==4,10m,0)))}
.param N_UP={if(MODE==5,10m,0)}
.param REF_COMMON={if(MODE==4,1,0)}
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
.param MODE=0
.step param MODE list 0 1 2 3 4 5
.options plotwinsize=0 reltol=0.0002 abstol=1e-12 solver=alt
.temp 25
.include SAVE_NODES.inc
.tran 0 {TSTOP} 0 {TMAX}
.include MEASUREMENTS.inc
