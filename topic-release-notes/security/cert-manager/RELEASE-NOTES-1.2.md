# cert-manager v1.2 Release Notes

Source: [v1.2.0](https://github.com/cert-manager/cert-manager/releases/tag/v1.2.0)

## Urgent Upgrade Notes

### (No, really, you MUST read this before you upgrade)

- **⚠️ BREAKING CHANGE ⚠️ The minimum supported Kubernetes version is now v1.16.0** as of cert-manager `v1.2.0`. Users still running Kubernetes `v1.15` or below should upgrade to a supported version before installing cert-manager or use cert-manager `v1.1`.
- The `User-Agent` request header sent by cert-manager has changed to reflect the ownership transfer to the CNCF — see ([#3515](https://github.com/jetstack/cert-manager/pull/3515), [@meyskens](https://github.com/meyskens))
- The `--renew-before-expiration-duration` flag of the cert-manager controller-manager has been deprecated. Please set the `Certificate.Spec.RenewBefore` field instead. This flag will be removed in the next release.
- Certificates issued by the Vault issuer have changed — the root CA instead of the issuing CA is now stored in `ca.crt` — see ([#3433](https://github.com/jetstack/cert-manager/pull/3433), [@sorah](https://github.com/sorah))

## Changes by Kind

### Feature

- Add  `cert-manager.io/usages` to ingress-shim to specify key usages. Server Auth is now also added as default key usage of ingress-shim ([#3545](https://github.com/jetstack/cert-manager/pull/3545), [@meyskens](https://github.com/meyskens))
- Add `kubectl cert-manager inspect secret` to print certificate info from a secret resource ([#3457](https://github.com/jetstack/cert-manager/pull/3457), [@meyskens](https://github.com/meyskens))
- Add category names to our CRDs so they appear in `kubectl get cert-manager` and `kubectl get cert-manager-acme` ([#3583](https://github.com/jetstack/cert-manager/pull/3583), [@meyskens](https://github.com/meyskens))
- Add creation of PKCS12 truststore.p12 using Certificate Authority ([#3489](https://github.com/jetstack/cert-manager/pull/3489), [@exceptionfactory](https://github.com/exceptionfactory))
- Add option to pass the Certificate duration to ACME (not supported by Let's Encrypt yet) ([#3347](https://github.com/jetstack/cert-manager/pull/3347), [@meyskens](https://github.com/meyskens))
- Added the ability to enable pprof profiling of the controller using the command line flag --enable-profiling. ([#3477](https://github.com/jetstack/cert-manager/pull/3477), [@tharun208](https://github.com/tharun208))
- Added the option to specify the OCSP server for certificates issued by the CA issuer ([#3505](https://github.com/jetstack/cert-manager/pull/3505), [@hugoboos](https://github.com/hugoboos))
- Allows customization of cainjector leader-election leases with new flags `--leader-election-lease-duration`, `--leader-election-renew-deadline` and `--leader-election-retry-period` ([#3527](https://github.com/jetstack/cert-manager/pull/3527), [@ndrpnt](https://github.com/ndrpnt))
- The ingress-shim now checks for `cert-manager.io/duration` and `cert-manager.io/renew-before` annotations and uses those values to set the Certificate.Spec.Duration and Certificate.Spec.RenewBefore fields. ([#3465](https://github.com/jetstack/cert-manager/pull/3465), [@wallrj](https://github.com/wallrj))
- Venafi Issuer now sets the CA.crt field of the Secret. ([#3533](https://github.com/jetstack/cert-manager/pull/3533), [@wallrj](https://github.com/wallrj))

### Bug or Regression

- Deprecated the --renew-before-expiration-duration flag of the cert-manager controller ([#3464](https://github.com/jetstack/cert-manager/pull/3464), [@wallrj](https://github.com/wallrj))
- Fix a bug in the AWS Route53 DNS01 challenge that to retrying over and over instead of observing an exponential back off ([#3485](https://github.com/jetstack/cert-manager/pull/3485), [@maelvls](https://github.com/maelvls))
- Relaxes Ingress validation rules to allow for Certificates to be created/updated for valid Ingress TLS entries even if the same Ingress contains some invalid TLS entries ([#3623](https://github.com/jetstack/cert-manager/pull/3623), [@irbekrm](https://github.com/irbekrm))
- Fix Vault issuer not to store a root CA into a certificate bundle (`tls.crt`). Also, Vault issuer now stores a root CA instead of an issuing CA into a CA bundle (`ca.crt`), from a CA chain returned from Vault. ([#3433](https://github.com/jetstack/cert-manager/pull/3433), [@sorah](https://github.com/sorah))
- Fix Helm chart type conversion bug ([#3647](https://github.com/jetstack/cert-manager/pull/3647), [@irbekrm](https://github.com/irbekrm))

### Other (Cleanup or Flake)

- Always install using admissionregistration.k8s.io/v1 ([#3519](https://github.com/jetstack/cert-manager/pull/3519), [@meyskens](https://github.com/meyskens))
- Change copyright owner to `The cert-manager Authors` ([#3500](https://github.com/jetstack/cert-manager/pull/3500), [@meyskens](https://github.com/meyskens))
- Migrate Ingress to networking.k8s.io/v1beta1 API group ([#3499](https://github.com/jetstack/cert-manager/pull/3499), [@meyskens](https://github.com/meyskens))
- Remove Jetstack from user-agent fields ([#3515](https://github.com/jetstack/cert-manager/pull/3515), [@meyskens](https://github.com/meyskens))
- Remove legacy release ([#3487](https://github.com/jetstack/cert-manager/pull/3487), [@meyskens](https://github.com/meyskens))
