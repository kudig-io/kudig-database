# cert-manager v0.14 Release Notes

Source: [v0.14.3](https://github.com/cert-manager/cert-manager/releases/tag/v0.14.3)

## Changes by Kind

### Other (Bug, Cleanup or Flake)

- Fix bug in webhook based validation on Kubernetes API servers older than 1.15 ([#2860](https://github.com/jetstack/cert-manager/pull/2860), [@munnerz ](https://github.com/munnerz))
- Fix case where cert-manager.io/issuer doesn't set `Issuer` kind ([#2838](https://github.com/jetstack/cert-manager/pull/2838), [@meyskens](https://github.com/meyskens))
- Fix validatingwebhookconfiguration to use correct URL path and to suport v1alpha3 API objects. ([#2832](https://github.com/jetstack/cert-manager/pull/2832), [@wallrj ](https://github.com/wallrj ))
- Limit `per_page` to 100 in Cloudfare API calls ([#2859](https://github.com/jetstack/cert-manager/pull/2859), [@sileht](https://github.com/sileht))