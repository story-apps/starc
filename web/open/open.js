/* Keep the URL contract in sync with src/include/deep_link.h. */
(() => {
  const downloads = 'https://starc.app/downloads/';
  const status = document.getElementById('status');
  const button = document.getElementById('open-app');
  const params = new URLSearchParams(window.location.search);
  const native = new URL('starc://open');
  const seen = new Set();
  const id = /^[1-9][0-9]*$/;
  const guid = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;
  let valid = window.location.href.length <= 4096 && !window.location.href.includes('#');
  for (const [key, value] of params) {
    if (seen.has(key)) valid = false;
    valid = valid && !/\s/.test(value);
    seen.add(key);
    if (key === 'teamId' || key === 'projectId') {
      valid = valid && id.test(value) && Number(value) <= 2147483647;
    } else if (key === 'documentGuid') {
      valid = valid && guid.test(value) && value !== '00000000-0000-0000-0000-000000000000';
    } else {
      valid = false;
    }
    native.searchParams.set(key, value);
  }
  valid = valid && (!params.has('documentGuid') || params.has('projectId'));
  if (!valid) {
    status.textContent = 'Некорректная ссылка Story Architect.';
    return;
  }

  button.href = native.href;
  button.hidden = false;
  let timer;
  let leftPage = false;
  const cancelFallback = () => {
    leftPage = true;
    window.clearTimeout(timer);
    status.textContent = 'Подтвердите открытие в браузере. Если приложение не открылось, нажмите «Открыть приложение» или скачайте его.';
  };
  // Browsers expose no reliable installed-app check. Do not redirect while an OS
  // prompt or the app has focus. Keep explicit retry/download links available.
  window.addEventListener('blur', cancelFallback);
  window.addEventListener('pagehide', cancelFallback);
  document.addEventListener('visibilitychange', () => {
    if (document.hidden) cancelFallback();
  });
  const open = () => {
    window.clearTimeout(timer);
    leftPage = false;
    status.textContent = 'Открываем ссылку в приложении…';
    timer = window.setTimeout(() => {
      if (!leftPage && !document.hidden) window.location.replace(downloads);
    }, 2500);
    try {
      window.location.href = native.href;
    } catch (_) {
      window.clearTimeout(timer);
      window.location.replace(downloads);
    }
  };
  button.addEventListener('click', event => {
    event.preventDefault();
    open();
  });
  open();
})();
