# cert-manager v0.1 Release Notes

Source: [v0.1.2](https://github.com/cert-manager/cert-manager/releases/tag/v0.1.2)

[Documentation](https://github.com/jetstack/cert-manager/tree/master/docs) & [User Guides](https://github.com/jetstack/cert-manager/tree/master/docs/user-guides)

## Changelog since v0.1.1

* Fix panic if the secret named in an ACME issuer exists but contains invalid data (or no data) (#165, @munnerz)
* Fix bug in ACME HTTP01 solver causing self-check to return true before paths have propagated (#166, @munnerz)
