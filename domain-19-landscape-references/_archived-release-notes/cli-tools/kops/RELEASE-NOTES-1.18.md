---
title: kops v1.18 Release Notes
description: kops v1.18 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.18 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- apiserver
- kubelet
- cilium
- calico
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
- kops v1.18 Release Notes 是什么
- 如何 kops v1.18 Release Notes
trigger_keywords:
- kops
- v1.18
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
- cilium-basics
- cni-basics
- etcd-basics
---



# kops v1.18 Release Notes

Source: [v1.18.3](https://github.com/kubernetes/kops/releases/tag/v1.18.3)

This version contains a **critical update** to etcd-manager: 1 year after creation (or first adopting etcd-manager), clusters will stop responding due to expiration of a TLS certificate.  Upgrading kops to 1.18.3 (or the latest versions of the 1.15, 1.16, 1.17 or 1.18 series) and running `kops update` followed by a `kops rolling-update` will fix the issue.  Please see [the advisory](https://kops.sigs.k8s.io/advisories/etcd-manager-certificate-expiration/) for the full details.

---
kops 1.18.3 is the next patch release in the 1.18 series of kops, offering support for [[Kubernetes|kubernetes]] 1.18.

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.18-NOTES.md) for the full list of changes. 


## Release notes for kops 1.18 series

# Significant changes

* **The default image has been updated to [Ubuntu 20.04 (Focal)](https://kops.sigs.k8s.io/operations/images/#ubuntu-2004-focal)**. Consequently, the SSH user changed to `ubuntu` and the Linux kernel changed to version 5.4.

* To address the [issue](https://github.com/kubernetes/kubernetes/issues/91507) of IPv4 only clusters being susceptible to MitM attacks via IPv6 rogue router advertisements, the affected components have been upgraded as follows:
    * Docker version 19.03.11 - [CVE-2020-13401](https://github.com/docker/docker-ce/releases/v19.03.11)
    * CNI plugins 0.8.6 - [CVE-2020-10749](https://github.com/containernetworking/plugins/releases/tag/v0.8.6)
    * Calico 3.15.1 - [CVE-2020-13597](https://www.projectcalico.org/security-bulletins/)
    * Weave Net 2.6.5 - [CVE-2020-11091](https://github.com/weaveworks/weave/security/advisories/GHSA-59qg-grp7-5r73)

* Support for [RHEL 8](https://kops.sigs.k8s.io/operations/images/#rhel-8) and [CentOS 8](https://kops.sigs.k8s.io/operations/images/#centos-8) has been added.

* Support for [Amazon Linux 2](https://kops.sigs.k8s.io/operations/images/#amazon-linux-2) has been improved and will work with the default Docker version.

* [containerd](https://github.com/containerd/containerd/blob/master/README.md) has been added and can be selected as an alternate container runtime for Kubernetes. Enable by using the `--container-runtime containerd` flag when creating a cluster or by setting `spec.containerRuntime: containerd`.

* Rolling updates now support surging and parallelism within an instance group. For details see [the documentation](https://kops.sigs.k8s.io/operations/rolling-update/).

* Cilium CNI can now use AWS networking natively through the AWS ENI IPAM mode. Kops can also run a Kubernetes cluster entirely without kube-proxy using Cilium's BPF NodePort implementation.

* Cilium CNI can now use a dedicated etcd cluster managed by etcd-manager for synchronizing agent state instead of CRDs.

* The Terraform target now supports Terraform 0.12 syntax (HCL2) by default. See the Required Actions item below.

* New clusters in GCE are configured to run the [metadata-proxy](https://github.com/kubernetes/kubernetes/tree/master/cluster/addons/metadata-proxy) by default. The proxy runs as a DaemonSet and lands on nodes with the nodeLabel `cloud.google.com/metadata-proxy-ready: "true"`. If you want to enable metadata-proxy on an existing cluster/instance group, add that nodeLabel to your instancegroup specs (`kops edit ig ...`) and run `kops update cluster`. When the changes are applied, the proxy will roll out to those targeted nodes.

* GCE has a new flag: `--gce-service-account`. This takes the email of an existing GCP service account and launches the instances with it. This setting applies to the whole cluster (ie: it is not currently designed to support Instance Groups with different service accounts). If you do not specify a service account during cluster creation, the default compute service account will be used which matches the prior behavior.

* Google API client libraries updated from v0.beta to v1.

* Support for [NodeLocalDNS cache](https://kops.sigs.k8s.io/cluster_spec/#node-local-dns-cache).

# Breaking changes

* Support for Docker versions 1.11, 1.12 and 1.13 has been removed because of the [dockerproject.org shut down](https://www.docker.com/blog/changes-dockerproject-org-apt-yum-repositories/). Those affected must upgrade to a newer Docker version.

* Terraform users on AWS may need to rename some resources in their state file in order to prepare for Terraform 0.12 support. See Required Actions below.

* Support for the CoreOS OS distribution has been removed. Users should consider Flatcar as a replacement.

* Support for the Debian 8 (Jessie) OS distribution has been removed.

* The Docker `health-check` service has been disabled by default. It shouldn't be needed anymore, but it can still be enabled by setting `spec.docker.healthCheck: true`. It is recommended to also check [node-problem-detector](https://github.com/kubernetes/node-problem-detector) and [draino](https://github.com/planetlabs/draino) as replacements. See Required Actions below.

* Network and internet access for `docker run` containers has been disabled by default, to avoid any unwanted interaction between the Docker firewall rules and the firewall rules of netwok plugins. This was the default since the early days of Kops, but a race condition in the Docker startup sequence changed this behaviour in more recent years. To re-enable, set `spec.docker.ipTables: true` and `spec.docker.ipMasq: true`.

* Lyft CNI plugin default subnet tags changed from from `Type: pod` to `KubernetesCluster: myclustername.mydns.io`. Subnets intended for use by the plugin will need to be tagged with this new tag and [additional tag filters](https://github.com/lyft/cni-ipvlan-vpc-k8s#other-configuration-flags) may need to be added to the cluster spec in order to achieve the desired set of subnets.

* Support for basic authentication has been disabled by default for Kubernetes 1.18 and will be [removed](https://github.com/kubernetes/kubernetes/pull/89069) in Kubernetes 1.19.

* Support for static tokens has been disabled by default for Kubernetes 1.18 and later. To re-enable, see the [Security Notes for Kubernetes](https://kops.sigs.k8s.io/security/#api-bearer-token). We intend to remove support entirely in a future kops version, so file an issue with your use case if you need this feature. 

* Support for Kubernetes versions prior to 1.9 has been removed.

* Kubernetes 1.9 users will need to enable the PodPriority feature gate. See Required Actions below.

* Support for the "Legacy" etcd provider has been removed for Kubernetes versions 1.18 and higher. Such clusters will need to migrate to the default "Manager" etcd provider. To migrate, see the [etcd migration documentation](https://kops.sigs.k8s.io/etcd3-migration/).

* A controller is now used to apply labels to nodes.  If you are not using AWS, GCE or OpenStack your (non-master) nodes may not have labels applied correctly.

* The `kops.k8s.io/v1alpha1` API has been removed. Users of `kops replace` will need to supply v1alpha2 resources.

* Please see the notes in the 1.15 release about the apiGroup changing from kops to kops.k8s.io

# Required Actions

* Terraform users on AWS may need to rename resources in their terraform state file in order to support Terraform 0.12.
  Terraform 0.12 [no longer supports resource names starting with digits](https://www.terraform.io/upgrade-guides/0-12.html#pre-upgrade-checklist). In Kops, both the default route and additional VPC CIDR associations are affected. See [#7957](https://github.com/kubernetes/kops/pull/7957) for more information.
  * The default route was named `aws_route.0-0-0-0--0` and will now be named `aws_route.route-0-0-0-0--0`.
  * Additional CIDR blocks associated with a VPC were similarly named the hyphenated CIDR block with two hyphens for the `/`, for example `aws_vpc_ipv4_cidr_block_association.10-1-0-0--16`. These will now be prefixed with `cidr-`, for example `aws_vpc_ipv4_cidr_block_association.cidr-10-1-0-0--16`.

  To prevent downtime, follow these steps with the new version of Kops:
  ```
  KOPS_FEATURE_FLAGS=-Terraform-0.12 kops update cluster --target terraform ...
  # Use Terraform <0.12
  terraform plan
  # Observe any aws_route or aws_vpc_ipv4_cidr_block_association resources being destroyed and recreated
  # Run these commands as necessary. The exact names may differ; use what is outputted by terraform plan
  terraform state mv aws_route.0-0-0-0--0 aws_route.route-0-0-0-0--0
  terraform state mv aws_vpc_ipv4_cidr_block_association.10-1-0-0--16 aws_vpc_ipv4_cidr_block_association.cidr-10-1-0-0--16
  terraform plan
  # Ensure these resources are no longer being destroyed and recreated
  terraform apply
  ```
  Kops will now output Terraform 0.12 syntax with the normal workflow:
  ```
  kops update cluster --target terraform ...
  # Use Terraform 0.12. This plan should be a no-op
  terraform plan
  ```

* Users that need the Docker `health-check` service will need to explicitly enable it:
```
  kops edit cluster
  # Add the following section
  spec:
    docker:
      healthCheck: true
  ```

* Kubernetes 1.9 users will need to enable the PodPriority feature gate. This is required for newer versions of Kops.

  To enable the Pod priority feature, follow these steps:
  ```
  kops edit cluster
  # Add the following section
  spec:
    kubelet:
      featureGates:
        PodPriority: "true"
  ```

* If a custom Kops build was used on a cluster, a kops-controller Deployment may have been created that should get deleted.
  Run `kubectl -n kube-system delete deployment kops-controller` after upgrading to Kops 1.16.0-beta.1 or later.

# Known Issues

* AWS clusters with an ACM certificate attached to the API ELB (the cluster's `spec.api.loadBalancer.sslCertificate` is set) will need to reenable basic auth to use the kubeconfig context created by `kops export kubecfg`. Set `spec.kubeAPIServer.disableBasicAuth: false` before running `kops export kubecfg`. See [#9756](https://github.com/kubernetes/kops/issues/9756) for more information.

# Deprecations

* Support for Kubernetes versions 1.9 and 1.10 are deprecated and will be removed in kops 1.19.

* Support for Ubuntu 16.04 (Xenial) has been deprecated and will be removed in future versions of Kops.

* Support for the Romana networking provider is deprecated and will be removed in kops 1.19.

* Support for legacy IAM permissions is deprecated and will be removed in kops 1.19.

---

## All changes from 1.18.2 to 1.18.3

* Mount the whole /etc/ssl/certs directory for k8s-ec2-srcdst [@kitos9112](https://github.com/kitos9112),[@hakman](https://github.com/hakman) [#10169](https://github.com/kubernetes/kops/pull/10169)
* Prevent unintended resource updates to LB attatchments [@rdrgmnzs](https://github.com/rdrgmnzs),[@rifelpet](https://github.com/rifelpet) [#9794](https://github.com/kubernetes/kops/pull/9794)
* Fix version of storage-aws addon manifest [@johngmyers](https://github.com/johngmyers) [#10247](https://github.com/kubernetes/kops/pull/10247)
* [weave] Add support for default version override [@dntosas](https://github.com/dntosas),[@hakman](https://github.com/hakman) [#10273](https://github.com/kubernetes/kops/pull/10273)
* Tolerate missing detached EC2 instances [@hwoarang](https://github.com/hwoarang) [#10319](https://github.com/kubernetes/kops/pull/10319)
* Remove dependency on TravisCI [@hakman](https://github.com/hakman) [#10366](https://github.com/kubernetes/kops/pull/10366)
* Cilium bump 1.18 [@olemarkus](https://github.com/olemarkus),[@codablock](https://github.com/codablock) [#10405](https://github.com/kubernetes/kops/pull/10405)
* Allow Calico to run on systems with loose reverse path forwarding [@hakman](https://github.com/hakman) [#10442](https://github.com/kubernetes/kops/pull/10442)
* Backport TargetGroup related fixes [@hakman](https://github.com/hakman) [#10462](https://github.com/kubernetes/kops/pull/10462)
* Update CNI plugins to v0.8.7 [@hakman](https://github.com/hakman) [#10481](https://github.com/kubernetes/kops/pull/10481)
* Manual cherry pick of #10361: Prefix etcd cluster names with letters  [@hakman](https://github.com/hakman) [#10535](https://github.com/kubernetes/kops/pull/10535)
* Don't allow ebs volume TF resource names to begin with digit [@rifelpet](https://github.com/rifelpet) [#10424](https://github.com/kubernetes/kops/pull/10424)
* Update machine types [@hakman](https://github.com/hakman) [#10587](https://github.com/kubernetes/kops/pull/10587)
* Require KOPS_TERRAFORM_0_12_RENAMED, to guard against tf breakage [@justinsb](https://github.com/justinsb),[@hakman](https://github.com/hakman) [#10602](https://github.com/kubernetes/kops/pull/10602)
* etcd-manager: Update to 3.0.20210122 [@justinsb](https://github.com/justinsb),[@hakman](https://github.com/hakman) [#10638](https://github.com/kubernetes/kops/pull/10638)
* Allow attaching same external load balancer to multiple instance groups [@hakman](https://github.com/hakman) [#10666](https://github.com/kubernetes/kops/pull/10666)

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.18-NOTES.md) for the full list of changes. 
