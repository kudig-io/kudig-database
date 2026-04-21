# thanos v0.8 Release Notes

Source: [v0.8.1](https://github.com/thanos-io/thanos/releases/tag/v0.8.1)

### Fixed

- [#1632](https://github.com/thanos-io/thanos/issues/1632) Removes the duplicated external labels detection on Thanos Querier. It is warning only now. Also made Store Gateway compatible with older Querier versions.
 NOTE: `thanos_store_nodes_grpc_connections` metric is now per `external_labels` and `store_type`. It is a recommended  metric for Querier storeAPIs. `thanos_store_node_info` is marked as obsolete and will be removed in next release.
 NOTE2: Store Gateway is now advertising artificial: `"@thanos_compatibility_store_type=store"` label. This is to have the current Store Gateway compatible with Querier pre v0.8.0. 
This label can be disabled by hidden `debug.advertise-compatibility-label=false` flag on Store Gateway.

See full [CHANGELOG here](./CHANGELOG.md)
 