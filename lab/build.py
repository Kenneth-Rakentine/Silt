import re
p=open('page.html').read()
eng=open('engine.js').read(); pat=open('patches.js').read(); ui=open('ui.js').read()
for s in (eng,pat,ui): assert '</script' not in s
p=p.replace('/*ENGINE*/',eng).replace('/*PATCHES*/',pat).replace('/*UI*/',ui)
open('silt-lab.html','w').write(p)
print(len(p))
