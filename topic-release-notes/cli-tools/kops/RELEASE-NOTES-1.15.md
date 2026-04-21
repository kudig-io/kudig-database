# kops v1.15 Release Notes

Source: [v1.15.3](https://github.com/kubernetes/kops/releases/tag/v1.15.3)

This version contains a **critical update** to etcd-manager: 1 year after creation (or first adopting etcd-manager), clusters will stop responding due to expiration of a TLS certificate.  Upgrading kops to 1.15.3 (or the latest versions of the 1.16, 1.17 or 1.18 series) and running `kops update` followed by a `kops rolling-update` will fix the issue.  Please see [the advisory](https://kops.sigs.k8s.io/advisories/etcd-manager-certificate-expiration/) for the full details.

---

Patch release of 1.15 series of kops, supporting kubernetes 1.15 and earlier.

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.15-NOTES.md) for the full list of changes. 

For existing clusters prior to 1.12, please update to kubernetes 1.12, then kubernetes 1.13, then kubernetes 1.14 before updating to kubernetes 1.15. Technically this is always required, but it is particularly important because of the etcd-upgrade that is in kops 1.12.

# Breaking changes

The kops apiGroup is changing from `kops` to `kops.k8s.io`, which means that
downgrading to kops 1.14 after upgrading to kops 1.15 will not recognize the
newer objects.  (In general it's better not to mix kops versions, but it is more
visible here.)  Please back up your manifest files using `kops get <clustername>
-oyaml` before upgrading, if the need arises these can later be restored with
kops 1.14 with `kops replace -f`.

It should also be possibe to rewrite the apiGroup fom `kops` to `kops.k8s.io` on
a yaml backup using `sed` or a similar tool, but taking a precautionary backup
is safer.


# Significant changes

* kops now supports running with objects as CRDs, stored in a kubernetes apiserver.
* The apiGroup for kops objects has changed from `kops` to `kops.k8s.io`, to
  support CRDs.  You can continue to provide either apiGroup as input (but you
  should ideally move to `kops.k8s.io`), but the output will always be of the
  `kops.k8s.io` form.
* Rolling updates are much faster by default.  A lot of the time-padding that
  was in previous versions has been replaced with reliance on validation.  The
  `--cloudonly` case is much faster than previously, which we believe to be
  correct because we expect this is normally for disaster-recovery scenarios,
  but you may want to specify longer timings via flags if you are relying on
  time-based delays.

# Required Actions

* If checking the output as a string (yaml or json), please note that the
  apiGroup will now be kops.k8s.io, not kops.  If performing strict string
  comparison you will need to update your expected values.

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

## Changes from 1.15.2 to 1.15.3

* Stabilize sequence of "export xx=xxx" statements [@mitch000001](https://github.com/mitch000001) [#8530](https://github.com/kubernetes/kops/pull/8530)
* Properly detect that bpffs has been mounted [@olemarkus](https://github.com/olemarkus) [#8612](https://github.com/kubernetes/kops/pull/8612)
* Fix uploading of file assets [@johngmyers](https://github.com/johngmyers) [#8720](https://github.com/kubernetes/kops/pull/8720)
* Update to etcd-manager 3.0.20200428 [@justinsb](https://github.com/justinsb) [#9043](https://github.com/kubernetes/kops/pull/9043)

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.15-NOTES.md) for the full list of changes. 

