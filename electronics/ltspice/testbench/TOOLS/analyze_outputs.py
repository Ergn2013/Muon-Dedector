"""Read LTspice forward binary RAW files using Python 3's standard library.

Counts crossings of a stated voltage, not actual STM32 hardware captures.
No signal is added, filtered, or repaired by this postprocessor.
"""
from pathlib import Path
import argparse,csv,json,math,mmap,re,struct,sys

CHANNELS={
 'STM_COINC':'v(stm_coinc)','STM_SING1':'v(stm_sing1)','STM_SING2':'v(stm_sing2)',
 'FAST':'v(coinc_fast)','CH1_PS':'v(ch1_ps)','CH2_PS':'v(ch2_ps)',
 'TLV1':'v(u3a_out)','TLV2':'v(u3b_out)'}

def as_float(s):
    m=re.fullmatch(r'([+-]?(?:\d*\.?\d+)(?:e[+-]?\d+)?)([a-z]*)',s.strip(),re.I)
    if not m:raise ValueError(s)
    unit=m.group(2).lower()
    scales={'':1,'t':1e12,'g':1e9,'meg':1e6,'k':1e3,'m':1e-3,'u':1e-6,'n':1e-9,'p':1e-12,'f':1e-15}
    return float(m.group(1))*scales[unit]

def log_steps(path):
    if not path.exists():return []
    b=path.read_bytes();s=b.decode('utf-16' if b[:2] in [b'\xff\xfe',b'\xfe\xff'] else 'utf-8',errors='replace')
    return [dict(re.findall(r'(\w+)\s*=\s*([^\s]+)',line,re.I)) for line in s.splitlines() if line.lower().startswith('.step ')]

class EdgeStats:
    def __init__(self,threshold):
        self.threshold=threshold;self.prev=None;self.rises=[];self.falls=[];self.widths=[];self.open_rise=None
        self.quiet_rises=0;self.high_time=0.;self.duration=0.;self.started_high=None;self.ended_high=False;self.peak=-math.inf
    def update(self,t,y):
        if t<.3e-6:self.prev=(t,y);return
        self.peak=max(self.peak,y)
        if self.started_high is None:self.started_high=y>self.threshold
        self.ended_high=y>self.threshold
        if self.prev is not None:
            t0,y0=self.prev
            if t>t0:
                low=max(t0,.3e-6);dt=t-low
                ya=y0+(y-y0)*(low-t0)/(t-t0)
                a=ya>self.threshold;b=y>self.threshold
                self.duration+=dt
                if a and b:self.high_time+=dt
                elif a!=b:
                    c=low+(self.threshold-ya)*dt/(y-ya)
                    self.high_time+=(c-low) if a else (t-c)
                    if b:
                        self.rises.append(c);self.open_rise=c
                        if .3e-6<=c<.7e-6:self.quiet_rises+=1
                    else:
                        self.falls.append(c)
                        if self.open_rise is not None:self.widths.append(c-self.open_rise);self.open_rise=None
        self.prev=(t,y)
    def result(self):
        return {'rises':len(self.rises),'quiet_rises':self.quiet_rises,'started_high':bool(self.started_high),
                'ended_high':self.ended_high,'high_fraction':self.high_time/self.duration if self.duration else 0.,
                'first_width_ns':self.widths[0]*1e9 if self.widths else None,
                'min_width_ns':min(self.widths)*1e9 if self.widths else None,
                'max_width_ns':max(self.widths)*1e9 if self.widths else None,
                'rise_times_us':[x*1e6 for x in self.rises[:50]],'peak_v':self.peak}

class RmsStats:
    def __init__(self):self.prev=None;self.integral=0.;self.duration=0.
    def update(self,t,y):
        if self.prev is not None:
            t0,y0=self.prev
            if t>t0:
                a=max(t0,.4e-6);b=min(t,.7e-6)
                if b>a:
                    ya=y0+(y-y0)*(a-t0)/(t-t0);yb=y0+(y-y0)*(b-t0)/(t-t0)
                    self.integral+=(b-a)*(ya*ya+ya*yb+yb*yb)/3;self.duration+=b-a
        self.prev=(t,y)
    def value(self):return math.sqrt(self.integral/self.duration) if self.duration else None

def analyse_raw(path,threshold=1.65):
    path=Path(path);steps=log_steps(path.with_suffix('.log'));results=[]
    with path.open('rb') as fh,mmap.mmap(fh.fileno(),0,access=mmap.ACCESS_READ) as mm:
        first=mm[:250000];enc='utf-16-le' if first.startswith(b'\xff\xfe') or (len(first)>1 and first[1]==0) else 'utf-8'
        marker='Binary:\n'.encode(enc);pos=first.find(marker)
        if pos<0:marker='Binary:\r\n'.encode(enc);pos=first.find(marker)
        if pos<0:raise ValueError('Binary RAW bekleniyor; ASCII RAW desteklenmiyor.')
        header=first[:pos].decode(enc,errors='replace');offset=pos+len(marker)
        flags=re.search(r'^Flags:\s*(.*)$',header,re.M).group(1).lower()
        if 'fastaccess' in flags:raise ValueError('FastAccess RAW desteklenmiyor; normal forward RAW kaydedin.')
        if 'complex' in flags:raise ValueError('Transient real RAW gerekiyor.')
        nv=int(re.search(r'No. Variables:\s*(\d+)',header).group(1));npnt=int(re.search(r'No. Points:\s*(\d+)',header).group(1))
        varsection=re.split(r'^Variables:\s*$',header,flags=re.M)[-1]
        variable_ids={name.lower():int(n) for n,name in re.findall(r'^\s*(\d+)\s+(\S+)\s+\S+',varsection,re.M)}
        missing=[name for name in CHANNELS.values() if name not in variable_ids]
        if missing:raise ValueError('Kaydedilmemis dugumler: '+', '.join(missing))
        # numdgt remains 6 in the supplied project; accept double-format RAW too.
        payload=len(mm)-offset
        stride_f=8+4*(nv-1);stride_d=8*nv
        if payload==npnt*stride_f:stride=stride_f;fmt='<f';value_bytes=4
        elif payload==npnt*stride_d:stride=stride_d;fmt='<d';value_bytes=8
        else:raise ValueError('RAW boyutu/baslik uyusmuyor; dosya tamamlanmamis olabilir.')
        unpack_t=struct.Struct('<d').unpack_from;unpack_v=struct.Struct(fmt).unpack_from
        offsets={name:8+(idx-1)*value_bytes for name,idx in variable_ids.items() if idx}
        def state():return ({key:EdgeStats(threshold) for key in CHANNELS},{key:RmsStats() for key in ['sma1','sma2','ref1','ref2']})
        edges,rms=state();last_t=-1.;step_index=0
        def finish():
            result={'file':path.name,'step':step_index+1,'step_parameters':steps[step_index] if step_index<len(steps) else {},'threshold_v':threshold,
                    'channels':{key:obj.result() for key,obj in edges.items()},'quiet_rms_v':{key:obj.value() for key,obj in rms.items()}}
            results.append(result)
        for i in range(npnt):
            base=offset+i*stride;t=unpack_t(mm,base)[0]
            if t<last_t:
                finish();step_index+=1;edges,rms=state()
            def value(name):return unpack_v(mm,base+offsets[name])[0]
            for key,name in CHANNELS.items():edges[key].update(t,value(name))
            if t<=.7e-6 or last_t<=.7e-6:
                for key,node in [('sma1','v(pmt1_sma)'),('sma2','v(pmt2_sma)')]:
                    if node in offsets:rms[key].update(t,value(node))
                for ch in [1,2]:
                    a=f'v(th{ch})';b=f'v(th{ch}_quiet)'
                    if a in offsets and b in offsets:rms[f'ref{ch}'].update(t,value(a)-value(b))
            last_t=t
        finish()
    return results

def write_outputs(results,directory):
    directory=Path(directory)
    (directory/'edge_counts.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
    rows=[]
    for r in results:
        row={'dosya':r['file'],'adim':r['step'],'ayarlar':json.dumps(r['step_parameters'],ensure_ascii=False),'sayim_esigi_V':r['threshold_v']}
        for k,s in r['channels'].items():
            row.update({k+'_kenar':s['rises'],k+'_bos_aralik_kenar':s['quiet_rises'],k+'_baslangicta_HIGH':int(s['started_high']),k+'_sonda_HIGH':int(s['ended_high']),k+'_HIGH_yuzde':100*s['high_fraction'],k+'_ilk_genislik_ns':s['first_width_ns']})
        for k,v in r['quiet_rms_v'].items():row[k+'_gurultu_mVrms']=None if v is None else v*1e3
        rows.append(row)
    if rows:
        with (directory/'edge_counts.csv').open('w',encoding='utf-8-sig',newline='') as f:
            w=csv.DictWriter(f,fieldnames=list(rows[0]),delimiter=';');w.writeheader();w.writerows(rows)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('raw',nargs='*',help='Bos birakilirsa proje klasorundeki test RAW dosyalarini oku.')
    parser.add_argument('--threshold',type=float,default=1.65)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    files=[Path(p).resolve() for p in args.raw] if args.raw else sorted(root.glob('[0-9][0-9]_*.raw'))
    files=[p for p in files if not p.name.endswith('.op.raw') and not p.name.startswith('12_BESLEME')]
    if not files:raise SystemExit('Calistirilmis test .raw dosyasi bulunamadi. Once LTspice testini calistirin.')
    results=[]
    for p in files:
        print('Okunuyor:',p.name,flush=True)
        results.extend(analyse_raw(p,args.threshold))
    write_outputs(results,root)
    print('Wrote edge_counts.csv and edge_counts.json.')
    print('Bunlar belirtilen voltaj esigindeki kenar sayilaridir; STM32 donanim/firmware simulasyonu degildir.')
if __name__=='__main__':main()
