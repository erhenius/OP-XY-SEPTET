from pathlib import Path
import re, math
s=Path(__file__).resolve().parents[1]/'septet'
pos=list(map(int,re.search(r'int px\[\]=\{([^}]+)',(s/'response.inc').read_text()).group(1).split(',')))
p=['<svg xmlns="http://www.w3.org/2000/svg" width="480" height="220" viewBox="0 0 480 220">','<rect width="480" height="220" fill="#090909"/>']
def path(name,d,style):
 p.append(f'<path id="{name}" d="{d}" {style}/>')
def text(x,y,t,anchor='end'):
 p.append(f'<text transform="matrix(1 0 0 1 {x} {y})" text-anchor="{anchor}" font-family="UniversNextPro-Light" font-size="9px" fill="#a8a8a8">{t}</text>')
x=lambda f:round(31+418*math.log(f/40)/math.log(400))
ticks=[40,100,300,1000,3000,10000,16000]
for i,(a,b) in enumerate(zip(ticks,ticks[1:])):
 color=['#161616','#1d1d1d'][i%2];p.append(f'<rect x="{x(a)}" y="21" width="{x(b)-x(a)}" height="144" fill="{color}"/>')
for db in (12,0,-12,-24):
 y=21+(12-db)*4
 if db: path('grid'+str(12-db),f'M31 {y}H449','stroke="#494949" stroke-width="1" fill="none"')
 text(26,y+3,str(db) if db<=0 else '+12')
text(26,12,'dB');text(449,179,'Hz')
for f in ticks[1:-1]:text(x(f),179,str(f) if f<1000 else str(f//1000)+'k','middle')
# Trine's stock-compatible topology: start at the closing edge, walk the
# live contour with explicit L commands, then return to the closing edge.
def contour(y):
 return 'M31 168 '+ ' '.join(f'L{n} {y}' for n in pos)+' L449 168 Z'
def ribbon(top,bottom):
 vertices=[(x,top) for x in pos]+[(x,bottom) for x in reversed(pos)]
 return 'M'+' L'.join(f'{x} {y}' for x,y in vertices)+' Z'
path('fillcurve',contour(70),'fill="#242424"')
zero_line=' '.join(f'M{left} 69H{min(left+4,449)}' for left in range(31,449,10))
path('zero',zero_line,'stroke="#777777" stroke-width="1" fill="none"')
path('secondary',ribbon(68,69),'fill="#999999" transform="matrix(1 0 0 1 -480 0)"')
path('curve',ribbon(68,70),'fill="#eeeeee"')
red='#ff4949'
for i,f in enumerate([80,160,320,640,1250,2500,8000]):
 path('dot'+str(i),'M-3 0Q-3 -3 0 -3Q3 -3 3 0Q3 3 0 3Q-3 3 -3 0Z','fill="#eeeeee" stroke="#aaaaaa" stroke-width="0.8"')
path('sweep_dot','M-2.5 0Q-2.5 -2.5 0 -2.5Q2.5 -2.5 2.5 0Q2.5 2.5 0 2.5Q-2.5 2.5 -2.5 0Z',
     f'fill="{red}" transform="matrix(1 0 0 1 22 197)"')
path('rail','M190 197H425 M190 194V200 M307 194V200 M425 194V200','fill="none" stroke="#777777" stroke-width="1.5"')
path('handle','M-4 0Q-4 -4 0 -4Q4 -4 4 0Q4 4 0 4Q-4 4 -4 0Z','fill="#eeeeee"')
icons=['M-15 -1H-10L-5 -7H5L10 -1H15V1H10L5 7H-5L-10 1H-15Z M-7 -1H7L4 -4H-4Z M-7 1L-4 4H4L7 1Z','M-11 8L0 -10L11 8Z M-7 5H7L0 -5Z','M-9 -8L-7 -6Q-15 0 -7 6L-9 8Q-19 0 -9 -8Z M9 -8Q19 0 9 8L7 6Q15 0 7 -6Z M-3 0Q-3 -3 0 -3Q3 -3 3 0Q3 3 0 3Q-3 3 -3 0Z']
ends=['M-9 -1H-6V-3H-2V-1H2V-3H6V-1H9V1H6V3H2V1H-2V3H-6V1H-9Z','M-9 -1H-6L-3 -5H3L6 -1H9V1H6L3 5H-3L-6 1H-9Z M-4 0L-2 3H2L4 0L2 -3H-2Z','M-9 1H-5Q0 -7 5 1H9V3H4Q0 -3 -4 3H-9Z',icons[1], 'M-3 0Q-3 -3 0 -3Q3 -3 3 0Q3 3 0 3Q-3 3 -3 0Z',icons[2].split(' M-3')[0]]
for i,d in enumerate(icons+ends):
 x={0:47,3:168,4:449}.get(i,-480)
 path('icon'+str(i),d,f'fill="#eeeeee" fill-rule="evenodd" transform="matrix(1 0 0 1 {x} 197)"')
for i in range(9):
 v=round(76+164*(i/8)**1.6);path('shade'+str(i),'M-8 -8h1v1z',f'fill="#{v:02x}{v:02x}{v:02x}"')
for i,v in enumerate(['#090909','#eeeeee','#909090'],9):
 stroke={10:f' stroke="{red}" stroke-width="1.6"',11:' stroke="#909090" stroke-width="1"'}.get(i,'')
 path('shade'+str(i),'M-8 -8h1v1z',f'fill="{v}"'+stroke)
p.append('</svg>');(s/'septet.svg').write_text('\n'.join(p)+'\n')
