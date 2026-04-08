# prometheus v0.12 Release Notes

Source: [0.12.0](https://github.com/prometheus/prometheus/releases/tag/0.12.0)

This is the release that fixes the annoying and embarrassing fingerprinting bug (https://github.com/prometheus/client_golang/issues/74). All metrics stored with previous versions of Prometheus cannot be used anymore. A version guard will protect you from accidentally running the Prometheus server with an incompatible storage. Implementing a conversion tool would be a lot of work (but if somebody wants to do it, be our guest...), so there is no other solution right now but wiping the storage or stick with v0.11.1.

To sweeten the deal, fingerprinting is now more efficient, and we have also thrown in new features (OR operator and vector matching options).
- [CHANGE] Use client_golang v0.3.1. THIS CHANGES FINGERPRINTING AND INVALIDATES
  ALL PERSISTED FINGERPRINTS. You have to wipe your storage to use this or
  later versions. There is a version guard in place that will prevent you to
  run Prometheus with the stored data of an older Prometheus.
- [BUGFIX] The change above fixes a weakness in the fingerprinting algorithm.
- [ENHANCEMENT] The change above makes fingerprinting faster and less allocation
  intensive.
- [FEATURE] OR operator and vector matching options. See docs for details.
- [ENHANCEMENT] Scientific notation and special float values (Inf, NaN) now
  supported by the expression language.
- [CHANGE] Dockerfile makes Prometheus use the Docker volume to store data
  (rather than /tmp/metrics).
- [CHANGE] Makefile uses Go 1.4.2.
