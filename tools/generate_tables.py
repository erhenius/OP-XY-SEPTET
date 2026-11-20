from pathlib import Path
import math, cmath
from math import sin, cos, sqrt, pi, log
s=Path(__file__).resolve().parents[1]/'septet'
hz=[80,160,320,640,1250,2500,8000]
freq=sorted(set([round(40*400**(i/32)) for i in range(33)]+hz))
def coeff(i,db):
 a=10**(db/40);w=2*pi*hz[i]/44100;c=cos(w);al=sin(w)/(2*sqrt(2))
 if i in [0,6]:
  # First-order analog shelves, bilinear warped so hz[i] is the dB midpoint.
  k=1/math.tan(w/2)
  if i==0:
   den=1/a+k
   g=(a-1/a)/den
   return [g,g,0,(k-1/a)/den,0]
  den=1+k/a
  g=(a-1/a)*k/den
  return [g,-g,0,(k/a-1)/den,0]
 else:b=[1+al*a,-2*c,1-al*a];d=[1+al/a,-2*c,1-al/a]
 return [(x-y)/d[0] for x,y in zip(b,d)]+[-d[1]/d[0],-d[2]/d[0]]
row=lambda xs:'{'+','.join(map(str,xs))+'}'
ct=[];rt=[];magnitudes=[[0]*len(freq) for _ in hz]
for i in range(7):
 cc=[];rr=[]
 for db in range(-12,13):
  p=coeff(i,db);cc.append(row(round(x*2**28) for x in p));points=[]
  for j,f in enumerate(freq):
   z=cmath.exp(-2j*pi*f/44100);v=(p[0]+p[1]*z+p[2]*z*z)/(1-p[3]*z-p[4]*z*z)
   re,im=(round(x*4096) for x in (v.real,v.imag))
   magnitudes[i][j]=max(magnitudes[i][j],math.ceil(math.hypot(re,im)))
   points.append(row((re,im)))
  rr.append(row(points))
 ct.append(row(cc));rt.append(row(rr))
# Bound each point even with all stages cascaded at their maximum gains.
# Allow two Q12 units for rounding each scaled complex pair.
for gains in zip(*magnitudes):
 bound=0
 for gain in gains:
  product=gain*(4096+bound+2)
  assert product < 2**31
  bound += (product+4095)//4096+2
 assert (bound+2)*4096 < 2**31
(s/'coefficients.inc').write_text('static const int coefficient[7][25][5] = '+row(ct)+';\n')
(s/'response.inc').write_text('static const short correction[7][25]['+str(len(freq))+'][2] = '+row(rt)+';\nstatic const unsigned points='+str(len(freq))+';\nstatic const int px[]='+row(round(31+418*log(f/40)/log(400)) for f in freq)+';\nstatic const int dot_index[]='+row(freq.index(f) for f in hz)+';\nstatic const int db_power[]='+row(round(65536*10**((-24+i/4)/10)) for i in range(145))+';\n')
