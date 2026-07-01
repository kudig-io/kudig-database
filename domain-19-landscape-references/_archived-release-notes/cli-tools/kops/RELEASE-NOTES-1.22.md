---
title: kops v1.22 Release Notes
description: kops v1.22 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.22 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- apiserver
- scheduler
- controller-manager
- cilium
- containerd
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- kops v1.22 Release Notes 是什么
- 如何 kops v1.22 Release Notes
trigger_keywords:
- kops
- v1.22
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
- cilium-basics
- etcd-basics
- gpu-scheduling-basics
---



# kops v1.22 Release Notes

Source: [v1.22.6](https://github.com/kubernetes/kops/releases/tag/v1.22.6)

## Release notes for kOps 1.22 series

# Significant changes

## Instance metadata [[Service|service]] version 2

 On AWS, kOps will enable [Instance Metadata Service Version 2](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/configuring-instance-metadata-service.html) and require tokens on new clusters with [[Kubernetes|Kubernetes]] 1.22. In addition, the following max hop limits will be set by default:

 * worker and API server Nodes, and bastions, will have a limit of 1 hop.
 * control plane nodes will have a limit of 3 hops to accommodate for controller [[Pods|Pods]] without host networking that need to assume roles.

This will increase security by default, but may break some types of workloads. In order to revert to old behavior, add the following to the InstanceGroup:

```
spec:
  instanceMetadata:
    httpTokens: optional
```

## External ServiceAccountPermissions

Many of kOps addons can now make direct use of external permissions.
This can be enabled by adding the following to the Cluster spec:

```
spec:
  iam:
    useServiceAccountExternalPermissions: true
```

Currently this is only available using the AWS cloud provider.

## Managed nvidia instances

kOps can now provision instances with nvidia GPUs and configure it for container workloads without the need of hooks and operators. See [GPU support](https://kops.sigs.k8s.io/gpu/)

## Breacking change in NodeLocalDNS

Since 1.22.0 Cluster `spec.kubeDNS.nodeLocalDNS.forwardToKubeDNS` default behaviour changes from `true` to `false`.

## Other significant changes

* New clusters on AWS will no longer provision an SSH public key by default. To provision
  an SSH public key on a new cluster, use the `--ssh-public-key` flag to `kops create cluster`.

* The kOps Terraform support now renders managed files through the Terraform configuration instead 
  of writing them to S3 directly. This defers changes to these files until the time of `terraform apply`.
  This feature may be temporarily disabled by turning off the `TerraformManagedFiles` feature flag
  using `export KOPS_FEATURE_FLAGS="-TerraformManagedFiles"`.

* kOps now implements graceful rotation of its Certificate Authorities and the service
  account signing key. See the documentation on [How to rotate all secrets / credentials](../operations/rotate-secrets.md)

* New clusters running Kubernetes 1.22 will have AWS EBS CSI driver enabled by default.

* kOps now supports Debian 11 (Bullseye).

* kOps can now use [external-dns](https://github.com/kubernetes-sigs/external-dns/) as a drop-in replacement for dns-controller.

# Breaking changes

## Control plane pods no longer mount /srv/kubernetes

 For security reasons, `/srv/kubernetes` is no longer mounted in the kube-apiserver and kube-controller-manager Pods. This also means the files in the default file assets path will be unavailable. If you have file assets or other files needed by kube-apiserver, you must put these into `/srv/kubernetes/kube-apiserver/` or `/srv/kubernetes/kube-controller-manager`, respectively.

For file assets, it means adding an explicit path as shown below:

```yaml
  fileAssets:
  - name: audit-policy-config
    path: /srv/kubernetes/kube-apiserver/audit-policy-config.yaml # make sure you add the path
    roles:
    - Master
    content: |
      apiVersion: audit.k8s.io/v1
      kind: Policy
      rules:
      - level: Metadata
```

## Other breaking changes

* Support for Kubernetes versions 1.15 and 1.16 has been removed.

* The legacy addons from `https://github.com/kubernetes/kops/tree/master/addons` have been deprecated and will not be available in Kubernetes 1.23+. Use [managed addons](https://kops.sigs.k8s.io/addons) instead.

* The legacy location for downloads `s3://https://kubeupv2.s3.amazonaws.com/kops/` has been deprecated and will not be used for new releases. The new canonical downloads location is `https://artifacts.k8s.io/binaries/kops/`.

* The `assets` phase of `kops update cluster` has been removed. It is replaced by the new `kops get assets --copy` command.

* Support for importing and converting kubeup clusters has been removed.

* Support for Cilium and RHEL 8 has been removed. Cilium users will need to migrate to a distribution with a newer Linux kernel.

# Required actions

* Amazon Linux 2 users are encouraged to use the AMIs based on the 5.10 Linux kernel. See [the documentation](../operations/images.md#amazon-linux-2) for more information.

* Terraform support now requires Terraform >=0.15.0.
  Users on older versions must follow Terraform's recommended upgrade path of applying one minor version at a time prior to running `kops update cluster --target terraform`.

* The kOps Terraform support now renders managed files through the Terraform configuration instead 
  of writing them to S3 directly. If, after upgrading kOps and applying a new Terraform plan,
  you subsequently downgrade to an earlier version of kOps, the generated plan will delete these
  files, breaking the cluster. Prior to applying the plan, you will need to orphan all the
  `aws_s3_bucket_object` objects the plan wants to destroy. Use `terraform state rm` on each of them.
  Then re-run `terraform plan` until there are no such objects in the plan.
  
  If you applied the plan without first orphaning all of these objects, fix the cluster by re-running
  `kops update cluster --target terraform`.

* Terraform users of clusters with names beginning with digits will need to move resources prior to upgrading to kOps 1.22. Some of the following commands will need to be run depending on the particular cluster configuration. Confirm the Terraform plan doesn't destroy any of these resources before running `terraform apply`.
  ```bash
  # View the existing terraform resource names for the exact value to use
  HYPHENATED_CLUSTER_NAME=123-cluster-example-com
  terraform state mv "aws_iam_openid_connect_provider.${HYPHENATED_CLUSTER_NAME}" "aws_iam_openid_connect_provider.prefix_${HYPHENATED_CLUSTER_NAME}"
  terraform state mv "aws_internet_gateway.${HYPHENATED_CLUSTER_NAME}" "aws_internet_gateway.prefix_${HYPHENATED_CLUSTER_NAME}"
  terraform state mv "aws_route_table.${HYPHENATED_CLUSTER_NAME}" "aws_route_table.prefix_${HYPHENATED_CLUSTER_NAME}"
  terraform state mv "aws_vpc.${HYPHENATED_CLUSTER_NAME}" "aws_vpc.prefix_${HYPHENATED_CLUSTER_NAME}"
  terraform state mv "aws_vpc_dhcp_options.${HYPHENATED_CLUSTER_NAME}" "aws_vpc_dhcp_options.prefix_${HYPHENATED_CLUSTER_NAME}"
  terraform state mv "aws_vpc_dhcp_options_association.${HYPHENATED_CLUSTER_NAME}" "aws_vpc_dhcp_options_association.prefix_${HYPHENATED_CLUSTER_NAME}"
  ```

# Deprecations

* Support for Kubernetes version 1.17 is deprecated and will be removed in kOps 1.23.

* Support for Kubernetes version 1.18 is deprecated and will be removed in kOps 1.24.

* Support for the Lyft CNI is deprecated and will be removed in kOps 1.23.

* Support for CentOS 7 is deprecated and will be removed in future versions of kOps.

* Support for CentOS 8 is deprecated and will be removed in future versions of kOps.

* Support for Debian 9 (Stretch) is deprecated and will be removed in future versions of kOps.

* Support for RHEL 7 is deprecated and will be removed in future versions of kOps.

* Support for Ubuntu 18.04 (Bionic) is deprecated and will be removed in future versions of kOps.

* All legacy addons are deprecated in favor of managed addons, including the [metrics server addon](https://github.com/kubernetes/kops/tree/master/addons/metrics-server) and the [autoscaler addon](https://github.com/kubernetes/kops/tree/master/addons/cluster-autoscaler).

* The `node-role.kubernetes.io/master` and `kubernetes.io/role` labels are deprecated and might be removed from control plane nodes in kOps 1.23.

* The `TerraformJSON` feature flag is deprecated and will be removed in kOps 1.23. Only native HCL2 Terraform output will be supported.

* Due to lack of maintainers, the Aliyun/Alibaba Cloud support has been deprecated. The current implementation will be left as-is until the implementation needs updates or otherwise becomes incompatible. At that point, it will be removed. We very much welcome anyone willing to contribute to this cloud provider.

* Due to lack of maintainers, the CloudFormation support has been deprecated. The current implementation will be left as-is until the implementation needs updates or otherwise becomes incompatible. At that point, it will be removed. We very much welcome anyone willing to contribute to this target.

# Other changes of note

* Support for shell completion has been substantially improved. kOps has added support for shell completion in `fish` and `PowerShell`.

* It is no longer necessary to set `AWS_SDK_LOAD_CONFIG=1` in the environment when using AWS assumed roles with the `kops` CLI.

* There is a new command `kops get assets` for listing image and file assets used by a cluster.
  It also includes a `--copy` flag to copy the assets to local repositories.
  See the documentation on [Using local asset repositories](../operations/asset-repository.md) for more information.
  
* kOps now provisions TLS server certificates signed by the Kubernetes general CA to kube-controller-manager and kube-scheduler.
  The previous behavior of using self-signed certs may be restored by setting `kubeControllerManager.tlsCertFile` and/or
  `kubeScheduler.tlsCertFile` to `""` in the cluster spec.

* Cilium now supports the wireguard protocol for transparent encryption.

## 1.22.5 to 1.22.6

* Add hashes for containerd and Docker in order to fix CVE-2022-23648 [@drequena](https://github.com/drequena) [#13606](https://github.com/kubernetes/kops/pull/13606)
* Avoid "/etc/resolv.conf" file loopback for Flatcar Container Linux distribution [@seh](https://github.com/seh) [#13617](https://github.com/kubernetes/kops/pull/13617)
* Update etcd-manager to v3.0.20220717 [@hakman](https://github.com/hakman) [#13990](https://github.com/kubernetes/kops/pull/13990)
* Update Go to v1.16.15 for kOps 1.22 [@hakman](https://github.com/hakman) [#13998](https://github.com/kubernetes/kops/pull/13998)
* Switch to latest MacOS version for CI [@hakman](https://github.com/hakman) [#14015](https://github.com/kubernetes/kops/pull/14015)
*  Check keyset existence before attempting to distrust [@yurrriq](https://github.com/yurrriq) [#14041](https://github.com/kubernetes/kops/pull/14041)