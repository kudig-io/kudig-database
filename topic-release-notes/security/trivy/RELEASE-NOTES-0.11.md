# trivy v0.11 Release Notes

Source: [v0.11.0](https://github.com/aquasecurity/trivy/releases/tag/v0.11.0)

## New features
### Support AWS Security Hub (#594)
See [here](https://github.com/aquasecurity/trivy/blob/master/docs/integration/security-hub.md) for the detail.

### Support --skip-dirs option (#595)
Specify the directory where the traversal is skipped.

```
$ trivy image --skip-dirs "/usr/lib/ruby/gems,/etc" fluent/fluentd:edge
```

### Support custom data sources (#613)
Your custom data source can be added into trivy-db. See #613 for details.

## Fixes
### fix(alpine): use source package (#607)
**IMPORTANT**: Trivy shows sub packages which are actually installed in the image, instead of an origin package. You should clear the cache once after Trivy is updated to v0.11.0.

For example, the origin package of `libcrypto1.1` is `openssl` and Trivy used to display vulnerabilities of `openssl` even when `openssl` is not installed. Now, Trivy displays vulnerabilities of `libcrypto1.1`.

Before:

```
alpine:3.10.2 (alpine 3.10.2)
=============================
Total: 5 (UNKNOWN: 0, LOW: 1, MEDIUM: 4, HIGH: 0, CRITICAL: 0)

+---------+------------------+----------+-------------------+---------------+--------------------------------+
| LIBRARY | VULNERABILITY ID | SEVERITY | INSTALLED VERSION | FIXED VERSION |             TITLE              |
+---------+------------------+----------+-------------------+---------------+--------------------------------+
| openssl | CVE-2019-1549    | MEDIUM   | 1.1.1c-r0         | 1.1.1d-r0     | openssl: information           |
|         |                  |          |                   |               | disclosure in fork()           |
+         +------------------+          +                   +---------------+--------------------------------+
|         | CVE-2019-1551    |          |                   | 1.1.1d-r2     | openssl: Integer overflow in   |
|         |                  |          |                   |               | RSAZ modular exponentiation on |
|         |                  |          |                   |               | x86_64                         |
+         +------------------+          +                   +---------------+--------------------------------+
|         | CVE-2019-1563    |          |                   | 1.1.1d-r0     | openssl: information           |
|         |                  |          |                   |               | disclosure in PKCS7_dataDecode |
|         |                  |          |                   |               | and CMS_decrypt_set1_pkey      |
+         +------------------+          +                   +---------------+--------------------------------+
|         | CVE-2020-1967    |          |                   | 1.1.1g-r0     | openssl: Segmentation fault in |
|         |                  |          |                   |               | SSL_check_chain causes denial  |
|         |                  |          |                   |               | of service                     |
+         +------------------+----------+                   +---------------+--------------------------------+
|         | CVE-2019-1547    | LOW      |                   | 1.1.1d-r0     | openssl: side-channel weak     |
|         |                  |          |                   |               | encryption vulnerability       |
+---------+------------------+----------+-------------------+---------------+--------------------------------+
```

After

```
alpine:3.10.2 (alpine 3.10.2)
=============================
Total: 10 (UNKNOWN: 0, LOW: 2, MEDIUM: 8, HIGH: 0, CRITICAL: 0)

+--------------+------------------+----------+-------------------+---------------+--------------------------------+
|   LIBRARY    | VULNERABILITY ID | SEVERITY | INSTALLED VERSION | FIXED VERSION |             TITLE              |
+--------------+------------------+----------+-------------------+---------------+--------------------------------+
| libcrypto1.1 | CVE-2019-1549    | MEDIUM   | 1.1.1c-r0         | 1.1.1d-r0     | openssl: information           |
|              |                  |          |                   |               | disclosure in fork()           |
+              +------------------+          +                   +---------------+--------------------------------+
|              | CVE-2019-1551    |          |                   | 1.1.1d-r2     | openssl: Integer overflow in   |
|              |                  |          |                   |               | RSAZ modular exponentiation on |
|              |                  |          |                   |               | x86_64                         |
+              +------------------+          +                   +---------------+--------------------------------+
|              | CVE-2019-1563    |          |                   | 1.1.1d-r0     | openssl: information           |
|              |                  |          |                   |               | disclosure in PKCS7_dataDecode |
|              |                  |          |                   |               | and CMS_decrypt_set1_pkey      |
+              +------------------+          +                   +---------------+--------------------------------+
|              | CVE-2020-1967    |          |                   | 1.1.1g-r0     | openssl: Segmentation fault in |
|              |                  |          |                   |               | SSL_check_chain causes denial  |
|              |                  |          |                   |               | of service                     |
+              +------------------+----------+                   +---------------+--------------------------------+
|              | CVE-2019-1547    | LOW      |                   | 1.1.1d-r0     | openssl: side-channel weak     |
|              |                  |          |                   |               | encryption vulnerability       |
+--------------+------------------+----------+                   +               +--------------------------------+
| libssl1.1    | CVE-2019-1549    | MEDIUM   |                   |               | openssl: information           |
|              |                  |          |                   |               | disclosure in fork()           |
+              +------------------+          +                   +---------------+--------------------------------+
|              | CVE-2019-1551    |          |                   | 1.1.1d-r2     | openssl: Integer overflow in   |
|              |                  |          |                   |               | RSAZ modular exponentiation on |
|              |                  |          |                   |               | x86_64                         |
+              +------------------+          +                   +---------------+--------------------------------+
|              | CVE-2019-1563    |          |                   | 1.1.1d-r0     | openssl: information           |
|              |                  |          |                   |               | disclosure in PKCS7_dataDecode |
|              |                  |          |                   |               | and CMS_decrypt_set1_pkey      |
+              +------------------+          +                   +---------------+--------------------------------+
|              | CVE-2020-1967    |          |                   | 1.1.1g-r0     | openssl: Segmentation fault in |
|              |                  |          |                   |               | SSL_check_chain causes denial  |
|              |                  |          |                   |               | of service                     |
+              +------------------+----------+                   +---------------+--------------------------------+
|              | CVE-2019-1547    | LOW      |                   | 1.1.1d-r0     | openssl: side-channel weak     |
|              |                  |          |                   |               | encryption vulnerability       |
+--------------+------------------+----------+-------------------+---------------+--------------------------------
```

### fix: remove error using no options (#539)

Before:

```
$ trivy 
2020-06-18T10:28:44.983+0100	ERROR	trivy requires at least 1 argument or --input option
NAME:
   trivy - A simple and comprehensive vulnerability scanner for containers
...
```

After:

```
$ trivy 
NAME:
   trivy - A simple and comprehensive vulnerability scanner for containers
...
```


## Changelog

f50b0ce feat(library): support a custom data source (#613)
ed8607b fix(alpine): use source package (#607)
ea28d3b test(vulnerability): fix usages of new trivy-db refactor changes (#611)
827cea3 refactor(bundler): remove unnecessary code (#610)
b2a0d83 codecov: Move into root directory (#608)
85e0139 fix: fullDescription field in SARIF output is not correctly escaped (#605)
80d5df0 chore(docs): add AWS Security Hub (#598)
3a54e5b refactor(writer): define the constructor for TemplateWriter (#597)
acc6a9b circleci: Allow coverage changes without a failure (#599)
96af6dc feat: add --skip-directories option (#595)
675e1b4 Added test and support of ASFF template (#594)
8ca484f fix: remove error using no options (#539)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.11.0`
- `docker pull docker.io/aquasec/trivy:latest`
