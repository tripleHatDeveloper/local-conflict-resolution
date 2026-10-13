% ASP-thresholds: rules only. Facts (dom, est, w, dry, dmin, dmax) are in real_block_facts.lp.
#defined dmin/5.
#defined dmax/5.
#defined dry/2.
group(P,K) :- dom(P,K,_).
lomin(P,K,L) :- group(P,K), L = #min { V : dom(P,K,V) }.
himax(P,K,H) :- group(P,K), H = #max { V : dom(P,K,V) }.
{ ge(P,K,V) : dom(P,K,V), not lomin(P,K,V) }.
ge(P,K,V) :- lomin(P,K,V).
:- ge(P,K,V), dom(P,K,V-10), not ge(P,K,V-10).
{ retract(E) } :- est(E,_,_,_).
:- est(E,P,K,V), not retract(E), not ge(P,K,V).
:- est(E,P,K,V), not retract(E), dom(P,K,V+10), ge(P,K,V+10).
{ dryon(P) } :- dry(P,_).
:- dryon(P), dry(P,G), ge(P,m,V), V > G.
:- dryon(P), dry(P,G), ge(P,p,V), V > G.
:- group(P,m), not dryon(P), ge(P,m,V), dom(P,p,V+10), not ge(P,p,V+10).
:- group(P,m), not dryon(P), ge(P,m,V), himax(P,p,H), V+10 > H.
:- dmin(X,KX,Y,KY,L), ge(Y,KY,V), himax(X,KX,H), V+L > H.
:- dmin(X,KX,Y,KY,L), ge(Y,KY,V), dom(X,KX,V+L), lomin(X,KX,Xm), V+L > Xm, not ge(X,KX,V+L).
:- dmax(X,KX,Y,KY,H), ge(X,KX,V), himax(Y,KY,Ym), V-H > Ym.
:- dmax(X,KX,Y,KY,H), ge(X,KX,V), dom(Y,KY,V-H), lomin(Y,KY,Ym), V-H > Ym, not ge(Y,KY,V-H).
#minimize { W,E : retract(E), w(E,W) }.
#project retract/1.
#show retract/1.
