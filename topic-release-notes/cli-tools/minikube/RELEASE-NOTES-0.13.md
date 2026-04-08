# minikube v0.13 Release Notes

Source: [v0.13.1](https://github.com/kubernetes/minikube/releases/tag/v0.13.1)

# Minikube v0.13.1

Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.13.1/CHANGELOG.md).

## Distribution

Minikube is distrubuted in binary form for Linux, OSX, and Windows systems for the v0.13.1 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.13.1/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.13.1/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.13.1/minikube-windows-amd64.exe)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.13.1/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.13.1/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]

Download the `minikube_0.13-1.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]

Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path

### Windows Installer [Experimental]

Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.13.1/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
77bc72679ca1beb09ad7f26ec8ba8b286283ddf7bee4e68163b88c5a439bc049

==> out/minikube-linux-amd64.sha256 <==
52706da92b6cf9a5eb4f59fe6034f119e3eb442c3fc57bf181d01db929e1289f

==> out/minikube-windows-amd64.exe.sha256 <==
5bbb30feb34f09a1d7cee5051bb3b28103eef719ae828bfd8fddb11efc56a5c7
```

### ISO

``` shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)= 
aadc8b6f5720d5a493a36e1f07f71bffb588780c76498d68cd761793d2ca344e
```
