// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See LICENSE. Browser editor logic; server-side validation remains authoritative.
#pragma once
namespace EnvNode {
constexpr const char PageEditorScript[] = R"EDITOR(
(function () {
if (!document.getElementById('page-formats')) return;
const previewOutput = document.getElementById('preview-output');
const refreshPreview = () => {
    const id = previewOutput.value === 'full' ? 'page-rendered' : 'page-rendered-' + previewOutput.value;
    const rendered = document.getElementById(id);
    document.getElementById('page-result').value = rendered ? rendered.textContent : '';
};
previewOutput.addEventListener('change', refreshPreview);
refreshPreview();
(function () {
    const options = document.getElementById('property-sources').content.querySelector('select').options;
    document.querySelectorAll('[data-property-source]').forEach(source => {
        const selected = source.value;
        const parts = selected.split('/');
        if (parts.length === 3 && /^[0-9]+$/.test(parts[1])) parts[1] = String(Number(parts[1]));
        const canonical = parts.join('/');
        source.replaceChildren(new Option('No source', ''));
        Array.from(options).forEach(option => source.appendChild(option.cloneNode(true)));
        const available = Array.from(source.options).some(option => option.value === canonical);
        if (!available) source.add(new Option('Unavailable: ' + selected, selected));
        source.value = available ? canonical : selected;
    });
    const types = new Map(Array.from(options, option => [option.value, option.dataset.boolean === '1']));
    document.querySelectorAll('[data-boolean-source]').forEach(panel => {
        const source = document.getElementById(panel.dataset.booleanSource);
        const update = () => {
            const parts = source.value.split('/');
            if (parts.length === 3 && /^[0-9]+$/.test(parts[1])) parts[1] = String(Number(parts[1]));
            panel.hidden = types.get(parts.join('/')) !== true;
        };
        source.addEventListener('input', update);
        source.addEventListener('change', update);
    });
    const states = new Map(Array.from(options, option => [option.value, JSON.parse(option.dataset.enum || '[]')]));
    document.querySelectorAll('[data-enum-source]').forEach(panel => {
        const source = document.getElementById(panel.dataset.enumSource);
        const fields = panel.querySelector('[data-enum-fields]');
        const values = new Map();
        const update = () => {
            fields.querySelectorAll('input').forEach(input => values.set(input.dataset.code, input.value));
            const parts = source.value.split('/');
            if (parts.length === 3 && /^[0-9]+$/.test(parts[1])) parts[1] = String(Number(parts[1]));
            const choices = states.get(parts.join('/')) || [];
            panel.hidden = choices.length === 0;
            if (!choices.length) return;
            fields.replaceChildren();
            choices.forEach(([code, text]) => {
                const label = document.createElement('label'); label.textContent = text;
                const input = document.createElement('input');
                input.name = 'e' + source.id.slice(1) + '_' + code;
                input.dataset.code = code; input.maxLength = 16; input.value = values.get(code) || '';
                label.appendChild(input); fields.appendChild(label);
            });
        };
        source.addEventListener('input', update);
        source.addEventListener('change', update);
    });
})();
})();
const DisplayPageEditor = (() => {
    function placeholders(text) {
        const result = [];
        if (unescape(encodeURIComponent(text)).length > 128) throw Error('Maximum 128 UTF-8 bytes per line.');
        for (let i = 0; i < text.length; ++i) {
            if (text.charCodeAt(i) < 32 || text.charCodeAt(i) === 127) throw Error('Control characters are not allowed.');
            if (text[i] !== '%') continue;
            if (text[i + 1] === '%') { ++i; continue; }
            const match = /^%(-0?|0-?)?([1-9]\d*)?(?:\.(\d+))?([fus])/.exec(text.slice(i));
            if (!match || Number(match[2]) > 64 || (match[3] !== undefined && (match[4] !== 'f' || Number(match[3]) > 6)) || (match[4] === 's' && (match[1] || '').includes('0')))
                throw Error('Invalid placeholder. Use %f, %u, %s or %%.');
            result.push({text: match[0], type: match[4]}); i += match[0].length - 1;
        }
        if (result.length > 4) throw Error('Maximum four sources per line.');
        return result;
    }
    function matchLines(previous, next) {
        const used = new Set(), mapping = Array(next.length).fill(-1);
        // Unique unchanged lines carry their controls even when moved.
        next.forEach((line, i) => {
            if (!line || next.filter(x => x === line).length !== 1) return;
            const matches = previous.map((x, j) => x === line ? j : -1).filter(j => j >= 0);
            if (matches.length === 1) { mapping[i] = matches[0]; used.add(matches[0]); }
        });
        next.forEach((line, i) => {
            if (mapping[i] < 0 && !used.has(i) && previous[i] === line) { mapping[i] = i; used.add(i); }
        });
        next.forEach((line, i) => {
            if (mapping[i] < 0 && !used.has(i)) { mapping[i] = i; used.add(i); }
        });
        next.forEach((line, i) => {
            if (mapping[i] >= 0) return;
            const old = previous.findIndex((_, j) => !used.has(j));
            mapping[i] = old; used.add(old);
        });
        return mapping;
    }
    return {placeholders, matchLines};
})();
(function () {
    const editor = document.getElementById('page-formats');
    if (!editor) return;
    const form = editor.form, preview = document.getElementById('page-result');
    const note = document.getElementById('page-editor-note');
    const groups = document.getElementById('page-sources');
    let rows = Array.from(groups.querySelectorAll('[data-page-row]'));
    let previous = rows.map(row => row.querySelector('[data-format]').value);
    const initialLines = [...previous];
    while (initialLines.length > 1 && !initialLines[initialLines.length - 1]) initialLines.pop();
    editor.value = initialLines.join('\n');
    const sourceOptions = Array.from(document.getElementById('property-sources').content.querySelector('select').options);
    const signature = text => { try { return DisplayPageEditor.placeholders(text).map(p => p.text).join('|'); } catch (_) { return '!'; } };
    function renumber(row, index) {
        row.querySelector('legend').textContent = 'Line ' + (index + 1);
        row.querySelectorAll('[name], [id], [for], [data-boolean-source], [data-enum-source]').forEach(el => {
            ['name', 'id', 'for', 'data-boolean-source', 'data-enum-source'].forEach(attr => {
                const value = el.getAttribute(attr);
                if (value) el.setAttribute(attr, value.replace(/^(f|s|true|false|e)\d+(?=_|$)/, (_, prefix) => prefix + index));
            });
        });
    }
    function update(initial = false) {
        const raw = editor.value.replace(/\r\n?/g, '\n').split('\n');
        if (raw.length > 6) { editor.setCustomValidity('Maximum six lines, including blank lines.'); note.textContent = editor.validationMessage; return; }
        editor.setCustomValidity('');
        const next = [...raw]; while (next.length < 6) next.push('');
        const mapping = DisplayPageEditor.matchLines(previous, next);
        const oldRows = rows;
        rows = mapping.map(i => oldRows[i]);
        let error = '';
        rows.forEach((row, i) => {
            const oldIndex = mapping[i], old = previous[oldIndex];
            const hasSources = Array.from(row.querySelectorAll('[data-property-source]')).some(s => s.value);
            const ambiguous = old !== next[i] && (oldIndex !== i || signature(old) !== signature(next[i]));
            if (!initial && hasSources && ambiguous) row.dataset.review = '1';
            // Repeated identical format lines cannot be distinguished after a structural edit.
            if (!initial && hasSources && previous.filter(x => x === old).length > 1 && previous.join('\n') !== next.join('\n') && mapping.some((n,j) => n !== j)) row.dataset.review = '1';
            renumber(row, i); groups.appendChild(row);
            row.querySelector('[data-format]').value = next[i];
            let tokens = [];
            try { tokens = DisplayPageEditor.placeholders(next[i]); }
            catch (e) { error = 'Line ' + (i + 1) + ': ' + e.message; }
            row.dataset.count = tokens.length;
            row.querySelectorAll('[data-source-slot]').forEach((field, j) => {
                const select = field.querySelector('select');
                field.hidden = j >= tokens.length;
                if (field.hidden) return;
                field.querySelector('label').textContent = 'Source ' + (j + 1) + ' · ' + tokens[j].text;
                const selected = select.value;
                select.replaceChildren(new Option('Select a source', ''));
                sourceOptions.filter(o => o.dataset.format === tokens[j].type).forEach(o => select.appendChild(o.cloneNode(true)));
                if (selected && !Array.from(select.options).some(o => o.value === selected)) {
                    select.add(new Option('Please review: ' + selected, selected));
                    row.dataset.review = '1';
                }
                select.value = selected;
            });
            const warning = row.querySelector('[data-review]');
            warning.hidden = row.dataset.review !== '1';
            row.hidden = tokens.length === 0 && warning.hidden;
        });
        previous = next;
        editor.setCustomValidity(error);
        note.textContent = error || (initial ? 'Use Preview / refresh to update the preview.' : 'Preview is out of date. Please refresh.');
        if (!initial) preview.dataset.stale = '1';
    }
    rows.forEach(row => {
        const warning = document.createElement('div'); warning.dataset.review = ''; warning.hidden = true;
        warning.className = 'help'; warning.textContent = 'Format changed: review sources and translations. Unused sources will be removed when saving. ';
        const button = document.createElement('button'); button.type = 'button'; button.textContent = 'Assignments reviewed';
        button.addEventListener('click', () => { delete row.dataset.review; update(true); });
        warning.appendChild(button); row.appendChild(warning);
    });
    editor.addEventListener('input', () => update());
    form.addEventListener('input', e => { if (e.target !== editor && e.target.id !== 'preview-output') note.textContent = 'Preview is out of date. Please refresh.'; });
    form.addEventListener('submit', e => {
        update(true);
        const pending = rows.some(r => r.dataset.review === '1');
        const missing = rows.some(r => Array.from(r.querySelectorAll('[data-source-slot]')).some(f => !f.hidden && !f.querySelector('select').value));
        if (!editor.checkValidity() || pending || missing) {
            e.preventDefault(); note.textContent = editor.validationMessage || (pending ? 'Please review the marked assignments.' : 'Bitte für jeden Platzhalter eine Select a source.'); editor.reportValidity();
        }
    });
    form.addEventListener('formdata', e => {
        // Keep the existing bounded server protocol; dormant controls never leak into a saved page.
        rows.forEach((row, i) => {
            const count = Number(row.dataset.count);
            for (let j = count; j < 4; ++j) {
                e.formData.set('s' + i + '_' + j, '');
                e.formData.set('true' + i + '_' + j, ''); e.formData.set('false' + i + '_' + j, '');
                for (const key of Array.from(e.formData.keys())) if (key.startsWith('e' + i + '_' + j + '_')) e.formData.delete(key);
            }
        });
    });
    update(true);
    editor.disabled = false;
    form.querySelectorAll('[data-editor-submit]').forEach(button => button.disabled = false);
})();

)EDITOR";
}
