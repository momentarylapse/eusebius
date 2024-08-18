# Eusebius

## A tiny operating system

Written in **kaba**.

## How to build and run

Mostly, you need the **kaba** compiler.

```
git clone https://github.com/momentarylapse/kaba.git
cd kaba
git checkout cf121a53a2bc88ad59a66ff2e260b3eff0744e84
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


