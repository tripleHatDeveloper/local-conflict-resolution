from paths import P
import re,sys,itertools,collections,time
from pysat.solvers import Cadical153
idx={}
for l in open(P('clauses-bloc1_zeta.corresp')):
    n,i=l.split(); idx[int(i)]=n
cls=[]
for l in open(P('clauses-bloc1_zeta')).read().split('\n')[1:]:
    t=l.split()
    if t: cls.append(list(map(int,t[1:])))
lab={}
for l in open(P('clauses-bloc1_zeta.set')).read().split('\n')[1:]:
    t=l.split()
    if len(t)==2: lab[int(t[0])]=t[1]
E=sorted(n for n,c in lab.items() if c=='E'); hard=[n for n,c in lab.items() if c!='E']
nv=max(abs(x) for c in cls for x in c)
sel={n:nv+1+k for k,n in enumerate(E)}
rsel={v:n for n,v in sel.items()}
S=Cadical153()
for n in hard: S.add_clause(cls[n-1])
for n in E: S.add_clause(cls[n-1]+[-sel[n]])
def sat(subset): return S.solve(assumptions=[sel[n] for n in subset])
def name(n): return idx[abs(cls[n-1][0])]
if __name__=='__main__':
    print(len(E),'soft (E),',len(hard),'hard (Id+Ic)')
    print('hard alone SAT:',S.solve())
    print('hard+all E SAT:',sat(E))
    print('individually inconsistent E:',[n for n in E if not sat([n])])
