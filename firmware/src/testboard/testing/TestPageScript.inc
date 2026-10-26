// EnvNode: see repository LICENSE.
static const char TestPageScript[] PROGMEM = R"SCRIPT(
(()=>{'use strict';
const $=id=>document.getElementById(id), app=$('test-app'), de=app.dataset.language==='de';
const tr=(a,b)=>de?a:b, ns='http://www.w3.org/2000/svg';
let board, state, busy=false, shown=-1, cells=[], socketLeds=[], contactLeds=[], portItems=[], activeMarker;
const colors=['#94a3b8','#5fbea6','#35dc73','#ff4747','#ff4747','#ff4747'];
const verdicts=[tr('ungeprüft','pending'),tr('bestanden','passed'),tr('Fehler','failed'),tr('unvollständig','incomplete')];
const verdictColors=['#94a3b8','#35dc73','#ff4747','#ffbf42'];
const kind={gpio:'GPIO / JST-PH',i2c:'I²C / JST-SH',module:tr('Modul / 2×7','Module / 2×7')};
const svg=(name,attrs,parent)=>{const el=document.createElementNS(ns,name);for(const[k,v]of Object.entries(attrs))el.setAttribute(k,v);parent.appendChild(el);return el;};
function drawing(){
 const root=svg('svg',{viewBox:board.bounds.join(' '),'aria-label':board.name},$('test-drawing'));
 svg('image',{x:board.bounds[0],y:board.bounds[1],width:board.bounds[2],height:board.bounds[3],href:'/test/board.svg'},root);
 board.socket.forEach(p=>{const el=svg('circle',{cx:p.x,cy:p.y,r:1,fill:colors[0],stroke:'#fff','stroke-width':.25},root);svg('title',{},el).textContent=`Socket ${p.number}: ${p.net}`;socketLeds.push(el);});
 board.connectors.forEach((c,i)=>{contactLeds[i]=c.pins.map(p=>{const el=svg('rect',{x:p.x-.65,y:p.y-.65,width:1.3,height:1.3,fill:p.covered?colors[0]:'#ffbf42',stroke:'#fff','stroke-width':.2},root);svg('title',{},el).textContent=`${c.id} / ${p.number}: ${p.net}`;return el;});const li=document.createElement('li'), button=document.createElement('button');button.type='button';button.onclick=()=>command('select',{connector:i});li.appendChild(button);$('test-connectors').appendChild(li);portItems.push(button);});
 activeMarker=svg('rect',{fill:'none',stroke:'#57baff','stroke-width':.7,'stroke-dasharray':'1 0.5',visibility:'hidden'},root);
}
function matrix(c){
 shown=state.step;cells=[];$('test-matrix').replaceChildren();if(!c)return;
 const head=document.createElement('tr'), label=document.createElement('th');label.textContent=tr('Kontakt / Netz','Contact / net');head.appendChild(label);
 for(let p=1;p<=38;p++){const th=document.createElement('th');th.textContent=p;head.appendChild(th);}$('test-matrix').appendChild(head);
 c.pins.forEach(p=>{const row=document.createElement('tr'),th=document.createElement('th');th.textContent=`${p.number} · ${p.net}${p.covered?'':' ⚠'}`;row.appendChild(th);for(let n=1;n<=38;n++){const td=document.createElement('td');td.textContent='·';td.title=`${c.id}/${p.number} → ${n}: ${p.expected.includes(n)?tr('Durchgang erwartet','continuity expected'):tr('Trennung erwartet','isolation expected')}`;row.appendChild(td);cells.push(td);}$('test-matrix').appendChild(row);});
}
function paint(){
 const c=board.connectors[state.step], phase=state.phase;
 if(shown!==state.step)matrix(c);
 $('test-load').disabled=busy||phase===2;$('test-start').disabled=busy||![1,3].includes(phase);$('test-start').textContent=phase===3?tr('Verbunden – erneut prüfen','Connected – test again'):tr('Verbunden – Prüfung starten','Connected – start test');$('test-next').disabled=busy||phase!==3;$('test-abort').disabled=busy||![1,2,3].includes(phase);$('test-fault').disabled=busy||![1,3].includes(phase);
 const active=[1,2,3].includes(phase)&&c;
 if(active){const xs=c.pins.map(p=>p.x),ys=c.pins.map(p=>p.y);for(const[k,v]of Object.entries({x:Math.min(...xs)-2,y:Math.min(...ys)-2,width:Math.max(...xs)-Math.min(...xs)+4,height:Math.max(...ys)-Math.min(...ys)+4,visibility:'visible'}))activeMarker.setAttribute(k,v);}else activeMarker.setAttribute('visibility','hidden');
 $('test-instruction').textContent=phase===3&&c?c.id+tr(' – Prüfung abgeschlossen',' – test complete'):active?tr(`Verbinde den ${kind[c.pigtail]}-Pigtail mit ${c.id} des Weather.`,`Connect the ${kind[c.pigtail]} pigtail to ${c.id} on Weather.`):phase===4?tr('Testlauf abgeschlossen','Session complete'):phase===5?tr('Testlauf abgebrochen','Session aborted'):tr('Board laden, um zu beginnen','Load a board to begin');
 $('test-summary').textContent=phase===2?tr(`Prüfung läuft: ${state.completed} / ${c.pins.length*38}`,`Test running: ${state.completed} / ${c.pins.length*38}`):phase===3?tr(`Anschluss: ${verdicts[state.verdicts[state.step]]}. Für den nächsten Schritt den Pigtail umstecken.`,`Connector: ${verdicts[state.verdicts[state.step]]}. Move the pigtail for the next step.`):phase===4?tr(`Gesamtergebnis: ${verdicts[state.overall]} – Simulation.`,`Overall result: ${verdicts[state.overall]} – simulation.`):tr('Jeder Anschluss muss vor seinem Test ausdrücklich bestätigt werden.','Confirm the connection before each connector test.');
 $('test-progress').max=c?c.pins.length*38:1;$('test-progress').value=state.completed;
 portItems.forEach((li,i)=>{li.textContent=`${board.connectors[i].id} · ${verdicts[state.verdicts[i]||0]}`;li.disabled=busy||phase===0||phase===2;li.setAttribute('aria-pressed',String(!!active&&i===state.step));li.parentElement.classList.toggle('current',!!active&&i===state.step);});
 board.connectors.forEach((port,i)=>contactLeds[i].forEach((led,p)=>{let color=verdictColors[Number((state.contacts||'')[i*14+p])||0];if(!port.pins[p].covered&&color!==verdictColors[2])color='#ffbf42';led.setAttribute('fill',color);}));
 socketLeds.forEach(el=>el.setAttribute('fill',colors[0]));
 if(!c)return;
 const values=Array.from(state.results,Number), symbols=['·','○','●','!','×','E'];
 values.forEach((v,i)=>{if(cells[i]){cells[i].textContent=symbols[v];cells[i].style.color=v>=3?'#a32828':v?'#177245':'#777';}});
 c.pins.forEach((p,i)=>{const row=values.slice(i*38,(i+1)*38),done=row.every(v=>v>0),fail=row.some(v=>v>=3);contactLeds[state.step][i].setAttribute('fill',fail?'#ff4747':!p.covered?'#ffbf42':done?'#35dc73':colors[0]);});
 const currentRow=Math.min(c.pins.length-1,Math.floor(Math.max(0,state.completed-(phase===2?0:1))/38));
 for(let i=0;i<38;i++)socketLeds[i].setAttribute('fill',colors[values[currentRow*38+i]||0]);
 if(phase===2&&state.completed<c.pins.length*38){socketLeds[state.completed%38].setAttribute('fill','#57baff');contactLeds[state.step][Math.floor(state.completed/38)].setAttribute('fill','#57baff');}
}
async function command(action, extra={}){
 if(busy||!state)return;busy=true;paint();$('test-error').textContent='';
 try{const response=await fetch('/test/action',{method:'POST',body:new URLSearchParams({action,generation:state.generation,board:board.id,fault:$('test-fault').value,...extra})});const body=await response.json();if(!response.ok)throw Error(body.error||response.status);state=body;shown=-1;paint();}catch(e){$('test-error').textContent=e.message;}finally{busy=false;paint();}
}
async function poll(){
 try{if(!busy){const response=await fetch('/test/state',{cache:'no-store'});if(!response.ok)throw Error(response.status);const next=await response.json();if(!state||next.generation>=state.generation||next.phase===0){state=next;paint();}}}catch(e){$('test-error').textContent=tr('Statusverbindung unterbrochen: ','Status connection interrupted: ')+e.message;}finally{setTimeout(poll,180);}
}
(async()=>{try{const response=await fetch('/test/definition',{cache:'no-store'});if(!response.ok)throw Error(response.status);board=await response.json();const option=document.createElement('option');option.value=board.id;option.textContent=`${board.name} · ${board.identityRevision} · PCB ${board.pcbRevision}`;$('test-board').appendChild(option);$('test-source').textContent=`PCB ${board.pcbRevision} · SHA-256 ${board.sha256}`;drawing();for(const [id,action]of [['test-load','load'],['test-start','start'],['test-next','next'],['test-abort','abort']])$(id).onclick=()=>command(action);await poll();}catch(e){$('test-error').textContent=e.message;}})();
})();
)SCRIPT";
