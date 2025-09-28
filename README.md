# Eusebius

## A tiny operating system

Written in **kaba**.

## How to build and run

Mostly, you need the **kaba** compiler.

```
git clone https://github.com/momentarylapse/kaba.git
cd kaba
git checkout 1754e1712ce3a6a6ea6d9757bddbaa5a2b7ce5c5
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


