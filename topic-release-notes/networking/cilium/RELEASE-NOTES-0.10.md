# cilium v0.10 Release Notes

Source: [0.10.1](https://github.com/cilium/cilium/releases/tag/0.10.1)

Bug Fixes
---------
* Fixed an issue where service IDs were leaked in etcd/consul. Services have
  been moved to a new prefix in the kvstore. Old, leaked service IDs are
  automatically removed when a fixed cilium-agent is started. (#1182, #1195)
* Fixed accuracy of policy revision field. The policy revision field was bumped
  after policy for an endpoint was recalculated. The policy revision field is
  now bumped *after* complete synchronization with the datapath has occurred
  (#1196)
* Fixed graceful connection closure where final ACK after FIN+ACK was dropped
  (#1186)
* Fixed unnecessary consumer map deletion attempt which resulted in confusion
  due to warning log messages (#1206)
* Fixed stateful connection recognition of reply|related packets from an
  endpoint to the host. This resulted in reply packets getting dropped if the
  path from endpoint to host was restricted by policy but a connection from
  the host to the endpoint was permitted (#1211)
* Fixed debian packages build process (#1153)
* Fixed a typo in the getting started guide examples section (#1213)
* Fixed Kubernetes CI test to use locally built container image (#1188)
* Fixed logic which picks up Kubernetes log files on failed CI testruns (#1169)

Kubernetes
----------
* Added support for Custom Resource Definition (CRD). Be aware that parallel
  usage of CRD and Third party Resources (TPR) leads to unexpected behaviour.
  See cilium.link/migrate-tpr for more details. Upgrade your
  CiliumNetworkPolicy resources to cilium.io/v2 in order to use CRD. Keep them
  at cilium.io/v1 to stay on TPR. (#1169, #1219)
* Added RBAC rules for v1/NetworkPolicy (#1188)
* Upgraded to kubeadm 1.7.0 (#1179)
* Upgraded Kubernetes example to 1.7.0 (#1180)