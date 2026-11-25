---
title: falco v0.10 Release Notes
description: falco v0.10 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.10 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
- daemonset
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.10 Release Notes 是什么
- 如何 falco v0.10 Release Notes
trigger_keywords:
- falco
- v0.10
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




# [[Falco|falco]] v0.10 Release Notes

Source: [0.10.0](https://github.com/falcosecurity/falco/releases/tag/0.10.0)

Released 2018-04-24

## Major Changes

* **Rules Directory Support**: Falco will read rules files from `/etc/falco/rules.d` in addition to `/etc/falco/falco_rules.yaml` and `/etc/falco/falco_rules.local.yaml`. Also, when the argument to `-r`/falco.yaml `rules_file` is a directory, falco will read rules files from that directory. [[#348](https://github.com/draios/falco/pull/348)] [[#187](https://github.com/draios/falco/issues/187)]
* Properly support all syscalls (e.g. those without parameter extraction by the kernel module) in falco conditions, so they can be included in `evt.type=<name>` conditions. [[#352](https://github.com/draios/falco/pull/352)]
* When packaged as a container, start building kernel module with gcc 5.0 instead of gcc 4.9. [[#331](https://github.com/draios/falco/pull/331)]
* New example puppet module for falco. [[#341](https://github.com/draios/falco/pull/341)] [[#115](https://github.com/draios/falco/issues/115)]
* When signaled with `USR1`, falco will close/reopen log files. Include a [logrotate](https://github.com/logrotate/logrotate) example that shows how to use this feature for log rotation. [[#347](https://github.com/draios/falco/pull/347)] [[#266](https://github.com/draios/falco/issues/266)]
* To improve resource usage, further restrict the set of system calls available to falco [[#351](https://github.com/draios/falco/pull/351)] [[draios/sysdig#1105](https://github.com/draios/sysdig/pull/1105)]

## Minor Changes

* Add gdb to the development Docker image (sysdig/falco:dev) to aid in debugging. [[#323](https://github.com/draios/falco/pull/323)]
* You can now specify -V multiple times on the command line to validate multiple rules files at once. [[#329](https://github.com/draios/falco/pull/329)]
* When run with `-v`, falco will print *dangling* macros/lists that are not used by any rules. [[#329](https://github.com/draios/falco/pull/329)]
* Add an example demonstrating cryptomining attack that exploits an open docker daemon using host mounts. [[#336](https://github.com/draios/falco/pull/336)]
* New falco.yaml option `json_include_output_property` controls whether the formatted string "output" is included in the json object when json output is enabled. [[#342](https://github.com/draios/falco/pull/342)]
* Centralize testing event types for consideration by falco into a single function [[draios/sysdig#1105](https://github.com/draios/sysdig/pull/1105)) [[#356](https://github.com/draios/falco/pull/356)]
* If a rule has an attribute `warn_evttypes`, falco will not complain about `evt.type` restrictions on that rule [[#355](https://github.com/draios/falco/pull/355)]
* When run with `-i`, print all ignored events/syscalls and exit. [[#359](https://github.com/draios/falco/pull/359)]

## Bug Fixes

* Minor bug fixes to k8s [[DaemonSet|daemonset]] configuration. [[#325](https://github.com/draios/falco/pull/325)] [[#296](https://github.com/draios/falco/pull/296)] [[#295](https://github.com/draios/falco/pull/295)]
* Ensure `--validate` can be used interchangeably with `-V`. [[#334](https://github.com/draios/falco/pull/334)] [[#322](https://github.com/draios/falco/issues/322)]
* Rule conditions like `fd.net` can now be used with the `in` operator e.g. `evt.type=connect and fd.net in ("127.0.0.1/24")`. [[draios/sysdig#1091](https://github.com/draios/sysdig/pull/1091)] [[#343](https://github.com/draios/falco/pull/343)]
* Ensure that `keep_alive` can be used both with file and program output at the same time. [[#335](https://github.com/draios/falco/pull/335)]
* Make it possible to append to a skipped macro/rule without falco complaining [[#346](https://github.com/draios/falco/pull/346)] [[#305](https://github.com/draios/falco/issues/305)]
* Ensure rule order is preserved even when rules do not contain any `evt.type` restriction. [[#354](https://github.com/draios/falco/issues/354)] [[#355](https://github.com/draios/falco/pull/355)]

## Rule Changes

* Make it easier to extend the `Change thread namespace` rule via a `user_known_change_thread_namespace_binaries` list. [[#324](https://github.com/draios/falco/pull/324)]
* Various FP fixes from users. [[#321](https://github.com/draios/falco/pull/321)] [[#326](https://github.com/draios/falco/pull/326)] [[#344](https://github.com/draios/falco/pull/344)] [[#350](https://github.com/draios/falco/pull/350)]
* New rule `Disallowed SSH Connection` detects ssh connection attempts to hosts outside of an expected set. In order to be effective, you need to override the macro `allowed_ssh_hosts` in a user rules file. [[#321](https://github.com/draios/falco/pull/321)]
* New rule `Unexpected K8s NodePort Connection` detects attempts to contact the K8s NodePort range from a program running inside a container. In order to be effective, you need to override the macro `nodeport_containers` in a user rules file. [[#321](https://github.com/draios/falco/pull/321)]
* Improve `Modify binary dirs` rule to work with new syscalls [[#353](https://github.com/draios/falco/pull/353)]
* New rule `Unexpected UDP Traffic` checks for udp traffic not on a list of expected ports. Somewhat FP-prone, so it must be explicitly enabled by overriding the macro `do_unexpected_udp_check` in a user rules file. [[#320](https://github.com/draios/falco/pull/320)] [[#357](https://github.com/draios/falco/pull/357)]

<!-- risk-assessed -->
