---
title: falco v0.40 Release Notes
description: falco v0.40 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.40 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- falco v0.40 Release Notes 是什么
- 如何 falco v0.40 Release Notes
trigger_keywords:
- falco
- v0.40
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Falco|falco]] v0.40 Release Notes

Source: [0.40.0](https://github.com/falcosecurity/falco/releases/tag/0.40.0)

[![LIBS](https://img.shields.io/badge/LIBS-0.20.0-yellow)](https://github.com/falcosecurity/libs/releases/tag/0.20.0)
[![DRIVER](https://img.shields.io/badge/DRIVER-8.0.0+driver-yellow)](https://github.com/falcosecurity/libs/releases/tag/8.0.0+driver)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm-x86_64      | [![rpm](https://img.shields.io/badge/Falco-0.40.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.40.0-x86_64.rpm)        |
| deb-x86_64      | [![deb](https://img.shields.io/badge/Falco-0.40.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.40.0-x86_64.deb) |
| tgz-x86_64      | [![tgz](https://img.shields.io/badge/Falco-0.40.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.40.0-x86_64.tar.gz) |
| tgz-static-x86_64      | [![tgz-static](https://img.shields.io/badge/Falco-0.40.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.40.0-static-x86_64.tar.gz) |
| rpm-aarch64      | [![rpm](https://img.shields.io/badge/Falco-0.40.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.40.0-aarch64.rpm)        |
| deb-aarch64      | [![deb](https://img.shields.io/badge/Falco-0.40.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.40.0-aarch64.deb) |
| tgz-aarch64      | [![tgz](https://img.shields.io/badge/Falco-0.40.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/aarch64/falco-0.40.0-aarch64.tar.gz) |

| Images                                                                    |
|---------------------------------------------------------------------------|
| `docker pull docker.io/falcosecurity/falco:0.40.0`                      |
| `docker pull public.ecr.aws/falcosecurity/falco:0.40.0`                 |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.40.0`        |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.40.0-buster` |
| `docker pull docker.io/falcosecurity/falco:0.40.0-debian`               |

## v0.40.0

Released on 2025-01-28

### Breaking Changes :warning:

* cleanup(userspac/falco)!: drop deprecated options. [[#3361](https://github.com/falcosecurity/falco/pull/3361)] - [@FedeDP](https://github.com/FedeDP)


### Major Changes

* new(docker): streamline docker images [[#3273](https://github.com/falcosecurity/falco/pull/3273)] - [@FedeDP](https://github.com/FedeDP)
* new(build): reintroduce static build [[#3428](https://github.com/falcosecurity/falco/pull/3428)] - [@LucaGuerra](https://github.com/LucaGuerra)
* new(cmake,ci): added support for using jemalloc allocator instead of glibc one and use it by default for release artifacts [[#3406](https://github.com/falcosecurity/falco/pull/3406)] - [@FedeDP](https://github.com/FedeDP)
* new(userspace,cmake): honor new plugins exposed suggested output formats [[#3388](https://github.com/falcosecurity/falco/pull/3388)] - [@FedeDP](https://github.com/FedeDP)
* new(userspace/falco): allow entirely disabling plugin hostinfo support. [[#3412](https://github.com/falcosecurity/falco/pull/3412)] - [@FedeDP](https://github.com/FedeDP)
* new(ci): use `zig` compiler instead of relying on centos7. [[#3307](https://github.com/falcosecurity/falco/pull/3307)] - [@FedeDP](https://github.com/FedeDP)
* new(falco): add buffer_format_base64 option, deprecate -b [[#3358](https://github.com/falcosecurity/falco/pull/3358)] - [@LucaGuerra](https://github.com/LucaGuerra)
* new(falco): add base_syscalls.all option to falco.yaml, deprecate -A [[#3352](https://github.com/falcosecurity/falco/pull/3352)] - [@LucaGuerra](https://github.com/LucaGuerra)
* new(falco): add falco_libs.snaplen option, deprecate -S / --snaplen [[#3362](https://github.com/falcosecurity/falco/pull/3362)] - [@LucaGuerra](https://github.com/LucaGuerra)


### Minor Changes

* update(cmake): bump falcoctl to v0.11.0 [[#3467](https://github.com/falcosecurity/falco/pull/3467)] - [@alacuku](https://github.com/alacuku)
* chore(ci): add attestation for falco [[#3216](https://github.com/falcosecurity/falco/pull/3216)] - [@cpanato](https://github.com/cpanato)
* chore(ci): build Falco in RelWithDebInfo, and upload Falco debug symbols as github artifacts [[#3452](https://github.com/falcosecurity/falco/pull/3452)] - [@FedeDP](https://github.com/FedeDP)
* update(build): DEB and RPM package requirements for dkms and kernel-devel are now suggestions [[#3450](https://github.com/falcosecurity/falco/pull/3450)] - [@jthiltges](https://github.com/jthiltges)


### Bug Fixes

* fix(userspace/falco): fix container_engines.cri.sockets not loading from config file [[#3453](https://github.com/falcosecurity/falco/pull/3453)] - [@zayaanmoez](https://github.com/zayaanmoez)
* fix(docker): /usr/src/'*' no longer created if $HOST_PATH/usr/src didn't exist at startup [[#3434](https://github.com/falcosecurity/falco/pull/3434)] - [@shane-lawrence](https://github.com/shane-lawrence)
* fix(docker): add brotli to the Falco image [[#3399](https://github.com/falcosecurity/falco/pull/3399)] - [@LucaGuerra](https://github.com/LucaGuerra)
* fix(userspace/engine): explicitly disallow appending/modifying a rule with different sources [[#3383](https://github.com/falcosecurity/falco/pull/3383)] - [@mstemm](https://github.com/mstemm)



### Non user-facing changes

* chore(falco.yaml): remove comments about cri cli arguments [[#3458](https://github.com/falcosecurity/falco/pull/3458)] - [@alacuku](https://github.com/alacuku)
* fix(ci): fixed reusable_build/publish_docker workflows. [[#3459](https://github.com/falcosecurity/falco/pull/3459)] - [@FedeDP](https://github.com/FedeDP)
* update(cmake): update libs and driver to latest master #3455](https://github.com/falcosecurity/falco/pull/3455)] - [@github-actions[bot(https://github.com/apps/github-actions)
* chore(ci): bumped actions/upload-download-artifact. [[#3454](https://github.com/falcosecurity/falco/pull/3454)] - [@FedeDP](https://github.com/FedeDP)
* chore(docker): drop unused libelf dep from container images [[#3451](https://github.com/falcosecurity/falco/pull/3451)] - [@leogr](https://github.com/leogr)
* chore(docs): update `plugins_hostinfo` config file comment. [[#3449](https://github.com/falcosecurity/falco/pull/3449)] - [@FedeDP](https://github.com/FedeDP)
* new(build): add RelWithDebInfo target [[#3440](https://github.com/falcosecurity/falco/pull/3440)] - [@shane-lawrence](https://github.com/shane-lawrence)
* chore(deps): Bump submodules/falcosecurity-rules from `283a62f` to `abf6637` #3448](https://github.com/falcosecurity/falco/pull/3448)] - [@dependabot[bot(https://github.com/apps/dependabot)
* update(ci): use 4cpu-16gb arm runners [[#3447](https://github.com/falcosecurity/falco/pull/3447)] - [@LucaGuerra](https://github.com/LucaGuerra)
* update(cmake): update libs and driver to latest master #3439](https://github.com/falcosecurity/falco/pull/3439)] - [@github-actions[bot(https://github.com/apps/github-actions)
* chore: avoid deprecated funcs to calculate sha256 [[#3442](https://github.com/falcosecurity/falco/pull/3442)] - [@federico-sysdig](https://github.com/federico-sysdig)
* chore(ci): enable jemalloc in musl build. [[#3436](https://github.com/falcosecurity/falco/pull/3436)] - [@FedeDP](https://github.com/FedeDP)
* docs(falco.yaml): correct `buffered_outputs` description [[#3427](https://github.com/falcosecurity/falco/pull/3427)] - [@leogr](https://github.com/leogr)
* fix(userspace/falco): use correct filtercheck_field_info. [[#3426](https://github.com/falcosecurity/falco/pull/3426)] - [@FedeDP](https://github.com/FedeDP)
* update(cmake): update libs and driver to latest master #3421](https://github.com/falcosecurity/falco/pull/3421)] - [@github-actions[bot(https://github.com/apps/github-actions)
* fix: update the url for the docs about the concurrent queue classes [[#3415](https://github.com/falcosecurity/falco/pull/3415)] - [@Issif](https://github.com/Issif)
* update(changelog): updated changelog for 0.39.2. [[#3410](https://github.com/falcosecurity/falco/pull/3410)] - [@FedeDP](https://github.com/FedeDP)
* update(cmake): update libs and driver to latest master #3392](https://github.com/falcosecurity/falco/pull/3392)] - [@github-actions[bot(https://github.com/apps/github-actions)
* fix(cmake,docker): avoid cpp-httplib requiring brotli. [[#3400](https://github.com/falcosecurity/falco/pull/3400)] - [@FedeDP](https://github.com/FedeDP)
* chore(deps): Bump submodules/falcosecurity-rules from `407e997` to `283a62f` #3391](https://github.com/falcosecurity/falco/pull/3391)] - [@dependabot[bot(https://github.com/apps/dependabot)
* update(cmake): bump libs to latest master. [[#3389](https://github.com/falcosecurity/falco/pull/3389)] - [@FedeDP](https://github.com/FedeDP)
* update(cmake): update libs and driver to latest master #3385](https://github.com/falcosecurity/falco/pull/3385)] - [@github-actions[bot(https://github.com/apps/github-actions)
* Make enable()/disable() virtual so they can be overridden [[#3375](https://github.com/falcosecurity/falco/pull/3375)] - [@mstemm](https://github.com/mstemm)
* fix(ci): fixed shasum computation for bump-libs CI. [[#3379](https://github.com/falcosecurity/falco/pull/3379)] - [@FedeDP](https://github.com/FedeDP)
* chore(ci): use redhat advised method to check rpmsign success. [[#3376](https://github.com/falcosecurity/falco/pull/3376)] - [@FedeDP](https://github.com/FedeDP)
* chore(deps): Bump submodules/falcosecurity-rules from `e38fb3f` to `407e997` #3374](https://github.com/falcosecurity/falco/pull/3374)] - [@dependabot[bot(https://github.com/apps/dependabot)
* Compile output clone [[#3364](https://github.com/falcosecurity/falco/pull/3364)] - [@mstemm](https://github.com/mstemm)
* fix(ci): fixed bump-libs workflow syntax. [[#3369](https://github.com/falcosecurity/falco/pull/3369)] - [@FedeDP](https://github.com/FedeDP)
* new(ci): add a workflow to automatically bump libs on each monday. [[#3360](https://github.com/falcosecurity/falco/pull/3360)] - [@FedeDP](https://github.com/FedeDP)
* chore(deps): Bump submodules/falcosecurity-rules from `b6ad373` to `e38fb3f` #3365](https://github.com/falcosecurity/falco/pull/3365)] - [@dependabot[bot(https://github.com/apps/dependabot)
* cleanup(falco): reformat options::define [[#3356](https://github.com/falcosecurity/falco/pull/3356)] - [@LucaGuerra](https://github.com/LucaGuerra)

### Statistics

|   MERGED PRS    | NUMBER |
|-----------------|--------|
| Not user-facing |     31 |
| Release note    |     18 |
| Total           |     49 |

#### Release Manager @FedeDP


<!-- risk-assessed -->
