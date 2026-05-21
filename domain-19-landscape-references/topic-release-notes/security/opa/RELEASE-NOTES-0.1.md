---
title: opa v0.1 Release Notes
description: opa v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.1 Release Notes 是什么
- 如何 opa v0.1 Release Notes
trigger_keywords:
- opa
- v0.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.1 Release Notes

Source: [v0.1.0](https://github.com/open-policy-agent/opa/releases/tag/v0.1.0)

### Language
- Basic value types: null, boolean, number, string, object, and array
- Reference and variables types
- Array comprehensions
- Built-in functions for basic arithmetic and aggregation
- Incremental and complete rule definitions
- Negation and disjunction
- Module system

### Compiler
- Reference resolver to support packages
- Safety check to prevent recursive rules
- Safety checks to ensure successful evaluation will bind all variables in head
  and body of rules

### Evaluation
- Initial top down query evaluation algorithm

### Storage
- Basic in-memory storage that exposes JSON Patch style API

### Tooling
- REPL that can be run to experiment with ad-hoc queries

### APIs
- Server mode supports HTTP APIs to manage policies, push and query documents, and execute ad-hoc queries.

### Infrastructure
- Basic build infrastructure to produce cross-platform builds, run
  style/lint/format checks, execute tests, static HTML site

### Documentation
- Introductions to policy, policy-enabling, and how OPA works
- Language reference that serves as guide for new users
- Tutorial that introduces users to the REPL
- Tutorial that introduces users to policy-enabling with a Docker Authorization plugin
