## Simple saver for `xsecurelock`

### Requirements
A working `xsecurelock` installation.

### Build & Install
Omit `mode=release` to enable debugging output.
```
make mode=release
sudo make install
```

### Usage
```
XSECURELOCK_SAVER=/usr/local/bin/saver_bastidest/saver_bastidest_random xsecurelock
```
