# Eusebius

## A tiny operating system

Written in **kaba**.

## How to build and run

Mostly, you need the **kaba** compiler.

```
git clone https://github.com/momentarylapse/kaba.git
cd kaba
git checkout a437b2e369cd230c5c851e8f10a296c423a96f3b
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


