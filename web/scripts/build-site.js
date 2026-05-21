#!/usr/bin/env node
/**
 * KUDIG Database Static Site Generator
 * Generates HTML pages from Markdown files for all 20 domains
 */

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import matter from 'gray-matter';
import { remark } from 'remark';
import remarkGfm from 'remark-gfm';
import remarkHtml from 'remark-html';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const REPO_ROOT = path.resolve(__dirname, '..', '..');
const OUTPUT_DIR = path.resolve(__dirname, '..', '..', 'site');

// Domain metadata
const DOMAIN_META = {
  'domain-01-cluster-fundamentals': { name: '集群基础架构', icon: '🏗️', desc: 'Kubernetes 集群架构、核心组件、API 机制' },
  'domain-02-workloads-applications': { name: '工作负载与应用', icon: '📦', desc: 'Pod、Deployment、StatefulSet、Job 等工作负载' },
  'domain-03-networking-traffic': { name: '网络与流量', icon: '🌐', desc: 'CNI、Service、Ingress、NetworkPolicy' },
  'domain-04-storage-data': { name: '存储与数据', icon: '💾', desc: 'PV、PVC、StorageClass、CSI、有状态应用' },
  'domain-05-security-compliance': { name: '安全与合规', icon: '🔒', desc: 'RBAC、NetworkPolicy、Secret、审计' },
  'domain-06-observability': { name: '可观测性', icon: '📊', desc: '监控、日志、追踪、告警体系' },
  'domain-07-platform-engineering': { name: '平台工程', icon: '⚙️', desc: 'GitOps、IaC、平台构建与交付' },
  'domain-08-release-change-management': { name: '发布与变更管理', icon: '🚀', desc: 'CI/CD、蓝绿部署、金丝雀发布' },
  'domain-09-reliability-engineering': { name: '可靠性工程', icon: '⛑️', desc: 'SLO/SLI、混沌工程、故障演练' },
  'domain-10-troubleshooting-diagnostics': { name: '故障诊断', icon: '🔧', desc: 'FTA、故障树、结构化排查、诊断技能' },
  'domain-11-production-operations': { name: '生产运维', icon: '🏭', desc: '生产最佳实践、SRE、运维手册' },
  'domain-12-cloud-providers': { name: '云服务商', icon: '☁️', desc: 'AWS、GCP、Azure、阿里云等云厂商' },
  'domain-13-container-runtime': { name: '容器运行时', icon: '🐳', desc: 'Docker、containerd、CRI、镜像管理' },
  'domain-14-ai-ml-infra': { name: 'AI/ML 基础设施', icon: '🤖', desc: 'GPU 调度、训练框架、推理服务' },
  'domain-15-specialized-tech': { name: '专项技术', icon: '🔬', desc: 'WebAssembly、eBPF、边缘计算等' },
  'domain-16-database-middleware': { name: '数据库与中间件', icon: '🗄️', desc: '数据库、缓存、消息队列' },
  'domain-17-system-foundation': { name: '系统基础', icon: '📚', desc: 'Linux、网络基础、存储基础' },
  'domain-18-manifests-patterns': { name: '清单与模式', icon: '📋', desc: 'YAML、Helm、Kustomize、设计模式' },
  'domain-19-landscape-references': { name: '全景与参考', icon: '🗺️', desc: 'CNCF 全景图、开源项目、论文' },
  'domain-20-application-patterns': { name: '应用模式', icon: '📐', desc: '微服务、Serverless、云原生应用' },
};

const EXCLUDE_DIRS = new Set([
  '.git', 'node_modules', 'site', 'web', '.obsidian',
  '_reports', '_raw', '_staging', '_archives', '_meta',
  'assets', 'corpus-config', '.ruff_cache', '.venv',
  '.claude', '.codebuddy', '.comate', '.understand-anything',
  '.wiki-meta', '.zread'
]);

// Scan all markdown files
function scanDocs() {
  const docs = [];
  
  function scanDir(dir, basePath = '') {
    let entries;
    try {
      entries = fs.readdirSync(dir, { withFileTypes: true });
    } catch (e) {
      return;
    }
    
    for (const entry of entries) {
      const relativePath = basePath ? `${basePath}/${entry.name}` : entry.name;
      const fullPath = path.join(dir, entry.name);
      
      if (entry.isDirectory()) {
        if (EXCLUDE_DIRS.has(entry.name) || entry.name.startsWith('.')) {
          continue;
        }
        scanDir(fullPath, relativePath);
      } else if (entry.isFile() && entry.name.endsWith('.md')) {
        // Only include domain files and root index/MOC
        if (!basePath) {
          if (!entry.name.startsWith('domain-') && entry.name !== 'index.md' && entry.name !== 'MOC.md') {
            continue;
          }
        }
        
        const content = fs.readFileSync(fullPath, 'utf-8');
        let parsed;
        try {
          parsed = matter(content);
        } catch (e) {
          console.warn(`  Skip (bad frontmatter): ${relativePath}`);
          continue;
        }
        
        const slug = relativePath.replace(/\.md$/, '').replace(/\/index$/, '');
        
        docs.push({
          slug,
          filePath: relativePath,
          fullPath,
          frontmatter: parsed.data,
          content: parsed.content,
          title: parsed.data.title || path.basename(relativePath, '.md').replace(/-/g, ' '),
        });
      }
    }
  }
  
  scanDir(REPO_ROOT);
  return docs.sort((a, b) => a.filePath.localeCompare(b.filePath));
}

// Group docs by domain
function groupByDomain(docs) {
  const groups = {};
  for (const doc of docs) {
    const domain = doc.slug.split('/')[0];
    if (!groups[domain]) {
      groups[domain] = [];
    }
    groups[domain].push(doc);
  }
  return groups;
}

// Render markdown to HTML
async function renderMarkdown(content) {
  const result = await remark()
    .use(remarkGfm)
    .use(remarkHtml, { allowDangerousHtml: true })
    .process(content);
  return String(result);
}

// Transform wikilinks
function transformWikilinks(content) {
  return content.replace(
    /\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g,
    (match, target, display) => {
      const text = display || target;
      return `<a href="#" class="wikilink">${text}</a>`;
    }
  );
}

// HTML template for pages
function pageTemplate({ title, description, domain, breadcrumb, htmlContent, domainNav, allDomains }) {
  const domainMeta = DOMAIN_META[domain] || { name: domain, icon: '📄', desc: '' };
  
  const domainsNav = Object.entries(allDomains)
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([slug, meta]) => `
      <a href="/kudig-database/${slug}/" class="domain-link ${slug === domain ? 'active' : ''}">
        <span class="domain-icon">${meta.icon}</span>
        <span class="domain-name">${meta.name}</span>
      </a>
    `).join('');

  return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta name="description" content="${description || title}">
<title>${title} — KUDIG Database</title>
<style>
:root {
  --bg: #0f172a;
  --card: #1e293b;
  --border: #334155;
  --text: #f1f5f9;
  --muted: #94a3b8;
  --primary: #3b82f6;
  --primary-light: #60a5fa;
}
* { margin: 0; padding: 0; box-sizing: border-box; }
html { scroll-behavior: smooth; }
body {
  font-family: 'Inter', system-ui, -apple-system, sans-serif;
  background: var(--bg);
  color: var(--text);
  line-height: 1.6;
  min-height: 100vh;
}

/* Header */
.header {
  position: sticky; top: 0; z-index: 100;
  background: rgba(15, 23, 42, 0.9);
  backdrop-filter: blur(12px);
  border-bottom: 1px solid var(--border);
}
.header-inner {
  max-width: 1400px; margin: 0 auto;
  padding: 0 24px;
  display: flex; align-items: center; justify-content: space-between;
  height: 64px;
}
.logo {
  display: flex; align-items: center; gap: 12px;
  text-decoration: none; color: var(--text);
}
.logo-mark {
  width: 36px; height: 36px;
  background: var(--primary);
  border-radius: 8px;
  display: flex; align-items: center; justify-content: center;
  font-weight: 700; font-size: 18px;
}
.logo-text { font-weight: 700; font-size: 16px; }
.logo-sub { font-size: 12px; color: var(--muted); }

/* Layout */
.layout {
  max-width: 1400px; margin: 0 auto;
  display: flex;
  min-height: calc(100vh - 64px);
}

/* Sidebar */
.sidebar {
  width: 280px; flex-shrink: 0;
  border-right: 1px solid var(--border);
  padding: 24px 0;
  overflow-y: auto;
  max-height: calc(100vh - 64px);
  position: sticky; top: 64px;
}
.sidebar-title {
  padding: 0 20px 16px;
  font-size: 12px; font-weight: 600;
  text-transform: uppercase; letter-spacing: 0.05em;
  color: var(--muted);
}
.domain-link {
  display: flex; align-items: center; gap: 10px;
  padding: 8px 20px;
  color: var(--muted); text-decoration: none;
  font-size: 14px;
  transition: all 0.15s;
}
.domain-link:hover {
  background: rgba(51, 65, 85, 0.3);
  color: var(--text);
}
.domain-link.active {
  background: rgba(59, 130, 246, 0.1);
  color: var(--primary-light);
  border-right: 2px solid var(--primary);
}
.domain-icon { font-size: 18px; }
.domain-name { white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }

/* Main content */
.main {
  flex: 1; min-width: 0;
  padding: 32px 48px;
  max-width: 900px;
}

/* Breadcrumb */
.breadcrumb {
  display: flex; align-items: center; gap: 8px;
  font-size: 14px; color: var(--muted);
  margin-bottom: 24px;
  flex-wrap: wrap;
}
.breadcrumb a {
  color: var(--muted); text-decoration: none;
}
.breadcrumb a:hover { color: var(--primary-light); }
.breadcrumb-sep { opacity: 0.5; }
.breadcrumb-current { color: var(--text); }

/* Prose */
.prose h1 {
  font-size: 32px; font-weight: 700;
  margin-bottom: 24px; padding-bottom: 16px;
  border-bottom: 1px solid var(--border);
}
.prose h2 {
  font-size: 24px; font-weight: 600;
  margin-top: 32px; margin-bottom: 16px;
  color: var(--text);
}
.prose h3 {
  font-size: 20px; font-weight: 600;
  margin-top: 24px; margin-bottom: 12px;
}
.prose p { margin-bottom: 16px; color: #cbd5e1; }
.prose ul, .prose ol {
  margin-bottom: 16px; padding-left: 24px;
}
.prose ul { list-style: disc; }
.prose ol { list-style: decimal; }
.prose li { margin-bottom: 4px; }
.prose a {
  color: var(--primary-light); text-decoration: none;
}
.prose a:hover { text-decoration: underline; }
.prose blockquote {
  border-left: 3px solid var(--primary);
  padding: 12px 16px; margin: 16px 0;
  background: var(--card); border-radius: 0 8px 8px 0;
  color: var(--muted);
}
.prose code {
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  font-size: 0.9em;
  background: var(--card);
  padding: 2px 6px; border-radius: 4px;
  color: var(--primary-light);
}
.prose pre {
  background: var(--card);
  padding: 16px; border-radius: 8px;
  overflow-x: auto; margin: 16px 0;
  border: 1px solid var(--border);
}
.prose pre code {
  background: none; padding: 0;
  color: #e2e8f0;
}
.prose table {
  width: 100%; border-collapse: collapse;
  margin: 16px 0; font-size: 14px;
}
.prose th, .prose td {
  border: 1px solid var(--border);
  padding: 10px 12px; text-align: left;
}
.prose th {
  background: var(--card); font-weight: 600;
}
.prose tr:nth-child(even) { background: rgba(30, 41, 59, 0.3); }
.prose hr { border: none; border-top: 1px solid var(--border); margin: 32px 0; }
.prose img { max-width: 100%; height: auto; border-radius: 8px; }

/* Footer */
.footer {
  border-top: 1px solid var(--border);
  padding: 24px 48px;
  text-align: center;
  font-size: 13px; color: var(--muted);
}

/* Scrollbar */
::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: var(--bg); }
::-webkit-scrollbar-thumb { background: var(--border); border-radius: 3px; }
::-webkit-scrollbar-thumb:hover { background: #475569; }

/* Wikilinks */
.wikilink {
  color: var(--primary-light);
  text-decoration: none;
  border-bottom: 1px dashed var(--primary);
}

/* Responsive */
@media (max-width: 1024px) {
  .sidebar { display: none; }
  .main { padding: 24px; max-width: none; }
}
</style>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
</head>
<body>
<header class="header">
  <div class="header-inner">
    <a href="/kudig-database/" class="logo">
      <div class="logo-mark">K</div>
      <div>
        <div class="logo-text">KUDIG Database</div>
        <div class="logo-sub">Kubernetes 生产运维全域知识库</div>
      </div>
    </a>
    <a href="https://github.com/kudig-io/kudig-database" target="_blank" style="color: var(--muted); text-decoration: none; font-size: 14px;">
      GitHub →
    </a>
  </div>
</header>

<div class="layout">
  <aside class="sidebar">
    <div class="sidebar-title">20 个知识域</div>
    ${domainsNav}
  </aside>
  
  <main class="main">
    <nav class="breadcrumb">
      <a href="/kudig-database/">首页</a>
      ${breadcrumb.map((crumb, i) => `
        <span class="breadcrumb-sep">/</span>
        ${i === breadcrumb.length - 1 
          ? `<span class="breadcrumb-current">${crumb.label}</span>`
          : `<a href="${crumb.href}">${crumb.label}</a>`
        }
      `).join('')}
    </nav>
    
    <article class="prose">
      ${htmlContent}
    </article>
  </main>
</div>

<footer class="footer">
  <p>© 2024-2026 KUDIG Team — Built with Astro & Node.js</p>
</footer>

<script src="https://unpkg.com/mermaid@10/dist/mermaid.min.js"></script>
<script>
  mermaid.initialize({
    theme: 'dark',
    themeVariables: {
      primaryColor: '#1e293b',
      primaryTextColor: '#f1f5f9',
      primaryBorderColor: '#3b82f6',
      lineColor: '#94a3b8',
    }
  });
  if (document.querySelector('.mermaid')) {
    mermaid.run({ querySelector: '.mermaid' });
  }
</script>
</body>
</html>`;
}

// Home page template
function homeTemplate({ domains, totalDocs }) {
  const domainCards = Object.entries(domains)
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([slug, meta]) => `
      <a href="/kudig-database/${slug}/" class="card">
        <div class="card-header">
          <span class="card-icon">${meta.icon}</span>
          <span class="card-count">${meta.count} 篇</span>
        </div>
        <h3 class="card-title">${meta.name}</h3>
        <p class="card-desc">${meta.desc}</p>
      </a>
    `).join('');

  return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta name="description" content="面向生产环境的 Kubernetes + AI Infrastructure 运维全域知识库，覆盖 20 个核心知识域">
<title>KUDIG Database — Kubernetes 生产运维全域知识库</title>
<style>
:root {
  --bg: #0f172a; --card: #1e293b; --border: #334155;
  --text: #f1f5f9; --muted: #94a3b8; --primary: #3b82f6;
}
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
  font-family: 'Inter', system-ui, sans-serif;
  background: var(--bg); color: var(--text); line-height: 1.6;
}
.header {
  position: sticky; top: 0; z-index: 100;
  background: rgba(15, 23, 42, 0.9); backdrop-filter: blur(12px);
  border-bottom: 1px solid var(--border);
}
.header-inner {
  max-width: 1200px; margin: 0 auto; padding: 0 24px;
  display: flex; align-items: center; justify-content: space-between; height: 64px;
}
.logo { display: flex; align-items: center; gap: 12px; text-decoration: none; color: var(--text); }
.logo-mark { width: 36px; height: 36px; background: var(--primary); border-radius: 8px;
  display: flex; align-items: center; justify-content: center; font-weight: 700; font-size: 18px; }
.logo-text { font-weight: 700; font-size: 16px; }
.logo-sub { font-size: 12px; color: var(--muted); }

.hero {
  text-align: center; padding: 80px 24px;
  background: linear-gradient(135deg, rgba(59, 130, 246, 0.1) 0%, transparent 50%);
}
.badge {
  display: inline-flex; align-items: center; gap: 8px;
  padding: 8px 16px; background: rgba(59, 130, 246, 0.1);
  border: 1px solid rgba(59, 130, 246, 0.2); border-radius: 999px;
  color: var(--primary); font-size: 14px; margin-bottom: 24px;
}
.badge-dot { width: 8px; height: 8px; background: var(--primary); border-radius: 50%; animation: pulse 2s infinite; }
@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }
h1 { font-size: 48px; font-weight: 700; margin-bottom: 20px; line-height: 1.2; }
h1 span { color: var(--primary); }
.hero-desc { font-size: 18px; color: var(--muted); max-width: 600px; margin: 0 auto 32px; }
.btn {
  display: inline-block; padding: 12px 28px; border-radius: 8px;
  text-decoration: none; font-weight: 500; transition: all 0.2s;
}
.btn-primary { background: var(--primary); color: white; }
.btn-primary:hover { background: #2563eb; }
.btn-secondary { background: var(--card); color: var(--text); border: 1px solid var(--border); margin-left: 12px; }
.btn-secondary:hover { background: var(--border); }

.stats {
  display: grid; grid-template-columns: repeat(4, 1fr); gap: 24px;
  max-width: 800px; margin: 0 auto; padding: 32px 24px;
  border-top: 1px solid var(--border); border-bottom: 1px solid var(--border);
}
.stat { text-align: center; }
.stat-value { font-size: 32px; font-weight: 700; color: var(--primary); }
.stat-label { font-size: 14px; color: var(--muted); margin-top: 4px; }

.domains { max-width: 1200px; margin: 0 auto; padding: 64px 24px; }
.domains-header { text-align: center; margin-bottom: 48px; }
.domains-header h2 { font-size: 32px; margin-bottom: 12px; }
.domains-header p { color: var(--muted); font-size: 16px; }

.grid {
  display: grid; grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 20px;
}
.card {
  background: var(--card); border: 1px solid var(--border); border-radius: 12px;
  padding: 24px; text-decoration: none; color: var(--text);
  transition: all 0.2s;
}
.card:hover {
  border-color: rgba(59, 130, 246, 0.4);
  transform: translateY(-2px);
  box-shadow: 0 8px 30px rgba(0, 0, 0, 0.2);
}
.card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px; }
.card-icon { font-size: 32px; }
.card-count { font-size: 12px; color: var(--muted); background: var(--bg); padding: 4px 10px; border-radius: 999px; }
.card-title { font-size: 18px; font-weight: 600; margin-bottom: 8px; }
.card-desc { font-size: 14px; color: var(--muted); line-height: 1.5; }

.footer { text-align: center; padding: 32px; border-top: 1px solid var(--border); color: var(--muted); font-size: 14px; }

@media (max-width: 768px) {
  h1 { font-size: 32px; }
  .stats { grid-template-columns: repeat(2, 1fr); }
  .grid { grid-template-columns: 1fr; }
}
</style>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
</head>
<body>
<header class="header">
  <div class="header-inner">
    <a href="/kudig-database/" class="logo">
      <div class="logo-mark">K</div>
      <div>
        <div class="logo-text">KUDIG Database</div>
        <div class="logo-sub">Kubernetes 生产运维全域知识库</div>
      </div>
    </a>
    <a href="https://github.com/kudig-io/kudig-database" target="_blank" style="color: var(--muted); text-decoration: none; font-size: 14px;">GitHub →</a>
  </div>
</header>

<section class="hero">
  <div class="badge"><span class="badge-dot"></span> v2.0 — 20 域全新架构</div>
  <h1>Kubernetes 生产运维<br><span>全域知识库</span></h1>
  <p class="hero-desc">覆盖集群架构、工作负载、网络、存储、安全、可观测性、AI/ML 基础设施等 20 个核心知识域，面向生产环境的系统性运维知识体系。</p>
  <div>
    <a href="#domains" class="btn btn-primary">浏览知识域</a>
    <a href="/kudig-database/MOC/" class="btn btn-secondary">导航地图</a>
  </div>
</section>

<section class="stats">
  <div class="stat"><div class="stat-value">20</div><div class="stat-label">知识域</div></div>
  <div class="stat"><div class="stat-value">${totalDocs.toLocaleString()}</div><div class="stat-label">知识文档</div></div>
  <div class="stat"><div class="stat-value">4,600+</div><div class="stat-label">Markdown 页面</div></div>
  <div class="stat"><div class="stat-value">24×7</div><div class="stat-label">持续更新</div></div>
</section>

<section class="domains" id="domains">
  <div class="domains-header">
    <h2>20 个核心知识域</h2>
    <p>从集群基础架构到 AI/ML 基础设施，系统化覆盖 Kubernetes 全栈知识体系</p>
  </div>
  <div class="grid">
    ${domainCards}
  </div>
</section>

<footer class="footer">
  <p>© 2024-2026 KUDIG Team — Built with Astro & Node.js</p>
</footer>
</body>
</html>`;
}

// Domain index page template
function domainIndexTemplate({ domain, docs, allDomains }) {
  const meta = DOMAIN_META[domain] || { name: domain, icon: '📄', desc: '' };
  
  // Group docs by subdirectory
  const groups = {};
  for (const doc of docs) {
    const parts = doc.slug.replace(domain + '/', '').split('/');
    const group = parts.length > 1 ? parts[0] : '(root)';
    if (!groups[group]) groups[group] = [];
    groups[group].push(doc);
  }
  
  const groupHtml = Object.entries(groups)
    .sort(([a], [b]) => a === '(root)' ? -1 : b === '(root)' ? 1 : a.localeCompare(b))
    .map(([group, groupDocs]) => `
      <div class="doc-group">
        <h3>${group === '(root)' ? '文档' : group.replace(/-/g, ' ').replace(/^topic-/, '')}</h3>
        <ul class="doc-list">
          ${groupDocs.map(d => `
            <li><a href="/kudig-database/${d.slug}/">${d.title}</a></li>
          `).join('')}
        </ul>
      </div>
    `).join('');

  const domainsNav = Object.entries(allDomains)
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([slug, dmeta]) => `
      <a href="/kudig-database/${slug}/" class="domain-link ${slug === domain ? 'active' : ''}">
        <span class="domain-icon">${dmeta.icon}</span>
        <span class="domain-name">${dmeta.name}</span>
      </a>
    `).join('');

  return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>${meta.name} — KUDIG Database</title>
<style>
:root {
  --bg: #0f172a; --card: #1e293b; --border: #334155;
  --text: #f1f5f9; --muted: #94a3b8; --primary: #3b82f6;
}
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: 'Inter', system-ui, sans-serif; background: var(--bg); color: var(--text); line-height: 1.6; }
.header { position: sticky; top: 0; z-index: 100; background: rgba(15, 23, 42, 0.9); backdrop-filter: blur(12px); border-bottom: 1px solid var(--border); }
.header-inner { max-width: 1400px; margin: 0 auto; padding: 0 24px; display: flex; align-items: center; justify-content: space-between; height: 64px; }
.logo { display: flex; align-items: center; gap: 12px; text-decoration: none; color: var(--text); }
.logo-mark { width: 36px; height: 36px; background: var(--primary); border-radius: 8px; display: flex; align-items: center; justify-content: center; font-weight: 700; font-size: 18px; }
.layout { max-width: 1400px; margin: 0 auto; display: flex; min-height: calc(100vh - 64px); }
.sidebar { width: 280px; flex-shrink: 0; border-right: 1px solid var(--border); padding: 24px 0; overflow-y: auto; max-height: calc(100vh - 64px); position: sticky; top: 64px; }
.sidebar-title { padding: 0 20px 16px; font-size: 12px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.05em; color: var(--muted); }
.domain-link { display: flex; align-items: center; gap: 10px; padding: 8px 20px; color: var(--muted); text-decoration: none; font-size: 14px; transition: all 0.15s; }
.domain-link:hover { background: rgba(51, 65, 85, 0.3); color: var(--text); }
.domain-link.active { background: rgba(59, 130, 246, 0.1); color: #60a5fa; border-right: 2px solid var(--primary); }
.domain-icon { font-size: 18px; }
.main { flex: 1; min-width: 0; padding: 32px 48px; max-width: 900px; }
.domain-header { display: flex; align-items: center; gap: 16px; margin-bottom: 32px; padding-bottom: 24px; border-bottom: 1px solid var(--border); }
.domain-header-icon { font-size: 48px; }
.domain-header-info h1 { font-size: 28px; margin-bottom: 8px; }
.domain-header-info p { color: var(--muted); font-size: 16px; }
.doc-group { margin-bottom: 32px; }
.doc-group h3 { font-size: 18px; font-weight: 600; margin-bottom: 12px; color: var(--primary); text-transform: capitalize; }
.doc-list { list-style: none; padding: 0; }
.doc-list li { margin-bottom: 8px; }
.doc-list a { color: var(--text); text-decoration: none; display: block; padding: 8px 12px; background: var(--card); border-radius: 6px; border: 1px solid var(--border); transition: all 0.15s; }
.doc-list a:hover { border-color: var(--primary); background: rgba(59, 130, 246, 0.05); }
.breadcrumb { display: flex; align-items: center; gap: 8px; font-size: 14px; color: var(--muted); margin-bottom: 24px; }
.breadcrumb a { color: var(--muted); text-decoration: none; }
.breadcrumb a:hover { color: var(--primary); }
.footer { border-top: 1px solid var(--border); padding: 24px 48px; text-align: center; font-size: 13px; color: var(--muted); }
@media (max-width: 1024px) { .sidebar { display: none; } .main { padding: 24px; max-width: none; } }
::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: var(--bg); }
::-webkit-scrollbar-thumb { background: var(--border); border-radius: 3px; }
</style>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
</head>
<body>
<header class="header">
  <div class="header-inner">
    <a href="/kudig-database/" class="logo">
      <div class="logo-mark">K</div>
      <div><div style="font-weight:700;font-size:16px;">KUDIG Database</div></div>
    </a>
    <a href="https://github.com/kudig-io/kudig-database" target="_blank" style="color:var(--muted);text-decoration:none;font-size:14px;">GitHub →</a>
  </div>
</header>
<div class="layout">
  <aside class="sidebar">
    <div class="sidebar-title">20 个知识域</div>
    ${domainsNav}
  </aside>
  <main class="main">
    <nav class="breadcrumb">
      <a href="/kudig-database/">首页</a>
      <span>/</span>
      <span style="color:var(--text);">${meta.name}</span>
    </nav>
    <div class="domain-header">
      <span class="domain-header-icon">${meta.icon}</span>
      <div class="domain-header-info">
        <h1>${meta.name}</h1>
        <p>${meta.desc} — 共 ${docs.length} 篇文档</p>
      </div>
    </div>
    ${groupHtml}
  </main>
</div>
<footer class="footer"><p>© 2024-2026 KUDIG Team</p></footer>
</body>
</html>`;
}

// Main build function
async function build() {
  console.log('🚀 KUDIG Database Static Site Generator');
  console.log('========================================');
  
  // Clean output directory
  console.log('\n📁 Cleaning output directory...');
  if (fs.existsSync(OUTPUT_DIR)) {
    fs.rmSync(OUTPUT_DIR, { recursive: true });
  }
  fs.mkdirSync(OUTPUT_DIR, { recursive: true });
  
  // Scan docs
  console.log('\n🔍 Scanning markdown files...');
  const docs = scanDocs();
  console.log(`   Found ${docs.length} markdown files`);
  
  // Group by domain
  const domainGroups = groupByDomain(docs);
  const domainSlugs = Object.keys(domainGroups).sort();
  console.log(`   Found ${domainSlugs.length} domains`);
  
  // Prepare domain metadata with counts
  const allDomains = {};
  for (const slug of domainSlugs) {
    allDomains[slug] = {
      ...DOMAIN_META[slug],
      count: domainGroups[slug].length,
    };
  }
  
  // Build home page
  console.log('\n🏠 Building home page...');
  const homeHtml = homeTemplate({ domains: allDomains, totalDocs: docs.length });
  fs.writeFileSync(path.join(OUTPUT_DIR, 'index.html'), homeHtml);
  
  // Build domain index pages and doc pages
  let processed = 0;
  const total = docs.length + domainSlugs.length;
  
  for (const domain of domainSlugs) {
    const domainDocs = domainGroups[domain];
    
    // Build domain index page
    const domainIndexHtml = domainIndexTemplate({
      domain,
      docs: domainDocs,
      allDomains,
    });
    const domainIndexPath = path.join(OUTPUT_DIR, domain, 'index.html');
    fs.mkdirSync(path.dirname(domainIndexPath), { recursive: true });
    fs.writeFileSync(domainIndexPath, domainIndexHtml);
    processed++;
    
    // Build doc pages
    for (const doc of domainDocs) {
      const transformed = transformWikilinks(doc.content);
      const htmlContent = await renderMarkdown(transformed);
      
      // Build breadcrumb
      const parts = doc.slug.split('/');
      const breadcrumb = parts.map((part, i) => ({
        label: part.replace(/-/g, ' '),
        href: `/kudig-database/${parts.slice(0, i + 1).join('/')}/`,
      }));
      
      const pageHtml = pageTemplate({
        title: doc.title,
        description: doc.frontmatter.description || doc.title,
        domain,
        breadcrumb,
        htmlContent,
        domainNav: '',
        allDomains,
      });
      
      const outputPath = path.join(OUTPUT_DIR, doc.slug, 'index.html');
      fs.mkdirSync(path.dirname(outputPath), { recursive: true });
      fs.writeFileSync(outputPath, pageHtml);
      
      processed++;
      if (processed % 500 === 0) {
        console.log(`   ${processed}/${total} pages built...`);
      }
    }
  }
  
  console.log(`\n✅ Build complete!`);
  console.log(`   Total pages: ${processed}`);
  console.log(`   Output: ${OUTPUT_DIR}`);
}

build().catch(err => {
  console.error('❌ Build failed:', err);
  process.exit(1);
});
