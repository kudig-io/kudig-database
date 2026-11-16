/* KUDIG Console — 云原生知识体系工作台（全库版）
   四个视图：总览 / 结构图 / 图表库 / 阅读。数据来自 console/data/ 与 console/content/，
   由 code/scripts/maintenance/console-corpus.py 生成；Mermaid 用 console/vendor 的
   mermaid.min.js 本地渲染，不依赖任何 CDN。 */
(() => {
  'use strict';

  const $ = (sel, root = document) => root.querySelector(sel);
  const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel));
  const esc = (s) => String(s == null ? '' : s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  const fmt = (n) => (n >= 10000 ? (n / 1000).toFixed(n >= 100000 ? 0 : 1) + 'k' : String(n));
  const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
  const PART_NAMES = { wiki: '语料正文', skills: 'Agent 技能卡', maintaining: '维护资产', code: '可执行资产' };
  const LAYERS = ['L0 总览', 'L1 概念', 'L2 架构', 'L3 操作', 'L4 检查', 'L5 故障', 'L6 参考'];
  const KINDS = { graph: '流程图', sequence: '时序图', state: '状态图', class: '类图', mindmap: '思维导图', timeline: '时间线', gantt: '甘特图', pie: '饼图', quadrant: '象限图', xychart: '图表', venn: '韦恩图', gitGraph: 'Git 图', er: 'ER 图', journey: '旅程图', requirement: '需求图', other: '其它' };

  const state = {
    index: null, domains: null, structure: null, coverage: null, diagram: null,
    byId: new Map(), pagesByDomain: new Map(),
    view: 'overview', domain: null, page: null,
    dgKind: null, dgDomain: null, dgRendered: false,
    openParts: new Set(['wiki']),
    content: new Map(), dgCodes: new Map(),
    mermaidReady: false,
  };

  /* ------------------------------------------------------------------ data */

  async function loadJSON(path) {
    const res = await fetch(path, { cache: 'no-cache' });
    if (!res.ok) throw new Error(`${path} → HTTP ${res.status}`);
    return res.json();
  }

  async function boot() {
    try {
      const [index, domains, structure, coverage, diagram] = await Promise.all([
        loadJSON('./data/index.json'),
        loadJSON('./data/domains.json'),
        loadJSON('./data/graph/structure.json'),
        loadJSON('./data/graph/coverage.json'),
        loadJSON('./data/graph/diagram.json'),
      ]);
      Object.assign(state, { index, domains, structure, coverage, diagram });
      index.pages.forEach((p) => state.byId.set(p.id, p));
      state.pageKinds = new Map(Object.entries(diagram.pages || {}));
      index.pages.forEach((p) => {
        if (!state.pagesByDomain.has(p.d)) state.pagesByDomain.set(p.d, []);
        state.pagesByDomain.get(p.d).push(p);
      });
    } catch (err) {
      $('#main').innerHTML = `<div class="empty" style="margin-top:24px"><b>语料未载入</b>${esc(err.message)}<br><br>
        先运行 <code>make console-sync</code> 生成 <code>console/data/</code>，再 <code>make console-serve</code> 访问
        <code>http://127.0.0.1:8766/</code>（file:// 打开会被浏览器拦截 fetch）。</div>`;
      return;
    }
    renderSide(); renderHeader(); bindGlobal(); route(); render();
    window.__consoleReady = true;
  }

  /* --------------------------------------------------------------- mermaid */

  function loadMermaid() {
    return new Promise((resolve) => {
      if (state.mermaidReady) return resolve(window.mermaid || null);
      if (!window.mermaid) return resolve(null);
      try {
        window.mermaid.initialize({
          startOnLoad: false, theme: 'base', securityLevel: 'loose', logLevel: 'fatal',
          fontFamily: "'PingFang SC','Helvetica Neue',sans-serif",
          themeVariables: {
            primaryColor: '#EFF3FE', primaryBorderColor: '#1E40AF', primaryTextColor: '#0D0D0D',
            lineColor: '#55555C', secondaryColor: '#F2F1EC', tertiaryColor: '#FAFAF8',
            noteBkgColor: '#FCF5EA', noteBorderColor: '#B86E0A', actorBkg: '#EFF3FE',
            actorBorder: '#1E40AF', signalColor: '#0D0D0D', labelBackground: '#FFFFFF',
          },
        });
        state.mermaidReady = true;
        resolve(window.mermaid);
      } catch (e) {
        resolve(null);
      }
    });
  }

  let mmSeq = 0;
  async function renderDiagram(host, code) {
    const mermaid = await loadMermaid();
    if (!mermaid) throw new Error('mermaid 未加载');
    mmSeq += 1;
    const id = 'mm' + mmSeq;
    const { svg } = await mermaid.render(id, code);
    if (host) host.innerHTML = svg;
    return svg;
  }

  /* ------------------------------------------------------------- chrome */

  function renderHeader() {
    const s = state.structure;
    $('#brand-sub').textContent = `全库 · ${s.domains} 域`;
    $('#sync').innerHTML = `<b>LIVE</b>${s.pages} 页`;
  }

  function renderSide() {
    const doms = state.domains.partitions;
    $('#side-body').innerHTML = doms.map((part) => `
      <div class="nav-group">
        <button class="nav-part" data-part="${part.id}" aria-expanded="${state.openParts.has(part.id)}">
          ${esc(part.name)}<span class="c">${part.count} 页 · ${part.diagrams} 图</span></button>
        <div class="nav-domains" data-domains="${part.id}" ${state.openParts.has(part.id) ? '' : 'hidden'}>
          ${part.domains.sort((a, b) => b.count - a.count).map((d) => `
            <a class="nav-dom" href="#overview/${d.id}" data-dom="${d.id}" aria-current="${state.domain === d.id}">
              <span class="n">${esc(d.name)}</span>
              <span class="k">${d.dm ? d.dm + '图' : ''}</span>
              <span class="d">${d.count}</span></a>`).join('')}
        </div>
      </div>`).join('');
  }

  function renderTabs() {}

  /* ------------------------------------------------------------- router */

  function parseHash() {
    const raw = (location.hash || '#overview').replace(/^#\/?/, '');
    const [path, qs] = raw.split('?');
    return { seg: path.split('/').filter(Boolean), q: new URLSearchParams(qs || '') };
  }

  function route() {
    const { seg, q } = parseHash();
    state.dgPage = null;
    const views = ['overview', 'structure', 'diagrams', 'read'];
    state.view = views.includes(seg[0]) ? seg[0] : 'overview';
    const decode = (x) => { try { return decodeURIComponent(x); } catch { return x; } };
    if (state.view === 'overview') state.domain = seg[1] ? decode(seg.slice(1).join('/')) : null;
    if (state.view === 'structure') state.domain = seg[1] ? decode(seg.slice(1).join('/')) : null;
    if (state.view === 'diagrams') {
      state.dgKind = q.get('kind') || null;
      state.dgDomain = q.get('domain') ? decode(q.get('domain')) : null;
      state.dgPage = q.get('page') ? (state.byId.has(q.get('page')) ? q.get('page') : null) : null;
    }
    if (state.view === 'read' && seg[1]) {
      const id = decode(seg.slice(1).join('/'));
      state.page = state.byId.has(id) ? id : state.page;
    }
    if (!state.page) state.page = state.index.pages[0].id;
  }

  function go(href) {
    history.replaceState(null, '', href);
    route();
    render();
  }

  /* ------------------------------------------------------------- render */

  function render() {
    $$('.views a').forEach((a) => a.setAttribute('aria-current', a.dataset.view === state.view ? 'page' : 'false'));
    $$('#side-body .nav-dom').forEach((a) => a.setAttribute('aria-current', a.dataset.dom === state.domain));
    document.body.classList.remove('nav-open');
    if (state.view === 'overview') renderOverview();
    else if (state.view === 'structure') renderStructure();
    else if (state.view === 'diagrams') renderDiagrams();
    else renderRead();
    $('main').scrollTop = 0;
  }

  function indexStrip() {
    const s = state.structure;
    return `<div class="index-strip" role="list">
      ${[['页', s.pages], ['域', s.domains], ['图', s.diagrams], ['行', fmt(s.lines)], ['链', fmt(s.links)]]
        .map(([k, v]) => `<div class="ix" role="listitem"><span class="k">${k}</span><span class="v">${v}</span></div>`).join('')}
      <div class="ix note" role="listitem">${s.tables.toLocaleString()} 张表 · 由 console-corpus.py 从 wiki 生成</div></div>`;
  }

  /* -------- 总览：分层谱 -------- */

  function renderOverview() {
    const rows = state.coverage.rows;
    const totals = state.coverage.totals;
    const focus = state.domain && state.pagesByDomain.has(state.domain) ? state.domain : null;
    const groups = [
      { title: '四分区', items: [{ id: null, name: '全库', count: state.structure.pages }].concat(
        state.domains.partitions.map((p) => ({ id: p.id, name: p.name, count: p.count, part: true }))) },
    ];
    const view = (list) => `<div class="spectrum">
      <div class="sp-head"><div class="sp-name">${focus ? '知识域' : '分区'}</div>
        ${LAYERS.map((l) => `<div class="sp-col"><b>${l.slice(0, 2)}</b><small>${l.slice(3)}</small></div>`).join('')}
        <div class="sp-col"><b>落锁</b><small>合计</small></div><div class="sp-col"><b>覆盖</b><small>%</small></div></div>
      ${list}</div>`;

    const partRow = (id, name, count) => {
      const members = id ? state.index.pages.filter((p) => p.p === id) : state.index.pages;
      const cells = LAYERS.map((_, i) => {
        const n = members.filter((p) => p.layers[i] === '1').length;
        const status = n === members.length ? 'full' : (n ? 'partial' : 'gap');
        return `<div class="sp-cell ${status}" title="${n}/${members.length}">${n || '–'}</div>`;
      }).join('');
      const sum = members.reduce((a, p) => a + (p.layers.match(/1/g) || []).length, 0);
      const pct = Math.round((sum / (members.length * 7)) * 100);
      return `<div class="sp-row" data-part-row="${id || ''}">
        <div class="sp-name"><span class="mark" style="background:${id ? 'var(--blue)' : 'var(--ink)'}"></span>
          <span class="n" data-full="${esc(name)}">${esc(name)}</span><span class="c">${count}</span></div>
        ${cells}<div class="sp-col"><b>${sum}</b></div>
        <div class="sp-bar"><span class="t"><i style="width:${pct}%"></i></span><b style="font:500 11px var(--font-mono)">${pct}%</b></div></div>`;
    };

    const domainRows = (partId) => state.domains.partitions.find((p) => p.id === partId).domains
      .sort((a, b) => b.count - a.count).map((d) => {
        const members = state.pagesByDomain.get(d.id) || [];
        const cells = LAYERS.map((_, i) => {
          const n = members.filter((p) => p.layers[i] === '1').length;
          const status = n === members.length ? 'full' : (n ? 'partial' : 'gap');
          return `<div class="sp-cell ${status}" title="${n}/${members.length}">${n || '–'}</div>`;
        }).join('');
        const sum = members.reduce((a, p) => a + (p.layers.match(/1/g) || []).length, 0);
        const pct = members.length ? Math.round((sum / (members.length * 7)) * 100) : 0;
        return `<div class="sp-row" data-goto="#overview/${d.id}">
          <div class="sp-name"><span class="mark"></span><a class="n" href="#overview/${encodeURI(d.id)}">${esc(d.name)}</a><span class="c">${d.count}</span></div>
          ${cells}<div class="sp-col"><b>${sum}</b></div>
          <div class="sp-bar"><span class="t"><i style="width:${pct}%"></i></span><b style="font:500 11px var(--font-mono)">${pct}%</b></div></div>`;
      }).join('');

    const layerTotals = totals.map((t) => `<div class="sp-col"><b>${t.landed}</b><small>${t.gapCount} 缺</small></div>`).join('');
    const drill = focus ? `<div class="sp-band">${esc(state.byId.get((state.pagesByDomain.get(focus) || [{}])[0].id)?.dn || focus)}
      <span>${(state.pagesByDomain.get(focus) || []).length} 页 · 点任一行进入该域</span>
      <a href="#overview" style="margin-left:auto;font:500 11px var(--font-mono)">返回四分区</a></div>`
      : `<div class="sp-band">四分区 · 七层覆盖
        <span>完整 = 该分区全部页面都落锁该层；部分 = 有的页面有；缺口 = 没有页面成段出现</span></div>`;

    $('#main').innerHTML = `
      <div class="crumbs"><span>工作台</span><span class="cur">全库总览</span></div>
      <div class="h-1"><h1>全库总览 <span>· 七层知识栈</span></h1>
        <div class="meta"><b>${state.structure.pages.toLocaleString()} 页</b> · ${state.structure.domains} 域<br>
        ${state.structure.diagrams} 张 Mermaid 图 · ${fmt(state.structure.links)} 条链接</div></div>
      <div class="tabs" role="tablist">
        <button role="tab" aria-selected="false" data-view="overview">分层谱<span class="c">4+86</span></button>
        <button role="tab" aria-selected="false" data-view="structure">结构图<span class="c">S0–S7</span></button>
        <button role="tab" aria-selected="false" data-view="diagrams">图表库<span class="c">${state.structure.diagrams}</span></button>
        <button role="tab" aria-selected="false" data-view="read">阅读<span class="c">${fmt(state.structure.pages)}</span></button>
      </div>
      ${drill}
      ${view(groups[0].items.map((it) => partRow(it.id, it.name, it.count)).join(''))}
      ${focus ? `<h3 style="font:600 14px var(--font-ui);margin:20px 0 10px">${esc(focus)} 下的页面</h3>
        <div class="panel"><div class="body" style="display:flex;flex-wrap:wrap;gap:6px">
          ${(state.pagesByDomain.get(focus) || []).map((p) => `<a class="chipbtn" href="#read/${encodeURI(p.id)}">${esc(p.t).slice(0, 34)}</a>`).join('')}
        </div></div>` : ''}
      ${indexStrip()}
      <div class="footer"><span><b>生成</b> make console-sync</span><span><b>Mermaid</b> 本地引擎渲染</span>
        <span class="r">${state.structure.diagramsOk} / ${state.structure.diagrams} 张图可直接渲染</span></div>`;

    bindCommon();
  }

  /* -------- 结构图：S0–S7 结构带 + 树 + 谱 -------- */

  function bandOf(page, i) {
    // S0 根 · S1 域入口 · S2 深入 · S3 未组织 · S4 空目录 · S5 0/7 · S6 6/7 · S7 7/7
  }

  function renderStructure() {
    const focus = state.domain;
    const pages = focus ? (state.pagesByDomain.get(focus) || []) : state.index.pages;
    const bandOfPage = (p) => {
      const landed = (p.layers.match(/1/g) || []).length;
      const file = p.id.split('/').pop();
      if (landed === 7) return 'S7';
      if (landed === 6) return 'S6';
      if (landed === 0) return 'S5';
      if (/^(index|README|MOC|SUMMARY|00-总览)$/i.test(file) || /总览/i.test(file)) return 'S1';
      if (landed >= 3) return 'S2';
      return 'S3';
    };
    const bands = [
      { k: 'S0', t: '根与索引', d: '目录入口：index / README / MOC / SUMMARY' },
      { k: 'S1', t: '总览页', d: '总览、适用场景与范围' },
      { k: 'S2', t: '深入页（≥3 层）', d: '有骨架的正文' },
      { k: 'S3', t: '未组织正文（0–2 层）', d: '只有段落，没有成节的层' },
      { k: 'S4', t: '空目录 / 仅索引', d: '目录存在但没有任何内容页' },
      { k: 'S5', t: '0/7 层', d: '没有任何一层落锁' },
      { k: 'S6', t: '6/7 层', d: '只差一层' },
      { k: 'S7', t: '7/7 层', d: '七层齐备，可作模板' },
    ];
    const bucket = new Map(bands.map((b) => [b.k, []]));
    pages.forEach((p) => {
      const file = p.id.split('/').pop();
      if (/^(index|README|MOC|SUMMARY)$/i.test(file)) bucket.get('S0').push(p);
      else {
        const landed = (p.layers.match(/1/g) || []).length;
        if (landed === 7) bucket.get('S7').push(p);
        else if (landed === 6) bucket.get('S6').push(p);
        else if (landed === 0) bucket.get('S5').push(p);
        else if (/总览/.test(file)) bucket.get('S1').push(p);
        else bucket.get(landed >= 3 ? 'S2' : 'S3').push(p);
      }
    });
    // S4: domains whose name matches a directory with no content pages is not in the index,
    // so S4 counts domain entries that resolve to zero pages when drilled (reported honestly as 0 if none).
    const maxLen = Math.max(...bands.map((b) => bucket.get(b.k).length), 1);

    const bandHtml = bands.map((b) => {
      const list = bucket.get(b.k);
      const ratio = list.length / Math.max(pages.length, 1);
      const shown = list.slice(0, 60);
      return `<div class="band">
        <div class="band-head"><span class="k">${b.k}</span>
          <div class="t">${esc(b.t)} <span>· ${esc(b.d)}</span></div>
          <span class="n">${list.length} 页 · ${(ratio * 100).toFixed(1)}%</span></div>
        <div class="band-rail"><span style="width:${(list.length / maxLen) * 100}%;background:${b.k === 'S5' ? 'var(--amber)' : (b.k === 'S7' ? 'var(--blue)' : 'var(--rule)')}"></span></div>
        <div class="band-cells">${shown.map((p) => `<a class="u ${(p.layers.match(/1/g) || []).length === 7 ? 'full' : ''}" href="#read/${encodeURI(p.id)}" title="${esc(p.t)}">${esc(p.id.split('/').pop().replace(/\.md$/, ''))}</a>`).join('')}
          ${list.length > shown.length ? `<span class="more">+${list.length - shown.length} 页（筛右侧或进入域查看）</span>` : ''}</div></div>`;
    }).join('');

    const treeSvg = renderTreeSvg(focus);
    const spectrumSvg = renderSpectrumSvg();
    const symmetries = symmetryFindings();

    $('#main').innerHTML = `
      <div class="crumbs"><span>工作台</span><span>结构图</span><span class="cur">${focus ? esc(focus) : '全库'}</span></div>
      <div class="h-1"><h1>结构图 <span>· ${focus ? esc(focus) : '全库分层与带状分布'}</span></h1>
        <div class="meta">${pages.length.toLocaleString()} 页 · ${bucket.get('S7').length} 页七层齐备<br>
        ${bucket.get('S0').length} 个索引 · ${bucket.get('S5').length} 页 0/7</div></div>
      <div class="tabs" role="tablist">
        <button role="tab" aria-selected="false" data-view="overview">分层谱<span class="c">4+86</span></button>
        <button role="tab" aria-selected="false" data-view="structure">结构图<span class="c">S0–S7</span></button>
        <button role="tab" aria-selected="false" data-view="diagrams">图表库<span class="c">${state.structure.diagrams}</span></button>
        <button role="tab" aria-selected="false" data-view="read">阅读<span class="c">${fmt(state.structure.pages)}</span></button>
      </div>
      <div class="panel"><header><span class="n">S0–S7</span><h2>结构带</h2>
        <span class="aside">按页在库中的位置与七层落锁数分带</span></header>
        <div class="body"><div class="bands">${bandHtml}</div></div></div>
      <div class="panel"><header><span class="n">LIBRARY TREE</span><h2>库图</h2>
        <span class="aside">分区 → 域 → 页面，按七层落锁数着色</span></header>
        <div class="body"><div class="libscroll">${treeSvg}</div>
        <div class="legend"><span><i class="full"></i>七层齐备 / 核心</span><span><i class="partial"></i>深层页</span><span><i class="gap"></i>未组织</span></div></div></div>
      <div class="panel"><header><span class="n">S-LAYER SPECTRUM</span><h2>层级谱</h2>
        <span class="aside">横轴 L0–L6，每根竖线是同一个域的落锁剖面</span></header>
        <div class="body"><div class="libscroll">${spectrumSvg}</div></div></div>
      <div class="panel"><header><span class="n">SYMMETRY</span><h2>对称性</h2>
        <span class="aside">同一判据在不同域上的覆盖差异</span></header>
        <div class="body">${symmetries}</div></div>
      ${indexStrip()}`;
    bindCommon();
  }

  function symmetryFindings() {
    const rows = state.coverage.rows.filter((r) => r.count >= 3).sort((a, b) => b.sum / b.count - a.sum / a.count);
    const top = rows.slice(0, 3);
    const bottom = rows.slice(-3);
    const line = (r) => `<div class="dg-meta"><span><b>${esc(r.n)}</b></span><span>${r.count} 页</span>
      <span>${r.sum}/${r.count * 7} 层</span><span>${r.cells.map((c) => c.layer + (c.status === 'full' ? '✓' : (c.status === 'partial' ? '·' : '–'))).join(' ')}</span></div>`;
    return `<p style="font-size:12.5px;color:var(--mut);margin:0 0 10px">覆盖最稠密的三个域与最稀疏的三个域，用同一套七层判据量出来：
      <b>稠密</b>说明该域已有成熟写法可复制，<b>稀疏</b>说明该域需要先补一份「模板页」。</p>
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:16px">
        <div><h4 style="font:500 11px var(--font-mono);letter-spacing:.12em;color:var(--mut-2);margin:0 0 6px">最稠密</h4>${top.map(line).join('')}</div>
        <div><h4 style="font:500 11px var(--font-mono);letter-spacing:.12em;color:var(--mut-2);margin:0 0 6px">最稀疏</h4>${bottom.map(line).join('')}</div>
      </div>`;
  }

  /* -------- 库图（SVG 树） -------- */

  function renderTreeSvg(focus) {
    const parts = state.domains.partitions;
    const rowH = 17;
    const partH = 22;
    const totalRows = parts.reduce((a, p) => a + 1 + p.domains.length, 0);
    const h = Math.max(300, totalRows * rowH + 30);
    const W = 1000;
    let y = 16;
    const nodes = [];
    const edges = [];
    parts.forEach((part) => {
      const py = y;
      nodes.push({ x: 12, y: py, w: 168, h: partH - 4, label: part.name, kind: 'part', count: part.count });
      y += partH;
      part.domains.slice().sort((a, b) => b.count - a.count).forEach((d) => {
        const isFocus = focus === d.id;
        nodes.push({ x: 60, y: y, w: 230, h: rowH - 4, label: d.name, kind: isFocus ? 'focus' : 'dom', count: d.count });
        const members = state.pagesByDomain.get(d.id) || [];
        const maxPip = Math.min(members.length, 40);
        for (let i = 0; i < maxPip; i += 1) {
          const p = members[i];
          const landed = (p.layers.match(/1/g) || []).length;
          nodes.push({ x: 310 + i * 16, y: y + 1, w: 12, h: rowH - 6, kind: landed === 7 ? 'core' : (landed >= 3 ? 'deep' : 'sub'), page: p });
        }
        if (members.length > maxPip) nodes.push({ x: 310 + maxPip * 16 + 4, y: y, w: 60, h: rowH - 4, label: `+${members.length - maxPip}`, kind: 'more' });
        edges.push([py + (partH - 4) / 2, y + (rowH - 4) / 2]);
        y += rowH;
      });
      y += 6;
    });
    const edgeSvg = edges.map(([y1, y2]) => `<path class="edge" d="M180 ${y1} C240 ${y1}, 20 ${y2}, 60 ${y2}"/>`).join('');
    const nodeSvg = nodes.map((n) => {
      if (n.kind === 'part' || n.kind === 'dom' || n.kind === 'focus') {
        const cls = n.kind === 'part' ? 'nodebox core' : (n.kind === 'focus' ? 'nodebox core' : 'nodebox');
        return `<g data-goto="${n.kind === 'part' ? '#overview' : '#overview/' + nodesDomId(n.label)}">
          <rect class="${cls}" x="${n.x}" y="${n.y}" width="${n.w}" height="${n.h}"/>
          <text x="${n.x + 6}" y="${n.y + n.h - 4}" style="font-size:11px">${esc(n.label)}</text>
          <text class="n" x="${n.x + n.w - 6}" y="${n.y + n.h - 4}" text-anchor="end">${n.count}</text></g>`;
      }
      if (n.kind === 'more') return `<text class="n" x="${n.x}" y="${n.y + n.h - 4}">${esc(n.label)}</text>`;
      const landed = (n.page.layers.match(/1/g) || []).length;
      const cls = n.kind === 'core' ? 'nodebox core' : (n.kind === 'deep' ? 'nodebox' : 'nodebox gap');
      return `<g data-goto="#read/${esc(n.page.id)}" data-tip="${esc(n.page.t)} · ${landed}/7 层"><rect class="${cls}" x="${n.x}" y="${n.y}" width="${n.w}" height="${n.h}">
        <title>${esc(n.page.t)}（${landed}/7）</title></rect></g>`;
    }).join('');
    return `<svg class="lib-svg" viewBox="0 0 ${W} ${h}" width="${W}" height="${h}" role="img" aria-label="库图：分区到域的树">${edgeSvg}${nodeSvg}</svg>`;
  }

  function nodesDomId(label) {
    const hit = state.domains.partitions.flatMap((p) => p.domains).find((d) => d.name === label);
    return hit ? hit.id : '';
  }

  function renderSpectrumSvg() {
    const rows = state.coverage.rows.slice().sort((a, b) => b.sum / b.count - a.sum / a.count);
    const left = 190;
    const colW = (1000 - left - 60) / 7;
    const rowH = 20;
    const h = rows.length * rowH + 40;
    const head = LAYERS.map((l, i) => `<text class="axislabel" x="${left + i * colW + colW / 2}" y="14" text-anchor="middle">${l.slice(0, 2)}</text>`).join('');
    const body = rows.map((r, ri) => {
      const y = 24 + ri * rowH;
      const label = `<text x="6" y="${y + 14}" style="font-size:11px">${esc(r.n)}</text><text class="n" x="${left - 12}" y="${y + 14}" text-anchor="end">${r.count}</text>`;
      const cells = r.cells.map((c, i) => {
        const x = left + i * colW;
        const ratio = c.total ? c.landed / c.total : 0;
        const fill = c.status === 'full' ? '#1E40AF' : (c.status === 'partial' ? '#93A9E8' : '#FCF5EA');
        return `<rect x="${x}" y="${y}" width="${colW - 2}" height="${rowH - 4}" fill="${fill}" stroke="${c.status === 'gap' ? '#B86E0A' : '#D6D5CD'}" ${c.status === 'gap' ? 'stroke-dasharray="3 2"' : ''}>
          <title>${c.layer}：${c.landed}/${c.total} 页落锁</title></rect>
          <text class="axislabel" x="${x + (colW - 2) / 2}" y="${y + 12}" text-anchor="middle" fill="${c.status === 'full' ? '#fff' : '#0D0D0D'}" style="font-size:10px">${Math.round(ratio * 100)}%</text>`;
      }).join('');
      return label + cells;
    }).join('');
    return `<svg class="lib-svg" viewBox="0 0 1000 ${h}" width="1000" height="${h}" role="img" aria-label="层级谱：每个域的七层落锁比例">${head}${body}</svg>`;
  }

  /* -------- 图表库 -------- */

  function diagramRows() {
    const pages = state.index.pages;
    const rows = [];
    pages.forEach((p) => {
      if (!p.dm) return;
      for (let i = 0; i < p.dm; i += 1) rows.push({ id: `${p.id}#${i}`, page: p, kind: null });
    });
    return rows;
  }

  function renderDiagrams() {
    const pages = state.index.pages.filter((p) => p.dm);
    const kinds = state.diagram.kinds;
    const kindCounts = new Map(kinds.map((k) => [k.k, k.n]));
    const domainCounts = new Map(state.diagram.byDomain.map((d) => [d.d, d.n]));
    const filtered = pages.filter((p) => (!state.dgKind || (p.dm && hasKind(p, state.dgKind))) && (!state.dgDomain || p.d === state.dgDomain));
    const rows = filtered.sort((a, b) => b.dm - a.dm).slice(0, 120);

    const kindBtns = kinds.map((k) => `<button class="chipbtn" aria-pressed="${state.dgKind === k.k}" data-kind="${k.k}">${esc(k.name)}<span class="c">${k.n}</span></button>`).join('');

    $('#main').innerHTML = `
      <div class="crumbs"><span>工作台</span><span class="cur">图表库</span></div>
      <div class="h-1"><h1>图表库 <span>· ${state.diagram.total} 张 Mermaid 图</span></h1>
        <div class="meta">${state.diagram.total} 张 · ${state.diagram.heaviest.length} 个高产页<br>
        ${kinds.slice(0, 3).map((k) => `${esc(k.name)} ${k.n}`).join(' · ')}</div></div>
      <div class="tabs" role="tablist">
        <button role="tab" aria-selected="false" data-view="overview">分层谱<span class="c">4+86</span></button>
        <button role="tab" aria-selected="false" data-view="structure">结构图<span class="c">S0–S7</span></button>
        <button role="tab" aria-selected="false" data-view="diagrams">图表库<span class="c">${state.structure.diagrams}</span></button>
        <button role="tab" aria-selected="false" data-view="read">阅读<span class="c">${fmt(state.structure.pages)}</span></button>
      </div>
      <div class="dg-controls"><span class="lbl">类型</span>
        <button class="chipbtn" aria-pressed="${!state.dgKind}" data-kind="">全部<span class="c">${state.diagram.total}</span></button>${kindBtns}</div>
      <div class="dg-controls"><span class="lbl">域</span>
        <button class="chipbtn" aria-pressed="${!state.dgDomain}" data-domfilter="">全部域</button>
        ${state.diagram.byDomain.slice(0, 10).map((d) => `<button class="chipbtn" aria-pressed="${state.dgDomain === d.d}" data-domfilter="${esc(d.d)}">${esc(d.d.split('/').pop())}<span class="c">${d.n}</span></button>`).join('')}</div>
      <div class="dg-list">
        ${rows.map((p) => {
          const kindsHere = kindsOf(p);
          return `<a class="dg-row" href="#diagrams?page=${encodeURI(p.id)}">
            <span class="k">${p.dm} 图</span>
            <span class="t">${esc(p.t)}</span>
            <span class="d">${esc(p.d)}</span>
            <span class="v">${kindsHere.map((k) => esc((KINDS[k] || k))).slice(0, 2).join(' / ')}</span></a>`;
        }).join('')}
      </div>
      <div id="dg-host"></div>
      <div class="footer"><span><b>渲染</b> 本地 mermaid.min.js（离线）</span><span><b>已校验</b> ${state.structure.diagramsOk || state.structure.diagrams} / ${state.structure.diagrams} 张可渲染</span><span class="r">个别图在运行时仍可能因引擎版本失败，失败时原文照常呈现</span></div>`;
    bindCommon();

    const viewer = $('#dg-host');
    const selected = state.dgPage ? state.byId.get(state.dgPage) : null;
    if (selected) {
      viewer.innerHTML = `<div class="panel"><header><span class="n">VIEWER</span><h2>${esc(selected.t)}</h2>
        <span class="aside">${esc(selected.d)} · ${selected.dm} 图</span></header><div class="body" id="dg-canvas"></div></div>`;
      loadPageContent(selected.id).then((segments) => {
        const canvas = $('#dg-canvas');
        if (!segments) return;
        segments.forEach(async (seg, idx) => {
          if (seg[0] !== 'm') return;
          const holder = document.createElement('div');
          holder.className = 'dg-viewer';
          holder.innerHTML = `<div class="dg-meta"><span>#${idx + 1}</span><span>${esc(KINDS[seg[2]] || seg[2])}${seg[4] && seg[4] !== seg[2] ? '（源码写作 ' + esc(seg[4]) + '）' : ''}</span>
            <span>${esc(selected.t)}</span><span style="margin-left:auto;cursor:pointer;color:var(--blue)" data-expand>全屏查看</span></div><div class="c"></div>`;
          canvas.appendChild(holder);
          const box = holder.querySelector('.c');
          box.innerHTML = '<div class="loading">渲染 Mermaid…</div>';
          try {
            await renderDiagram(box, seg[3]);
          } catch (e) {
            box.innerHTML = `<div class="dg-fallback"><div class="why">Mermaid 解析失败：${esc(String(e.message || e).slice(0, 160))}</div><pre>${esc(seg[3])}</pre></div>`;
          }
        });
      });
    } else if (!viewer.innerHTML) {
      viewer.innerHTML = `<div class="empty">选择上方任意一页查看其全部图表；图表库只负责列目录，图在阅读页里原位呈现。</div>`;
    }
  }

  function pageKindCounts(p) {
    return (state.pageKinds && state.pageKinds.get(p.id)) || null;
  }

  function hasKind(p, kind) {
    const own = pageKindCounts(p);
    if (own) return (own[kind] || 0) > 0;
    const row = state.diagram.byDomain.find((d) => d.d === p.d);
    return row ? Boolean(row.kinds[kind]) : false;
  }

  function kindsOf(p) {
    const own = pageKindCounts(p);
    if (own) return Object.entries(own).sort((a, b) => b[1] - a[1]).map(([k]) => k);
    const row = state.diagram.byDomain.find((d) => d.d === p.d);
    return row ? Object.entries(row.kinds).sort((a, b) => b[1] - a[1]).map(([k]) => k) : [];
  }

  /* -------- 阅读 -------- */

  async function loadPageContent(id) {
    if (state.content.has(id)) return state.content.get(id);
    try {
      const segs = await loadJSON(`./content/${id.split('/').map(encodeURIComponent).join('/')}.json`);
      state.content.set(id, segs);
      return segs;
    } catch (e) {
      return null;
    }
  }

  function renderMarkdownText(text) {
    const codes = [];
    let s = text.replace(/`([^`]+)`/g, (_, c) => { codes.push(c); return '§C' + (codes.length - 1) + '§'; });
    s = esc(s);
    s = s.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
    s = s.replace(/(^|[^*])\*([^*\s][^*]*)\*/g, '$1<em>$2</em>');
    s = s.replace(/\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g, (_, t, label) => {
      const id = t.replace(/\.md$/, '');
      if (state.byId.has(id)) return `<a href="#read/${encodeURI(id)}">${esc(label || id.split('/').pop())}</a>`;
      const hit = [...state.byId.values()].find((p) => p.id.endsWith('/' + id) || p.id === id);
      return hit ? `<a href="#read/${encodeURI(hit.id)}">${esc(label || hit.t)}</a>` : `<span class="wl">${esc(label || t)}</span>`;
    });
    s = s.replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, (m, label, href) => /^https?:/.test(href) ? `<a href="${href}" target="_blank" rel="noopener">${label}</a>` : `<span class="wl">${label}</span>`);
    s = s.replace(/§C(\d+)§/g, (_, i) => `<code>${esc(codes[Number(i)])}</code>`);
    return s;
  }

  function renderTextBlocks(text) {
    const lines = text.split('\n');
    const out = [];
    let para = [];
    let list = null;
    let quote = null;
    let table = null;
    const flush = () => {
      if (para.length) { out.push(`<p>${renderMarkdownText(para.join(' '))}</p>`); para = []; }
      if (list) { out.push(`<${list.tag}>${list.items.map((i) => `<li>${renderMarkdownText(i)}</li>`).join('')}</${list.tag}>`); list = null; }
      if (quote) { out.push(`<blockquote><p>${renderMarkdownText(quote.join(' '))}</p></blockquote>`); quote = null; }
      if (table) {
        out.push(`<div class="md-tw"><table><thead><tr>${table.head.map((c) => `<th>${renderMarkdownText(c)}</th>`).join('')}</tr></thead>
          <tbody>${table.rows.map((r) => `<tr>${r.map((c) => `<td>${renderMarkdownText(c)}</td>`).join('')}</tr>`).join('')}</tbody></table></div>`);
        table = null;
      }
    };
    lines.forEach((raw) => {
      const line = raw.replace(/\s+$/, '');
      const h = /^(#{1,6})\s+(.+)$/.exec(line);
      if (h) { flush(); out.push(`<h${h[1].length}>${renderMarkdownText(h[2])}</h${h[1].length}>`); return; }
      if (/^\s*>/.test(line)) { if (!quote) { flush(); quote = []; } quote.push(line.replace(/^\s*>\s?/, '')); return; }
      const li = /^\s*([-*+]|\d+\.)\s+(.+)$/.exec(line);
      if (li) { if (!list) { flush(); list = { tag: /\d/.test(li[1]) ? 'ol' : 'ul', items: [] }; } list.items.push(li[2]); return; }
      if (/^\|.*\|$/.test(line)) {
        const cells = line.trim().replace(/^\|/, '').replace(/\|$/, '').split('|').map((c) => c.trim());
        if (!table) { if (list || quote) flush(); table = { head: cells, rows: [] }; return; }
        if (/^[-: ]+$/.test(cells.join(''))) return;
        table.rows.push(cells); return;
      }
      if (/^(-{3,}|\*{3,})$/.test(line.trim())) { flush(); out.push('<hr>'); return; }
      if (!line.trim()) { flush(); return; }
      if (list || quote || table) flush();
      para.push(line.trim());
    });
    flush();
    return out.join('');
  }

  async function renderRead() {
    const page = state.byId.get(state.page) || state.index.pages[0];
    state.page = page.id;
    const landed = (page.layers.match(/1/g) || []).length;
    $('#main').innerHTML = `
      <div class="crumbs"><span>工作台</span><span>阅读</span><span class="cur">${esc(page.d)}</span></div>
      <div class="h-1"><h1>${esc(page.t)}</h1>
        <div class="meta">${esc(page.id.split('/').slice(0, -1).join('/'))}<br>${page.n} 行 · ${page.sec} 节 · ${landed}/7 层</div></div>
      <div class="tabs" role="tablist">
        <button role="tab" aria-selected="true">原文<span class="c">${page.dm} 图</span></button>
        <button role="tab" aria-selected="false" data-goto="#structure/${page.d}">结构<span class="c">S</span></button>
        <button role="tab" aria-selected="false" data-goto="#diagrams?page=${encodeURI(page.id)}">图表<span class="c">${page.dm}</span></button>
      </div>
      <div class="reader"><div class="reader-main"><article class="md" id="md"><div class="loading">载入正文…</div></article></div>
      <aside class="reader-aside"><h4>这张页</h4>
        <div style="font:500 11px var(--font-mono);color:var(--mut-2)">${esc(page.d)}</div>
        <div style="display:flex;gap:2px;margin:6px 0">${page.layers.split('').map((b) => `<i style="width:13px;height:9px;border:1px solid ${b === '1' ? 'var(--blue)' : 'var(--amber)'};background:${b === '1' ? 'var(--blue)' : 'var(--amber-wash)'}"></i>`).join('')}</div>
        <div style="font:11px/1.6 var(--font-mono);color:var(--mut-2)">${page.layers.split('').map((b, i) => LAYERS[i].slice(0, 2) + (b === '1' ? '✓' : '–')).join(' ')}</div>
        <h4>编码</h4>
        <div style="font:11px/1.6 var(--font-mono);color:var(--mut)">${esc(page.tier || '—')} · ${esc(page.cat || '—')} · ${esc(page.diff || '—')}</div>
        <h4>计数</h4>
        <div style="font:11px/1.6 var(--font-mono);color:var(--mut)">${page.dm} 图 · ${page.tb} 表行 · ${page.links} 链接</div>
        <h4>目录</h4><ul class="toc" id="toc"></ul>
      </aside></div>
      ${indexStrip()}`;
    bindCommon();

    const segs = await loadPageContent(page.id);
    const md = $('#md');
    if (!segs) { md.innerHTML = '<div class="empty"><b>正文未载入</b>运行 make console-sync 生成 console/content/。</div>'; return; }
    const parts = [];
    let mermaidSeq = 0;
    segs.forEach((seg) => {
      if (seg[0] === 't') parts.push(renderTextBlocks(seg[1]));
      else if (seg[0] === 'c') parts.push(`<pre data-lang="${esc(seg[1])}"><code>${esc(seg[2])}</code></pre>`);
      else if (seg[0] === 'm') {
        mermaidSeq += 1;
        state.dgCodes.set(parts.length, seg[3]);
        parts.push(`<div class="dg" data-idx="${parts.length}" data-kind="${esc(seg[2])}">
          <div class="dg-bar"><span class="k">${esc(KINDS[seg[2]] || seg[2])}</span><span>Mermaid · #${mermaidSeq}</span>
          <span>${esc(seg[4] && seg[4] !== seg[2] ? '源码写作 ' + seg[4] : '')}</span>
          <span class="big" data-big>放大</span></div>
          <div class="dg-svg"><div class="loading">渲染中…</div></div></div>`);

      } else if (seg[0] === 'd') {
        parts.push(`<div class="dg fail"><div class="dg-bar"><span class="k" style="color:var(--amber-text)">Mermaid 源码</span>
          <span>${esc(KINDS[seg[2]] || seg[2])} · 解析失败（${esc(seg[3])}）</span></div>
          <pre><code>${esc(seg[4])}</code></pre></div>`);
      }
    });
    md.innerHTML = parts.join('');
    renderAllDiagrams(md);

    const toc = $('#toc');
    const heads = [...md.querySelectorAll('h1,h2,h3')].slice(0, 60);
    heads.forEach((h, i) => {
      if (!h.id) h.id = 'h-' + i;
      const li = document.createElement('li');
      li.innerHTML = `<a href="#h-${i}" style="padding-left:${(Number(h.tagName[1]) - 1) * 8 + 10}px">${esc(h.textContent)}</a>`;
      li.querySelector('a').addEventListener('click', (e) => { e.preventDefault(); h.scrollIntoView({ block: 'start' }); });
      toc.appendChild(li);
    });
  }

  async function renderAllDiagrams(root) {
    const hosts = $$('.dg', root);
    for (const host of hosts) {
      const box = host.querySelector('.dg-svg');
      const code = state.dgCodes.get(Number(host.dataset.idx));
      try {
        await renderDiagram(box, code);
      } catch (e) {
        host.classList.add('fail');
        box.innerHTML = `<div class="dg-fallback"><div class="why">Mermaid 解析失败：${esc(String(e.message || e).slice(0, 180))}</div><pre>${esc(code)}</pre></div>`;
      }
      host.querySelector('[data-big]')?.addEventListener('click', async () => {
        const html = box.innerHTML;
        const w = window.open('', '_blank');
        if (w) { w.document.write(`<title>${esc(document.title)}</title><body style="margin:20px;background:#fff">${html}</body>`); w.document.close(); }
      });
    }
  }

  /* ------------------------------------------------------------ bindings */

  function bindCommon() {
    $$('.tabs button[data-view]').forEach((b) => b.addEventListener('click', () => go('#' + b.dataset.view + (b.dataset.view === 'structure' && state.domain ? '/' + state.domain : ''))));
    $$('[data-goto]').forEach((el) => el.addEventListener('click', (e) => {
      const href = el.getAttribute('data-goto');
      if (!href) return;
      if (el.tagName === 'a' && el.getAttribute('href') === href) return; // let the anchor handle it
      e.preventDefault();
      go(href);
    }));
    $$('[data-kind]').forEach((b) => b.addEventListener('click', () => {
      const k = b.dataset.kind;
      go('#diagrams?' + new URLSearchParams({ ...(k ? { kind: k } : {}), ...(state.dgDomain ? { domain: state.dgDomain } : {}) }).toString());
    }));
    $$('[data-domfilter]').forEach((b) => b.addEventListener('click', () => {
      const d = b.dataset.domfilter;
      go('#diagrams?' + new URLSearchParams({ ...(state.dgKind ? { kind: state.dgKind } : {}), ...(d ? { domain: d } : {}) }).toString());
    }));
  }

  function bindGlobal() {
    $$('.views a').forEach((a) => a.addEventListener('click', (e) => { e.preventDefault(); go('#' + a.dataset.view); }));
    $$('.nav-part').forEach((b) => b.addEventListener('click', () => {
      const part = b.dataset.part;
      if (state.openParts.has(part)) state.openParts.delete(part); else state.openParts.add(part);
      b.setAttribute('aria-expanded', String(state.openParts.has(part)));
      $(`[data-domains="${part}"]`).hidden = !state.openParts.has(part);
    }));
    $('#side-body').addEventListener('click', (e) => {
      const a = e.target.closest('.nav-dom');
      if (!a) return;
      e.preventDefault();
      state.domain = a.dataset.dom;
      go('#overview/' + encodeURIComponent(state.domain));
    });
    $('#side-search').addEventListener('input', (e) => filterNav(e.target.value));
    $('#side-search').addEventListener('focus', () => openPalette(true));
    $('#search').addEventListener('focus', () => openPalette(true));
    $('#search').addEventListener('click', () => openPalette(true));
    $('#top-search-btn').addEventListener('click', () => openPalette(true));
    $('#pal-input').addEventListener('input', (e) => drawPalette(e.target.value));
    $('#pal-input').addEventListener('keydown', palKeys);
    $('#pal-list').addEventListener('click', (e) => { const li = e.target.closest('li[data-href]'); if (li) go(li.dataset.href); });
    $('#palette').addEventListener('click', (e) => { if (e.target.id === 'palette') openPalette(false); });
    $('#menu-btn').addEventListener('click', () => {
      const open = !document.body.classList.contains('nav-open');
      document.body.classList.toggle('nav-open', open);
      $('#menu-btn').setAttribute('aria-expanded', String(open));
    });
    document.addEventListener('keydown', (e) => {
      const typing = /^(INPUT|TEXTAREA|SELECT)$/.test(document.activeElement.tagName);
      if ((e.key === 'k' || e.key === 'K') && (e.metaKey || e.ctrlKey)) { e.preventDefault(); openPalette(true); return; }
      if (e.key === '/' && !typing) { e.preventDefault(); openPalette(true); return; }
      if (!typing && !$('#palette').classList.contains('show') && /^[1234]$/.test(e.key)) {
        go('#' + ['overview', 'structure', 'diagrams', 'read'][Number(e.key) - 1]);
      }
      if (e.key === 'Escape') {
        openPalette(false);
        document.body.classList.remove('nav-open');
        $('#menu-btn').setAttribute('aria-expanded', 'false');
      }
    });
    window.addEventListener('hashchange', () => { route(); render(); });
  }

  function filterNav(q) {
    const s = q.trim().toLowerCase();
    state.domains.partitions.forEach((part) => {
      const box = $(`[data-domains="${part.id}"]`);
      if (!box) return;
      let shown = 0;
      $$('.nav-dom', box).forEach((a) => {
        const hit = !s || a.textContent.toLowerCase().includes(s);
        a.style.display = hit ? '' : 'none';
        if (hit) shown += 1;
      });
      if (s) { box.hidden = shown === 0; $(`.nav-part[data-part="${part.id}"]`).setAttribute('aria-expanded', String(shown > 0)); }
    });
  }

  /* -------------------------------------------------------- palette */

  let palIndex = 0;
  function paletteItems(q) {
    const s = q.trim().toLowerCase();
    const items = [];
    if (!s) {
      state.domains.partitions.forEach((p) => p.domains.forEach((d) => items.push({ kind: 'DOMAIN', label: d.name, sub: p.name + ' · ' + d.count + ' 页', href: '#overview/' + d.id })));
      return items.slice(0, 40);
    }
    for (const p of state.index.pages) {
      if (items.length > 4000) break;
      if (p.t.toLowerCase().includes(s) || p.id.toLowerCase().includes(s)) items.push({ kind: 'PAGE', label: p.t, sub: p.d + ' · ' + p.dm + ' 图', href: '#read/' + encodeURI(p.id) });
    }
    return items.slice(0, 60);
  }
  function openPalette(open) {
    const pal = $('#palette');
    if (open === pal.classList.contains('show')) return;
    pal.classList.toggle('show', open);
    pal.setAttribute('aria-hidden', String(!open));
    if (open) { $('#pal-input').value = ''; palIndex = 0; drawPalette(''); setTimeout(() => $('#pal-input').focus(), 0); }
  }
  function drawPalette(q) {
    const items = paletteItems(q);
    $('#pal-list').innerHTML = items.map((it, i) => `<li role="option" aria-selected="${i === palIndex}" data-href="${it.href}">
      <span class="k">${it.kind}</span><span>${esc(it.label)}</span><span class="s">${esc(it.sub)}</span></li>`).join('')
      || '<li><span class="k">—</span><span>没有匹配</span><span class="s"></span></li>';
  }
  function palKeys(e) {
    const items = paletteItems($('#pal-input').value);
    if (e.key === 'ArrowDown') { e.preventDefault(); palIndex = Math.min(palIndex + 1, items.length - 1); drawPalette($('#pal-input').value); }
    else if (e.key === 'ArrowUp') { e.preventDefault(); palIndex = Math.max(palIndex - 1, 0); drawPalette($('#pal-input').value); }
    else if (e.key === 'Enter' && items[palIndex]) { openPalette(false); go(items[palIndex].href); }
  }

  document.addEventListener('DOMContentLoaded', boot);
})();