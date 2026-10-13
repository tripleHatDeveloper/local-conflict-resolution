% ASP-values: rules only. Facts (dom, est, w, dry, dmin, dmax) are in real_block_facts.lp.
#defined dmin/5.
#defined dmax/5.
#defined dry/2.
group(P,K) :- dom(P,K,_).
1 { lev(P,K,V) : dom(P,K,V) } 1 :- group(P,K).
{ retract(E) } :- est(E,_,_,_).
:- est(E,P,K,V), not retract(E), not lev(P,K,V).
dryok(P,G,G) :- dry(P,G).
:- lev(P,m,M), lev(P,p,Q), Q-M < 10, not dryok(P,M,Q).
:- lev(X,KX,A), lev(Y,KY,B), dmin(X,KX,Y,KY,L), A-B < L.
:- lev(X,KX,A), lev(Y,KY,B), dmax(X,KX,Y,KY,H), A-B > H.
#minimize { W,E : retract(E), w(E,W) }.
#project retract/1.
#show retract/1.
