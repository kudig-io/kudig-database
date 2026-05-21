import fs from 'node:fs';
import path from 'node:path';
import matter from 'gray-matter';

const REPO_ROOT = path.resolve(process.cwd(), '..');

export function scanDocs() {
  const docs = [];
  let count = 0;
  
  function scanDir(dir, basePath = '') {
    if (count >= 50) return;
    
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    
    for (const entry of entries) {
      if (count >= 50) return;
      
      const relativePath = basePath ? `${basePath}/${entry.name}` : entry.name;
      const fullPath = path.join(dir, entry.name);
      
      if (entry.isDirectory()) {
        if (entry.name.startsWith('.') || 
            ['node_modules', 'site', 'web', '.git', '_reports', '_raw', '_staging', 'assets', 'corpus-config', '_archives'].includes(entry.name)) {
          continue;
        }
        scanDir(fullPath, relativePath);
      } else if (entry.isFile() && entry.name.endsWith('.md')) {
        if (!basePath && !entry.name.startsWith('domain-') && entry.name !== 'index.md' && entry.name !== 'MOC.md') {
          continue;
        }
        
        const content = fs.readFileSync(fullPath, 'utf-8');
        let parsed;
        try {
          parsed = matter(content);
        } catch (e) {
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
          category: parsed.data.category || 'other',
        });
        
        count++;
      }
    }
  }
  
  scanDir(REPO_ROOT);
  return docs.sort((a, b) => a.filePath.localeCompare(b.filePath));
}

export function groupByDomain(docs) {
  const groups = {};
  for (const doc of docs) {
    const domain = doc.slug.split('/')[0];
    if (!groups[domain]) {
      groups[domain] = { domain, docs: [], count: 0 };
    }
    groups[domain].docs.push(doc);
    groups[domain].count++;
  }
  return groups;
}

export function getDomainMeta(domainSlug) {
  const domainMap = {
    'domain-01-cluster-fundamentals': { name: '集群基础架构', icon: '🏗️', color: 'blue' },
    'domain-02-workloads-applications': { name: '工作负载与应用', icon: '📦', color: 'green' },
    'domain-03-networking-traffic': { name: '网络与流量', icon: '🌐', color: 'cyan' },
    'domain-04-storage-data': { name: '存储与数据', icon: '💾', color: 'purple' },
    'domain-05-security-compliance': { name: '安全与合规', icon: '🔒', color: 'red' },
    'domain-06-observability': { name: '可观测性', icon: '📊', color: 'orange' },
    'domain-07-platform-engineering': { name: '平台工程', icon: '⚙️', color: 'slate' },
    'domain-08-release-change-management': { name: '发布与变更管理', icon: '🚀', color: 'indigo' },
    'domain-09-reliability-engineering': { name: '可靠性工程', icon: '⛑️', color: 'rose' },
    'domain-10-troubleshooting-diagnostics': { name: '故障诊断', icon: '🔧', color: 'amber' },
    'domain-11-production-operations': { name: '生产运维', icon: '🏭', color: 'emerald' },
    'domain-12-cloud-providers': { name: '云服务商', icon: '☁️', color: 'sky' },
    'domain-13-container-runtime': { name: '容器运行时', icon: '🐳', color: 'teal' },
    'domain-14-ai-ml-infra': { name: 'AI/ML 基础设施', icon: '🤖', color: 'violet' },
    'domain-15-specialized-tech': { name: '专项技术', icon: '🔬', color: 'fuchsia' },
    'domain-16-database-middleware': { name: '数据库与中间件', icon: '🗄️', color: 'pink' },
    'domain-17-system-foundation': { name: '系统基础', icon: '📚', color: 'zinc' },
    'domain-18-manifests-patterns': { name: '清单与模式', icon: '📋', color: 'stone' },
    'domain-19-landscape-references': { name: '全景与参考', icon: '🗺️', color: 'neutral' },
    'domain-20-application-patterns': { name: '应用模式', icon: '📐', color: 'lime' },
  };
  return domainMap[domainSlug] || { name: domainSlug, icon: '📄', color: 'gray' };
}

export async function renderMarkdown(content) {
  const { remark } = await import('remark');
  const remarkGfm = await import('remark-gfm');
  const remarkHtml = await import('remark-html');
  
  const result = await remark()
    .use(remarkGfm.default)
    .use(remarkHtml.default, { allowDangerousHtml: true })
    .process(content);
  
  return String(result);
}

export function transformWikilinks(content) {
  return content.replace(
    /\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g,
    (match, target, display) => {
      const text = display || target;
      const slug = target.trim().toLowerCase().replace(/\s+/g, '-').replace(/[^a-z0-9\-\/]/g, '');
      return `[${text}](/kudig-database/${slug})`;
    }
  );
}

export function buildNavTree(docs, domain) {
  const domainDocs = docs.filter(d => d.slug.startsWith(domain + '/'));
  const tree = {};
  
  for (const doc of domainDocs) {
    const parts = doc.slug.replace(domain + '/', '').split('/');
    let current = tree;
    
    for (let i = 0; i < parts.length; i++) {
      const part = parts[i];
      if (i === parts.length - 1) {
        current[part] = { ...doc, isFile: true };
      } else {
        if (!current[part]) {
          current[part] = { children: {}, isDir: true, name: part };
        }
        current = current[part].children;
      }
    }
  }
  
  return tree;
}
