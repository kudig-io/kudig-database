# kops v1.17 Release Notes

Source: [v1.17.2](https://github.com/kubernetes/kops/releases/tag/v1.17.2)

This version contains a **critical update** to etcd-manager: 1 year after creation (or first adopting etcd-manager), clusters will stop responding due to expiration of a TLS certificate.  Upgrading kops to 1.17.2 (or the latest versions of the 1.16, 1.17 or 1.18 series) and running `kops update` followed by a `kops rolling-update` will fix the issue.  Please see [the advisory](https://kops.sigs.k8s.io/advisories/etcd-manager-certificate-expiration/) for the full details.

---

kops 1.17.2 is the next patch release in the kops 1.17 series, supporting kubernetes version 1.17.x and earlier.

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.17-NOTES.md) for the full list of changes. 

# Significant changes

* The default Docker version has been changed to 19.03.4. Optional support for Docker 19.03.8 has been added and will be the default in future versions. Enable by setting `spec.docker.version: 19.03.8`.

* The default instance type for AWS has been changed to t3.medium. This should provide better performance and reduced costs in clusters where the average CPU usage is low.

* Support for [Ubuntu 20.04 (Focal)](../operations/images.md#ubuntu-2004-focal) has been added.

# Breaking changes

* Support for Docker versions 1.11, 1.12 and 1.13 has been removed because of the [dockerproject.org shut down](https://www.docker.com/blog/changes-dockerproject-org-apt-yum-repositories/). Those affected must upgrade to a newer Docker version.
 
* Terraform users on AWS may need to rename some resources in their state file in order to prepare for future Terraform 0.12 support. See Required Actions below.

* Please see the notes in the 1.15 release about the apiGroup changing from kops
  to kops.k8s.io

* Since 1.16, a controller is now used to apply labels to nodes.  If
  you are not using AWS, GCE or OpenStack your (non-master) nodes may
  not have labels applied correctly.

# Required Actions

* Terraform users on AWS may need to rename resources in their terraform state file in order to prepare for future Terraform 0.12 support.
  Terraform 0.12 [no longer supports resource names starting with digits](https://www.terraform.io/upgrade-guides/0-12.html#pre-upgrade-checklist). In Kops, both the default route and additional VPC CIDR associations are affected. See [#7957](https://github.com/kubernetes/kops/pull/7957) for more information.
  * The default route was named `aws_route.0-0-0-0--0` and will now be named `aws_route.route-0-0-0-0--0`.
  * Additional CIDR blocks associated with a VPC were similarly named the hyphenated CIDR block with two hyphens for the `/`, for example `aws_vpc_ipv4_cidr_block_association.10-1-0-0--16`. These will now be prefixed with `cidr-`, for example `aws_vpc_ipv4_cidr_block_association.cidr-10-1-0-0--16`.
  
  To prevent downtime, follow these steps with the new version of Kops:
  ```
  kops update cluster --target terraform ...
  terraform plan
  # Observe any aws_route or aws_vpc_ipv4_cidr_block_association resources being destroyed and recreated
  # Run these commands as necessary. The exact names may differ; use what is outputted by terraform plan
  terraform state mv aws_route.0-0-0-0--0 aws_route.route-0-0-0-0--0
  terraform state mv aws_vpc_ipv4_cidr_block_association.10-1-0-0--16 aws_vpc_ipv4_cidr_block_association.cidr-10-1-0-0--16
  terraform plan
  # Ensure these resources are no longer being destroyed and recreated
  terraform apply
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
 
* If either a Kops 1.17 alpha release or a custom Kops build was used on a cluster,
  a kops-controller Deployment may have been created that should get deleted because it has been replaced with a DaemonSet.
  Run `kubectl -n kube-system delete deployment kops-controller` after upgrading to Kops 1.17.0-alpha.2 or later.

# Deprecations

* Support for Kubernetes releases prior to 1.9 is deprecated and will be removed in kops 1.18.

* The `kops/v1alpha1` API is deprecated and will be removed in kops 1.18. Users of `kops replace` will need to supply v1alpha2 resources.

* Support for Ubuntu 16.04 (Xenial) has been deprecated and will be removed in future versions of Kops.

* Support for Debian 8 (Jessie) has been deprecated and will be removed in future versions of Kops. 
 
* Support for CoreOS has been deprecated and will be removed in future versions of Kops. Those affected should consider using [Flatcar](../operations/images.md#flatcar) as a replacement.

* Support for the "Legacy" etcd provider has been deprecated. It will not be supported for Kubernetes 1.18 or later. To migrate to the default "Manager" etcd provider see the [etcd migration documentation](../etcd3-migration.md).

# Known Issues

* None at the present time

# Changes from 1.17.1 to 1.17.2

* Use fixed UID for etcd user and restrict to legacy provider [@johngmyers](https://github.com/johngmyers) [#9581](https://github.com/kubernetes/kops/pull/9581)
* Fix int to string conversions [@hakman](https://github.com/hakman) [#9630](https://github.com/kubernetes/kops/pull/9630)
* fixes(openstack): auth problem for kops-controller [@zetaab](https://github.com/zetaab) [#9659](https://github.com/kubernetes/kops/pull/9659)

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.17-NOTES.md) for the full list of changes. 
