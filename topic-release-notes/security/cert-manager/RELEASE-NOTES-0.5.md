# cert-manager v0.5 Release Notes

Source: [v0.5.2](https://github.com/cert-manager/cert-manager/releases/tag/v0.5.2)

Two releases in one day!

This release contains a single additional patch over v0.5.1.

In cases where you have defined Ingress resources with multiple different hostnames, that only enable TLS for a subset of those hostnames - if ingress-shim is enabled for these Ingress resources, the hosts that did *not* have TLS enabled would be removed from the Ingress resource.

* Fix bug when cleaning up ingress resources after performing ACME HTTP01 validation (#1082, @munnerz)
