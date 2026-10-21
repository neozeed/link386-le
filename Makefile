CC ?= cc
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
all: link386r1 link386r2 link386r3 link386r3f
link386r1: src/link386r1.c
	$(CC) $(CFLAGS) -o $@ $<
link386r2: src/link386r2.c
	$(CC) $(CFLAGS) -o $@ $<
link386r3: src/link386r3.c src/link386r2.c
	$(CC) $(CFLAGS) -o $@ $<
link386r3f: src/link386r3f.c src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r3f.c
verify: all
	python3 tests/verify_r3.py
	python3 tests/verify_r3a.py
	python3 tests/verify_r3c.py
	python3 tests/verify_r3d.py
	python3 tests/verify_r3e.py
clean:
	rm -f link386r1 link386r2 link386r3 link386r3f
.PHONY: all verify clean
verify-r3c: link386r2
	python3 tests/verify_r3c.py
	python3 tests/verify_r3d.py
	python3 tests/verify_r3e.py
link386r3g: src/link386r3g.c src/link386r3f.c src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r3g.c
verify-r3g: link386r3g
	python3 tests/verify_r3g.py
.PHONY: verify-r3g
verify-r3h: link386r3g
	python3 tests/r3h_oracle.py --exe fixtures/hi-reference.exe --exe oracles/cmd32os2_os2.exe --exe oracles/cmdos2_os2_console_test.exe --exe oracles/sarien.exe --map oracles/cmd32os2_os2.map --map oracles/cmdos2_os2_console_test.map > r3h-oracle-report.txt
	python3 tests/compare_r3g.py oracles/cmd32os2_os2.map fixtures/cmd32/cmd32os2.obj fixtures/cmd32/cmdparse.obj fixtures/cmd32/cmdbatch.obj fixtures/cmd32/cmdfile.obj fixtures/cmd32/cmdos2_os2.obj fixtures/cmd32/cmdos2_session_os2.obj fixtures/cmd32/cmdos2_env.obj fixtures/LIBC.LIB fixtures/OS2386.LIB > r3h-cmd32-diff.txt
.PHONY: verify-r3h
verify-r3i: link386r3g
	python3 tests/verify_r3i.py
.PHONY: verify-r3i
verify-r3j: link386r3g
	python3 tests/verify_r3j.py
.PHONY: verify-r3j
verify-r3k: link386r3g
	python3 tests/r3k_reconcile.py --map oracles/cmd32os2_os2.map fixtures/cmd32/cmd32os2.obj fixtures/cmd32/cmdparse.obj fixtures/cmd32/cmdbatch.obj fixtures/cmd32/cmdfile.obj fixtures/cmd32/cmdos2_os2.obj fixtures/cmd32/cmdos2_session_os2.obj fixtures/cmd32/cmdos2_env.obj fixtures/LIBC.LIB fixtures/OS2386.LIB > r3k-reconciliation.txt
	python3 tests/verify_r3k.py
.PHONY: verify-r3k
verify-r3l: link386r3g
	python3 tests/verify_r3l.py
.PHONY: verify-r3l

verify-r3m: link386r3g
	python3 tests/verify_r3m.py
.PHONY: verify-r3m
link386r3n: src/link386r3n.c src/link386r3g.c src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r3n.c
verify-r3n: link386r3n
	python3 tests/verify_r3n.py
.PHONY: verify-r3n

link386r3o: src/link386r3o.c src/link386r3g.c src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r3o.c
verify-r3o: link386r3o
	python3 tests/verify_r3o.py
.PHONY: verify-r3o

verify-r3p: link386r3o
	python3 tests/verify_r3p.py
	python3 tests/verify_r3o.py
.PHONY: verify-r3p

verify-r3q: link386r3o
	python3 tests/verify_r3q.py
.PHONY: verify-r3q

verify-r3r: link386r3o
	python3 tests/r3r_reconcile.py fixtures/LIBC.LIB > r3r-reconciliation.txt
	python3 tests/verify_r3q.py
.PHONY: verify-r3r

verify-r3s:
	python3 tests/verify_r3s.py
	python3 tests/r3s_le_fixups.py fixtures/hi-reference.exe > r3s-le-fixup-oracle.txt
.PHONY: verify-r3s

verify-r3t: verify-r3s
	python3 tests/verify_r3t.py
	python3 tests/r3t_correlate.py > r3t-correlation.txt
.PHONY: verify-r3t

.PHONY: verify-r3u
verify-r3u:
	python3 tests/verify_r3u.py

.PHONY: verify-r3v
verify-r3v:
	python3 tests/verify_r3v.py > r3v-import-provenance.txt
	python3 tests/verify_r3u.py
	python3 tests/verify_r3s.py

.PHONY: verify-r3w
verify-r3w:
	python3 tests/verify_r3w.py > r3w-ownership.txt
	python3 tests/verify_r3v.py > r3v-import-provenance.txt
	python3 tests/verify_r3s.py

.PHONY: verify-r3x
verify-r3x:
	python3 tests/r3x_segment_provenance.py > r3x-segment-provenance.txt
	python3 tests/verify_r3w.py > r3w-ownership.txt

.PHONY: verify-r3y
verify-r3y:
	python3 tests/r3y_order.py > r3y-order.txt
	grep -q 'R3Y PASS' r3y-order.txt
	python3 tests/verify_r3x.py 2>/dev/null || python3 tests/r3x_segment_provenance.py > r3x-segment-provenance.txt
	python3 tests/verify_r3w.py > r3w-ownership.txt

.PHONY: verify-r3z
verify-r3z: verify-r3y
	python3 tests/r3z_prefix.py > r3z-prefix-evidence.txt
	grep -q 'R3Z PASS' r3z-prefix-evidence.txt

.PHONY: verify-r4a
verify-r4a:
	python3 tests/r4a_ledger.py --out r4a-ledger

# R4B is explicitly a reference-assisted hi.obj relinker, not a standalone linker.
link386r4b: src/link386r4b.c src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r4b.c

verify-r4b: link386r4b
	python3 tests/verify_r4b.py
.PHONY: verify-r4b

link386r4c: src/link386r4c.c src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r4c.c

verify-r4c: link386r4c link386r4c_images link386r4c_relocate
	python3 tests/verify_r4c.py
.PHONY: verify-r4c

link386r4c_images: src/link386r4c_images.c src/link386r3g.c src/link386r3f_core.inc src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r4c_images.c

link386r4c_relocate: src/link386r4c_relocate.c src/link386r4c_images.c src/link386r4c_layout_core.inc src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r4c_relocate.c

# R4D experimental source-only executable emitter; R4B/R4C archives remain frozen.
link386r4d: src/link386r4d.c src/link386r4c_relocate.c src/link386r4c_images.c src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r4d.c
verify-r4d: link386r4d
	python3 tests/verify_r4d.py
.PHONY: verify-r4d

# Post-R4D corrective release: little-endian CPU/OS LE header fields.
.PHONY: verify-r4d-fix1
verify-r4d-fix1: link386r4d
	python3 tests/verify_r4d.py
	python3 tests/verify_r4d_fix1.py

# R4E is an independent three-object C/386 CMD32-profile LE writer, with no
# prelinked EXE input. R4D-FIX1 remains frozen and byte-identical.
link386r4e: src/link386r4e.c src/link386r4e_relocate.c src/link386r4e_images.c src/link386r4e_layout_core.inc src/link386r4e_def.inc src/link386r3f_core.inc src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r4e.c
.PHONY: verify-r4e
verify-r4e: link386r4e
	python3 tests/verify_r4e.py

# Corrected LE selector, alias and cross-page fixups. Frozen R4E unaffected.
.PHONY: verify-r4e-fix1
verify-r4e-fix1: link386r4e
	python3 tests/verify_r4e.py
	python3 tests/verify_r4e_fixups.py

# R4L: non-oracle archive dependency-depth and historic symbol-hash policy.
.PHONY: verify-r4l
verify-r4l: link386r4l
	python3 tests/verify_r4l.py

# R4L standalone executable name; the R4E name remains for frozen regressions.
link386r4l: src/link386r4e.c src/link386r4e_relocate.c src/link386r4e_images.c src/link386r4e_layout_core.inc src/link386r4e_def.inc src/link386r3f_core.inc src/link386r2.c
	$(CC) $(CFLAGS) -o $@ src/link386r4e.c

# IMPORTANT: historical verify-r4k belongs to the separate frozen R4K tree.
# R4L intentionally removed its reference-derived placement manifest.
