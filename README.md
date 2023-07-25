# Eusebius

## A tiny operating system

Written in **kaba**.

## How to build and run

Mostly, you need the **kaba** compiler.

```
git clone https://github.com/momentarylapse/kaba.git
cd kaba
git checkout 26ef4b0d83bade7821fbce8fcff0fdf420eb976e
mkdir build
cd build
ccmake ..
make
sudo make install
```

You also need **qemu-system-x86_64**.

Then in the **eusebius** folder:
```
make
bash run.sh
```


