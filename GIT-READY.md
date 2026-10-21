# Source-only Git handoff (R4L)

This companion source archive intentionally excludes all original Microsoft
LIBC.LIB/OS2386.LIB, precompiled OBJ files, Microsoft's reference executables,
and the binary Sarien application. **Review ownership and licensing of all
sources before making the repository public.** No license is automatically
asserted for this project or third-party input files.

The C89 linker itself builds without the fixtures:

```
make link386r4l
```

To run the fixture tests, privately populate `fixtures/` from your authorized
copy of the full R4L private test archive (or the original toolchain). Run:

```
make verify-r4l verify-r4e-fix1 verify-r4d-fix1
```

Example initial local repository:

```
git init
 git add .
 git commit -m "LINK386 R4L: generic OMF dependency layout, frozen R4K proof"
 git tag r4l-build-verified
```

`R4K` is a separate **immutable** archived live-proven snapshot. Its source
still contains the old evidence-derived placement manifest; R4L removed it.
R4L's test inputs and outputs are not included in this Git source archive.
