---
title: Wiki Frontmatter Schema
description: Wiki Frontmatter Schema — Kubernetes 生产运维知识库
category: references
tags:
- schema
- metadata
- frontmatter
- llm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Wiki Frontmatter Schema 是什么
- 如何 Wiki Frontmatter Schema
trigger_keywords:
- Wiki
- Frontmatter
- Schema
prerequisites:
- kubectl-basics
---

# Wiki Schema

This document defines the conventions for the compiled wiki.
It co-evolves: the agent proposes changes, the user approves.

## Page Categories

| Category | Directory | What goes here |
|---|---|---|
| **Entities** | `wiki/entities/` | People, systems, projects, organizations, tools |
| **Concepts** | `wiki/concepts/` | Ideas, patterns, technologies, methods, algorithms |
| **Syntheses** | `wiki/syntheses/` | Cross-cutting summaries, comparisons, trend analyses |
| **Sources** | `wiki/sources/` | One page per ingested source: summary + metadata |
| **Reports** | `wiki/reports/` | Agent-generated dashboards (don't edit manually) |

## Naming Conventions

- Filenames: `kebab-case.md` using ASCII characters only (e.g., `reinforcement-learning.md`)
- Titles: Title Case in frontmatter `title:` field

## Wikilink Conventions

- **Obsidian resolves wikilinks by FILENAME only** — not by frontmatter title or aliases
- Always use `[[filename|Display Title]]` format
- Example: `[[convolutional-neural-network|Convolutional Neural Network]]`
- For sections: `[[filename#Section|Display Text]]`
- Every entity/concept mentioned should have a corresponding page (or be on the "missing pages" list)
- Run the skill's lint/fix flow after bulk page creation to auto-fix wikilink format and markdown structure issues

## Frontmatter Required Fields

Use simple unquoted `key: value` format — no quoted keys, no block scalars,
no multiline values. Tags and aliases use inline `[a, b]` arrays.

```yaml
title: <string>
type: entity | concept | synthesis | source | report
tags: [<from taxonomy.md>]
sources: [<raw file paths>]
created: YYYY-MM-DD
updated: YYYY-MM-DD
```

Special pages `index.md` and `log.md` use `type: index` and `type: log` respectively.
Special pages (`index.md`, `log.md`) use reduced frontmatter: only `title`, `type`, and `updated` are required. They are excluded from orphan detection and content-page lint checks.

## Optional Frontmatter

```yaml
confidence: high | medium | low
status: active | review | stale | archived
aliases: [<alternative names>]
```

## Source Attribution

- Frontmatter `sources:` lists which raw files contributed
- Inline `%%from: raw/path/to/source.md%%` for paragraph-level attribution
- Inline `%%inferred%%` for LLM synthesis across multiple sources
- Inline `%%ambiguous: explanation%%` when sources disagree
- **Do NOT use `^[...]`** — that is Obsidian's inline footnote syntax

## Content Guidelines

- **Be concise.** Wiki pages are reference material, not essays.
- **Lead with the definition.** First sentence should define the entity/concept.
- **Use bullet lists** for structured information.
- **Cross-reference aggressively.** If a concept is mentioned, link it.
- **Track open questions.** Every page can have a `## Open Questions` section.
- **Note contradictions explicitly.** Don't silently pick a side.

## Evolution

When you (the agent) identify a convention that should change:
1. Propose the change in conversation
2. If approved, update this schema
3. Log the schema change in `wiki/log.md`
