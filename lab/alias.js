const H=require('./harness.js');
function fft(re,im){const n=re.length;for(let i=1,j=0;i<n;i++){let bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j){[re[i],re[j]]=[re[j],re[i]];[im[i],im[j]]=[im[j],im[i]];}}
for(let len=2;len<=n;len<<=1){const ang=-2*Math.PI/len,wr=Math.cos(ang),wi=Math.sin(ang);for(let i=0;i<n;i+=len){let cr=1,ci=0;for(let j=0;j<len/2;j++){const a=i+j,b=a+len/2,br=re[b]*cr-im[b]*ci,bi=re[b]*ci+im[b]*cr;re[b]=re[a]-br;im[b]=im[a]-bi;re[a]+=br;im[a]+=bi;const t=cr*wr-ci*wi;ci=cr*wi+ci*wr;cr=t;}}}}
function measure(note,mod){
 const e=H.newEngine().p; const pt=H.makePatch('Init: saw'); Object.assign(pt.P,{drift:0,heat:0,drive:0,reso:0,spMix:0,keytrack:0,vol:0.3,velCut:0},mod.P||{});
 pt.S.forEach(s=>{s.cutoff=20000; Object.assign(s,mod.S||{}); s.level=1;}); e.msg({type:'globals',P:pt.P}); e.msg({type:'states',S:pt.S});
 const r=H.render(e,1.5,[{t:0,type:'on',n:note,v:1}]);
 const N=32768, re=new Float64Array(N), im=new Float64Array(N); for(let i=0;i<N;i++){const x=2*Math.PI*i/N, w=0.35875-0.48829*Math.cos(x)+0.14128*Math.cos(2*x)-0.01168*Math.cos(3*x); re[i]=r.L[24000+i]*w;}
 fft(re,im); const f0=440*Math.pow(2,(note-69)/12), bin=48000/N; let harm=0,alias=0;
 for(let k=2;k<N/2;k++){const f=k*bin, p=re[k]*re[k]+im[k]*im[k]; const h=f/f0, d=Math.abs(h-Math.round(h))*f0; if(d<8*bin && Math.round(h)>=1) harm+=p; else alias+=p;}
 return (10*Math.log10(alias/harm)).toFixed(1);
}
const cases={'saw plain':{},'saw fold .6':{S:{fold:0.6}},'square tbl':{P:{aPos:4/7}},'glass bank':{P:{aBank:4,aPos:0.5}},'bitmask':{P:{aBank:5,aPos:2/7}},'pm .5 w/ B':{P:{bOn:1,pmAmt:0.5,bSemi:7}},'ring A*B':{P:{bOn:1,bSemi:7,aLvl:0,bLvl:0},S:{ring:1}},'derez .4':{P:{derez:0.4}},'pulsar f4 c2':{P:{aForm:4,aCyc:2,aWin:0.6}}};
for(const [k,m] of Object.entries(cases)) console.log(k.padEnd(14), [48,72,84,96].map(n=>'n'+n+' '+measure(n,m)+'dB').join('  '));
