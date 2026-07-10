import fs from 'node:fs';
import path from 'node:path';
import matter from 'gray-matter';
import { remark } from 'remark';
import remarkGfm from 'remark-gfm';
import remarkHtml from 'remark-html';

const REPO_ROOT = path.resolve(process.cwd(), '..');

// 20 个中文知识域名（目录重命名后用于替代 domain- 前缀检测）
export const DOMAIN_DIRS = new Set([
  '集群基础', '工作负载', '网络', '存储', '安全', '可观测性',
  '平台工程', '发布变更', '可靠性', '故障诊断', '生产运维', '云厂商',
  '容器运行时', 'AI基础设施', '专项技术', '数据库中间件', '系统基础',
  '清单模式', '生态参考', '应用模式',
]);

/**
 * Scan all markdown files in the repository root
 */
export function scanDocs() {
  const docs = [];
  
  function scanDir(dir, basePath = '') {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    
    for (const entry of entries) {
      const relativePath = basePath ? `${basePath}/${entry.name}` : entry.name;
      const fullPath = path.join(dir, entry.name);
      
      // Skip non-content directories (aligned with STRUCTURE.md corpus-exclusion rules).
      if (entry.isDirectory()) {
        if (entry.name.startsWith('.') ||
            entry.name === 'node_modules' ||
            entry.name === 'site' ||
            entry.name === 'web' ||
            entry.name === 'assets' ||
            entry.name === 'corpus-config' ||
            entry.name === '_reports' ||
            entry.name === '_archives' ||
            entry.name === '_meta' ||
            entry.name === '_raw' ||
            entry.name === '_staging' ||
            entry.name === 'man' ||
            entry.name === 'scripts' ||
            entry.name === 'templates' ||
            entry.name === 'prompts' ||
            entry.name === 'video-scripts' ||
            entry.name === 'release-notes' ||
            entry.name === 'reports') {
          continue;
        }
        scanDir(fullPath, relativePath);
      } else if (entry.isFile() && entry.name.endsWith('.md')) {
        // Skip loose root-level files (only allow index.md, MOC.md, and files inside domain dirs)
        if (!basePath && entry.name !== 'index.md' && entry.name !== 'MOC.md') {
          continue;
        }
        
        const content = fs.readFileSync(fullPath, 'utf-8');
        let parsed;
        try {
          parsed = matter(content);
        } catch (e) {
          // Skip files with invalid frontmatter
          console.warn(`Skipping ${relativePath}: ${e.message.split('\n')[0]}`);
          continue;
        }
        
        // Generate slug from path
        const slug = relativePath
          .replace(/\.md$/, '')
          .replace(/\/index$/, '');
        
        docs.push({
          slug,
          filePath: relativePath,
          fullPath,
          frontmatter: parsed.data,
          content: parsed.content,
          title: parsed.data.title || path.basename(relativePath, '.md').replace(/-/g, ' '),
          category: parsed.data.category || inferCategory(relativePath),
        });
      }
    }
  }
  
  scanDir(REPO_ROOT);
  return docs.sort((a, b) => a.filePath.localeCompare(b.filePath));
}

function inferCategory(filePath) {
  const parts = filePath.split('/');
  if (DOMAIN_DIRS.has(parts[0])) {
    return parts[0];
  }
  return 'other';
}

/**
 * Group docs by domain
 */
export function groupByDomain(docs) {
  const groups = {};
  
  for (const doc of docs) {
    const domain = doc.slug.split('/')[0];
    if (!groups[domain]) {
      groups[domain] = {
        domain,
        docs: [],
        count: 0,
      };
    }
    groups[domain].docs.push(doc);
    groups[domain].count++;
  }
  
  return groups;
}

/**
 * Get domain metadata
 */
export function getDomainMeta(domainSlug) {
  const domainMap = {
    '集群基础': { name: '集群基础架构', icon: '🏗️', color: 'blue' },
    '工作负载': { name: '工作负载与应用', icon: '📦', color: 'green' },
    '网络': { name: '网络与流量', icon: '🌐', color: 'cyan' },
    '存储': { name: '存储与数据', icon: '💾', color: 'purple' },
    '安全': { name: '安全与合规', icon: '🔒', color: 'red' },
    '可观测性': { name: '可观测性', icon: '📊', color: 'orange' },
    '平台工程': { name: '平台工程', icon: '⚙️', color: 'slate' },
    '发布变更': { name: '发布与变更管理', icon: '🚀', color: 'indigo' },
    '可靠性': { name: '可靠性工程', icon: '⛑️', color: 'rose' },
    '故障诊断': { name: '故障诊断', icon: '🔧', color: 'amber' },
    '生产运维': { name: '生产运维', icon: '🏭', color: 'emerald' },
    '云厂商': { name: '云服务商', icon: '☁️', color: 'sky' },
    '容器运行时': { name: '容器运行时', icon: '🐳', color: 'teal' },
    'AI基础设施': { name: 'AI/ML 基础设施', icon: '🤖', color: 'violet' },
    '专项技术': { name: '专项技术', icon: '🔬', color: 'fuchsia' },
    '数据库中间件': { name: '数据库与中间件', icon: '🗄️', color: 'pink' },
    '系统基础': { name: '系统基础', icon: '📚', color: 'zinc' },
    '清单模式': { name: '清单与模式', icon: '📋', color: 'stone' },
    '生态参考': { name: '全景与参考', icon: '🗺️', color: 'neutral' },
    '应用模式': { name: '应用模式', icon: '📐', color: 'lime' },
  };
  
  return domainMap[domainSlug] || { name: domainSlug, icon: '📄', color: 'gray' };
}

/**
 * Render markdown to HTML
 */
export async function renderMarkdown(content) {
  const result = await remark()
    .use(remarkGfm)
    .use(remarkHtml, { allowDangerousHtml: true })
    .process(content);
  
  return String(result);
}

/**
 * Transform wikilinks to regular links.
 * 保留中文路径段（知识域名已是中文），仅清理文件名扩展名与空格。
 * [[故障诊断/topic-fta/README.md|显示名]] → [显示名](/kudig-database/故障诊断/topic-fta/README)
 */
export function transformWikilinks(content) {
  return content.replace(
    /\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g,
    (match, target, display) => {
      const text = display || target;
      const slug = target
        .trim()
        .replace(/\.md$/, '')      // 去掉末尾 .md
        .replace(/\/index$/, '')   // 去掉末尾 /index
        .replace(/\s+/g, '-');     // 空格转连字符
      return `[${text}](/kudig-database/${slug})`;
    }
  );
}

/**
 * Build navigation tree for a domain
 */
export function buildNavTree(docs, domain) {
  const domainDocs = docs.filter(d => d.slug.startsWith(domain + '/'));
  const tree = {};

  for (const doc of domainDocs) {
    const parts = doc.slug.replace(domain + '/', '').split('/');
    let current = tree;

    for (let i = 0; i < parts.length; i++) {
      const part = parts[i];
      const isLeaf = i === parts.length - 1;

      // 若该段不存在，或存在但缺少 children（例如先作为文件节点被创建），
      // 则补一个目录容器 children，避免后续 .children 访问 undefined。
      if (!current[part]) {
        current[part] = { children: {}, isDir: true, name: part };
      } else if (!current[part].children) {
        current[part].children = {};
        current[part].isDir = true;
      }

      if (isLeaf) {
        // 叶子节点：标记为文件并写入文档元数据，保留已有 children（若有更深层路径）。
        current[part] = { ...current[part], ...doc, isFile: true };
      } else {
        current = current[part].children;
      }
    }
  }

  return tree;
}
