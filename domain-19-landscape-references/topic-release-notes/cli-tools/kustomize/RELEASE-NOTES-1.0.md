---
title: kustomize v1.0 Release Notes
description: kustomize v1.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- job
- cronjob
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kustomize v1.0 Release Notes 是什么
- 如何 kustomize v1.0 Release Notes
trigger_keywords:
- kustomize
- v1.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# kustomize v1.0 Release Notes

Source: [v1.0.11](https://github.com/kubernetes-sigs/kustomize/releases/tag/v1.0.11)

## Changelog

ebf1efe Add StorageClass to the list of ordered objects
4827d99 Add example for generatorOptions
eed16af Add removeAll to fakeFs
4daa655 Add test coverage to gitloader.
8ba2ea9 Add test for mutatefield
d05bb6b Add/fix some documentation and vars names.
793577d Consult history in fileloader.
02d2d38 Deal with branch spec in simpleGitCloner.
a40c250 Delete hashicorp cloner.
4f9d00c Enforce relocatabile kustomizations.
6b93973 Fix #560 (kinda/sorta)
421ca3f Fix typo in namereference path for cronjobs
538aaaf Fix typos: expectd->expected, cluser->cluster
885c195 Improve test coverage.
d9ba209 Introduce simple git cloner.
3e1a3d8 Minor tweaks
25415c5 Remove -t flag in build and add configurations field in kustomization.yaml
6cddc25 Remove stray comment.
a8fbe35 Rename disableHash to disableNameSuffixHash
910eb32 Rename gitloader to gitcloner.
d04877a Simplify some code and add TODOs.
7c1277f Turn off hashicorp cloner.
e0ec802 Update TransformerConfig.Merge function to handle nil
57a5fa5 Update default var reference link
95fed47 Update generatorOptions.md
3488b54 add edit command option for editing name suffix
9d82d54 add fallback for GVK comparison
a14609f add suffix field to ResId
ecbf3c5 add support .yml extension for kusotmization file
93094c7 add transformer for appending suffix
51e9fec allow accessing labels and annotations in vars
f714e9f another tweak
d481dba combine transformers
04a030b enable nameSuffix field of kustomization.yaml
4cf916e fix incorrect path in default namereference configs
b8c2ed2 fix the command usage
c1e7f1b fix the order of YAMLs
5947f69 make sure the objects loaded have name and kind
a898457 refactor test code for readability
83bc67c remove glog dependency from kustomize code (#542)
7dc8ef1 update build command testdata
6ec77b2 update crd example by using configurations file list
e574948 update docs for vars
59df8a0 update docs, examples, comments
727b5eb update vendor_kustomize.sh
a094be4 update vendor_kustomize.sh with run-in-gopath.sh (#545)
