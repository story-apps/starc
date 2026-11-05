const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const script = fs.readFileSync(path.join(__dirname, '../../web/open/open.js'), 'utf8');

function page(search, hash = '') {
  const events = {};
  const timers = new Map();
  const status = { textContent: '' };
  const button = { hidden: true, addEventListener: (type, fn) => events[type] = fn };
  const location = { search, hash, href: 'https://starc.app/open/' + search + hash,
    replace: value => location.redirect = value };
  const document = { hidden: false, getElementById: id => id === 'status' ? status : button,
    addEventListener: (type, fn) => events[type] = fn };
  const window = { location, addEventListener: (type, fn) => events[type] = fn,
    setTimeout: fn => { const id = Symbol(); timers.set(id, fn); return id; },
    clearTimeout: id => timers.delete(id) };
  vm.runInNewContext(script, { window, document, URL, URLSearchParams, Set });
  return { location, status, button, events, document,
    expire: () => { for (const fn of [...timers.values()]) fn(); } };
}

test('forwards team, project and document without changing their values', () => {
  const p = page('?teamId=12&projectId=34&documentGuid=7a46aa60-9fc3-4d43-a883-b3131f25a4d3');
  const url = new URL(p.location.href);
  assert.equal(url.protocol, 'starc:');
  assert.equal(url.searchParams.get('teamId'), '12');
  assert.equal(url.searchParams.get('projectId'), '34');
  assert.equal(url.searchParams.get('documentGuid'), '7a46aa60-9fc3-4d43-a883-b3131f25a4d3');
  assert.equal(p.button.hidden, false);
});
test('no handler leads to the exact downloads URL', () => {
  const p = page('?projectId=34'); p.expire();
  assert.equal(p.location.redirect, 'https://starc.app/downloads/');
});
for (const event of ['blur', 'pagehide', 'visibilitychange']) {
  test(`${event} cancels fallback while the app or OS prompt is active`, () => {
    const p = page('?teamId=12');
    if (event === 'visibilitychange') p.document.hidden = true;
    p.events[event](); p.expire();
    assert.equal(p.location.redirect, undefined);
  });
}
test('retry after a canceled prompt starts a new fallback', () => {
  const p = page('?projectId=34'); p.events.blur();
  let prevented = false;
  p.events.click({ preventDefault: () => prevented = true }); p.expire();
  assert.equal(prevented, true);
  assert.equal(p.location.redirect, 'https://starc.app/downloads/');
});
for (const query of [
  '?projectId=0', '?teamId=-1', '?projectId=2147483648', '?projectId=01',
  '?projectId=1&projectId=2', '?projectId=', '?redirect=https://evil.example',
  '?documentGuid=7a46aa60-9fc3-4d43-a883-b3131f25a4d3',
  '?projectId=1&documentGuid=00000000-0000-0000-0000-000000000000',
  '?projectId=1&documentGuid=bad', '?projectId=%ZZ', '?projectId=1%0A',
]) {
  test(`rejects ${query}`, () => {
    const p = page(query); p.expire();
    assert.equal(p.button.hidden, true);
    assert.equal(p.location.redirect, undefined);
    assert.match(p.status.textContent, /Некорректная/);
  });
}
test('rejects URL fragments and oversized input', () => {
  assert.equal(page('?projectId=1', '#x').button.hidden, true);
  assert.equal(page('?projectId=1', '#').button.hidden, true);
  assert.equal(page('?projectId=' + '1'.repeat(5000)).button.hidden, true);
});
