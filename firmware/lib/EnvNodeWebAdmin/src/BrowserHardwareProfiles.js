// EnvNode: see repository LICENSE.
(function (global) {
  'use strict';
  const base = 'https://docs.envnode.de/assets/hardware/v1/';
  const origin = 'https://docs.envnode.de/';
  const maxCatalog = 16384, maxProfile = 2048;
  const utf8 = new TextDecoder('utf-8', {fatal: true});
  const encoder = new TextEncoder();
  const filename = s => typeof s === 'string' && s.length > 0 && s.length <= 175 && s[0] !== '.' && !s.includes('..') && /^[a-z0-9.-]+$/.test(s);
  const boundedText = (s, max, empty = false) => typeof s === 'string' && (empty || s.length > 0) && encoder.encode(s).length <= max && !/[\x00-\x1f\x7f]/.test(s);
  const integer = (v, max) => Number.isInteger(v) && v >= 0 && v <= max;
  const hex = bytes => Array.from(bytes, b => b.toString(16).padStart(2, '0')).join('');
  class Cbor {
    constructor(bytes) { this.bytes = bytes; this.offset = 0; }
    byte() { if (this.offset >= this.bytes.length) throw Error('TruncatedCatalog'); return this.bytes[this.offset++]; }
    length(ai) {
      if (ai < 24) return ai;
      const n = ai === 24 ? 1 : ai === 25 ? 2 : ai === 26 ? 4 : 0;
      if (!n) throw Error('UnsupportedEncoding');
      let value = 0; for (let i = 0; i < n; ++i) value = value * 256 + this.byte();
      if (value < (n === 1 ? 24 : n === 2 ? 256 : 65536)) throw Error('NonCanonicalCatalog');
      return value;
    }
    array() { const head = this.byte(); if (head >> 5 !== 4) throw Error('InvalidCatalog'); return this.length(head & 31); }
    read(depth = 0) {
      if (depth > 6) throw Error('CatalogNesting');
      const head = this.byte(), kind = head >> 5, n = this.length(head & 31);
      if (kind === 0) return n;
      if (kind === 3) {
        if (n > maxCatalog || this.offset + n > this.bytes.length) throw Error('TruncatedCatalog');
        const value = utf8.decode(this.bytes.subarray(this.offset, this.offset + n)); this.offset += n; return value;
      }
      if (kind === 4 && n <= 64) return Array.from({length: n}, () => this.read(depth + 1));
      throw Error('UnsupportedCatalogValue');
    }
  }
  function decodeCatalog(bytes) {
    if (!(bytes instanceof Uint8Array) || !bytes.length || bytes.length > maxCatalog) throw Error('CatalogSize');
    const r = new Cbor(bytes);
    if (r.array() !== 2 || r.read() !== 1) throw Error('CatalogVersion');
    const count = r.array(); if (count > 16) throw Error('TooManyProfiles');
    const entries = [], ids = new Set();
    for (let i = 0; i < count; ++i) {
      const start = r.offset, data = r.read(), end = r.offset;
      if (!Array.isArray(data) || data.length !== 9 || end - start > 1024) throw Error('InvalidEntry');
      const [id, name, kind, revision, minimum, image, file, size, digest] = data;
      if (!boundedText(id, 95) || !filename(id) || ids.has(id) || !boundedText(name, 95) || !integer(kind, 1)
          || !Array.isArray(revision) || revision.length !== 2 || !revision.every(n => integer(n, 255))
          || !Array.isArray(minimum) || minimum.length !== 3 || !minimum.every(n => integer(n, 65535))
          || !boundedText(image, 223, true) || (image && (!image.startsWith('assets/') || image.includes('..') || image.endsWith('/') || !/^[a-zA-Z0-9/_.-]+$/.test(image)))
          || !filename(file) || !integer(size, maxProfile) || !size || typeof digest !== 'string' || !/^[0-9a-f]{64}$/.test(digest)
          || file !== id + '.' + digest + (kind ? '.module.cbor' : '.portmap.cbor')) throw Error('InvalidEntry');
      ids.add(id);
      entries.push(Object.freeze({id, name, kind, revision, minimum, image, filename: file, size, digest, key: id + '|' + digest, raw: bytes.slice(start, end)}));
    }
    if (r.offset !== bytes.length) throw Error('TrailingCatalogData');
    return entries;
  }
  function createClient(fetcher) {
    let entries = [];
    const held = new Map(), cache = new Map();
    async function ready() {
      const abort = new AbortController(), timer = setTimeout(() => abort.abort(), 8000);
      try {
        const response = await fetcher('/hardware/profile/state', {cache: 'no-store', signal: abort.signal});
        if (!response.ok) throw Error(response.status === 409 ? 'ProvisioningLocked' : 'DeviceUnavailable');
      } finally { clearTimeout(timer); }
    }
    async function download(url, maximum) {
      const abort = new AbortController(), timer = setTimeout(() => abort.abort(), 8000);
      try {
        const response = await fetcher(url, {mode: 'cors', credentials: 'omit', redirect: 'error', cache: 'no-store', signal: abort.signal});
        if (!response.ok) throw Error('HTTP ' + response.status);
        const bytes = new Uint8Array(await response.arrayBuffer());
        if (!bytes.length || bytes.length > maximum) throw Error('DownloadSize');
        return bytes;
      } finally { clearTimeout(timer); }
    }
    return {
      entries: () => entries.slice(),
      async refresh() {
        await ready();
        const next = decodeCatalog(await download(base + 'provisioning-catalog.cbor', maxCatalog));
        for (const e of next) if (held.has(e.id) && held.get(e.id) !== hex(e.raw)) throw Error('ProfileIdentityConflict');
        for (const e of next) held.set(e.id, hex(e.raw));
        entries = next; return entries.slice();
      },
      async prepare(key) {
        await ready();
        const entry = entries.find(e => e.key === key);
        if (!entry) throw Error('SelectionStale');
        if (!cache.has(key)) {
          const bytes = await download(base + entry.filename, entry.size);
          if (bytes.length !== entry.size || global.sha256(bytes) !== entry.digest) throw Error('ProfileHashMismatch');
          cache.set(key, bytes);
        }
        return {entry, bytes: cache.get(key)};
      }
    };
  }
  function install(root) {
    const de = root.dataset.de === '1', client = createClient(global.fetch.bind(global));
    const byId = id => root.querySelector('#' + id);
    const form = byId('remote-profile-form'), select = byId('remote-profile-key'), refresh = byId('remote-profile-refresh');
    const status = byId('remote-profile-status'), image = byId('remote-profile-image'), submit = byId('remote-profile-submit');
    const report = message => { status.textContent = message; };
    root.dataset.remoteReady = '1';
    let busy = false;
    function working(value) { busy = value; refresh.disabled = value; select.disabled = value; submit.disabled = value || !!(root.provisioning && !root.provisioning.canWrite()); }
    function selected() {
      if (root.provisioning) return;
      const e = client.entries().find(e => e.key === select.value);
      if (!e) return;
      byId('remote-module-fields').hidden = e.kind !== 1;
      byId('remote-profile-serial').maxLength = e.kind ? 64 : 17;
      byId('remote-profile-minimum').textContent = (de ? 'Betriebs-Firmware ab ' : 'Runtime firmware ≥ ') + e.minimum.join('.');
      byId('remote-profile-confirm').checked = false;
      image.hidden = true; image.onload = () => { image.hidden = false; }; image.onerror = () => { image.hidden = true; };
      if (e.image) { image.alt = e.name; image.src = origin + e.image; } else image.removeAttribute('src');
    }
    async function update() {
      if (busy) return;
      working(true); report(de ? 'Katalog wird im Browser geladen …' : 'Loading catalog in browser …');
      try {
        const entries = (await client.refresh()).filter(e => root.dataset.modules === '1' || !e.kind);
        if (root.provisioning) { root.provisioning.online(entries); report(de ? 'Katalog bereit.' : 'Catalog ready.'); return; }
        const previous = select.value; select.textContent = '';
        for (const e of entries) { const option = root.ownerDocument.createElement('option'); option.value = e.key; option.textContent = e.name + ' ' + e.revision.join('.'); select.appendChild(option); }
        if (entries.some(e => e.key === previous)) select.value = previous;
        form.hidden = !entries.length; selected();
        report(entries.length ? (de ? 'Katalog bereit.' : 'Catalog ready.') : (de ? 'Keine Profile verfügbar.' : 'No profiles available.'));
      } catch (error) { report((de ? 'Katalog nicht geladen. Eingebaute Profile bleiben verfügbar: ' : 'Catalog unavailable. Built-in profiles remain available: ') + error.message); }
      finally { working(false); }
    }
    select.addEventListener('change', selected); refresh.addEventListener('click', update);
    form.addEventListener('submit', async event => {
      if (root.provisioning && !client.entries().some(e => e.key === select.value)) return;
      event.preventDefault();
      if (busy || !form.reportValidity()) return;
      const key = select.value;
      working(true); report(de ? 'Profil wird im Browser geprüft …' : 'Checking profile in browser …');
      if(root.provisioning)root.provisioning.report(de ? 'Profil und EEPROM werden geprüft …' : 'Checking profile and EEPROM …');
      let posting=false;
      try {
        const {entry, bytes} = await client.prepare(key);
        if (select.value !== key) throw Error('SelectionStale');
        if (!form.reportValidity()) throw Error('ConfirmationRequired');
        byId('remote-profile-entry').value = hex(entry.raw);
        byId('remote-profile-data').value = hex(bytes);
        // Disabled controls are omitted by FormData; explicitly bind the selection.
        const body = new FormData(form); body.set('remoteProfileKey', key);
        posting=true;
        const response = await global.fetch('/hardware/profile/write', {method: 'POST', body});
        if (response.status === 409 && root.provisioning) { await root.provisioning.changed(); working(false); return; }
        const html = await response.text();
        root.ownerDocument.open(); root.ownerDocument.write(html); root.ownerDocument.close();
      } catch (error) { const message=(posting ? (de?'Schreibergebnis unklar. EEPROM erneut lesen: ':'Write result unknown. Read EEPROM again: ') : (de?'Nicht provisioniert: ':'Not provisioned: '))+error.message;report(message);if(root.provisioning){if(posting)root.provisioning.invalidate();root.provisioning.report(message);}working(false); }
    });
    update();
    return {client, update};
  }
  global.EnvNodeHardwareProfiles = {decodeCatalog, createClient, hex, install};
  if (typeof document !== 'undefined') {
    const start = () => { const root = document.getElementById('remote-hardware-profiles'); if (root) install(root); };
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start);
    else start();
  }
})(typeof window !== 'undefined' ? window : globalThis);
