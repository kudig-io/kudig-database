# velero v1.14 Release Notes

Source: [v1.14.1](https://github.com/vmware-tanzu/velero/releases/tag/v1.14.1)

## v1.14.1

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.14.1

### Container Image
`velero/velero:v1.14.1`

### Documentation
https://velero.io/docs/v1.14/

### Upgrading
https://velero.io/docs/v1.14/upgrade-to-1.14/

### All Changes
  * Avoid wrapping failed PVB status with empty message. (#8037, @mrnold)
  * Make PVPatchMaximumDuration timeout configurable (#8035, @shubham-pampattiwar)
  * Reuse existing plugin manager for get/put volume info (#8016, @sseago)
  * Skip PV patch step in Restoe workflow for WaitForFirstConsumer VolumeBindingMode Pending state PVCs (#8006, @shubham-pampattiwar)
  * Check whether the namespaces specified in namespace filter exist. (#7998, @blackpiglet)
  * Check whether the volume's source is PVC before fetching its PV. (#7976, @blackpiglet)
  * Fix issue #7904, add the limitation clarification for change PVC selected-node feature (#7949, @Lyndon-Li)
  * Expose the VolumeHelper to third-party plugins. (#7944, @blackpiglet)
  * Don't consider unschedulable pods unrecoverable (#7926, @sseago)