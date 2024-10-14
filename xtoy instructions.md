Build xcc:
```sh
make ARCHTYPE=xtoy PLATFORM=xtoy cc1 cpp xcc
```
Run xcc:
```sh
./xcc -S examples/minimal.c -o test.xasm
```

https://github.com/JoshuaWierenga/xasm
Build xasm:
```sh
make
```

Copy test.xasm to xasm folder, it currently only looks for files with that name in it's own folder.
Run xasm:
```sh
./xasm
```

Copy output without `Output:` line into Visual X-Toy and run as normal.

