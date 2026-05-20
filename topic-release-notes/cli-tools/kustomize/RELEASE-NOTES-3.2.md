---
title: kustomize v3.2 Release Notes
description: kustomize v3.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- elasticsearch
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kustomize v3.2 Release Notes 是什么
- 如何 kustomize v3.2 Release Notes
trigger_keywords:
- kustomize
- v3.2
- Release
- Notes
- release
- notes
---

# kustomize v3.2 Release Notes

Source: [v3.2.0](https://github.com/kubernetes-sigs/kustomize/releases/tag/v3.2.0)

## Changelog

f59d7998 Add an example of reusable builtin plugins with custom config.
3f1b2bb7 Add configs
aabbbf05 Add cover target to Makefile
5dfa9299 Add create subcommand
ed91bce2 Add example plugin for go-getter
7783a76b Add internal tooling library for index queries.
66fa2de0 Add main backend service and configurations
64341a81 Add short version flag
e898c522 Add test for name conflict with base reuse
aa2bf7ed Adds frontend + configs to interal/tools/ui
02f6b3ec Allow replicas to find modified names.
6a4150d1 Amend go-getter plugin document according to comments
963913f9 Automatically anchor resource selector patterns
1237ae43 Consider currentId when replacing/merging resources
c2d6f09e Crawler performance improvements, better structure
24c173a4 Detect ID conflicts in namespace transformer
dd5b3c1e Do not prefix/suffix APIService resources
2de052ec Download submodules when using base from git
2050afde Ease doing custom configuration of builtin plugins.
74ed0b30 Example of configuring builtin plugin.
351df67e First draft of documentation for internal/tools
44b62a8e Fix indirect git resource cycle detection
fa834f95 Fix non-travis tests.
8e9c08ea Fix patch path example
bafd6b54 Fix typo in patches definition
594a06d3 Fixes to create sub-command
adbb6228 Handle git:: prefix in urls containing _git
96c5b4aa Handle ordering patches with SMP delete directives
ca41674d Implementation of basic crawler organisation.
62edcae2 Implementation of configurable github crawler.
ac6918d7 Implementation of github query helper library.
e0d388c6 Implements search query partitioning by filesize.
c02b4f3a Initial (temporary) implementation of search doc.
2e6dd481 IsInKustomizeCtx should use end of nameprefix array (1/3)
6e13acfa IsInKustomizeCtx should use end of nameprefix array (2/3)
93cedbaa IsInKustomizeCtx should use end of nameprefix array (3/3)
31262ccc IsInKustomizeCtx should use end of nameprefix array (code review)
fe8ba8e4 Log loader errors during resource accumulation
54f19521 Log output from git on errors
df779fd7 Modify document for elasticsearch migration.
e904f612 Move commands/edit utils package up to commands
eeafd435 Remove import of k8sdeps from create command
a68f95b6 Rename commands utility function file
eaae7af5 Retain replicas field in edit marshal path
ed3c29be Simplify name reference candidate resmap building
ed920afb Support setting command in go-getter plugin
a0815349 Test custom configuration of a builtin plugin.
423a8a6e Test examples against HEAD as well as against latest release.
33bd221a Update README.md
fe45157b Update crawler to cache web request form github.
b4d6e89f Update zh-README.md
86f22161 Update zh-example-README.md
6c44da52 add PriorityClass to the order list
46905588 add document for inline patch (#1411)
95168800 add inline patch document
35481ec6 add inline patch support for Strategic Merge Patch and JSON patch
e6fffc8b add makefile
b4038a6c add testting for patch transformers
e011f3be change "bases:" to "resources:"
716a7307 feat: Add instructions for setting key in configmap
e455acc1 fix
aedb3625 fix doc
73660af1 fix environment variable typo.
34287e51 fix example-zh-README.md
d3d4908f fix latest version
c2cc93a0 fix: tempfile(?)
af298558 fix: windows builds
bc303c46 in plugin executor remove unnecessary code and improve error messages
a279c08f make repospec memebers public
4cb88386 plugin/go-getter: support urls including `:`
2e7ad48b properly omitempty for 'inventory' in 'kustomize'
d3022ccd rename to tools directory
78c97292 translate-zh: glossary.md
6cf8b9e2 update examples-zh
a4e1ba05 update zh doc
6fcb7840 use `kubectl apply -k` # (#1495)
