# minikube v0.25 Release Notes

Source: [v0.25.2](https://github.com/kubernetes/minikube/releases/tag/v0.25.2)

# Minikube v0.25.2
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.25.2/CHANGELOG.md).

## Distribution
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v0.25.2 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.25.2/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.25.2/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.25.2/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.25.2/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.25.2/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_0.25-2.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.25.2/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
dc5b00c4a06e8160bd607732c9a2294598d803716e353293b4463cc2c9539eec

==> out/minikube-linux-amd64.sha256 <==
41d666ddc9ea1eee3d08a939b1075347da7e670c93836d2756ee5ef1daaa1457

==> out/minikube-windows-amd64.exe.sha256 <==
fea03201be88d466ea7cf2da34cd22812d927842100f56c51c7ca1d8b30db32f
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
20f313bcd23da6223540c3e8aa600358c6a1bf13fa9c250980fe1d3827ceec97
```
