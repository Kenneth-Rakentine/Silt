const H=require('./harness.js');
// coupling check
{ const e=H.newEngine().p; const pt=H.makePatch('5 · Cloud to tone'); e.msg({type:'globals',P:pt.P}); e.msg({type:'states',S:pt.S});
  const evs=[{t:0,type:'on',n:48,v:0.9}]; let out=[];
  for(const T of [0.2,0.8,2,3.5]){ H.render(e,T-(out.length?[0.2,0.8,2,3.5][out.length-1]:0),out.length?[]:evs); out.push(e.lastV.A.R.toFixed(2)); }
  console.log('cloud coherence R at 0.2/0.8/2/3.5s:',out.join(' ')); }
// pulsar rise
{ const e=H.newEngine().p; const pt=H.makePatch('6 · Pulsar vowels'); e.msg({type:'globals',P:pt.P}); e.msg({type:'states',S:pt.S});
  let r=[]; H.render(e,0.05,[{t:0,type:'on',n:48,v:0.9}]); r.push((e.lastV.A.inc[0]*48000).toFixed(1)); H.render(e,1.5,[]); r.push((e.lastV.A.inc[0]*48000).toFixed(1)); H.render(e,2,[]); r.push((e.lastV.A.inc[0]*48000).toFixed(1));
  console.log('pulsar emission Hz at 0.05/1.55/3.55s:', r.join(' ')); }
// random stress
let worst=0,nan=0,maxcpu=0;
const keys=Object.keys(H.makePatch('Init: saw').P);
for(let t=0;t<40;t++){
  const e=H.newEngine().p; const names=Object.keys(H.PATCHES); const pt=H.makePatch(names[t%names.length]);
  const R=Math.random, P=pt.P;
  Object.assign(P,{aBank:(R()*6)|0,aPos:R(),aMat:R()<0.2?1:0,aForm:Math.pow(2,R()*6-1),aCyc:0.1+R()*8,aWin:R(),aN:1+((R()*6)|0),aDet:R()*80,aWander:R(),
   bOn:R()<0.6?1:0,bBank:(R()*6)|0,bForm:Math.pow(2,R()*5-1),bCyc:0.1+R()*4,bN:1+((R()*6)|0),bSemi:Math.round(R()*48-24),bSync:R()<0.3?1:0,pmAmt:R(),
   derez:R(),fMode:(R()*4)|0,reso:R(),drive:R(),heat:R(),auth:R(),stiff:0.2+R()*30,visc:0.05+R()*3,temp:R(),mass:R()*2-1,grainM:R()*2-1,allure:R(),
   vocOn:R()<0.5?1:0,vocBody:(R()*5)|0,vocSkin:(R()*3)|0,vocQ:0.7+R()*20,vocShift:R()*3-1.5,vocSpread:0.5+R()*1.5,scrapeRate:R()*400,scrapeDepth:R(),
   fbMix:R(),fbRegen:R()*1.1,fbDrive:0.3+R()*8,fbQ:0.5+R()*20,fbType:(R()*3)|0,spMix:R(),spDecay:0.2+R()*20,spSize:0.35+R()*1.7,loop:R()<0.5?1:0,loopTime:0.02+R()*3,curve:R()*2-1,resDecay:0.01+R()*4,dustRate:R()*3000,dropRate:R()*300,poly:6,listen:(R()*7)|0});
  pt.S.forEach(s=>{for(const k in s){ if(k==='cutoff') s[k]=20*Math.pow(1000,R()); else if(k==='pitch') s[k]=R()*9-7; else if(k==='formant') s[k]=R()*6-3; else if(k==='position') s[k]=R()*2-1; else s[k]=R()*(k==='level'?1.3:1);} });
  e.msg({type:'globals',P}); e.msg({type:'states',S:pt.S});
  const evs=[]; for(let i=0;i<10;i++){const n=24+((R()*72)|0); evs.push({t:i*0.25,type:'on',n,v:R()}); evs.push({t:i*0.25+0.3+R(),type:'off',n});} evs.sort((a,b)=>a.t-b.t);
  const r=H.render(e,4,evs); const s=H.stats(r.L,r.R); worst=Math.max(worst,s.peak); nan+=s.nan; maxcpu=Math.max(maxcpu,r.cpu);
}
console.log('stress: nan',nan,'worst peak',worst.toFixed(3),'max cpu',(maxcpu*100).toFixed(1)+'%');
