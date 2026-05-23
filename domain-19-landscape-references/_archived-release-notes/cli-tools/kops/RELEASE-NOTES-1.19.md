---
title: kops v1.19 Release Notes
description: kops v1.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- kubelet
- cilium
- calico
- containerd
- docker
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- kops v1.19 Release Notes 是什么
- 如何 kops v1.19 Release Notes
trigger_keywords:
- kops
- v1.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
created: "2026-05-23"
---

# kops v1.19 Release Notes

Source: [v1.19.3](https://github.com/kubernetes/kops/releases/tag/v1.19.3)

kOps 1.19.3 is the latest in the 1.19 series, with support for [[Kubernetes|kubernetes]] 1.19.

# Significant changes

* **Terraform users on AWS should read the [_Required Actions_](#required-actions) section below to avoid potential [[etcd|etcd]] data loss.**

* 1.19.3 (like 1.19.1 and 1.19.2) no longer blocks [[containerd|containerd]] with kubenet, kopeio, gce and external networking modes; this limitation was specific to 1.19.0 only.

## Changes to kubernetes config export

kOps will no longer automatically export the kubernetes config on `kops update cluster`. In order to export the config on cluster update, you need to either add the `--user <user>` to reference an existing user, or `--admin` to export the cluster admin user. If neither flag is passed, the kubernetes config will not be modified. This makes it easier to reuse user definitions across clusters should you, for example, use OIDC for authentication.

Similarly, `kops export kubecfg` will also require passing either the `--admin` or `--user` flag if the context does not already exist.

By default, the credentials of any exported admin user now have a lifetime of 18 hours. The lifetime of the exported
credentials may be specified as a value of the `--admin` flag. To get the previous behavior, specify `--admin=87600h` to either `kops update cluster` or `kops export kubecfg`.

`kops create cluster --yes` exports the admin user along with rest of the cluster config, as was the previous behaviour (except for the 18-hour validity).

## ARM64 support

kOps will install ARM64 artifacts and containerd images when the instance group supports ARM64 for both machine type and OS image. At the moment this is known to work with AWS m6g, c6g, r6g and t4g instances, with the latest Ubuntu 20.04 OS images for ARM64.

## OpenStack Cinder plugin

kOps will install the Cinder plugin for kOps running kubernetes 1.16 or newer. If you already have this plugin installed you should remove it before upgrading.

If you already have a default `StorageClass`, you should set `cloudConfig.Openstack.BlockStorage.CreateStorageClass: false` to prevent kOps from installing one.

## Other significant changes by kind

### General

* New clusters will now have one nodes group per zone. The number of nodes now defaults to the number of zones.

* On AWS kOps now defaults to using launch templates instead of launch configurations.

* There is now Alpha support for Hashicorp Vault as a store for secrets and keys. See the [Vault state store docs](/state/#vault-vault).

* The lifetimes of certificates used by various components have been substantially reduced.
The certificates on a node will expire sometime between 455 and 485 days after the node's creation.
The expiration times vary randomly so that nodes are likely to have their certs expire at different times than other nodes.

* kOps now supports using an AWS Network Load Balancer (NLB) for API access.
See the [documentation](/cluster_spec/#load-balancer-class) for more info.

* Allow users to partially compress user-data, check the instance groups docs for more details.

* Worker nodes on AWS will now be bootstrapped using kops-controller.

### CLI

* The `kops update cluster` command will now refuse to run on a cluster that
has been updated by a newer version of kOps unless it is given the `--allow-kops-downgrade` flag.

* New command for deleting a single instance: [kops delete instance](/docs/cli/kops_delete_instance/)

### CNI

* Clusters using the Amazon VPC CNI provider now perform an `ec2.DescribeInstanceTypes` call at instance launch time. In large clusters or AWS accounts this may lead to API throttling which could delay node readiness. If this becomes a problem please open a GitHub issue.

* Clusters using Calico with `CrossSubnet` enabled will switch to the new [awsSrcDstCheck](https://docs.projectcalico.org/reference/resources/felixconfig#spec) for disabling the [AWS source/destination checks](https://docs.aws.amazon.com/vpc/latest/userguide/VPC_NAT_Instance.html#EIP_Disable_SrcDestCheck). The previous implementation using [k8s-ec2-srcdst](https://github.com/ottoyiu/k8s-ec2-srcdst) is now deprecated.

* Clusters using Calico can now enable the [eBPF dataplane](https://docs.projectcalico.org/about/about-ebpf) mode for Ubuntu 20.04 (Focal) hosts. Add `spec.networking.calico.bpfEnabled: true` and `spec.kubeProxy.enabled: false` to the cluster spec to enable.

* Clusters using Calico can now encrypt pod-to-pod traffic with [WireGuard](https://docs.projectcalico.org/security/encrypt-cluster-pod-traffic) for Ubuntu hosts. Add `spec.networking.calico.wireguardEnabled: true` to the cluster spec to enable.

* New clusters running Cilium now enable BPF NodePort by default if the Kubernetes version is 1.12 or newer.

### Addons

* Metrics Server is now available as a configurable addon. Add `spec.metricsServer.enabled: true` to the cluster spec to enable.

* Cluster Autoscaler is now availalble as a configurable addon. Add `spec.clusterAutoscaler.enabled: true` to the cluster spec to enable.

* AWS Node Termination Handler is now available as a configurable addon. Add `spec.nodeTerminationHandler.enabled: true` to the cluster spec to enable.

* Cilium will enable `enable-remote-identity` by default. This can affect network policies. If you want to keep old behaviour, add `spec.networking.cilium.enableRemoteIdentity: false` in your cluster spec.

# Breaking changes

* Support for Kubernetes 1.9 and 1.10 has been removed.

* Support for the Romana networking provider has been removed.

* Support for legacy IAM permissions has been removed. This removal may be temporarily deferred to kOps 1.20 by setting the `LegacyIAM` feature flag.

# Required Actions

* See note about [Openstack Cinder plugin](#openstack-cinder-plugin) above.

* Terraform 0.12 users on AWS, in order to prevent downtime you will have to remove the state of any existing ELB or TargetGroup attachments from your Terraform state file. This is due to migrating the attachments to the in-line `aws_autoscaling_group` fields. See the [terraform documentation](https://registry.terraform.io/providers/hashicorp/aws/latest/docs/resources/autoscaling_group) for more information about the difference. This migration is required due to a bug described in [#9913](https://github.com/kubernetes/kops/issues/9913).

  To prevent downtime, follow these steps with the new version of Kops:
  ```bash
  kops update cluster --target terraform ...
  terraform plan
  terraform state list | grep aws_autoscaling_attachment | xargs -L1 terraform state rm
  terraform plan
  # Ensure these resources are no longer being destroyed and recreated
  terraform apply
  ```

* Terraform 0.12 users on AWS migrating clusters from Launch Configurations to Launch Templates may need to remove the state of the old Launch Configuration. This is due to potential errors with Terraform attempting to delete the Launch Configuration before updating the AutoScalingGroup to use the Launch Template. The Launch Configurations will need to be manually deleted afterwards.
  More information including detailed remediation steps is available in [#10017](https://github.com/kubernetes/kops/issues/10017).
  ```bash
  kops update cluster --target terraform ...
  terraform state list | grep aws_launch_configuration | xargs -L1 terraform state rm
  terraform plan
  # Ensure launch configurations are not being destroyed
  terraform apply
  ```

* Terraform users on AWS may need to rename their EBS Volume resources to match 0.12's stricter naming requirements. Volumes whose Terraform resource name begin with a digit are now prefixed with `ebs-`. This change will be made regardless of `Terraform-0.12` feature flag value. More information is available in [#9982](https://github.com/kubernetes/kops/issues/9982).
  When upgrading to kOps 1.19, follow these steps to determine if a rename is necessary:
  ```bash
  kops update cluster --target terraform ...
  terraform plan
  # Look for any EBS volumes being recreated
  # Adjust these arguments as necessary
  terraform state mv aws_ebs_volume.1a-etcd-events-foo-k8s-local aws_ebs_volume.ebs-1a-etcd-events-foo-k8s-local
  terraform plan
  # Confirm no EBS volumes being changed
  terraform apply
  ```

* If you are using Terraform with an additional .tf file and using "aws_autoscaling_attachment" to attach additional Load Balancers or ALB/NLB Target Groups you'll need to migrate to [attaching them through the InstanceGroup spec instead](https://kops.sigs.k8s.io/instance_groups/#externalloadbalancers).

* AWS clusters using an ACM Certificate on the API ELB (`.spec.api.loadBalancer.sslCertificateID`) will need to migrate from Classic LoadBalancer (CLB) to Network LoadBalancer (NLB) prior to upgrading to Kubernetes 1.19 by setting `.spec.api.loadBalancer.class: Network`.
  Any kubeconfig files using kOps' admin client credentials will need to be regenerated with `kops export kubecfg --admin`.
  For more information see [this page](https://github.com/kubernetes/kops/blob/master/permalinks/acm_nlb.md).

# Deprecations

* Support for Kubernetes versions 1.11 and 1.12 has been deprecated and will be removed in kOps 1.20.

* Support for Docker container runtime has been deprecated and will be replaced with containerd for new clusters as of kOps 1.20. For existing clusters the default behaviour can be overridden by setting `spec.containerRuntime: containerd` in the cluster spec. This change closely tracks the upstream [deprecation of Docker](https://kubernetes.io/blog/2020/12/02/dont-panic-kubernetes-and-docker).

* Support for Terraform version 0.11 has been deprecated and will be removed in kOps 1.20.

* Support for feature flag `Terraform-0.12` has been deprecated and will be removed in kOps 1.20. All generated Terraform HCL2/JSON files will support versions `0.12.26+` and `0.13.0+`.

* The [manifest based metrics server addon](https://github.com/kubernetes/kops/tree/master/addons/metrics-server) has been deprecated in favour of a configurable addon.

* The [manifest based cluster autoscaler addon](https://github.com/kubernetes/kops/tree/master/addons/cluster-autoscaler) has been deprecated in favour of a configurable addon.

* The experimental node authorizer is now ignored if you are using kubernetes 1.19. The feature will be removed in 1.20. Worker nodes will instead be authorized using kops-controller.


## Changes since 1.19.2

* Remove Calico bgppeer KeepOriginalNextHop default [@hakman](https://github.com/hakman) [#11203](https://github.com/kubernetes/kops/pull/11203)
* Always secure api -> kubelet communication [@olemarkus](https://github.com/olemarkus) [#11185](https://github.com/kubernetes/kops/pull/11185)
* Exclude nodes from load balancers upon cordoning [@johngmyers](https://github.com/johngmyers) [#11273](https://github.com/kubernetes/kops/pull/11273)
* Make it possible to detect field changes when mixedInstancePolicy is removed [@h3poteto](https://github.com/h3poteto) [#11255](https://github.com/kubernetes/kops/pull/11255)
* Filter servers using cluster name in tags [@zetaab](https://github.com/zetaab) [#11305](https://github.com/kubernetes/kops/pull/11305)
* Update Calico to v3.17.4 for kOps 1.19 [@hakman](https://github.com/hakman) [#11334](https://github.com/kubernetes/kops/pull/11334)
* Use etcd-manager built from etcdadm repo [@justinsb](https://github.com/justinsb),[@hakman](https://github.com/hakman) [#11098](https://github.com/kubernetes/kops/pull/11098)
* Verify all versions are set correctly [@johngmyers](https://github.com/johngmyers) [#11413](https://github.com/kubernetes/kops/pull/11413)
* Update verify-terraform to use latest 0.14 and 0.11 versions [@rifelpet](https://github.com/rifelpet) [#11437](https://github.com/kubernetes/kops/pull/11437)
* Backport rename of service-account key to 1.19 [@johngmyers](https://github.com/johngmyers) [#11390](https://github.com/kubernetes/kops/pull/11390)

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.19-NOTES.md) for the full list of changes. 
