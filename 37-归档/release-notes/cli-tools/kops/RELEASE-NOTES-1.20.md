---
title: kops v1.20 Release Notes
description: kops v1.20 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.20 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- cilium
- calico
- helm
- containerd
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.20 Release Notes 是什么
- 如何 kops v1.20 Release Notes
trigger_keywords:
- kops
- v1.20
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- iac-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kops v1.20 Release Notes

Source: [v1.20.3](https://github.com/kubernetes/kops/releases/tag/v1.20.3)

## Release notes for kOps 1.20 series

# Significant changes

* Default [[22-概念/15-运行时与系统/container-runtime.md|container runtime]] is now set to `[[containerd|containerd]]` for new clusters running [[Kubernetes|Kubernetes]] 1.20.0+.

* Added experimental Azure support. To get started check the [docs](https://kops.sigs.k8s.io/getting_started/azure/)

* Default settings for AWS instances are updated to take advantage of recent performance and security features:
    * Default [[etcd|etcd]] volumes encryption changes to enabled for newly created clusters
    * Default root volume encryption changes to enabled
    * Default etcd volumes type changes from `gp2` to `gp3`
    * Default root volume type changes from `gp2` to `gp3`

* Added [template funtions](https://kops.sigs.k8s.io/operations/cluster_template/#template-functions) for kubernetes version based on channel data.

* kOps now use helm3 functions for merging template `--set` and `--values` arguments. This has slightly different behaviour than previous helm2-like logic.

* Following kubeadm, control plane nodes are now labelled with `node-role.kubernetes.io/control-plane=""`

* Default node image for GCE changed from COS to Ubuntu for K8s versions >= 1.18.0. This is to more closely align with the AWS implementation (the most mature support) and because COS limits the ability to modify files on its disk.

# Breaking changes

* Support for Kubernetes 1.11 and 1.12 has been removed.

* Support for Terraform version 0.11 has been removed.

* Support for the feature flag `Terraform-0.12` has been removed.  All generated Terraform HCL2/JSON files will support versions `0.12.26+` and `0.13.0+`.

# Required Actions

* If you are using the Calico network plugin in a cross-subnet setup, you may have to manually remove the AWS Source/Dest Check controller (`k8s-ec2-srcdst`) deployment that was previously deprecated and replaced with the new [awsSrcDstCheck](https://docs.projectcalico.org/reference/resources/felixconfig#spec) feature.

* If you are using self-hosted channels files, you have to add the new `architectureID` field, with one of the `amd64` or `arm64` values.

* If you are running `kops toolbox template` in an airgapped environment, you have to set `--channel` to point to a local channel file.

* If your workload targets control plane nodes, you need to change them to select the `node-role.kubernetes.io/control-plane=""` label. You should also add the `node-role.kubernetes.io/control-plane:NoSchedule` toleration to these workloads. This taint will not be added to control plane nodes before kOps 1.22.

# Deprecations

* Support for Kubernetes versions 1.13 and 1.14 are deprecated and will be removed in kOps 1.21.

* The [manifest based metrics server addon](https://github.com/kubernetes/kops/tree/master/addons/metrics-server) has been deprecated in favour of a configurable addon.

* The [manifest based cluster autoscaler addon](https://github.com/kubernetes/kops/tree/master/addons/cluster-autoscaler) has been deprecated in favour of a configurable addon.

* The `node-role.kubernetes.io/master` and `kubernetes.io/role` labels are deprecated and will be removed from control plane nodes in kOps 1.22

* The experimental node-authorizer that could be enabled using `nodeAuthorization` has been removed. Setting this value is now forbidden.

* Due to lack of maintainers, the Aliyun/Alibaba Cloud support has been deprecated. The current implementation will be left as-is until the implementation needs updates or otherwise becomes incompatible. At that point, it will be removed. We very much welcome anyone willing to contribute to this cloud provider.

* Support for AWS LaunchConfiguration has been deprecated and will be removed in kOps 1.21.

# Full change list since 1.20.2 release

* Also set haveUserInfo=true in case --user was provided in "kops export kubecfg" [@codablock](https://github.com/codablock) [#11778](https://github.com/kubernetes/kops/pull/11778)
* Handle containerExec hooks when using containerd [@hakman](https://github.com/hakman) [#11852](https://github.com/kubernetes/kops/pull/11852)
* Update aws-sdk-go to v1.37.33 for kOps 1.20 [@hakman](https://github.com/hakman) [#11858](https://github.com/kubernetes/kops/pull/11858)
* Include GCP Project in terraform HCL2 output [@rifelpet](https://github.com/rifelpet) [#11901](https://github.com/kubernetes/kops/pull/11901)
* cluster validation - allow flapping of validation errors [@rifelpet](https://github.com/rifelpet) [#11049](https://github.com/kubernetes/kops/pull/11049)
* Add log rotation for etcd-cilium.log [@hakman](https://github.com/hakman) [#11943](https://github.com/kubernetes/kops/pull/11943)
* Don't ignore channel value in toolbox template [@hakman](https://github.com/hakman) [#12464](https://github.com/kubernetes/kops/pull/12464)
* Update containerd and Docker for kOps 1.20 [@hakman](https://github.com/hakman) [#12509](https://github.com/kubernetes/kops/pull/12509)

<!-- risk-assessed -->
