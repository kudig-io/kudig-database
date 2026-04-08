# cert-manager v1.8 Release Notes

Source: [v1.8.2](https://github.com/cert-manager/cert-manager/releases/tag/v1.8.2)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

v1.8.2 is in effect a bug fix release which increases some hard-coded timeouts which were preventing the use of certain ACME issuers
which sometimes had slower response times. This is known to include ZeroSSL and Sectigo.

These issues were reported by many different users and We'd like to thank the following for their help, suggestions and feedback on this topic:

- @JoooostB
- @fatz
- @jgreat
- @sashokbg
- @mycloudedu
- @hadogenes
- @SudonymTM
- @amalucelli
- @MilheiroSantos
- @dverbeek84
- @kxs-jnadeau
- @fablarosa
- @nik-nazarov
- @omBratteng
- @shubham-root
- @alphabet5
- @hawksight

Thanks also to the cert-manager maintainers who were involved in reviewing this fix and helping to move things forwards:

- @SgtCoDFish 
- @irbekrm
- @jahrlin
- @maelvls
- @JoshVanL
- @wallrj
- @jakexks
- @munnerz

## Changes since v1.8.1

### Bug

- Increase timeouts for issuer and clusterissuer controllers to 2 minutes and increase ACME client HTTP timeouts to 90 seconds, in order to enable the use of slower ACME issuers which take a long time to process certain requests. ([#5231](https://github.com/cert-manager/cert-manager/pull/5231), @JoooostB @SgtCoDFish)

### Other (Cleanup)

- Bump distroless base images to latest versions ([#5235](https://github.com/cert-manager/cert-manager/pull/5235), @SgtCoDFish)
