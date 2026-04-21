# kops v1.16 Release Notes

Source: [v1.16.4](https://github.com/kubernetes/kops/releases/tag/v1.16.4)

This version contains a **critical update** to etcd-manager: 1 year after creation (or first adopting etcd-manager), clusters will stop responding due to expiration of a TLS certificate.  Upgrading kops to 1.16.2 (or the latest versions of the 1.16, 1.17 or 1.18 series) and running `kops update` followed by a `kops rolling-update` will fix the issue.  Please see [the advisory](https://kops.sigs.k8s.io/advisories/etcd-manager-certificate-expiration/) for the full details.

---

kops 1.16.4 is a patch release in the kops 1.16 series, supporting kubernetes version 1.16.x and earlier.

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.16-NOTES.md) for the full list of changes. 

# Breaking changes

* Please see the notes in the 1.15 release about the apiGroup changing from kops
  to kops.k8s.io

* A controller is now used to apply labels to nodes.  If you are not using AWS,
  GCE or OpenStack your (non-master) nodes may not have labels applied
  correctly.

# Significant changes

* If upgrading from 1.11 or earlier, please see the notes in previous releases
  about upgrading through kubernetes 1.12, with the etcd3 upgrade.

* A new component runs on the master nodes now: kops-controller.
  kops-controller currently labels nodes, but will likely perform additional
  functionality in future releases.

# Required Actions

* If either a Kops 1.16 alpha release or a custom Kops build was used on a cluster,
  a kops-controller Deployment may have been created that should get deleted.
  Run `kubectl -n kube-system delete deployment kops-controller` after upgrading to Kops 1.16.0-beta.1 or later.

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
 
# Deprecations

* Support for Kubernetes releases prior to 1.9 is deprecated and will be removed in kops 1.18.

* The `kops/v1alpha1` API is deprecated and will be removed in kops 1.18. Users of `kops replace` will need to supply v1alpha2 resources.


## Changes from 1.16.3 to 1.16.4

* Update etcd-manager to 3.0.20200531 [@hakman](https://github.com/hakman) [#9237](https://github.com/kubernetes/kops/pull/9237)
* Use CNI 0.8.6 for Kubernetes 1.15+ [@hakman](https://github.com/hakman) [#9256](https://github.com/kubernetes/kops/pull/9256)
* Use Docker 19.03.11 for Kubernetes 1.17+ [@hakman](https://github.com/hakman) [#9314](https://github.com/kubernetes/kops/pull/9314)
* Fix missing changes in Weave manifest [@hakman](https://github.com/hakman) [#8965](https://github.com/kubernetes/kops/pull/8965)
* Update Weave Net to 2.6.5 [@hakman](https://github.com/hakman) [#9330](https://github.com/kubernetes/kops/pull/9330)
* Update Calico for CVE-2020-13597 [@hakman](https://github.com/hakman) [#9331](https://github.com/kubernetes/kops/pull/9331)
* Add support for c5a aws ec2 instance types [@coolstang](https://github.com/coolstang) [#9386](https://github.com/kubernetes/kops/pull/9386)


Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.16-NOTES.md) for the full list of changes. 
