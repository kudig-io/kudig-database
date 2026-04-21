# calico v2.4 Release Notes

Source: [v2.4.1](https://github.com/projectcalico/calico/releases/tag/v2.4.1)

# Release notes for Calico v2.4.1

## Changes to [libcalico-go](https://github.com/projectcalico/libcalico-go)
 - [#488](https://github.com/projectcalico/libcalico-go/pull/488): bugfix: fix handling of empty namespaceSelector when using Kubernetes datastore driver (@gunjan5)
 - [#486](https://github.com/projectcalico/libcalico-go/pull/486): bugfix: properly resync node IPs during Felix restart in Kubernetes datastore driver (@bcreane)