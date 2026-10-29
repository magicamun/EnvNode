// EnvNode: see repository LICENSE.
const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs'),path=require('node:path');
class Element {
 constructor(){this.value='';this.dataset={};this.children=[];this.listeners={};this.hidden=false;this.checked=false;this.disabled=false;}
 addEventListener(n,f){(this.listeners[n]??=[]).push(f);}
 async fire(n){const e={defaultPrevented:false,preventDefault(){this.defaultPrevented=true;}};for(const f of this.listeners[n]||[])await f(e);return e;}
 appendChild(o){this.children.push(o);} get options(){return this.children;}
 set textContent(v){this.text=v;this.children=[];this.value='';}get textContent(){return this.text||'';}
 removeAttribute(n){delete this[n];}reportValidity(){return !!items['remote-profile-confirm'].checked;}
}
const items={};
for(const id of ['provision-result','provision-snapshot-token','provision-read','provision-adopt','provision-current-status','provision-current-target','provision-current-fields','provision-comparison','remote-profile-slot','remote-profile-batch','remote-profile-date','provision-details','remote-profile-form','remote-profile-key','provision-hardware-kind','remote-profile-image','remote-profile-confirm','remote-profile-submit','remote-profile-entry','remote-profile-data','remote-module-fields','remote-profile-serial','remote-profile-minimum','provision-board-key','remote-profile-status'])items[id]=new Element();
const root=new Element();root.dataset.de='1';root.querySelector=s=>items[s.slice(1)];
const doc={readyState:'complete',html:'',getElementById:()=>root,createElement:()=>new Element(),open(){},write(s){this.html=s;},close(){}};
for(const [key,name,kind,revision] of [['board:2:0.9','EnvNode Mini',0,'0.9'],['board:1:0.3','EnvNode Weather',0,'0.3'],['module:duo-relay-0.3','EnvNode DuoRelay',1,'0.3']]){
 const o=new Element();o.value=key;o.dataset={name,kind:String(kind),revision,minimum:'0.8.2',image:'https://docs.envnode.de/assets/images/example.png'};items['remote-profile-key'].appendChild(o);
}
items['provision-hardware-kind'].value='0';items['remote-profile-slot'].value='A';
let current={status:'Valid',token:'b:old',name:'EnvNode Mini',revision:'0.9',serial:'OLD-SERIAL',batch:'OLD-BATCH',date:'2026-10-29'},writes=0,unavailable=false;
const calls=[];
async function fetch(url,options={}){
 calls.push(url);
 if(options.method==='POST'){
  if(options.body.get('snapshotToken')!==current.token)return {status:409};
  ++writes;return {status:200,text:async()=>'<html>Written</html>'};
 }
 if(unavailable)throw Error('Offline');
 const value=url.includes('kind=module')?{...current,name:'EnvNode DuoRelay',revision:'0.3',token:url.includes('slot=B')?'B:module':'A:module'}:{...current};
 return {ok:true,status:200,json:async()=>value};
}
class FormData extends Map{constructor(){super();this.set('snapshotToken',items['provision-snapshot-token'].value);}}
vm.runInNewContext(fs.readFileSync(path.resolve(__dirname,'../../lib/EnvNodeWebAdmin/src/ProvisioningDialog.js'),'utf8'),{document:doc,Set,Number,Array,fetch,FormData,AbortController,setTimeout,clearTimeout});
async function settled(){for(let i=0;i<10;i++)await new Promise(r=>setImmediate(r));}
(async()=>{
 await settled();
 const select=items['remote-profile-key'],kind=items['provision-hardware-kind'],confirm=items['remote-profile-confirm'],form=items['remote-profile-form'];
 assert.equal(items['provision-current-status'].textContent,'Gültig');assert.equal(items['provision-snapshot-token'].value,'b:old');
 assert.equal(items['remote-profile-serial'].value,'');assert.equal(items['remote-profile-batch'].value,'');assert.equal(items['provision-adopt'].disabled,false);
 assert.equal(select.options.length,3);assert.equal(select.value,'');assert.equal(items['remote-profile-submit'].disabled,true);
 select.value='board:2:0.9';await select.fire('change');await settled();
 assert.equal(form.action,'/hardware/board-identity/write');assert.equal(items['provision-board-key'].value,'2:0.9');assert.equal(items['remote-module-fields'].hidden,true);
 assert.ok(items['provision-comparison'].textContent.includes('OLD-SERIAL → (leer)'));
 await items['provision-adopt'].fire('click');assert.equal(items['remote-profile-serial'].value,'OLD-SERIAL');assert.equal(items['remote-profile-batch'].value,'OLD-BATCH');assert.equal(confirm.checked,false);
 items['remote-profile-batch'].value='NEW-BATCH';await items['remote-profile-batch'].fire('input');assert.ok(items['remote-profile-minimum'].textContent.includes('0.8.5'));assert.ok(items['provision-comparison'].textContent.includes('OLD-BATCH → NEW-BATCH'));
 await form.fire('submit');assert.equal(writes,0);
 confirm.checked=true;current={...current,token:'b:other',serial:'OTHER-DUT'};await form.fire('submit');await settled();assert.equal(writes,0);assert.equal(confirm.checked,false);assert.equal(items['provision-snapshot-token'].value,'b:other');assert.equal(items['remote-profile-batch'].value,'NEW-BATCH');assert.ok(items['provision-result'].textContent.includes('Nichts geschrieben'));
 confirm.checked=true;await form.fire('submit');assert.equal(writes,1);assert.ok(doc.html.includes('Written'));
 confirm.checked=true;await items['provision-read'].fire('click');await settled();assert.equal(confirm.checked,false);
 kind.value='1';await kind.fire('change');await settled();assert.equal(select.options.length,2);assert.equal(items['provision-snapshot-token'].value,'A:module');assert.equal(items['remote-module-fields'].hidden,false);
 select.value='module:duo-relay-0.3';await select.fire('change');assert.equal(form.action,'/hardware/module-descriptor/write');assert.equal(items['remote-profile-serial'].maxLength,64);
 items['remote-profile-slot'].value='B';confirm.checked=true;await items['remote-profile-slot'].fire('change');await settled();assert.equal(confirm.checked,false);assert.equal(items['provision-snapshot-token'].value,'B:module');assert.ok(calls.includes('/hardware/eeprom?kind=module&slot=B'));
 root.provisioning.online([{key:'duo|123',name:'EnvNode DuoRelay',kind:1,revision:[0,3],minimum:[0,5,0],image:'assets/images/duorelay-assembly/top.png'}]);assert.equal(select.options.length,2);assert.equal(select.value,'duo|123');assert.equal(confirm.checked,false);
 confirm.checked=true;assert.equal((await form.fire('submit')).defaultPrevented,true);root.dataset.remoteReady='1';confirm.checked=true;assert.equal((await form.fire('submit')).defaultPrevented,false);
 unavailable=true;await items['provision-read'].fire('click');await settled();assert.equal(items['provision-snapshot-token'].value,'');assert.equal(items['provision-adopt'].disabled,true);assert.equal(items['remote-profile-submit'].disabled,true);assert.equal(confirm.checked,false);
 assert.ok(items['provision-current-status'].textContent.includes('Lesen fehlgeschlagen'));
 console.log('PASS shared dialog: EEPROM display, explicit adoption, before/after comparison, stale-write refusal, reconfirmation, board/module/slot binding, offline failure and online deduplication');
})().catch(e=>{console.error(e);process.exitCode=1;});
