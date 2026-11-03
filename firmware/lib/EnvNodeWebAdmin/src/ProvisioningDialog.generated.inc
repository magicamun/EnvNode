// EnvNode: see repository LICENSE. Generated; do not edit.
static const char ProvisioningDialogScript[] = R"ENUI(// EnvNode: see repository LICENSE. Offline controller shared by tester and EnvNode.
(function () {
  function install() {
    const root = document.getElementById('remote-hardware-profiles');
    if (!root) return;
    const find = id => root.querySelector('#' + id), de = root.dataset.de === '1';
    const form = find('remote-profile-form'), select = find('remote-profile-key');
    const kind = find('provision-hardware-kind'), image = find('remote-profile-image');
    const confirmation = find('remote-profile-confirm'), submit = find('remote-profile-submit');
    const local = Array.from(select.options).filter(o => o.value).map(o => ({
      key: o.value, name: o.dataset.name, revision: o.dataset.revision.split('.').map(Number),
      kind: Number(o.dataset.kind), minimum: o.dataset.minimum.split('.').map(Number),
      image: o.dataset.image, local: true
    }));
    let online = [], snapshot = null, snapshotTarget = '', readGeneration = 0, reading = false;
    const token = find('provision-snapshot-token'), readButton = find('provision-read'), adopt = find('provision-adopt');
    const currentStatus = find('provision-current-status'), currentFields = find('provision-current-fields');
    const target = () => kind.value === '1' ? 'module&slot=' + find('remote-profile-slot').value : 'board';
    const canWrite = () => !!select.value && !reading && !!token.value;
    function comparison() {
      const entry = entries().find(e => e.key === select.value);
      const output = find('provision-comparison');
      if (!entry || !snapshot || reading) { output.textContent = ''; return; }
      const empty = de ? '(leer)' : '(empty)';
      const oldName = snapshot.status === 'Valid' ? snapshot.name + ' ' + snapshot.revision : (de ? 'EEPROM ' : 'EEPROM: ') + currentStatus.textContent;
      const changes = [];
      const nextName = entry.name + ' ' + entry.revision.join('.');
      if (oldName !== nextName) changes.push(oldName + ' → ' + nextName);
      for (const [key, id, label] of [['serial','remote-profile-serial',de?'Seriennummer':'Serial'],['batch','remote-profile-batch',de?'Charge':'Batch'],['date','remote-profile-date',de?'Datum':'Date']]) {
        const before = snapshot[key] || '', after = find(id).value;
        if (before !== after) changes.push(label + ': ' + (before || empty) + ' → ' + (after || empty));
      }
      output.textContent = changes.length ? (de ? 'Geplante Änderungen: ' : 'Planned changes: ') + changes.join(' · ') : (de ? 'Profil und Fertigungsdaten unverändert. Das EEPROM wird neu provisioniert.' : 'Profile and manufacturing data unchanged. EEPROM will be provisioned again.');
    }
    async function readSnapshot(force = false) {
      const requested = target();
      if (!force && snapshotTarget === requested && snapshot) return;
      const generation = ++readGeneration; snapshotTarget = requested;
      snapshot = null; token.value = ''; reading = true; confirmation.checked = false; submit.disabled = true; adopt.disabled = true;
      currentFields.textContent = ''; currentStatus.textContent = de ? 'EEPROM wird gelesen …' : 'Reading EEPROM …';
      find('provision-current-target').textContent = kind.value === '1' ? (de ? 'Modul-Slot ' : 'Module slot ') + find('remote-profile-slot').value : 'Board · 0x50';
      comparison(); find('provision-result').textContent = ''; readButton.disabled = true;
      const abort = new AbortController(), timer = setTimeout(() => abort.abort(), 8000);
      try {
        const response = await fetch('/hardware/eeprom?kind=' + requested, {cache:'no-store', signal:abort.signal});
        if (!response.ok) throw Error(response.status === 409 ? 'ProvisioningLocked' : 'ReadFailed');
        const result = await response.json();
        if (generation !== readGeneration || requested !== target()) return;
        const statuses = de ? {Valid:'Gültig',Empty:'Leer / nicht provisioniert',Corrupt:'Beschädigt oder nicht lesbar als gültiger Datensatz',Unavailable:'Nicht erreichbar'} : {Valid:'Valid',Empty:'Empty / not provisioned',Corrupt:'Corrupt or invalid record',Unavailable:'Unavailable'};
        if (!Object.prototype.hasOwnProperty.call(statuses,result.status)) throw Error('InvalidSnapshot');
        snapshot = result;
        currentStatus.textContent = statuses[result.status];
        token.value = result.status !== 'Unavailable' ? result.token : '';
        if (result.status === 'Valid') {
          for (const [label,value] of [[de?'Hardware':'Hardware',result.name + ' ' + result.revision],[de?'Seriennummer':'Serial number',result.serial],[de?'Produktionscharge':'Production batch',result.batch],[de?'Fertigungsdatum':'Production date',result.date]]) {
            const dt = document.createElement('dt'), dd = document.createElement('dd');dt.textContent=label;dd.textContent=value || '—';currentFields.appendChild(dt);currentFields.appendChild(dd);
          }
          adopt.disabled = false;
        }
      } catch (error) {
        if (generation !== readGeneration) return;
        snapshot = null; token.value = ''; currentStatus.textContent = (de ? 'Lesen fehlgeschlagen: ' : 'Read failed: ') + error.message;
      } finally {
        clearTimeout(timer);
        if (generation === readGeneration) { reading = false; readButton.disabled = false; submit.disabled = !canWrite(); comparison(); }
      }
    }
    async function changed() {
      await readSnapshot(true); confirmation.checked = false;
      find('provision-result').textContent = de ? 'EEPROM seit der Anzeige verändert oder nicht erreichbar. Nichts geschrieben. Aktuellen Inhalt prüfen und neu bestätigen.' : 'EEPROM changed or unavailable. Nothing written. Review current content and confirm again.';
    }
    readButton.addEventListener('click', () => readSnapshot(true));
    adopt.addEventListener('click', () => {
      if (!snapshot || snapshot.status !== 'Valid' || snapshotTarget !== target()) return;
      find('remote-profile-serial').value = snapshot.serial || '';find('remote-profile-batch').value = snapshot.batch || '';find('remote-profile-date').value = snapshot.date || '';manufacturingChanged();
    });
    const signature = e => [e.kind, e.name, e.revision.join('.')].join('|');
    const entries = () => {
      const remote = new Set(online.map(signature));
      return local.filter(e => !remote.has(signature(e))).concat(online);
    };
    function manufacturingChanged() {
      confirmation.checked = false; comparison();
      const entry = entries().find(e => e.key === select.value);
      if (!entry) return;
      let minimum = entry.minimum;
      if (!entry.kind && (find('remote-profile-batch').value || find('remote-profile-date').value)
          && minimum[0] === 0 && (minimum[1] < 8 || (minimum[1] === 8 && minimum[2] < 5))) minimum = [0,8,5];
      find('remote-profile-minimum').textContent = entry.name + ' ' + entry.revision.join('.') + ' · ' + (de ? 'Betriebs-Firmware ab ' : 'Runtime firmware ≥ ') + minimum.join('.');
    }
    for (const id of ['remote-profile-serial','remote-profile-batch','remote-profile-date']) find(id).addEventListener('input', manufacturingChanged);
    function selected() {
      const entry = entries().find(e => e.key === select.value);
      confirmation.checked = false;
      find('provision-details').hidden = !entry;
      find('remote-profile-entry').value = ''; find('remote-profile-data').value = '';
      find('remote-module-fields').hidden = kind.value !== '1';
      find('remote-profile-serial').maxLength = kind.value === '1' ? 64 : 17;
      find('remote-profile-serial').pattern = kind.value === '1' ? '.*' : '[A-Za-z0-9._-]*';
      submit.disabled = !entry || !canWrite();
      submit.textContent = kind.value === '1' ? (de ? 'Modul provisionieren' : 'Provision module') : (de ? 'Board provisionieren' : 'Provision board');
      find('remote-profile-minimum').textContent = ''; manufacturingChanged();
      image.hidden = true; image.onload = () => { image.hidden = false; }; image.onerror = () => { image.hidden = true; };
      if (entry && entry.image) { image.alt = entry.name + ' ' + entry.revision.join('.'); image.src = entry.local ? entry.image : 'https://docs.envnode.de/' + entry.image; }
      else image.removeAttribute('src');
      find('provision-board-key').value = entry && entry.local && !entry.kind ? entry.key.slice(6) : '';
      form.action = entry && entry.local ? (entry.kind ? '/hardware/module-descriptor/write' : '/hardware/board-identity/write') : '/hardware/profile/write';
      readSnapshot(); comparison();
    }
    function render(previous) {
      if (!previous || !previous.key) previous = entries().find(e => e.key === select.value);
      const key = previous && signature(previous);
      select.textContent = '';
      const placeholder = document.createElement('option'); placeholder.value = ''; placeholder.textContent = de ? 'Profil auswählen' : 'Select profile'; select.appendChild(placeholder);
      const profiles = entries().filter(e => e.kind === Number(kind.value));
      profiles.sort((a,b) => (a.name + ' ' + a.revision.join('.')).localeCompare(b.name + ' ' + b.revision.join('.'), de ? 'de' : 'en', {numeric:true,sensitivity:'base'}));
      for (const entry of profiles) {
        const option = document.createElement('option'); option.value = entry.key;
        option.textContent = entry.name + ' ' + entry.revision.join('.') + ' · ' + (entry.local ? (de ? 'eingebaut' : 'built-in') : 'online');
        select.appendChild(option); if (signature(entry) === key) select.value = entry.key;
      }
      selected();
    }
    find('remote-profile-slot').addEventListener('change', () => { confirmation.checked = false; readSnapshot(true); });
    select.addEventListener('change', selected); kind.addEventListener('change', render);
    form.addEventListener('submit', async event => {
      const entry = entries().find(e => e.key === select.value);
      if (!entry || !canWrite() || !form.reportValidity()) { event.preventDefault(); confirmation.checked=false; return; }
      if (!entry.local) {
        if (!root.dataset.remoteReady) { event.preventDefault(); confirmation.checked=false; find('remote-profile-status').textContent = de ? 'Online-Import noch nicht bereit.' : 'Online import is not ready.'; }
        return;
      }
      event.preventDefault(); submit.disabled = true; find('provision-result').textContent = de ? 'EEPROM wird geprüft …' : 'Checking EEPROM …';
      try {
        const response = await fetch(form.action,{method:'POST',body:new FormData(form)});
        if (response.status === 409) { await changed(); return; }
        const html = await response.text();document.open();document.write(html);document.close();
      } catch (error) { confirmation.checked=false;find('provision-result').textContent=(de?'Schreibergebnis unklar. EEPROM erneut lesen: ':'Write result unknown. Read EEPROM again: ')+error.message;token.value=''; }
      finally { submit.disabled = !canWrite(); }
    });
    root.provisioning = {canWrite, changed, invalidate() { token.value='';confirmation.checked=false;submit.disabled=true; }, report(message) { find('provision-result').textContent = message; }, online(values) { const previous = entries().find(e => e.key === select.value); online = values; render(previous); }};
    render();
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', install); else install();
})();
)ENUI";
