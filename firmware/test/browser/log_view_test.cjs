// EnvNode: see repository LICENSE.
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
class Element {
 constructor(){this.dataset={};this.children=[];this.listeners={};this.checked=true;this.textContent='';}
 appendChild(child){this.children.push(child);return child;}
 append(...children){this.children.push(...children);}
 replaceChildren(...children){this.children=children;}
 addEventListener(name,fn){this.listeners[name]=fn;}
}
async function run(offset){
 const ids=['log-content','log-count','log-auto-refresh','log-refresh-status','log-page-offset','log-pagination','log-range'];
 const items=Object.fromEntries(ids.map(id=>[id,new Element()]));
 items['log-count'].textContent='64 / 64';items['log-page-offset'].textContent=String(offset);
 items['log-content'].dataset.newestSequence='64';items['log-pagination'].dataset.de='1';
 let callback,calls=0,fail=false;
 const entry={sequence:55,time:'now',level:'WARN',message:'<script>untrusted</script>'};
 const context={document:{getElementById:id=>items[id],createElement:()=>new Element()},
  EnvNodeUi:{On2S:'On',On2SRefreshFailed:'Failed',Paused:'Paused',SEQ:'SEQ',TIME:'TIME',LEVEL:'LEVEL',MESSAGE:'MESSAGE'},
  setInterval(fn,ms){assert.equal(ms,2000);callback=fn;},
  async fetch(url,options){++calls;assert.equal(url,`/logs/data?offset=${offset}&limit=8`);assert.equal(options.cache,'no-store');if(fail)throw Error('network');return {ok:true,json:async()=>({count:64,capacity:64,entries:[entry]})};}};
 vm.createContext(context);
 const cpp=fs.readFileSync(path.join(__dirname,'../../lib/EnvNodeWebAdmin/src/LogWebView.cpp'),'utf8');
 const script=cpp.match(/R"LOGJS\(([\s\S]*?)\)LOGJS"/)[1];vm.runInContext(script,context);
 await callback();assert.equal(calls,1);assert.equal(items['log-refresh-status'].textContent,'On');
 assert.equal(items['log-pagination'].children.length,offset?2:1);
 assert.equal(items['log-pagination'].children.at(-1).href,`/logs?offset=${offset+8}`);
 assert.equal(items['log-range'].textContent,`${offset+1}–${offset+1} / 64`);
 const table=items['log-content'].children[0].children[0],row=table.children[1].children[0];
 assert.equal(row.children[3].textContent,entry.message);
 assert.equal(row.children[3].children.length,0); // Text, never interpreted HTML.
 items['log-auto-refresh'].checked=false;await callback();assert.equal(calls,1);
 items['log-auto-refresh'].checked=true;fail=true;await callback();
 assert.equal(items['log-refresh-status'].textContent,'Failed');
 assert.equal(items['log-pagination'].children.length,offset?2:1);
}
(async()=>{await run(0);await run(8);console.log('Log view: pagination, polling, safe text, pause and failed refresh passed.');})().catch(error=>{console.error(error);process.exitCode=1;});
