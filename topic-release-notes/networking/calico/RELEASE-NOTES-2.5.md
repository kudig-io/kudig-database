# calico v2.5 Release Notes

Source: [v2.5.1](https://github.com/projectcalico/calico/releases/tag/v2.5.1)

# Release notes for Calico v2.5.1

**Attention Kubernetes datastore users upgrading to v2.5.x**:
Users upgrading from Calico v2.4.x or older to v2.5.x or higher with Kubernetes datastore backend must follow the one-time configuration migration task to upgrade the cluster: https://github.com/projectcalico/calico/blob/master/upgrade/v2.5/README.md (@gunjan5)

## Changes to [Felix](https://github.com/projectcalico/felix)
 - [#1538](https://github.com/projectcalico/felix/pull/1538): Add read/write timeouts to Typha connection; fixes that Felix wouldn't spot if TCP connection was dropped without being cleanly shut down.
