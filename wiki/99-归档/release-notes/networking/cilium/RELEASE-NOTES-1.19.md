---
title: cilium v1.19 Release Notes
description: cilium v1.19 Release Notes — Kubernetes 生产运维知识库
summary: cilium v1.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- envoy
- cilium
- helm
- docker
- opa
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- cilium v1.19 Release Notes 是什么
- 如何 cilium v1.19 Release Notes
trigger_keywords:
- cilium
- v1.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- ebpf-basics
- cilium-basics
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Cilium|cilium]] v1.19 Release Notes

Source: [v1.19.2](https://github.com/cilium/cilium/releases/tag/v1.19.2)

Summary of Changes
------------------

**Minor Changes:**
* ztunnel/helm: move ztunnel [[DaemonSet|daemonset]] management from operator to [[Helm|helm]] (Backport PR cilium/cilium#44593, Upstream PR cilium/cilium#43763, @nddq)

**Bugfixes:**
* Add rate limiting to neighbor reconciler to reduce CPU usage and memory churn (Backport PR cilium/cilium#44699, Upstream PR cilium/cilium#43928, @dylandreimerink)
* bpf: nodeport: use hairpin redirect for L7 LB on bridge devices (Backport PR cilium/cilium#44760, Upstream PR cilium/cilium#44658, @smagnani96)
* cilium-dbg: fix seg-fault `ip get -l reserved:host` (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44443, @aanm)
* clustermesh: fix a few minor typo/issues in the MCS-API documentation (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44299, @MrFreezeex)
* clustermesh: fix a goroutine leak related to EndpointSliceSync when removing cluster (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44444, @MrFreezeex)
* clustermesh: fix a race condition where EndpointSlices created just before a cluster is removed could be left uncleaned (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44503, @MrFreezeex)
* Enable Cilium upgrade and downgrade when existing XDP attach types differ from new XDP programs (Backport PR cilium/cilium#44496, Upstream PR cilium/cilium#44209, @dylandreimerink)
* Fix a bug where node IPv6 updates and deletes were not correctly propagated to the Linux kernel neighbor subsystem. (Backport PR cilium/cilium#44593, Upstream PR cilium/cilium#44540, @tklauser)
* Fix bug where more Helm options were gated by `loadbalancer` option than intended (Backport PR cilium/cilium#44699, Upstream PR cilium/cilium#42916, @mliner)
* Fix envoy admin socket being created as world-accessible (Backport PR cilium/cilium#44593, Upstream PR cilium/cilium#44512, @0xch4z)
* Fix IPSec key rotation race condition where packets were dropped due to XFRM states not being ready when peers started using the new key. Also adds logging for key rotation flow. (Backport PR cilium/cilium#44699, Upstream PR cilium/cilium#44335, @daanvinken)
* Fix tearing down wrong pod's veth in aws-cni chaining when using deterministic pod names (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44494, @aanm)
* Fixed a bug in service load balancing where backend slot assignments could have gaps when maintenance backends exist, potentially causing traffic misrouting. (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#43902, @Aman-Cool)
* Fixed a bug where bandwidth priority updates were not applied when only the priority annotation was changed on a Pod. (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44329, @zbb88888)
* Fixed an issue where wildcard FQDN network policy identities were not correctly pushed to Envoy when using SNI-based policies. (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44462, @liyihuang)
* Fixed VTEP ARP responses returning 00:00:00:00:00:00 MAC due to interface MAC missing from eBPF Overlay configuration. (Backport PR cilium/cilium#44699, Upstream PR cilium/cilium#44513, @akos011221)
* gateway-api: Fix hostname intersection bug that was preventing cert-manager challenges from working correctly. (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44492, @youngnick)
* gateway-api: Fixed some issues with TLSRoute attachment that will be covered by new conformance tests soon. (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44397, @youngnick)
* Grant permissions to the cilium-operator so that it can reconcile ServiceImport when the when the admission plugin OwnerReferencesPermissionEnforcement is activated (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44458, @MrFreezeex)
* helm/ztunnel: Add host field to readiness probe to bind the health check port 15021 to 127.0.0.1 instead of 0.0.0.0 (Backport PR cilium/cilium#44593, Upstream PR cilium/cilium#44196, @nddq)
* ingress: Ensure that the shared ingress exposes port 443 so that it can pass upstream loadbalancer health checks. (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44229, @xtineskim)
* ipam: Fix concurrent map access to multipool map (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44150, @christarazi)
* l7lb: fix bypassing ingress policies for local backends (Backport PR cilium/cilium#44800, Upstream PR cilium/cilium#44693, @smagnani96)
* loadbalancer/healthserver: refresh ProxyRedirect per request (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44286, @mhofstetter)
* policy: Improve PASS handling for non-consecutive tiers and wildcard fallbacks (Backport PR cilium/cilium#44418, Upstream PR cilium/cilium#43917, @TheBeeZee)

**CI Changes:**
* .github/workflows: eks-cluster-pool-manager: fix race condition and c… (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44283, @aanm)
* ci: add k8s 1.35 for AKS (Backport PR cilium/cilium#44699, Upstream PR cilium/cilium#44550, @Artyop)
* ci: add k8s 1.35 for gke tests (Backport PR cilium/cilium#44699, Upstream PR cilium/cilium#44549, @Artyop)
* ci: k8s 1.35 to EKS matrix (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44403, @Artyop)
* ci: reduce number of k8s versions tested on EKS (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44426, @Artyop)
* docs: Bump k8s compat version (Backport PR cilium/cilium#44593, Upstream PR cilium/cilium#44516, @joestringer)
* gh: e2e-upgrade: don't hardcode IPsec encryption algorithm (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44381, @julianwiedmann)
* test/helpers: ignore error creating lease lock message (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44282, @aanm)

**Misc Changes:**
* [v1.19] fix: add Documentation/cmdref/cilium-dbg_policy_subject-selectors.md (cilium/cilium#44644, @jingyuanliang)
* Added circuit breaker configuration (max connections, requests, and retries) for Cilium Envoy ingress, egress, and external envoy. (Backport PR cilium/cilium#44699, Upstream PR cilium/cilium#44195, @liyihuang)
* bgp: Clean up unused RouteReflector and improve GoBGP test commands (Backport PR cilium/cilium#44632, Upstream PR cilium/cilium#44074, @liyihuang)
* bgp: Introduce bgp/peers Hive Shell command (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44067, @YutaroHayakawa)
* bgp: Introduce bgp/routes Hive Shell command (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44220, @YutaroHayakawa)
* bgp: Make the BGP instance name retrievable from GoBGP (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44024, @YutaroHayakawa)
* chore(deps): update all github action dependencies (v1.19) (cilium/cilium#44475, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.19) (cilium/cilium#44572, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.19) (cilium/cilium#44673, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.19) (cilium/cilium#44788, @cilium-renovate[bot])
* chore(deps): update all-dependencies (v1.19) (cilium/cilium#44573, @cilium-renovate[bot])
* chore(deps): update base-images (v1.19) (cilium/cilium#44574, @cilium-renovate[bot])
* chore(deps): update base-images (v1.19) (cilium/cilium#44668, @cilium-renovate[bot])
* chore(deps): update dependency cilium/cilium-cli to v0.19.2 (v1.19) (cilium/cilium#44568, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/certgen docker tag to v0.4.1 (v1.19) (cilium/cilium#44671, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1771585526-532310e626e42c7086de4ef3ea913736125bbd31 (v1.19) (cilium/cilium#44472, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1772889061-409b87726267dd621aab2cc455bad504fa5006d0 (v1.19) (cilium/cilium#44669, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1773220507-ffc0948a7ec4868e6b552a71cf4d3860e78b53cc (v1.19) (cilium/cilium#44723, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1773656288-7b052e66eb2cfc5ac130ce0a5be66202a10d83be (v1.19) (cilium/cilium#44787, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.19) (patch) (cilium/cilium#44473, @cilium-renovate[bot])
* contrib: Auto-find source files in check-source-info.sh (Backport PR cilium/cilium#44628, Upstream PR cilium/cilium#44506, @YutaroHayakawa)
* contrib: Minor cleanups for check-source-info.sh (Backport PR cilium/cilium#44628, Upstream PR cilium/cilium#44431, @YutaroHayakawa)
* docs(ztunnel): fix some typo (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44294, @alagoutte)
* docs: add policy language chapter headline (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44204, @orangecms)
* docs: Fix duplicate `--version` in Helm OCI install/upgrade documentation examples. (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44380, @gma1k)
* docs: Fix some "parsed-literal" blocks (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44385, @qmonnet)
* Docs: improve docs around ipsec upgrade in 1.18 (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44302, @darox)
* docs: Point to cilium.io for community blogs (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44420, @qmonnet)
* fix(deps): update all-dependencies (v1.19) (cilium/cilium#44471, @cilium-renovate[bot])
* fix(deps): update k8s.io patch updates stable (v1.19) (cilium/cilium#44474, @cilium-renovate[bot])
* fix(deps): update k8s.io patch updates stable to 0f775a3 (v1.19) (cilium/cilium#44570, @cilium-renovate[bot])
* fix(deps): update k8s.io patch updates stable to v0.35.2 (v1.19) (patch) (cilium/cilium#44571, @cilium-renovate[bot])
* fix(deps): update module sigs.k8s.io/controller-runtime to v0.23.3 (v1.19) (cilium/cilium#44670, @cilium-renovate[bot])
* fix(deps): update sigs.k8s.io/mcs-api/controllers digest to 15301c2 (v1.19) (cilium/cilium#44785, @cilium-renovate[bot])
* fix(deps): update sigs.k8s.io/mcs-api/controllers digest to 6a4a49e (v1.19) (cilium/cilium#44672, @cilium-renovate[bot])
* fix: helm value rendering bug for operator.unmanagedPodWatcher.intervalSeconds (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44211, @jayl1e)
* k8s/client/fake: let update operations respect resource versioning (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44264, @giorio94)
* loadbalancer/healthserver: stabilize proxy-redirect test (Backport PR cilium/cilium#44517, Upstream PR cilium/cilium#44323, @mhofstetter)
* test: fix goleak check in combination with script tests (Backport PR cilium/cilium#44398, Upstream PR cilium/cilium#44228, @giorio94)

**Other Changes:**
* [v1.19] ipam: Use existing mutex for multipool capacity synchronization (cilium/cilium#44777, @christarazi)
* install: Update image digests for v1.19.1 (cilium/cilium#44410, @cilium-release-bot[bot])


## Docker Manifests

### cilium

`quay.io/cilium/cilium:v1.19.2@sha256:7bc7e0be845cae0a70241e622cd03c3b169001c9383dd84329c59ca86a8b1341`

### clustermesh-apiserver

`quay.io/cilium/clustermesh-apiserver:v1.19.2@sha256:d1f44a78a0d0996ab1841f7564bc6fbd6e242d4ef673a2a8bfdd7385ef68018d`

### docker-plugin

`quay.io/cilium/docker-plugin:v1.19.2@sha256:1ba743852ab063d83955c3917d75b2d296ff78d944d09fc1802f85f07ebee334`

### hubble-relay

`quay.io/cilium/hubble-relay:v1.19.2@sha256:9987c73bad48c987fd065185535fd15a6717cbe8a8caf7fc7ef0413532cf490e`

### operator-alibabacloud

`quay.io/cilium/operator-alibabacloud:v1.19.2@sha256:90bdedf6b0d3108245f8194f8c69262af2c8d839480f99d2396deed057899142`

### operator-aws

`quay.io/cilium/operator-aws:v1.19.2@sha256:6eaa299ad267d7b8fcb4bb17ee1008b391052e2e35f690b21783b1b23b5c0bf2`

### operator-azure

`quay.io/cilium/operator-azure:v1.19.2@sha256:9c040a57f4584782eda9a91f7cf3292ca5d0fb41d75f4aa41ece29d66e145293`

### operator-generic

`quay.io/cilium/operator-generic:v1.19.2@sha256:e363f4f634c2a66a36e01618734ea17e7b541b949b9a5632f9c180ab16de23f0`

### operator

`quay.io/cilium/operator:v1.19.2@sha256:56ea76f4c1dfc8a899581b35bb2fc87b3110ee57ff0ab4003ae26d5a27d81448`



<!-- risk-assessed -->
