// Enhanced UI v3 — Font sizing, reading progress, breadcrumb, back-to-top
// Design: purposeful, zero-bounce motion; Inter-first typography
(function () {
  'use strict';

  // ───── Force Light Theme ─────
  (function forceLight() {
    try {
      var stored = localStorage.getItem('mdbook-theme');
      if (stored && stored !== 'light') {
        localStorage.setItem('mdbook-theme', 'light');
      }
      var html = document.documentElement;
      html.classList.remove('coal', 'navy', 'ayu', 'rust');
      if (!html.classList.contains('light')) html.classList.add('light');
    } catch (e) { /* noop */ }
  })();

  // ───── Restore Font Size Early (avoid FOUC) ─────
  (function restoreFontSize() {
    try {
      var saved = localStorage.getItem('kudig-font-size') || 'md';
      var html = document.documentElement;
      ['sm', 'md', 'lg', 'xl'].forEach(function (k) { html.classList.remove('font-size-' + k); });
      html.classList.add('font-size-' + saved);
    } catch (e) { /* noop */ }
  })();

  // ───── Reading Progress Bar ─────
  function initReadingProgress() {
    var bar = document.createElement('div');
    bar.className = 'reading-progress';
    document.body.appendChild(bar);

    var ticking = false;
    function update() {
      var y = window.scrollY || document.documentElement.scrollTop;
      var h = document.documentElement.scrollHeight - window.innerHeight;
      bar.style.width = h > 0 ? Math.min(y / h * 100, 100) + '%' : '0%';
    }

    window.addEventListener('scroll', function () {
      if (!ticking) { requestAnimationFrame(function () { update(); ticking = false; }); ticking = true; }
    }, { passive: true });
    update();
  }

  // ───── Back to Top ─────
  function initBackToTop() {
    var btn = document.createElement('button');
    btn.className = 'back-to-top';
    btn.setAttribute('aria-label', '回到顶部');
    btn.innerHTML = '<svg width="16" height="16" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M8 12V4M4 7l4-4 4 4"/></svg>';
    document.body.appendChild(btn);

    var ticking = false;
    function toggle() {
      var y = window.scrollY || document.documentElement.scrollTop;
      btn.classList.toggle('visible', y > 400);
    }

    window.addEventListener('scroll', function () {
      if (!ticking) { requestAnimationFrame(function () { toggle(); ticking = false; }); ticking = true; }
    }, { passive: true });

    btn.addEventListener('click', function () { window.scrollTo({ top: 0, behavior: 'smooth' }); });
    toggle();
  }

  // ───── Breadcrumb ─────
  function initBreadcrumb() {
    var active = document.querySelector('.sidebar-scrollbox a.active');
    if (!active) return;

    var crumbs = [];
    var el = active.parentElement;
    while (el) {
      if (el.classList && el.classList.contains('sidebar-scrollbox')) break;
      if (el.classList && el.classList.contains('chapter-item')) {
        var link = el.querySelector(':scope > .chapter-link-wrapper > a[href]');
        if (link) {
          crumbs.unshift({
            text: link.textContent.trim().replace(/^\d+\.\s*/, ''),
            href: link.getAttribute('href'),
            isCurrent: link.classList.contains('active')
          });
        }
      }
      el = el.parentElement;
    }

    if (crumbs.length < 2) return;

    var nav = document.createElement('nav');
    nav.className = 'breadcrumb';
    nav.setAttribute('aria-label', '面包屑导航');

    var home = document.createElement('a');
    home.href = '/';
    home.textContent = '首页';
    nav.appendChild(home);

    for (var i = 0; i < crumbs.length; i++) {
      var sep = document.createElement('span');
      sep.className = 'separator';
      sep.textContent = '/';
      nav.appendChild(sep);

      if (crumbs[i].isCurrent) {
        var span = document.createElement('span');
        span.className = 'current';
        span.textContent = crumbs[i].text.length > 40 ? crumbs[i].text.substring(0, 37) + '…' : crumbs[i].text;
        nav.appendChild(span);
      } else {
        var a = document.createElement('a');
        a.href = crumbs[i].href;
        a.textContent = crumbs[i].text.length > 30 ? crumbs[i].text.substring(0, 27) + '…' : crumbs[i].text;
        nav.appendChild(a);
      }
    }

    var mainContent = document.querySelector('#content main');
    if (mainContent && mainContent.firstChild) {
      mainContent.insertBefore(nav, mainContent.firstChild);
    }
  }

  // ───── Page Meta ─────
  function initPageMeta() {
    var mainContent = document.querySelector('#content main');
    if (!mainContent) return;

    var text = mainContent.textContent || '';
    var cjk = (text.match(/[\u4e00-\u9fff\u3400-\u4dbf\uf900-\ufaff]/g) || []).length;
    var latin = text.replace(/[\u4e00-\u9fff\u3400-\u4dbf\uf900-\ufaff]/g, ' ')
      .split(/\s+/).filter(function (w) { return w.length > 0; }).length;
    var total = cjk + latin;
    if (total < 50) return;

    var mins = Math.max(1, Math.ceil(total / 350));
    var meta = document.createElement('div');
    meta.className = 'page-meta';

    var timeEl = document.createElement('span');
    timeEl.className = 'page-meta-item';
    timeEl.innerHTML = '<svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5"><circle cx="8" cy="8" r="6.5"/><path d="M8 4.5V8l2.5 1.5" stroke-linecap="round"/></svg>'
      + '<span>约 ' + mins + ' 分钟阅读</span>';
    meta.appendChild(timeEl);

    var countEl = document.createElement('span');
    countEl.className = 'page-meta-item';
    countEl.innerHTML = '<svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5"><rect x="2.5" y="2.5" width="11" height="11" rx="2"/><path d="M5 5.5h6M5 8h6M5 10.5h4" stroke-linecap="round"/></svg>'
      + '<span>' + total.toLocaleString() + ' 字</span>';
    meta.appendChild(countEl);

    var h1 = mainContent.querySelector('h1');
    if (h1 && h1.nextSibling) {
      h1.parentNode.insertBefore(meta, h1.nextSibling);
    } else {
      var bc = mainContent.querySelector('.breadcrumb');
      if (bc && bc.nextSibling) mainContent.insertBefore(meta, bc.nextSibling);
    }
  }

  // ───── Smooth Anchors ─────
  function initSmoothAnchors() {
    document.addEventListener('click', function (e) {
      var link = e.target.closest('a[href^="#"]');
      if (!link) return;
      var id = link.getAttribute('href').slice(1);
      var target = document.getElementById(id);
      if (target) {
        e.preventDefault();
        target.scrollIntoView({ behavior: 'smooth', block: 'start' });
        history.pushState(null, '', '#' + id);
      }
    });
  }

  // ───── External Links ─────
  function initExternalLinks() {
    var links = document.querySelectorAll('#content main a[href^="http"]');
    links.forEach(function (link) {
      if (!link.getAttribute('href').includes(window.location.hostname)) {
        link.setAttribute('target', '_blank');
        link.setAttribute('rel', 'noopener noreferrer');
      }
    });
  }

  // ═══════════════════════════════════════════════════════════════════════
  //  Font Size Control — pill segmented control in menu bar
  // ═══════════════════════════════════════════════════════════════════════
  function initFontSizeControl() {
    var LEVELS = [
      { key: 'sm', label: '小',  title: '小号 14px' },
      { key: 'md', label: '标准', title: '标准 15px' },
      { key: 'lg', label: '大',  title: '大号 17px' },
      { key: 'xl', label: '特大', title: '特大 18px' }
    ];

    var saved = 'md';
    try { saved = localStorage.getItem('kudig-font-size') || 'md'; } catch (e) {}

    var wrap = document.createElement('div');
    wrap.className = 'font-size-control';
    wrap.setAttribute('aria-label', '字号调节');
    wrap.setAttribute('role', 'radiogroup');

    // "A" prefix icon
    var prefix = document.createElement('span');
    prefix.className = 'fs-prefix';
    prefix.textContent = 'A';
    wrap.appendChild(prefix);

    var buttons = [];
    LEVELS.forEach(function (lv) {
      var btn = document.createElement('button');
      btn.setAttribute('title', lv.title);
      btn.setAttribute('data-size', lv.key);
      btn.setAttribute('role', 'radio');
      btn.setAttribute('aria-checked', lv.key === saved ? 'true' : 'false');
      btn.textContent = lv.label;
      if (lv.key === saved) btn.classList.add('active');

      btn.addEventListener('click', function () { setFontSize(lv.key); });
      wrap.appendChild(btn);
      buttons.push(btn);
    });

    function setFontSize(key) {
      var html = document.documentElement;
      LEVELS.forEach(function (l) { html.classList.remove('font-size-' + l.key); });
      html.classList.add('font-size-' + key);
      buttons.forEach(function (b) {
        var isActive = b.getAttribute('data-size') === key;
        b.classList.toggle('active', isActive);
        b.setAttribute('aria-checked', isActive ? 'true' : 'false');
      });
      try { localStorage.setItem('kudig-font-size', key); } catch (e) {}
    }

    // Insert into menu bar right-buttons
    var menuBar = document.getElementById('menu-bar') || document.getElementById('mdbook-menu-bar');
    var rightButtons = menuBar ? menuBar.querySelector('.right-buttons') : null;
    if (rightButtons) {
      rightButtons.insertBefore(wrap, rightButtons.firstChild);
    } else if (menuBar) {
      menuBar.appendChild(wrap);
    }
  }

  // ───── Init ─────
  function init() {
    initFontSizeControl();
    initReadingProgress();
    initBackToTop();
    initBreadcrumb();
    initPageMeta();
    initSmoothAnchors();
    initExternalLinks();
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', function () { setTimeout(init, 120); });
  } else {
    setTimeout(init, 120);
  }
})();
