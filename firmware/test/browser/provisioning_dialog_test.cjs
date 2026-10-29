// EnvNode: see repository LICENSE.
const assert = require('node:assert/strict'), vm = require('node:vm'), fs = require('node:fs'), path = require('node:path');
class Element {
 constructor(){this.value='';this.dataset={};this.children=[];this.listeners={};this.hidden=false;this.checked=false;this.disabled=false;}
 addEventListener(n,f){(this.listeners[n]??=[]).push(f);}
 fire(n){const e={defaultPrevented:false,preventDefault(){this.defaultPrevented=true;}};for(const f of this.listeners[n]||[])f(e);return e;}
 appendChild(o){this.children.push(o);}
 get options(){return this.children;}
 set textContent(v){this.text=v;this.children=[];this.value='';}get textContent(){return this.text;}
 removeAttribute(n){delete this[n];}
 reportValidity(){return !!items['remote-profile-confirm'].checked;}
}
const items={};
for(const id of ['remote-profile-slot','remote-profile-batch','remote-profile-date','provision-details','remote-profile-form','remote-profile-key','provision-hardware-kind','remote-profile-image','remote-profile-confirm','remote-profile-submit','remote-profile-entry','remote-profile-data','remote-module-fields','remote-profile-serial','remote-profile-minimum','provision-board-key','remote-profile-status'])items[id]=new Element();
const root=new Element();root.dataset.de='1';root.querySelector=s=>items[s.slice(1)];
const doc={readyState:'complete',getElementById:()=>root,createElement:()=>new Element()};
for(const [key,name,kind,revision] of [['board:2:0.9','EnvNode Mini',0,'0.9'],['board:1:0.3','EnvNode Weather',0,'0.3'],['module:duo-relay-0.3','EnvNode DuoRelay',1,'0.3']]){
 const o=new Element();o.value=key;o.dataset={name,kind:String(kind),revision,minimum:'0.8.2',image:'https://docs.envnode.de/assets/images/example.png'};items['remote-profile-key'].appendChild(o);
}
items['provision-hardware-kind'].value='0';
vm.runInNewContext(fs.readFileSync(path.resolve(__dirname,'../../lib/EnvNodeWebAdmin/src/ProvisioningDialog.js'),'utf8'),{document:doc,Set,Number,Array});
const select=items['remote-profile-key'],kind=items['provision-hardware-kind'],confirm=items['remote-profile-confirm'],form=items['remote-profile-form'];
assert.equal(select.options.length,3);assert.equal(select.value,'');assert.equal(items['remote-profile-submit'].disabled,true);
select.value='board:2:0.9';select.fire('change');assert.equal(form.action,'/hardware/board-identity/write');assert.equal(items['provision-board-key'].value,'2:0.9');assert.equal(items['remote-module-fields'].hidden,true);assert.equal(items['remote-profile-image'].src,'https://docs.envnode.de/assets/images/example.png');
items['remote-profile-batch'].value='batch-42';items['remote-profile-batch'].fire('input');assert.ok(items['remote-profile-minimum'].textContent.includes('0.8.5'));
assert.equal(form.fire('submit').defaultPrevented,true);confirm.checked=true;assert.equal(form.fire('submit').defaultPrevented,false);
kind.value='1';kind.fire('change');assert.equal(select.options.length,2);assert.equal(confirm.checked,false);assert.equal(items['remote-module-fields'].hidden,false);
select.value='module:duo-relay-0.3';select.fire('change');assert.equal(form.action,'/hardware/module-descriptor/write');assert.equal(items['remote-profile-serial'].maxLength,64);
const remote={key:'duo|123',name:'EnvNode DuoRelay',kind:1,revision:[0,3],minimum:[0,5,0],image:'assets/images/duorelay-assembly/top.png'};
root.provisioning.online([remote]);assert.equal(select.options.length,2);assert.equal(select.value,remote.key);assert.equal(form.action,'/hardware/profile/write');assert.equal(confirm.checked,false);assert.equal(items['remote-profile-image'].src,'https://docs.envnode.de/assets/images/duorelay-assembly/top.png');
confirm.checked=true;assert.equal(form.fire('submit').defaultPrevented,true);root.dataset.remoteReady='1';assert.equal(form.fire('submit').defaultPrevented,false);
kind.value='0';kind.fire('change');assert.equal(select.options.length,3);assert.equal(select.value,'');assert.equal(items['remote-profile-serial'].maxLength,17);
console.log('PASS shared dialog: offline boards/modules, one filtered selection, confirmation, exact write targets, image, online deduplication and selection retention');
