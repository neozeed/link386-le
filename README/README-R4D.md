# LINK386 R4D — first independent (source-only) C/386 LE executable builder

`link386r4d` is a portable ISO C89 utility that links the *specific verified* Microsoft C/386 `hi.obj` profile against the original `LIBC.LIB` and `OS2386.LIB`, writes its own MZ/LE headers, page map, fixup tables, DOSCALLS imports and data pages, and does **not** need a prelinked Microsoft `hi.exe` scaffold.

**Status: Static/oracle verified. Live Windows runtime not yet tested.** It is **not a general-purpose LINK386 substitute**.

## Build

```sh
make link386r4d
make verify-r4d
./link386r4d -o build/r4d/hi-r4d-source-only.exe fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
```

Using MinGW-w64 on Windows (in the extracted project root):

```sh
gcc -O2 -std=c89 -pedantic -Wall -Wextra -Werror -o link386r4d.exe src/link386r4d.c
link386r4d.exe -o hi-r4d.exe fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
```

The output directory needs to exist. `link386r4d` writes an additional `relocation-candidates-r4d.tsv` report and `object*-r4d-candidate-relocated.bin` snapshots there.

The included `build/r4d/hi-r4d-source-only.exe` and `hi-yo-r4d-source-only.exe` are generated from real original and checksum-correct modified `hi.obj` respectively.

The verification suite reads the Microsoft reference *only after generating the outputs*, then compares **all virtual object bytes** and **all 171 loader-site and target tuples**.

See `HANDOFF-R4D.md` for historical details, accuracy boundaries and next tasks.
