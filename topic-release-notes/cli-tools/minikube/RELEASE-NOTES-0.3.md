# minikube v0.3 Release Notes

Source: [v0.3.0](https://github.com/kubernetes/minikube/releases/tag/v0.3.0)

# Minikube v0.3.0

Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

## Distribution

Minikube is only distributed in binary form for Linux and OSX systems for the v0.3.0 release. Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.3.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.3.0/minikube-linux-amd64)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.3.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.3.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/master/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare SHA1 hashes with these values:

### OSX

``` shell
$ openssl sha1 out/minikube-darwin-amd64
SHA1(out/minikube-darwin-amd64)= d87ac03acee70008b419bfaaea4be0404e703e35
```

### Linux

``` shell
$ openssl sha1 out/minikube-linux-amd64
SHA1(out/minikube-linux-amd64)= 79693d1ec96c65eb0e267b8f9bd11eb2d5f0c75f
```

### ISO

``` shell
$ openssl sha1 deploy/iso/minikube.iso
SHA1(deploy/iso/minikube.iso)= 351dba63523aaac5f11e890fe31068a054a3458a
```
