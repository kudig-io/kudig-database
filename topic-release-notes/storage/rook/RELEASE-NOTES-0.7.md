# rook v0.7 Release Notes

Source: [v0.7.1](https://github.com/rook/rook/releases/tag/v0.7.1)

Rook v0.7.1 is a patch release limited in scope and focusing on bug fixes.

### Improvements
* The version of Ceph has been updated to [Luminous 12.2.4](http://docs.ceph.com/docs/master/release-notes/#v12-2-4-luminous) (@bassam)
* When a Ceph monitor is failed over, it will be assigned an appropriate IP address when host networking is being used (@galexrt)
* The [upgrade user guide](https://rook.io/docs/rook/v0.7/upgrade.html) has been updated to include steps for upgrading from v0.6.x to the v0.7 releases (@travisn)
* An issue was fixed that prevented the Helm charts from being correctly published to https://charts.rook.io/ (@bassam)
* In environments where the Kubernetes cluster does not have a version set, the Helm charts will now appropriately proceed (@TimJones)