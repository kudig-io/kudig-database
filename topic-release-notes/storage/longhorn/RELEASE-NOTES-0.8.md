# longhorn v0.8 Release Notes

Source: [v0.8.1](https://github.com/longhorn/longhorn/releases/tag/v0.8.1)

**Highlights:**
1. **[New Longhorn website](https://longhorn.io)**. Please visit https://longhorn.io/docs for latest documentations.
1. **[Default node/disk configuration](https://github.com/longhorn/longhorn/issues/1053)**. This enhancement allows the user to customize the default disks and node configurations in Longhorn for newly added nodes using Kubernetes label and annotation, instead of using Longhorn API or UI. See [here](https://longhorn.io/docs/0.8.1/users-guide/default-disk-and-node-config/) for details. 
1. **[Air Gap installation support](https://github.com/longhorn/longhorn/issues/812)**. We've added Registry Secret support and now Longhorn supports installation in Air Gap using the Helm Chart. See [here](https://longhorn.io/docs/0.8.1/advanced-resources/deploy/airgap/) for details.
1. **[Volume Expansion Rollback](https://github.com/longhorn/longhorn/issues/978)**. With this feature, Volume Expansion is no longer an experimental feature.

The minimal Kubernetes version supported is v1.14.0.

**Important upgrade notes:**
1. **Live upgrade is supported from v0.8.0 to v0.8.1**. You can follow the instruction [here](https://longhorn.io/docs/0.8.1/deploy/upgrade/) to upgrade from v0.8.0. To perform the non-disruptive upgrade for Longhorn engine from v0.8.0 to v0.8.1, follow the instruction [here](https://longhorn.io/docs/0.8.1/deploy/upgrade/upgrade-engine/#live-upgrade) after you upgraded the Longhorn manager.
1. **Longhorn v0.8.1 supports upgrading from v0.8.0 only**. Please upgrade to v0.8.0 first before upgrading to v0.8.1.
1. **Driver name migration**. During the upgrade from v0.6.2 to v0.7.0, we've changed our Kubernetes driver name from `io.rancher.longhorn` to `driver.longhorn.io`. In v0.7.0, we've deployed a compatible CSI plugin to accommodate the old `io.rancher.longhorn` driver provisioned volume. **We're going to remove the compatible CSI plugin in the GA release.** So we highly recommended the users that still using PVs provisioned by `io.rancher.longhorn` to convert the PVs to the new driver name. You can follow the steps [here](https://longhorn.io/docs/0.8.1/deploy/upgrade/longhorn-manager/#migrate-pvs-and-pvcs-for-the-volumes-launched-in-v062-or-older) to convert your old PVs.