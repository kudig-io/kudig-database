# cert-manager v1.0 Release Notes

Source: [v1.0.4](https://github.com/cert-manager/cert-manager/releases/tag/v1.0.4)

## Changes by Kind

### Other (Bug, Cleanup or Flake)

- Fix a bug where the Venafi Issuer and ClusterIssuer did not set the Ready condition and message if there was an API connection or API authentication failure. The Ready condition will now always be set, including details of any errors during setup. ([#3389](https://github.com/jetstack/cert-manager/pull/3389), [@wallrj](https://github.com/wallrj))
- Fix a panic when changing the max concurrent challenges to a lower value ([#3418](https://github.com/jetstack/cert-manager/pull/3418), [@meyskens](https://github.com/meyskens))
- Fix bug in AWS route53 zone lookup that caused too many IAM requests ([#3375](https://github.com/jetstack/cert-manager/pull/3375), [@supriya-premkumar](https://github.com/supriya-premkumar))
- Fix logic in patchDuplicateKeyUsage when signing and digital signature were set ([#3352](https://github.com/jetstack/cert-manager/pull/3352), [@meyskens](https://github.com/meyskens))
- Fix nil pointer error in Cloud DNS when specific config was used. ([#3420](https://github.com/jetstack/cert-manager/pull/3420), [@meyskens](https://github.com/meyskens))