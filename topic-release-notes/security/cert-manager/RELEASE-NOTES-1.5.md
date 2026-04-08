# cert-manager v1.5 Release Notes

Source: [v1.5.5](https://github.com/cert-manager/cert-manager/releases/tag/v1.5.5)

# Changelog since v1.5.4

In 1.5.5, we have reverted a change that caused a regression in the ACME Issuer.
Before 1.5.4, the Ingress created by cert-manager while solving an HTTP-01 challenge contained the `kubernetes.io/ingress.class` annotation:

```yaml
apiVersion: networking.k8s.io/v1beta1
kind: Ingress
metadata:
  annotations:
    kubernetes.io/ingress.class: istio # The `class` present on the Issuer.
```
In 1.5.4, the Ingress does not contain the annotation anymore. Instead, cert-manager uses the `ingressClassName` field:

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
spec:
  ingressClassName: istio # 🔥 Breaking change!
```

This broke many users that either don't use an Ingress controller that supports the field (such as ingress-gce and Azure AGIC), as well as people who did not need to create an IngressClass previously (such as with Istio and Traefik).

The regression is present in cert-manager 1.5.4, 1.6.0, 1.6.1. It is only present on Kubernetes 1.19+ and only appears when using an Issuer or ClusterIssuer with an ACME HTTP-01 solver configured.

In 1.5.5, we have restored the original behavior which is to use the annotation. We will also backport this fix to 1.5.5 and 1.6.2, allowing people to upgrade safely.

Most people won't have any trouble upgrading from a version that contains the regression to 1.7.0, 1.6.2 or 1.5.5. If you are using Gloo, Contour, Skipper, or kube-ingress-aws-controller, you shouldn't have any issues. If you use the default "class" (e.g., `istio` for Istio) for Traefik, Istio, Ambassador, or ingress-nginx, then these should also continue to work without issue.

If you are using Traefik, Istio, Ambassador, or ingress-nginx _and_ you are using a non-default value for the class (e.g., `istio-internal`), or if you experience any issues with your HTTP-01 challenges please read the [notes on Ingress v1 compatibility].

[notes on Ingress v1 compatibility]: https://cert-manager.io/docs/installation/upgrading/ingress-class-compatibility/

## Changes by Kind

### Bug or Regression

- Fixed a regression where cert-manager was creating Ingresses using the field `ingressClassName` instead of the annotation `kubernetes.io/ingress.class`. More details about this regression are available [in the 1.7 release notes](https://cert-manager.io/next-docs/release-notes/release-notes-1.7/#ingress-class-semantics). ([#4783](https://github.com/jetstack/cert-manager/pull/4783), [@maelvls](https://github.com/maelvls))

### Other (Cleanup or Flake)

- cert-manager now does one call to the ACME API instead of two when an Order fails. This belongs to the effort towards mitigating [the high load](https://github.com/jetstack/cert-manager/issues/3298) that cert-manager deployments have on the Let's Encrypt API ([#4618](https://github.com/jetstack/cert-manager/pull/4618), [@irbekrm](https://github.com/irbekrm))
