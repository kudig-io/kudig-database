---
title: falco v0.31 Release Notes
description: falco v0.31 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.31 Release Notes 是什么
- 如何 falco v0.31 Release Notes
trigger_keywords:
- falco
- v0.31
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Falco|falco]] v0.31 Release Notes

Source: [0.31.1](https://github.com/falcosecurity/falco/releases/tag/0.31.1)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm      | [![rpm](https://img.shields.io/badge/Falco-0.31.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.31.1-x86_64.rpm)        |
| deb      | [![deb](https://img.shields.io/badge/Falco-0.31.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.31.1-x86_64.deb) |
| tgz      | [![tgz](https://img.shields.io/badge/Falco-0.31.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.31.1-x86_64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.31.1`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.31.1`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.31.1`             |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.31.1`                 |

### Major Changes


* new: add a new drop category `n_drops_scratch_map` [[#1916](https://github.com/falcosecurity/falco/pull/1916)] - [@Andreagit97](https://github.com/Andreagit97)
* new: allow to specify multiple --cri options [[#1893](https://github.com/falcosecurity/falco/pull/1893)] - [@FedeDP](https://github.com/FedeDP)


### Minor Changes

* refactor(userspace/falco): replace direct getopt_long() cmdline option parsing with third-party cxxopts library. [[#1886](https://github.com/falcosecurity/falco/pull/1886)] - [@mstemm](https://github.com/mstemm)
* update: driver version is b7eb0dd [[#1923](https://github.com/falcosecurity/falco/pull/1923)] - [@LucaGuerra](https://github.com/LucaGuerra)


### Bug Fixes

* fix(userspace/falco): correct plugins init config conversion from YAML to JSON [[#1907](https://github.com/falcosecurity/falco/pull/1907)] - [@jasondellaluce](https://github.com/jasondellaluce)
* fix(userspace/engine): for rules at the informational level being loaded at the notice level [[#1885](https://github.com/falcosecurity/falco/pull/1885)] - [@mike-stewart](https://github.com/mike-stewart)
* chore(userspace/falco): fixes truncated -b option description. [[#1915](https://github.com/falcosecurity/falco/pull/1915)] - [@andreabonanno](https://github.com/andreabonanno)
* update(falco): updates usage description for -o, --option [[#1903](https://github.com/falcosecurity/falco/pull/1903)] - [@andreabonanno](https://github.com/andreabonanno)


### Rule Changes

* rule(Detect outbound connections to common miner pool ports): fix url in rule output [[#1918](https://github.com/falcosecurity/falco/pull/1918)] - [@jsoref](https://github.com/jsoref)
* rule(macro somebody_becoming_themself): renaming macro to somebody_becoming_themselves [[#1918](https://github.com/falcosecurity/falco/pull/1918)] - [@jsoref](https://github.com/jsoref)
* rule(list package_mgmt_binaries): `npm` added [[#1866](https://github.com/falcosecurity/falco/pull/1866)] - [@rileydakota](https://github.com/rileydakota)
* rule(Launch Package Management Process in Container): support for detecting `npm` usage [[#1866](https://github.com/falcosecurity/falco/pull/1866)] - [@rileydakota](https://github.com/rileydakota)
* rule(Polkit Local Privilege Escalation Vulnerability): new rule created to detect CVE-2021-4034 [[#1877](https://github.com/falcosecurity/falco/pull/1877)] - [@darryk10](https://github.com/darryk10)
* rule(macro: modify_shell_history): avoid false-positive alerts triggered by modifications to .zsh_history.new and .zsh_history.LOCK files [[#1832](https://github.com/falcosecurity/falco/pull/1832)] - [@m4wh6k](https://github.com/m4wh6k)
* rule(macro: truncate_shell_history): avoid false-positive alerts triggered by modifications to .zsh_history.new and .zsh_history.LOCK files [[#1832](https://github.com/falcosecurity/falco/pull/1832)] - [@m4wh6k](https://github.com/m4wh6k)
* rule(macro sssd_writing_krb): fixed a false-positive alert that was being generated when SSSD updates /etc/krb5.keytab [[#1825](https://github.com/falcosecurity/falco/pull/1825)] - [@mac-chaffee](https://github.com/mac-chaffee)
* rule(macro write_etc_common): fixed a false-positive alert that was being generated when SSSD updates /etc/krb5.keytab [[#1825](https://github.com/falcosecurity/falco/pull/1825)] - [@mac-chaffee](https://github.com/mac-chaffee)
* upgrade macro(keepalived_writing_conf) [[#1742](https://github.com/falcosecurity/falco/pull/1742)] - [@pabloopez](https://github.com/pabloopez)
* rule_output(Delete Bucket Public Access Block) typo [[#1888](https://github.com/falcosecurity/falco/pull/1888)] - [@pabloopez](https://github.com/pabloopez)


### Non user-facing changes

* fix(build): fix civetweb linking in cmake module [[#1919](https://github.com/falcosecurity/falco/pull/1919)] - [@LucaGuerra](https://github.com/LucaGuerra)
* chore(userspace/engine): remove unused lua functions and state vars [[#1908](https://github.com/falcosecurity/falco/pull/1908)] - [@jasondellaluce](https://github.com/jasondellaluce)
* fix(userspace/falco): applies FALCO_INSTALL_CONF_FILE as the default … [[#1900](https://github.com/falcosecurity/falco/pull/1900)] - [@andreabonanno](https://github.com/andreabonanno)
* fix(scripts): correct typo in `falco-driver-loader` help message [[#1899](https://github.com/falcosecurity/falco/pull/1899)] - [@leogr](https://github.com/leogr)
* update(build)!: replaced various `PROBE` with `DRIVER` where necessary. [[#1887](https://github.com/falcosecurity/falco/pull/1887)] - [@FedeDP](https://github.com/FedeDP)
* Add [Fairwinds](https://fairwinds.com) to the adopters list [[#1917](https://github.com/falcosecurity/falco/pull/1917)] - [@sudermanjr](https://github.com/sudermanjr)
* build(cmake): several cmake changes to speed up/simplify builds for external projects and copying files from source-to-build directories [[#1905](https://github.com/falcosecurity/falco/pull/1905)] - [@mstemm](https://github.com/mstemm)

### Statistics

| Merged PRs      | Number |
| --------------- | ------ |
| Not user-facing | 11     |
| Release note    | 13     |
| Total           | 24     |


#### Release Manager @LucaGuerra