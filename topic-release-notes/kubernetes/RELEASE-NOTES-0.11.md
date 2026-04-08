# Kubernetes v0.11 Release Notes

Source: GitHub Release [v0.11.0](https://github.com/kubernetes/kubernetes/releases/tag/v0.11.0)

### Changes since 0.10.0
- Secret API Resources
- Better error handling in various places
- Improved RackSpace support
- Fix `kubectl` patch behavior
- Health check failures fire events
- Don't delete the pod infrastructure container on health check failures
- Improvements to Pod Status detection and reporting
- Reduce the size of scheduled pods in etcd
- Fix some bugs in namespace clashing
- More detailed info on failed image pulls
- Remove pods from a failed node
- Safe format and mount of GCE PDs
- Make events more resilient to etcd watch failures
- Upgrade to container-vm 01-29-2015
  
  | binary | hash alg | hash |
  | --- | --- | --- |
  | `kubernetes.tar.gz` | md5 | `b7e67a4a4b09ce120379f83b8193ac3f` |
  | `kubernetes.tar.gz` | sha1 | `aa884b8200681d3bb8ca0f12398c7424942be500` |
