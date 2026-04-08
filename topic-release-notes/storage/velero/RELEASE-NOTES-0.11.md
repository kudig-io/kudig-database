# velero v0.11 Release Notes

Source: [v0.11.1](https://github.com/vmware-tanzu/velero/releases/tag/v0.11.1)

## v0.11.1
#### 2019-05-17

### Download
- https://github.com/heptio/velero/releases/tag/v0.11.1

### Highlights
* Added the `velero migrate-backups` command to migrate legacy Ark backup metadata to the current Velero format in object storage. This command needs to be run in preparation for upgrading to v1.0, **if** you have backups that were originally created prior to v0.11 (i.e. when the project was named Ark).