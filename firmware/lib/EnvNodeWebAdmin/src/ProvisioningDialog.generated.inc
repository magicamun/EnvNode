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
    let online = [];
    const signature = e => [e.kind, e.name, e.revision.join('.')].join('|');
    const entries = () => {
      const remote = new Set(online.map(signature));
      return local.filter(e => !remote.has(signature(e))).concat(online);
    };
    function manufacturingChanged() {
      confirmation.checked = false;
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
      submit.disabled = !entry;
      submit.textContent = kind.value === '1' ? (de ? 'Modul provisionieren' : 'Provision module') : (de ? 'Board provisionieren' : 'Provision board');
      find('remote-profile-minimum').textContent = ''; manufacturingChanged();
      image.hidden = true; image.onload = () => { image.hidden = false; }; image.onerror = () => { image.hidden = true; };
      if (entry && entry.image) { image.alt = entry.name + ' ' + entry.revision.join('.'); image.src = entry.local ? entry.image : 'https://docs.envnode.de/' + entry.image; }
      else image.removeAttribute('src');
      find('provision-board-key').value = entry && entry.local && !entry.kind ? entry.key.slice(6) : '';
      form.action = entry && entry.local ? (entry.kind ? '/hardware/module-descriptor/write' : '/hardware/board-identity/write') : '/hardware/profile/write';
    }
    function render(previous) {
      if (!previous || !previous.key) previous = entries().find(e => e.key === select.value);
      const key = previous && signature(previous);
      select.textContent = '';
      const placeholder = document.createElement('option'); placeholder.value = ''; placeholder.textContent = de ? 'Profil auswählen' : 'Select profile'; select.appendChild(placeholder);
      for (const entry of entries().filter(e => e.kind === Number(kind.value))) {
        const option = document.createElement('option'); option.value = entry.key;
        option.textContent = entry.name + ' ' + entry.revision.join('.') + ' · ' + (entry.local ? (de ? 'eingebaut' : 'built-in') : 'online');
        select.appendChild(option); if (signature(entry) === key) select.value = entry.key;
      }
      selected();
    }
    find('remote-profile-slot').addEventListener('change', () => { confirmation.checked = false; });
    select.addEventListener('change', selected); kind.addEventListener('change', render);
    form.addEventListener('submit', event => {
      const entry = entries().find(e => e.key === select.value);
      if (!entry || !form.reportValidity()) { event.preventDefault(); return; }
      if (!entry.local && !root.dataset.remoteReady) {
        event.preventDefault(); find('remote-profile-status').textContent = de ? 'Online-Import noch nicht bereit.' : 'Online import is not ready.';
      }
    });
    root.provisioning = {online(values) { const previous = entries().find(e => e.key === select.value); online = values; render(previous); }};
    render();
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', install); else install();
})();
)ENUI";
