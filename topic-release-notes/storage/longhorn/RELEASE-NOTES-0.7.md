# longhorn v0.7 Release Notes

Source: [v0.7.0](https://github.com/longhorn/longhorn/releases/tag/v0.7.0)

Longhorn v0.7.0 is the first **beta** release of Longhorn. And it's the first release after Longhorn joined CNCF.

The minimal Kubernetes version supported is **v1.14.0**.

Longhorn CRD API Version has been upgraded from `longhorn.rancher.io/v1alpha1` to `longhorn.io/v1beta1`.

Highlights:
1. [Automatically reattach volume](https://github.com/longhorn/longhorn/issues/851) if the detaching is caused by the node reboots, Docker restarts, Kubernetes upgrades, etc. See [here](https://github.com/longhorn/longhorn/blob/v0.7.0/docs/recover-volume.md) for details.
1. [Container Storage Interface (CSI) v1.1.0](https://github.com/longhorn/longhorn/issues/347), with [raw block volume support](https://github.com/longhorn/longhorn/issues/678).
1. [K3S v0.10.0+](https://github.com/longhorn/longhorn/issues/835). For K3S < v0.10.0, please refer to the [CSI configuration doc](https://github.com/longhorn/longhorn/blob/v0.7.0/docs/csi-config.md#k3s).
1. [NFSv4.1 support](https://github.com/longhorn/longhorn/issues/823).

Upgrade:
1. Only upgrading from Longhorn v0.6.2 is supported.
    1. For other Longhorn version users, please upgrade to v0.6.2 first before upgrading to v0.7.0.
    1. The upgrade instruction is available [here](https://github.com/longhorn/longhorn/blob/v0.7.0/docs/upgrade-from-v0.6.2-to-v0.7.0.md).
