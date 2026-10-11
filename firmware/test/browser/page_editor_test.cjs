// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See LICENSE. Run: node firmware/test/browser/page_editor_test.cjs
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../../include/PageEditorScript.h'), 'utf8').split('R"EDITOR(')[1].split(')EDITOR"')[0];
const catalog = fs.readFileSync(path.join(__dirname, '../../include/WebTextCatalog.inc'), 'utf8');
const entries = Array.from(catalog.matchAll(/ENVNODE_WEB_TEXT\((\w+), ("(?:\\.|[^"\\])*"), ("(?:\\.|[^"\\])*"|nullptr)\)/g));
const ids = new Set(entries.map(([, id]) => id));
const exportedSource = fs.readFileSync(path.join(__dirname, '../../src/WebLocalization.cpp'), 'utf8');
const exportedIds = new Set(Array.from(exportedSource.matchAll(/\{"(\w+)", TextId::\w+\}/g), m => m[1]));
for (const relative of ['../../include/PageEditorScript.h', '../../src/LogWebView.cpp', '../../src/SelectorWebView.cpp', '../../src/WebService.cpp']) {
    const text = fs.readFileSync(path.join(__dirname, relative), 'utf8');
    for (const [, id] of text.matchAll(/EnvNodeUi\.(\w+)/g)) {
        assert(ids.has(id), `Unknown browser text ID: ${id}`);
        assert(exportedIds.has(id), `Browser text ID is not exported: ${id}`);
    }
}
for (const language of ['en', 'de']) {
const EnvNodeUi = Object.fromEntries(entries.map(([, id, english, german]) => [id,
    language === 'de' && german !== 'nullptr' ? JSON.parse(german) : JSON.parse(english)]));
const context = vm.createContext({document: {getElementById: () => null}, EnvNodeUi});
vm.runInContext(source, context);
const editor = vm.runInContext('DisplayPageEditor', context);
const plain = value => JSON.parse(JSON.stringify(value));
assert.deepEqual(plain(editor.placeholders('Level: %.0f l %.1f%% / %s / %03u')).map(p => p.type), ['f','f','s','u']);
assert.equal(editor.placeholders('100%% literal').length, 0);
for (const invalid of ['%','%d','%n','%00f','%--f','%0s','%.2s','%.f','%65f','%.7f','%f\t','%f %f %f %f %f','ä'.repeat(65)]) {
    assert.throws(() => editor.placeholders(invalid), undefined, invalid);
}
for (const valid of ['%-08.2f','%0-8.2f','%64s','%.0006f','ä'.repeat(64)]) editor.placeholders(valid);
const old = ['A %f','B %s','C %u','','',''];
assert.deepEqual(plain(editor.matchLines(old, ['','A %f','B %s','C %u','',''])).slice(1,4), [0,1,2]);
assert.deepEqual(plain(editor.matchLines(old, ['C %u','A %f','B %s','','',''])).slice(0,3), [2,0,1]);
assert.deepEqual(plain(editor.matchLines(old, ['A %.1f','B %s','C %u','','',''])), [0,1,2,3,4,5]);
const duplicateMapping = plain(editor.matchLines(['%s','%s','x','','',''], ['x','%s','%s','','','']));
assert.equal(new Set(duplicateMapping).size, 6);
assert.equal(duplicateMapping[0], 2);
assert.throws(() => editor.placeholders('%d'), error => error.message === EnvNodeUi.InvalidPlaceholderUseFUSOr);
console.log(`Page editor: placeholder validation and line mapping passed (${language}).`);
}
