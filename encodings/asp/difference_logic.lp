% ASP-DL: rules only. Facts (dom, est, w, dry, dmin, dmax) are in real_block_facts.lp.
#defined dmin/5.
#defined dmax/5.
#defined dry/2.
group(P,K) :- dom(P,K,_).
lomin(P,K,L) :- group(P,K), L = #min { V : dom(P,K,V) }.
himax(P,K,H) :- group(P,K), H = #max { V : dom(P,K,V) }.
&diff { lvl(P,K) - 0 } >= L :- lomin(P,K,L).
&diff { lvl(P,K) - 0 } <= H :- himax(P,K,H).
{ retract(E) } :- est(E,_,_,_).
&diff { lvl(P,K) - 0 } = V :- est(E,P,K,V), not retract(E).
{ dryon(P) } :- dry(P,_).
&diff { lvl(P,m) - lvl(P,p) } <= -10 :- group(P,m), not dryon(P).
&diff { lvl(P,m) - 0 } = G :- dryon(P), dry(P,G).
&diff { lvl(P,p) - 0 } = G :- dryon(P), dry(P,G).
&diff { lvl(Y,KY) - lvl(X,KX) } <= -L :- dmin(X,KX,Y,KY,L).
&diff { lvl(X,KX) - lvl(Y,KY) } <= H :- dmax(X,KX,Y,KY,H).
#minimize { W,E : retract(E), w(E,W) }.
#show retract/1.
