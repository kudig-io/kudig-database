/* ==========================================================================
   KUDIG Database — Custom JavaScript
   ========================================================================== */

/* ---------- Font Size Control ---------- */
(function() {
  const STORAGE_KEY = 'kd-font-size';
  const sizes = ['sm', 'md', 'lg', 'xl'];
  let current = localStorage.getItem(STORAGE_KEY) || 'md';

  function apply(size) {
    sizes.forEach(s => document.documentElement.classList.remove('font-size-' + s));
    document.documentElement.classList.add('font-size-' + size);
    localStorage.setItem(STORAGE_KEY, size);
    current = size;
  }

  // Apply saved size early to prevent FOUC
  apply(current);

  // Expose for potential UI controls
  window.kdSetFontSize = apply;
  window.kdFontSizes = sizes;
})();

/* ---------- Reading Progress Bar ---------- */
(function() {
  const bar = document.createElement('div');
  bar.className = 'reading-progress';
  document.body.appendChild(bar);

  function update() {
    const scrollTop = window.scrollY;
    const docHeight = document.documentElement.scrollHeight - window.innerHeight;
    const pct = docHeight > 0 ? (scrollTop / docHeight) * 100 : 0;
    bar.style.width = Math.min(100, pct) + '%';
  }

  window.addEventListener('scroll', update, { passive: true });
  update();
})();

/* ---------- External Links ---------- */
(function() {
  document.querySelectorAll('a[href^="http"]').forEach(function(link) {
    if (!link.hostname.includes(window.location.hostname)) {
      link.setAttribute('target', '_blank');
      link.setAttribute('rel', 'noopener noreferrer');
    }
  });
})();

/* ---------- Smooth Scroll for Anchors ---------- */
document.querySelectorAll('a[href^="#"]').forEach(function(anchor) {
  anchor.addEventListener('click', function(e) {
    const target = document.querySelector(this.getAttribute('href'));
    if (target) {
      e.preventDefault();
      target.scrollIntoView({ behavior: 'smooth', block: 'start' });
      history.pushState(null, null, this.getAttribute('href'));
    }
  });
});

/* ---------- Reading Time Estimation ---------- */
(function() {
  function estimateReadingTime() {
    const article = document.querySelector('article') || document.querySelector('main') || document.body;
    const text = article.innerText || article.textContent || '';
    const words = text.split(/\s+/).filter(function(w) { return w.length > 0; }).length;
    const minutes = Math.ceil(words / 200);
    return minutes;
  }

  function createReadingTimeElement() {
    const minutes = estimateReadingTime();
    const el = document.createElement('span');
    el.className = 'reading-time';
    el.setAttribute('aria-label', 'Estimated reading time');
    el.innerHTML = '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M12 6v6l4 2"/></svg> ' + minutes + 'min read';
    return el;
  }

  function inject() {
    const target = document.querySelector('h1');
    if (!target) return;
    const rt = createReadingTimeElement();
    target.parentNode.insertBefore(rt, target.nextSibling);
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', inject);
  } else {
    inject();
  }

  window.kdEstimateReadingTime = estimateReadingTime;
})();

/* ---------- Search Shortcut "/" ---------- */
(function() {
  let searchInput = null;

  function findSearchInput() {
    // Try MkDocs Material search input
    searchInput = searchInput || document.querySelector('.md-search__input');
    // Try default MkDocs
    searchInput = searchInput || document.querySelector('input[type="search"]');
    // Try generic search
    searchInput = searchInput || document.querySelector('input[placeholder*="search" i], input[placeholder*="搜索" i]');
    return searchInput;
  }

  document.addEventListener('keydown', function(e) {
    if (e.key === '/' && !e.ctrlKey && !e.metaKey && !e.altKey) {
      const tag = document.activeElement.tagName;
      if (tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT') return;
      if (document.activeElement.isContentEditable) return;

      const input = findSearchInput();
      if (input) {
        e.preventDefault();
        input.focus();
        input.select();
      }
    }
  });
})();

/* ---------- Mermaid Initialization ---------- */
(function() {
  if (typeof mermaid !== 'undefined') {
    mermaid.initialize({
      startOnLoad: true,
      theme: 'default',
      securityLevel: 'loose',
      flowchart: { useMaxWidth: true, htmlLabels: true },
      sequence: { useMaxWidth: true, diagramMarginX: 20 },
      gantt: { topPadding: 50, leftPadding: 120 }
    });
  }
})();