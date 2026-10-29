// EnvNode: see repository LICENSE. Uses the vendored library under its MIT license.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const crypto = require('node:crypto');
const root = path.resolve(__dirname, '../..');
const assets = path.resolve(root, '../docs/site/assets/hardware/v1');
const base = 'https://docs.envnode.de/assets/hardware/v1/';
function context() {
  const c = {Uint8Array, ArrayBuffer, TextEncoder, TextDecoder, AbortController, setTimeout, clearTimeout, console, crypto: undefined, isSecureContext: false};
  c.window = c; vm.createContext(c);
  vm.runInContext(fs.readFileSync(path.join(root, 'lib/EnvNodeWebAdmin/vendor/js-sha256/sha256.min.js'), 'utf8'), c);
  vm.runInContext(fs.readFileSync(path.join(root, 'lib/EnvNodeWebAdmin/src/BrowserHardwareProfiles.js'), 'utf8'), c);
  return c;
}
const bytes = name => new Uint8Array(fs.readFileSync(path.join(assets, name)));
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
function encode(v) {
  function head(major, n) {
    if (n < 24) return Buffer.from([(major << 5) | n]);
    if (n < 256) return Buffer.from([(major << 5) | 24, n]);
    if (n < 65536) return Buffer.from([(major << 5) | 25, n >> 8, n & 255]);
    throw Error('FixtureTooLarge');
  }
  if (Array.isArray(v)) return Buffer.concat([head(4, v.length), ...v.map(encode)]);
  if (typeof v === 'number') return head(0, v);
  const data = Buffer.from(v); return Buffer.concat([head(3, data.length), data]);
}
const fields = e => [e.id, e.name, e.kind, Array.from(e.revision), Array.from(e.minimum), e.image, e.filename, e.size, e.digest];
function transport(c) {
  const state = {locked: false, offline: false, corrupt: false, files: new Map(), calls: [], posts: []};
  state.fetch = async (url, options = {}) => {
    state.calls.push({url, options});
    if (url === '/hardware/profile/state') return {ok: !state.locked, status: state.locked ? 409 : 200};
    if (url === '/hardware/profile/write') {
      state.posts.push(options.body);return {ok: true, status: 200, text: async () => '<html>Provisioned</html>'};
    }
    assert.ok(url.startsWith(base)); assert.equal(options.credentials, 'omit'); assert.equal(options.redirect, 'error');assert.equal(options.mode, 'cors');
    if (state.offline) throw Error('Offline');
    const name = url.slice(base.length);
    let data = state.files.has(name) ? state.files.get(name) : bytes(name);
    data = Uint8Array.from(data);
    if (state.corrupt && name !== 'provisioning-catalog.cbor') data[0] ^= 1;
    if (state.onDownload && name !== 'provisioning-catalog.cbor') state.onDownload();
    return {ok: true, status: 200, arrayBuffer: async () => data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength)};
  };
  return state;
}
class Element {
  constructor(id, doc) { this.id=id;this.ownerDocument=doc;this.listeners={};this.children=[];this.value='';this.hidden=false;this.disabled=false;this.checked=false;this.dataset={};this._text=''; }
  addEventListener(name, f) { this.listeners[name] = f; }
  fire(name) { return this.listeners[name]?.({preventDefault(){}}); }
  appendChild(e) { this.children.push(e); if (!this.value) this.value=e.value; }
  set textContent(value) { this._text=value;this.children=[];if(this.id==='remote-profile-key')this.value=''; }
  get textContent() { return this._text; }
  removeAttribute(name) { delete this[name]; }
  reportValidity() { return this.ownerDocument.items['remote-profile-confirm'].checked; }
}
function ui(c, t) {
  const doc={items:{},html:'',createElement:()=>new Element('',doc),open(){},write(value){this.html=value;},close(){}};
  for (const id of ['remote-profile-form','remote-profile-key','remote-profile-refresh','remote-profile-status','remote-profile-image','remote-profile-submit','remote-module-fields','remote-profile-serial','remote-profile-minimum','remote-profile-confirm','remote-profile-entry','remote-profile-data'])doc.items[id]=new Element(id,doc);
  const r=new Element('remote-hardware-profiles',doc);r.dataset={de:'1',modules:'1'};r.querySelector=s=>doc.items[s.slice(1)];
  c.fetch=t.fetch;
  c.FormData=class extends Map {
    constructor(){super();this.set('remoteSerial','BROWSER');this.set('remoteSlot','A');this.set('confirmRemoteProvisioning',doc.items['remote-profile-confirm'].checked?'1':'');this.set('remoteProfileEntry',doc.items['remote-profile-entry'].value);this.set('remoteProfileData',doc.items['remote-profile-data'].value);}
  };
  const installed=c.EnvNodeHardwareProfiles.install(r);
  return {doc,r,...installed};
}
async function idle(u) { for(let i=0;i<50&&u.doc.items['remote-profile-refresh'].disabled;++i)await new Promise(resolve=>setImmediate(resolve));assert.equal(u.doc.items['remote-profile-refresh'].disabled,false); }
let tests=0;
async function test(name, run) { await run();++tests;console.log('PASS',name); }
(async()=>{
  await test('published browser bundle matches firmware SRI pin and contains MIT notice',()=>{
    const header=fs.readFileSync(path.join(root,'lib/EnvNodeWebAdmin/src/BrowserHardwareProfiles.generated.inc'),'utf8');
    const url=header.match(/BrowserHardwareProfilesUrl\[\] = "([^"]+)"/)[1];
    const sri=header.match(/BrowserHardwareProfilesIntegrity\[\] = "([^"]+)"/)[1];
    const data=fs.readFileSync(path.join(assets,path.basename(url)));
    assert.equal('sha256-'+crypto.createHash('sha256').update(data).digest('base64'),sri);
    assert.ok(url.includes(hash(data)));assert.match(data.toString(),/MIT License/);
  });
  await test('asynchronously loaded bundle starts even after DOMContentLoaded',()=>{
    for(const readyState of ['loading','complete']){
      let reads=0,callback;
      const c=context();c.document={readyState,getElementById(){++reads;return null;},addEventListener(event,fn){assert.equal(event,'DOMContentLoaded');callback=fn;}};
      vm.runInContext(fs.readFileSync(path.join(root,'lib/EnvNodeWebAdmin/src/BrowserHardwareProfiles.js'),'utf8'),c);
      if(readyState==='loading'){assert.equal(reads,0);callback();}
      assert.equal(reads,1);
    }
  });
  await test('SHA-256 works without Web Crypto on insecure HTTP pages',()=>{
    const c=context();assert.equal(c.crypto,undefined);assert.equal(c.sha256('abc'),'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');
    for(const file of fs.readdirSync(assets).filter(n=>n.endsWith('.cbor')))assert.equal(c.sha256(bytes(file)),hash(bytes(file)));
  });
  await test('all catalog entries preserve exact raw metadata for upload',()=>{
    const c=context(),entries=c.EnvNodeHardwareProfiles.decodeCatalog(bytes('provisioning-catalog.cbor'));assert.equal(entries.length,6);
    for(const e of entries){assert.equal(hash(bytes(e.filename)),e.digest);assert.ok(e.raw.length<1024);}
  });
  await test('malformed, noncanonical, duplicate and unsafe catalogs are rejected',()=>{
    const c=context(),api=c.EnvNodeHardwareProfiles,b=bytes('provisioning-catalog.cbor');
    for(let n=0;n<b.length;++n)assert.throws(()=>api.decodeCatalog(b.slice(0,n)));
    assert.throws(()=>api.decodeCatalog(Uint8Array.from([0x82,0x18,1,0x80])));
    const entries=api.decodeCatalog(b).map(fields);entries[1]=entries[0];assert.throws(()=>api.decodeCatalog(Uint8Array.from(encode([1,entries]))));
    entries.length=1;entries[0][5]='assets/../../outside.svg';assert.throws(()=>api.decodeCatalog(Uint8Array.from(encode([1,entries]))));
  });
  await test('downloads use direct HTTPS, anonymous CORS, no redirects and exact hash',async()=>{
    const c=context(),t=transport(c),client=c.EnvNodeHardwareProfiles.createClient(t.fetch);const entries=await client.refresh();
    for(const e of entries){const loaded=await client.prepare(e.key);assert.equal(hash(loaded.bytes),e.digest);}
    assert.equal(t.posts.length,0);
  });
  await test('hash mismatch never populates cache or sends an upload',async()=>{
    const c=context(),t=transport(c),client=c.EnvNodeHardwareProfiles.createClient(t.fetch);const [entry]=await client.refresh();t.corrupt=true;
    await assert.rejects(client.prepare(entry.key),/ProfileHashMismatch/);assert.equal(t.posts.length,0);t.corrupt=false;await client.prepare(entry.key);
  });
  await test('failed refresh preserves catalog and checked offline cache',async()=>{
    const c=context(),t=transport(c),client=c.EnvNodeHardwareProfiles.createClient(t.fetch);const [entry]=await client.refresh();await client.prepare(entry.key);t.offline=true;
    await assert.rejects(client.refresh(),/Offline/);assert.equal(client.entries().length,6);await client.prepare(entry.key);
  });
  await test('conflicting identity and stale selections are rejected',async()=>{
    const c=context(),t=transport(c),client=c.EnvNodeHardwareProfiles.createClient(t.fetch);const list=await client.refresh(),next=list.map(fields);next[0][1]='Conflicting';t.files.set('provisioning-catalog.cbor',Uint8Array.from(encode([1,next])));
    await assert.rejects(client.refresh(),/ProfileIdentityConflict/);await assert.rejects(client.prepare('stale'),/SelectionStale/);
  });
  await test('running tester blocks internet requests before catalog or profile download',async()=>{
    const c=context(),t=transport(c),client=c.EnvNodeHardwareProfiles.createClient(t.fetch);await client.refresh();t.locked=true;const before=t.calls.filter(x=>x.url.startsWith('https:')).length;
    await assert.rejects(client.refresh(),/ProvisioningLocked/);await assert.rejects(client.prepare(client.entries()[0].key),/ProvisioningLocked/);assert.equal(t.calls.filter(x=>x.url.startsWith('https:')).length,before);
  });
  await test('UI loads catalog, switches module fields and resets confirmation',async()=>{
    const c=context(),t=transport(c),u=ui(c,t);await idle(u);assert.equal(u.doc.items['remote-profile-key'].children.length,6);
    const module=u.client.entries().find(e=>e.kind);u.doc.items['remote-profile-key'].value=module.key;u.doc.items['remote-profile-confirm'].checked=true;u.doc.items['remote-profile-key'].fire('change');
    assert.equal(u.doc.items['remote-module-fields'].hidden,false);assert.equal(u.doc.items['remote-profile-serial'].maxLength,64);assert.equal(u.doc.items['remote-profile-confirm'].checked,false);
  });
  await test('UI does not download or upload without explicit confirmation',async()=>{
    const c=context(),t=transport(c),u=ui(c,t);await idle(u);const before=t.calls.length;await u.doc.items['remote-profile-form'].fire('submit');assert.equal(t.calls.length,before);assert.equal(t.posts.length,0);
  });
  await test('confirmed UI uploads exact checked bytes and selected metadata locally',async()=>{
    const c=context(),t=transport(c),u=ui(c,t);await idle(u);const e=u.client.entries()[0];u.doc.items['remote-profile-confirm'].checked=true;await u.doc.items['remote-profile-form'].fire('submit');
    assert.equal(t.posts.length,1);const body=t.posts[0];assert.equal(body.get('remoteProfileKey'),e.key);assert.equal(body.get('remoteProfileEntry'),Buffer.from(e.raw).toString('hex'));assert.equal(body.get('remoteProfileData'),Buffer.from(bytes(e.filename)).toString('hex'));assert.match(u.doc.html,/Provisioned/);
  });
  await test('UI keeps page usable and never uploads corrupt downloads',async()=>{
    const c=context(),t=transport(c),u=ui(c,t);await idle(u);t.corrupt=true;u.doc.items['remote-profile-confirm'].checked=true;await u.doc.items['remote-profile-form'].fire('submit');
    assert.equal(t.posts.length,0);assert.equal(u.doc.items['remote-profile-submit'].disabled,false);assert.match(u.doc.items['remote-profile-status'].textContent,/ProfileHashMismatch/);
  });
  await test('selection changes during download cannot write the wrong profile',async()=>{
    const c=context(),t=transport(c),u=ui(c,t);await idle(u);u.doc.items['remote-profile-confirm'].checked=true;t.onDownload=()=>{u.doc.items['remote-profile-key'].value='stale';};await u.doc.items['remote-profile-form'].fire('submit');assert.equal(t.posts.length,0);assert.match(u.doc.items['remote-profile-status'].textContent,/SelectionStale/);
  });
  console.log(`${tests} browser profile tests passed`);
})().catch(error=>{console.error(error);process.exitCode=1;});
