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