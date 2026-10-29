# EnvNode: see repository LICENSE. Third-party source retains its MIT license.
import argparse
import base64
import hashlib
import json
import html
from pathlib import Path
def generate():
    root = Path(__file__).resolve().parents[1]
    web = root/'lib/EnvNodeWebAdmin'
    license = (web/'vendor/js-sha256/LICENSE.txt').read_text()
    script = '/* js-sha256 0.11.1 — third-party MIT license:\n'+license+'*/\n'
    script += (web/'vendor/js-sha256/sha256.min.js').read_text()+'\n'
    script += (web/'src/BrowserHardwareProfiles.js').read_text()
    data = script.encode()
    digest = hashlib.sha256(data).digest()
    filename = 'browser-profile-client.'+digest.hex()+'.js'
    header = '// EnvNode: see repository LICENSE. Generated; do not edit.\n'
    header += 'static const char BrowserHardwareProfilesUrl[] = "https://docs.envnode.de/assets/hardware/v1/'+filename+'";\n'
    header += 'static const char BrowserHardwareProfilesIntegrity[] = "sha256-'+base64.b64encode(digest).decode()+'";\n'
    outputs = {web/'src/BrowserHardwareProfiles.generated.inc': header,
               root.parent/'docs/site/assets/hardware/v1'/filename: script}
    local = (web/'src/ProvisioningDialog.js').read_text()
    outputs[web/'src/ProvisioningDialog.generated.inc'] = '// EnvNode: see repository LICENSE. Generated; do not edit.\nstatic const char ProvisioningDialogScript[] = R"ENUI('+local+')ENUI";\n'
    module=json.loads((root/'hardware-profiles/modules/duo-relay-0.3.json').read_text())
    d=module['provisioning'];revision=d['identity']['hardwareRevision'];revision=str(revision['major'])+'.'+str(revision['minor'])
    escape=lambda text: html.escape(str(text),quote=True)
    option="<option value='module:"+escape(module['id'])+"' data-kind='1' data-name='"+escape(module['name'])+"' data-revision='"+revision+"' data-minimum='"+escape(d['compatibility']['minimumFirmwareVersion'])+"' data-image='"+escape(module['image'])+"'>"+escape(module['name'])+' '+revision+'</option>'
    outputs[web/'src/ProvisioningModule.generated.inc'] = '// EnvNode: see repository LICENSE. Generated; do not edit.\nstatic const char ProvisioningBuiltinModule[] = R"ENUI('+option+')ENUI";\n'
    return outputs

if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args()
    for path,content in generate().items():
        if args.check:
            if not path.exists() or path.read_text()!=content: raise SystemExit('Browser profile assets out of date: '+str(path))
        else:
            path.parent.mkdir(parents=True,exist_ok=True);path.write_text(content)
    print('Browser profile assets checked' if args.check else 'Browser profile assets generated')
