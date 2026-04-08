# velero v1.5 Release Notes

Source: [v1.5.4](https://github.com/vmware-tanzu/velero/releases/tag/v1.5.4)

## v1.5.4
### 2021-03-31
### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.5.4

### Container Image
`velero/velero:v1.5.4`

### Documentation
https://velero.io/docs/v1.5/

### Upgrading
https://velero.io/docs/v1.5/upgrade-to-1.5/

  * Fixed a bug where restic volumes would not be restored when using a namespace mapping. (#3475, @zubron)
  * Add CAPI Cluster and ClusterResourceSets to default restore priorities so that the capi-controller-manager does not panic on restores. (#3446, @nrb)
