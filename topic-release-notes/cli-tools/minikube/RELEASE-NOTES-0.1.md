# minikube v0.1 Release Notes

Source: [v0.1.0](https://github.com/kubernetes/minikube/releases/tag/v0.1.0)

# Minikube v0.1.0

This is the initial release of Minikube. Minikube is still under active development, and features may change at any time.

## Distribution

Minikube is only distributed in binary form for Linux and OSX systems for the v0.1.0 release. Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.1.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.1.0/minikube-linux-amd64)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.1.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.1.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/master/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare SHA1 hashes with these values:

### OSX

``` shell
$ openssl sha1 minikube-darwin-amd64
SHA1(minikube-darwin-amd64)= 3bb14d8edbfce78a629a8fae2aa851222da5d2b6
```

### Linux

``` shell
$ openssl sha1 minikube-linux-amd64
SHA1(minikube-linux-amd64)= 232fab1b77aeeb49efc157811e79ce031b72a182
```

### ISO

``` shell
$ openssl sha1 minikube.iso
SHA1(minikube.iso)= b817e54b1ea44889dcacbb89aa68736b306017c5
```
