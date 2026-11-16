/* KUDIG Console · 原型脚本
   三个结构视图（拆解树 / 一致性热力 / 对比矩阵）+ 证据抽屉 + 回到原文定位行。
   所有数字来自 ../data/graph/blocks.json、../data/index.json、../data/domains.json。 */
(() => {
  'use strict';
  const $ = (s, r = document) => r.querySelector(s);
  const $$ = (s, r = document) => Array.from(r.querySelectorAll(s));
  const esc = (s) => String(s ?? '').replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  const FM = /^---\n[\s\S]*?\n---\n/;

  //: Dimension columns come from the controlled vocabulary (console/spec/dimensions.json).
  //: Until it loads, a small default keeps the page from flashing an empty header.
  let DIMS = [['d-troubleshoot', '排障'], ['d-monitor', '监控'], ['d-backup', '备份']];
  const DIM_SPLIT = 14; // dimensions shown in the heat header
  const MATRIX_DIMS = [['d-backup', '备份恢复'], ['d-monitor', '监控告警'], ['d-ha', '高可用 / 故障转移']];
  const SHAPE_NAME = {
    'index-like': '索引页', prose: '纯叙述', 'table-heavy': '表格为主', 'ops-heavy': '操作手册', structured: '结构化',
  };

  const S = { index: null, domains: null, blocks: null, byId: new Map(), domain: null, view: 'tree', page: null, content: new Map(),
            entities: null, entityGroup: 'database' };

  async function load(url) {
    const r = await fetch(url, { cache: 'no-cache' });
    if (!r.ok) throw new Error(`${url} ${r.status}`);
    return r.json();
  }

  async function boot() {
    try {
      const [index, domains, blocks, vocab, entities] = await Promise.all([
        load('../data/index.json'), load('../data/domains.json'), load('../data/graph/blocks.json'),
        load('../../spec/dimensions.json').catch(() => null),
        load('../data/graph/entities.json').catch(() => null),
      ]);
      S.entities = entities;
      if (vocab && Array.isArray(vocab.dimensions)) {
        DIMS = vocab.dimensions.map((d) => [d.id, d.name]);
        S.dimVersion = vocab.version;
      }
      Object.assign(S, { index, domains, blocks });
      index.pages.forEach((p) => S.byId.set(p.id, p));
    } catch (e) {
      $('#main').innerHTML = `<div class="panel"><div class="body">语料未载入：${esc(e.message)}<br>
        请先在仓库根目录运行 <code>make console-serve</code>（原型页通过 HTTP 读取 ../data/）。</div></div>`;
      return;
    }
    renderSide();
    try {
      route();
      render();
    } catch (err) {
      $('#main').innerHTML = `<div class="panel"><div class="body"><b>渲染失败</b><pre style="white-space:pre-wrap">${esc(err && (err.stack || err.message || err))}</pre></div></div>`;
    }
    window.__protoReady = true;
  }

  /* ---------------------------------------------------------------- side */
  function renderSide() {
    const partId = S.partition || 'wiki';
    const part = S.domains.partitions.find((p) => p.id === partId) || S.domains.partitions[0];
    $('#part-switch').innerHTML = S.domains.partitions.map((p) =>
      `<button data-part="${p.id}" aria-pressed="${p.id === part.id}">${esc(p.name)}<em>${p.count}</em></button>`).join('');
    $$('#part-switch button').forEach((b) => b.addEventListener('click', () => {
      S.partition = b.dataset.part;
      renderSide();
    }));
    $('#side-list').innerHTML = part.domains
      .slice().sort((a, b) => b.count - a.count)
      .map((d) => `<a class="dom" href="#/${encodeURIComponent(d.id)}" data-dom="${esc(d.id)}">
        <span>${esc(d.name)}</span><em>${d.count}</em></a>`).join('');
    $('#side-title').textContent = `${part.name} · ${part.domains.length} 域`;
    $('#side-count').textContent = part.count;
    const f = $('#side-filter');
    f.oninput = () => {
      const q = f.value.trim().toLowerCase();
      $$('.dom').forEach((a) => { a.style.display = !q || a.textContent.toLowerCase().includes(q) ? '' : 'none'; });
    };
  }

  function pagesOf(domain) {
    return Object.keys(S.blocks.pages).filter((k) => k.startsWith(domain + '/')).sort();
  }

  function route() {
    const seg = (location.hash || '#/').replace(/^#\/?/, '').split('/').filter(Boolean).map((x) => decodeURIComponent(x));
    // Domain keys are 2 segments under wiki/ and 3 under skills/ & friends
    // (skills/NN-x/sub). Build both so the router can match either shape.
    const allDomains = [...new Set(Object.keys(S.blocks.pages).flatMap((k) => {
      const p = k.split('/');
      return p[0] === 'wiki' ? [p.slice(0, 2).join('/')] : [p.slice(0, 3).join('/')];
    }))];

    // Hashes come in two shapes:
    //   #/<domain>/<view>              — domain percent-encoded as ONE segment ("wiki%2F05-网络")
    //   #/<partition>/<a>/<b>/<view>   — domain split across raw segments
    // Try the single-segment form first, then widen the leading run one segment at a time.
    const VIEWS = ['tree', 'heat', 'matrix', 'page'];
    let dom = null;
    let rest = [];
    for (let k = 1; k <= 3 && !dom; k += 1) {
      if (seg.length <= k) break;
      if (VIEWS.includes(seg[k - 1])) break;
      // The whole prefix must name the domain itself; never widen past it, or "wiki/05-网络"
      // silently becomes "wiki/05-网络/00-总览" (a real sub-domain that also has pages).
      const cand = seg.slice(0, k).join('/');
      if (allDomains.includes(cand)) { dom = cand; rest = seg.slice(k); }
    }
    if (!dom && VIEWS.includes(seg[0])) rest = seg; // view-only hash, keep the default domain
    if (!dom) dom = allDomains.find((d) => d.includes('07-数据库中间件')) || allDomains.sort()[0];

    S.domain = dom;
    const viewSeg = (rest[0] || '').split('?');
    S.view = viewSeg[0] && ['tree', 'heat', 'matrix', 'page'].includes(viewSeg[0]) ? viewSeg[0] : 'tree';
    S.page = S.view === 'page' && rest[1] ? rest.slice(1).join('/').split('?')[0] : null;
    const q = new URLSearchParams(viewSeg[1] || '');
    if (q.get('group')) S.entityGroup = q.get('group');
  }

  function go(hash) { history.replaceState(null, '', hash); route(); render(); }

  /* --------------------------------------------------------------- render */
  function render() {
    $$('.views a').forEach((a) => a.setAttribute('aria-current', a.dataset.view === S.view || (S.view === 'page' && a.dataset.view === 'tree') ? 'page' : 'false'));
    $$('.dom').forEach((a) => a.setAttribute('aria-current', a.dataset.dom === S.domain));
    if (S.view === 'heat') renderHeat();
    else if (S.view === 'matrix') renderMatrix();
    else if (S.view === 'page') renderPage();
    else renderTree();
    $('main').scrollTop = 0;
  }

  function head(title, sub) {
    const pages = pagesOf(S.domain);
    return `<div class="crumbs"><span>原型</span><span>${esc(S.domain)}</span><span>${esc(title)}</span></div>
      <h1>${esc(title)} <span>· ${esc(S.domain.split('/').pop())}</span></h1>
      <div class="sub">${esc(sub)} · ${pages.length} 页</div>
      <div class="tabs" role="tablist">
        <button data-tab="tree" aria-selected="${S.view !== 'heat' && S.view !== 'matrix'}">拆解树<span class="c">${pages.length}</span></button>
        <button data-tab="heat" aria-selected="${S.view === 'heat'}">一致性热力<span class="c">${DIMS.length} 维</span></button>
        <button data-tab="matrix" aria-selected="${S.view === 'matrix'}">对比矩阵<span class="c">4 产品</span></button>
      </div>`;
  }

  /* ----------------------------------------------------- 拆解树 (domain) */
  function renderTree() {
    const pages = pagesOf(S.domain);
    const rows = pages.map((pid) => {
      const b = S.blocks.pages[pid];
      const p = S.byId.get(pid) || {};
      const landed = (p.layers || '').split('').filter((x) => x === '1').length;
      const bars = Array.from({ length: 7 }, (_, i) => `<i style="height:${(p.layers || '')[i] === '1' ? 12 : 3}px;opacity:${(p.layers || '')[i] === '1' ? '.8' : '.25'}"></i>`).join('');
      return `<tr>
        <td><a href="#/${encodeURIComponent(S.domain)}/page/${encodeURI(pid)}">${esc(p.t || pid.split('/').pop())}</a></td>
        <td class="num">${esc(SHAPE_NAME[b.shape] || b.shape)}</td>
        <td class="num">${b.h2}/${b.h3}</td>
        <td class="num"><span class="tb ${b.tables.length ? 'has' : ''}">表 ${b.tables.length}</span><span class="tb ${b.codes.length ? 'has' : ''}">码 ${b.codes.length}</span></td>
        <td><span class="spark">${bars}</span></td>
        <td class="num">${landed}/7</td>
        <td class="anchor">${b.outline.length ? 'L' + b.outline[0].s : '—'}</td>
      </tr>`;
    }).join('');
    $('#main').innerHTML = head('拆解树', '每页的骨架：H2/H3 数量、表与代码载荷、七层落锁') + `
      <div class="panel"><header><span class="n">OUTLINE</span><h2>域的页骨架</h2>
        <span class="aside">点页名 → 单页拆解；锚点为正文行号</span></header>
        <div class="body"><table class="tree"><thead><tr>
          <th>页面</th><th>页形</th><th>H2 / H3</th><th>载荷</th><th>七层</th><th>落锁</th><th>首节</th>
        </tr></thead><tbody>${rows}</tbody></table></div></div>
      ${foot('拆解树')}`;
    bindTabs();
  }

  /* --------------------------------------------------------- 单页拆解 */
  function renderPage() {
    const pid = S.page;
    const b = S.blocks.pages[pid];
    if (!b) { $('#main').innerHTML = head('单页拆解', '未找到该页') + '<div class="panel"><div class="body">该页暂无 blocks 数据。</div></div>'; bindTabs(); return; }
    const p = S.byId.get(pid) || {};
    const rows = b.outline.map((o) => `
      <tr class="l${o.l}" data-line="${o.s}" data-title="${esc(o.t)}">
        <td>${esc(o.t)}</td>
        <td class="num">H${o.l}</td>
        <td class="num">${o.tb ? '<span class="tb has">表 ' + o.tb + '</span>' : ''}${o.cd ? '<span class="tb has">码 ' + o.cd + '</span>' : ''}</td>
        <td class="num">${o.e - o.s + 1} 行</td>
        <td class="anchor">L${o.s}–${o.e}</td>
      </tr>`).join('');
    const dims = b.dims.map((d) => `<span class="tb has" title="L${d.line}">${esc(d.name)}${d.strength === 'subsection' ? ' · H3' : ''}</span>`).join('') || '<span class="tb">无命中</span>';
    $('#main').innerHTML = head('单页拆解', p.t || pid) + `
      <div class="panel"><header><span class="n">PAGE</span><h2>${esc(p.t || pid)}</h2>
        <span class="aside">${esc(SHAPE_NAME[b.shape] || b.shape)} · ${b.lines} 行 · ${esc(pid.split('/').slice(-1)[0])}</span></header>
        <div class="body">
          <dl class="kv" style="display:grid;grid-template-columns:120px 1fr;gap:6px 12px;font-size:12.5px">
            <dt style="font:500 11px var(--font-mono);color:var(--mut-2)">七层</dt><dd>${(p.layers || '').split('').map((x, i) => 'L' + i + (x === '1' ? '✓' : '–')).join(' ')}</dd>
            <dt style="font:500 11px var(--font-mono);color:var(--mut-2)">维度</dt><dd>${dims}</dd>
          </dl>
          <table class="tree"><thead><tr><th>小节</th><th>级</th><th>载荷</th><th>跨度</th><th>锚点</th></tr></thead>
          <tbody>${rows}</tbody></table>
        </div></div>
      ${foot('单页拆解')}`;
    bindTabs();
    $$('tbody tr[data-line]').forEach((tr) => tr.addEventListener('click', () => openEvidence(pid, Number(tr.dataset.line), tr.dataset.title)));
  }

  /* ------------------------------------------------------------- 热力 */
  function renderHeat() {
    const pages = pagesOf(S.domain);
    const all = pages.filter((pid) => {
      const b = S.blocks.pages[pid];
      return b && b.shape !== 'index-like';
    });
    // domain-level density decides whether the full vocabulary fits on screen
    const dense = all.filter((pid) => (S.blocks.pages[pid].dims || []).length >= 4).length;
    const cols = dense / Math.max(all.length, 1) >= 0.35 ? DIMS : DIMS.slice(0, DIM_SPLIT);
    const rows = all;
    const body = rows.map((pid) => {
      const p = S.byId.get(pid) || {};
      const b = S.blocks.pages[pid];
      const map = new Map(b.dims.map((d) => [d.id, d]));
      const cells = cols.map(([id, name]) => {
        const d = map.get(id);
        if (!d) return `<td><div class="cell gap" data-gap="1" data-pid="${esc(pid)}" data-dim="${id}" data-name="${esc(name)}">–</div></td>`;
        return `<td><div class="cell ${d.strength}" data-pid="${esc(pid)}" data-line="${d.line}" data-dim="${id}" data-title="${esc(d.title)}">
          ${d.strength === 'subsection' ? '<span class="k">H3</span>' : '<span class="k">H2</span>'}</div></td>`;
      }).join('');
      return `<tr><td><a href="#/${encodeURIComponent(S.domain)}/page/${encodeURI(pid)}">${esc((p.t || pid).slice(0, 34))}</a></td>${cells}</tr>`;
    }).join('');
    const totals = cols.map(([id]) => {
      const n = rows.filter((pid) => (S.blocks.pages[pid].dims || []).some((d) => d.id === id)).length;
      return `<th title="${n} 页">${n}</th>`;
    }).join('');
    $('#main').innerHTML = head('一致性热力', '同一维度在这一域里是成节、仅子节，还是没写') + `
      <div class="panel"><header><span class="n">HEAT</span><h2>维度 × 页面</h2>
        <span class="aside">实底 H2 = 一级标题成节 · 浅底 H3 = 仅在子节 · 虚线 = 未成节</span></header>
        <div class="body" style="overflow:auto">
          <table class="heat"><thead><tr><th class="row">页面</th>${cols.map(([, n]) => `<th>${n}</th>`).join('')}</tr>
          <tr><th class="row" style="font-size:11px">命中页数</th>${totals}</tr></thead><tbody>${body}</tbody></table>
        </div></div>
      ${foot('一致性热力')}`;
    bindTabs();
    $$('.cell').forEach((c) => {
      if (c.dataset.gap) {
        c.addEventListener('click', () => showToast(`「${c.dataset.name}」在该页未成节（不是缺失，是没写）`));
        return;
      }
      c.addEventListener('click', () => openEvidence(c.dataset.pid, Number(c.dataset.line), c.dataset.title));
    });
  }

  /* ------------------------------------------------------------- 矩阵 */
  /* ------------------------------------------------------------- 矩阵（实体 × 维度组） */

  //: Each column is a dimension GROUP, resolved against the vocabulary by id prefix/name.
  const COLUMNS = [
    { key: 'backup', label: '备份恢复', dims: ['d-backup'], prefer: ['备份', '恢复'] },
    { key: 'monitor', label: '监控告警', dims: ['d-monitor'], prefer: ['监控', '告警'] },
    { key: 'ha', label: '高可用', dims: ['d-ha'], prefer: ['故障转移', '节点替换', '副本集', '高可用拓扑', '高可用与拓扑'] },
    { key: 'deploy', label: '部署', dims: ['d-deploy'], prefer: ['部署', '安装'] },
    { key: 'perf', label: '性能与容量', dims: ['d-perf', 'd-scale', 'd-storage'], prefer: ['性能', '调优', '扩缩容', '存储', '容量'] },
    { key: 'trouble', label: '排障', dims: ['d-troubleshoot'], prefer: ['故障', '排查', '排障'] },
  ];

  function groupEntities(groupKey) {
    if (!S.entities) return [];
    return (S.entities.entities || []).filter((e) => e.group === groupKey);
  }

  /** Pick the section that best answers a column for one entity. */
  function anchorFor(ent, col) {
    const dims = ent.dims || {};
    const hits = [];
    col.dims.forEach((id) => {
      const d = dims[id];
      if (!d) return;
      hits.push({ ...d, dim: id });
    });
    if (!hits.length) return null;
    hits.sort((a, b) => {
      const rank = (h) => {
        const i = col.prefer.findIndex((k) => (h.title || '').includes(k));
        return i === -1 ? 99 : i;
      };
      return rank(a) - rank(b);
    });
    return { ...hits[0], alt: hits.length - 1 };
  }

  function renderMatrix() {
    if (!S.entities) {
      $('#main').innerHTML = head('对比矩阵', '实体索引未生成') + '<div class="panel"><div class="body">运行 make console-sync 生成 entities.json。</div></div>';
      bindTabs(); return;
    }
    const groups = S.entities.groups || {};
    const groupKey = groups[S.entityGroup] ? S.entityGroup : Object.keys(groups)[0];
    S.entityGroup = groupKey;
    const ents = groupEntities(groupKey);
    const rows = ents.map((ent) => {
      const tds = COLUMNS.map((col) => {
        const hit = anchorFor(ent, col);
        if (!hit) return `<td class="absent" data-state="absent" data-ent="${esc(ent.id)}">未成节</td>`;
        const neq = hit.alt > 0 ? `<span class="neq" title="另有 ${hit.alt} 节相关，取第一个">+${hit.alt}</span>`
          : (hit.strength === 'subsection' ? '<span class="neq" title="命中在子节">≠</span>' : '');
        return `<td><span class="sec-title">${esc(hit.title || '')}</span>
          <span class="anchor" data-pid="${esc(ent.page)}" data-line="${hit.line}" role="button" tabindex="0">L${hit.line}</span>${neq}</td>`;
      }).join('');
      const noPage = ent.page ? '' : ' <em class="muted" title="语料中没有成节的产品页">(仅 FTA)</em>';
      const filled = COLUMNS.filter((col) => anchorFor(ent, col)).length;
      const cover = `<span class="cov ${filled === 0 ? 'z' : (filled >= 4 ? 'hi' : '')}" title="该实体在语料中覆盖的维度组">${filled}/${COLUMNS.length}</span>`;
      return `<tr><td class="ent">${esc(ent.name)}${noPage} ${cover}<div class="entd">${esc((ent.page || '').split('/').slice(-1)[0] || '')}</div></td>${tds}</tr>`;
    }).join('');
    const switches = Object.entries(groups).map(([k, g]) =>
      `<button class="chipbtn" aria-pressed="${k === groupKey}" data-group="${k}">${esc(g.name)}<span class="c">${g.members.length}</span></button>`).join('');
    const filled = rows ? (rows.match(/class="anchor"/g) || []).length : 0;
    $('#main').innerHTML = head('对比矩阵', '同一维度组下，同组实体各自写在哪一节') + `
      <div class="panel"><header><span class="n">MATRIX</span><h2>${esc(groups[groupKey].name)} · ${ents.length} 个实体</h2>
        <span class="aside">每格 = 命中小节 + 行号锚点 · 空格为语料未写</span></header>
        <div class="body">
          <div class="groupswitch">${switches}</div>
          <div style="overflow:auto">
          <table class="mx"><thead><tr><th>实体</th>${COLUMNS.map((c) => `<th>${c.label}</th>`).join('')}</tr></thead>
          <tbody>${rows}</tbody></table>
          </div>
          <div class="legend"><span><i class="full"></i>H2 成节</span><span><i class="sub"></i>H3 子节（≠）</span><span><i class="gap"></i>未成节</span></div>
        </div></div>
      ${foot('对比矩阵')}`;
    bindTabs();
    $$('.groupswitch button').forEach((b) => b.addEventListener('click', () => {
      S.entityGroup = b.dataset.group;
      history.replaceState(null, '', `#/${encodeURIComponent(S.domain)}/matrix?group=${b.dataset.group}`);
      renderMatrix();
    }));
    $$('.anchor[data-line]').forEach((a) => {
      const open = () => openEvidence(a.dataset.pid, Number(a.dataset.line), '');
      a.addEventListener('click', open);
      a.addEventListener('keydown', (e) => { if (e.key === 'Enter') open(); });
    });
    $$('td.absent').forEach((td) => td.addEventListener('click', () => showToast('该实体页没有这一维度的成节内容（H2/H3 都未命中）')));
  }

  function foot(what) {
    return `<div class="foot">原型 · ${esc(what)} · 数据 <code>console/data/graph/blocks.json</code> · 口径见 <code>console/spec/data-contract.md</code></div>`;
  }

  function bindTabs() {
    $$('.tabs button[data-tab]').forEach((b) => b.addEventListener('click', () => go(`#/${encodeURIComponent(S.domain)}/${b.dataset.tab}`)));
  }

  /* ------------------------------------------------------------ 抽屉 */
  async function loadContent(pid) {
    if (S.content.has(pid)) return S.content.get(pid);
    const r = await fetch(`../content/${pid}.json`, { cache: 'no-cache' });
    if (!r.ok) return null;
    const segs = await r.json();
    S.content.set(pid, segs);
    return segs;
  }

  async function openEvidence(pid, line, title) {
    const d = $('#drawer');
    const p = S.byId.get(pid) || {};
    const b = S.blocks.pages[pid] || {};
    const node = (b.outline || []).find((o) => o.s === line);
    d.dataset.open = 'true';
    d.setAttribute('aria-hidden', 'false');
    $('#drawer-title').textContent = title || node?.t || '证据';
    $('#drawer-body').innerHTML = `
      <dl class="kv">
        <dt>页面</dt><dd>${esc(p.t || pid)}</dd>
        <dt>路径</dt><dd style="font:400 11px var(--font-mono);word-break:break-all">${esc(pid)}</dd>
        <dt>锚点</dt><dd><span class="anchor">L${line}</span>（正文行号，去 frontmatter）</dd>
        <dt>小节</dt><dd>${node ? esc(node.t) : '—'} ${node ? `· 至 L${node.e}` : ''}</dd>
        <dt>来源</dt><dd>blocks.json outline（${b.shape || '—'}）</dd>
      </dl>
      <div style="display:flex;gap:10px;margin-bottom:14px">
        <button id="goto-src" style="border:1px solid var(--ink);background:var(--blue);color:#fff;padding:6px 12px;cursor:pointer;font:500 11.5px var(--font-ui)">定位原文</button>
        <button id="copy-link" style="border:1px solid var(--rule);background:var(--vellum);padding:6px 12px;cursor:pointer;font:500 11.5px var(--font-ui)">复制链接</button>
      </div>
      <div id="drawer-snippet" style="font:400 11.5px/1.7 var(--font-mono);color:var(--mut)">载入原文…</div>`;
    $('#copy-link').addEventListener('click', () => {
      navigator.clipboard?.writeText(`#read/${pid}?at=L${line}`).then(() => showToast('已复制：' + `#read/${pid}?at=L${line}`), () => showToast('复制失败'));
    });
    $('#goto-src').addEventListener('click', () => showSource(pid, line));
    const raw = await fetch(`/__src/${pid}.md`).then((r) => (r.ok ? r.text() : null)).catch(() => null);
    const box = $('#drawer-snippet');
    if (!raw) { box.textContent = '原文不可读（原型经 HTTP 读取仓库文件，需 make console-serve）'; return; }
    const body = raw.replace(FM, '').split('\n');
    const from = Math.max(0, line - 1);
    box.innerHTML = body.slice(from, from + 8)
      .map((l, i) => `<div class="ln ${i === 0 ? 'hit' : ''}"><b>${line + i}</b><span>${esc(l) || '&nbsp;'}</span></div>`).join('');
  }

  async function showSource(pid, line) {
    const raw = await fetch(`/__src/${pid}.md`).then((r) => (r.ok ? r.text() : null)).catch(() => null);
    const v = $('#reader');
    v.dataset.open = 'true';
    $('#reader-title').textContent = (S.byId.get(pid) || {}).t || pid;
    if (!raw) { $('#reader-body').textContent = '原文不可读'; return; }
    const lines = raw.replace(FM, '').split('\n');
    $('#reader-body').innerHTML = lines.map((l, i) => `<div class="ln ${i + 1 === line ? 'hit' : ''}" id="ln${i + 1}"><b>${i + 1}</b><span>${esc(l) || '&nbsp;'}</span></div>`).join('');
    const target = $(`#ln${line}`);
    if (target) target.scrollIntoView({ block: 'center' });
  }

  function showToast(msg) {
    const t = $('#toast');
    t.textContent = msg;
    t.classList.add('show');
    clearTimeout(showToast._t);
    showToast._t = setTimeout(() => t.classList.remove('show'), 2600);
  }

  document.addEventListener('DOMContentLoaded', () => {
    boot();
    window.addEventListener('hashchange', () => { route(); render(); });
    $('#drawer-close').addEventListener('click', () => { $('#drawer').dataset.open = 'false'; });
    $('#reader-close').addEventListener('click', () => { $('#reader').dataset.open = 'false'; });
    document.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') { $('#drawer').dataset.open = 'false'; $('#reader').dataset.open = 'false'; }
    });
  });
})();