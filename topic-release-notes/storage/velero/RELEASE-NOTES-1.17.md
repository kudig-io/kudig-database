# velero v1.17 Release Notes

Source: [v1.17.2](https://github.com/vmware-tanzu/velero/releases/tag/v1.17.2)

## v1.17.2

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.17.2

### Container Image
`velero/velero:v1.17.2`

### Documentation
https://velero.io/docs/v1.17/

### Upgrading
https://velero.io/docs/v1.17/upgrade-to-1.17/

### All Changes
  * Track actual resource names for GenerateName in restore status (#9409, @shubham-pampattiwar)
  * Fix managed fields patch for resources using GenerateName (#9408, @shubham-pampattiwar)
  * don't copy securitycontext from first container if configmap found (#9394, @sseago)
  * Add Role, RoleBinding, ClusterRole, and ClusterRoleBinding in restore sequence. (#9479, @blackpiglet)