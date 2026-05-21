---
title: cilium v0.11 Release Notes
description: cilium v0.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- cilium
- daemonset
- rbac
- networkpolicy
- crd
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v0.11 Release Notes 是什么
- 如何 cilium v0.11 Release Notes
trigger_keywords:
- cilium
- v0.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
- etcd-basics
---

# cilium v0.11 Release Notes

Source: [v0.11](https://github.com/cilium/cilium/releases/tag/v0.11)

Bug Fixes
---------
* Fixed an issue where service IDs were leaked in etcd/consul. Services have
  been moved to a new prefix in the kvstore. Old, leaked service IDs are
  automatically removed when a fixed cilium-agent is started. (#1182, #1195)
* Fixed accuracy of policy revision field. The policy revision field was bumped
  after policy for an endpoint was recalculated. The policy revision field is
  now bumped *after* complete synchronization with the datapath has occurred
  (#1196)
* Fixed graceful connection closure where final ACK after FIN+ACK was dropped
  (#1186)
* Fixed several bugs in endpoint restore functionality where endpoints were not
  correctly recovered after agent restart (#1140, #1242, #1330, #1338)
* Fixed unnecessary consumer map deletion attempt which resulted in confusion
  due to warning log messages (#1206)
* Fixed stateful connection recognition of reply|related packets from an
  endpoint to the host. This resulted in reply packets getting dropped if the
  path from endpoint to host was restricted by policy but a connection from
  the host to the endpoint was permitted (#1211)
* Fixed debian packages build process (#1153)
* Fixed a typo in the getting started guide examples section (#1213)
* Fixed Kubernetes CI test to use locally built container image (#1188)
* Fixed logic which picks up Kubernetes log files on failed CI testruns (#1169)
* Agent now fails during bootup if kvstore cannot be reached (#1266)
* Fixed the L7 redirection logic to only report the new PolicyRevision after
  the proxy has started listening on the port. This resolves a race condition
  when deploying both policy and workload at the same time and the proxy is not
  up yet. (#1286)
* Fixed a bug in cilium monitor memory allocation with regard to handling data
  from the perf ring buffer (#1304)
* Correctly ignore policy resources with an empty ruleset (#1296, #1297)
* Ignore the controller-revision-hash label to derive security identity (#1320)
* Removed `ip:` field name for CIDR policy rules, CIDR rules are now a slice of
  strings describing prefixes (#1322)
* Ignore Kubernetes annotations done by cilium which show up as labels on the
  container when deriving security identity (#1338)
* Increased the `ReadTimeout` of the HTTP proxy to 120 seconds (#1349)
* Fixed use of node address when running with IPv4 disabled (#1260)
* Several fixes around when an endpoint should go into policy enforcement for
  Kubernetes and non-Kubernetes environments (#1328)
* When creating the Kubernetes client, wait for Kubernetes cluster to be in
  ready state (#1350)
* Fixed drop notifications to include as much metadata as possible (#1427, #1444)
* Fixed a bug where the compilation of the base programs and writing of header
  files could occur in parallel with compilation of programs for endpoints which
  could lead to temporary compilation errors (#1440)
* Fail gracefully when configuring more than the maximum supported L4 ports in
  the policy (#1406)
* Fixed a bug where not all policy rules were JSON validated before sending it
  to the agent (#1406)
* Fixed a bug in the SHA256 calculation (#1454)
* Fixed the datapath to differentiate the packets from a regular local process
  and packets originating from the proxy (previously redirected to by the
  datapath). (#1459)

Features
--------
* The monitor now supports multiple readers, you can run `cilium monitor`
  multiple times in parallel. All monitors will see all events. (#1288)
* `cilium policy trace` can now trace policy decisions based on Kubernetes pod
  names, security identities, endpoint IDs and Kubernetes YAML resources
  [Deployments, ReplicaSets, ReplicationControllers, Pods ](#1124)
* It is now possible to reach the local host on IPs which are within the
  overall cluster prefix (#1394)
* The `cilium identity get` CLI and API can now resolve global identities with
  the help of the kvstore (#1313)
* Use new probe functionality of LLVM to automatically use new BPF compare
  instructions if supported by both LLVM and the kernel (#1356)
* CIDR network policy is now visible in `cilium endpoint get` (#1328)
* Set minimum amount of compilation workers to 4 (#1227)
* Removed local backend (#1235)
* Reduced use of cgo in in bpf packages (#1275)
* Do sparse checks during BPF compilation (#1175)
* New `cilium bpf lb list` command (#1317)
* New optimized kvstore interaction code (#1365, #1397, #1370)
* The access log now includes a SHA hash for each reported label to allow for
  validation with the kvstore (#1425)

CI
--
* Improved CI testing infrastructure (#1262, #1207, #1380, #1373, #1390, #1385, #1410)
* Upgraded to kubeadm 1.7.0 (#1179)


Documentation
-------------
* Multi networking documentation (#1244)
* Documentation of the policy specification (#1344)
* New improved top level structuring of the sections (#1344)
* Example for etcd configuration file (#1268)
* Tutorial on how to use cilium monitor for troubleshooting (#1451)

Mesos
-----
* Getting started guide with L7 policy example (#1301, #1246)

Kubernetes
----------
* Added support for Custom Resource Definition (CRD). Be aware that parallel
  usage of CRD and Third party Resources (TPR) leads to unexpected behaviour.
  See cilium.link/migrate-tpr for more details. Upgrade your
  CiliumNetworkPolicy resources to cilium.io/v2 in order to use CRD. Keep them
  at cilium.io/v1 to stay on TPR. (#1169, #1219)
* The CiliumNetworkPolicy resource now has a status field which contains the
  status of each node enforcing the policy (#1354)
* Added RBAC rules for v1/NetworkPolicy (#1188)
* Upgraded Kubernetes example to 1.7.0 (#1180)
* Delay pod healthcheck for 180 seconds to account for endpoint restore (#1271)
* Added tolerations to DaemonSet to schedule Cilium onto master nodes as well (#1426)