This package adds the following regular expression matching kfun to DGD, using
the DGD extension interface:
```
    regexp(pattern, str, case_insensitive, reverse)
```
For a description of the regular expression language accepted by this kfun,
please read `doc-0.3/regexps`.

To install, move `regexp.c` and `libiberty` into `dgd/src`, apply
`Makefile.diff` to `dgd/src/Makefile`, and fully recompile DGD.

This package is based on Robert Leslie's regular expression matching
package `rgx-0.3`.  The files in the directory `libiberty` were copied from
`gdb-6.8`, and are released under the GNU LGPL.  The file `doc-0.3/regexps` is
from rgx 0.3 (slightly modified by me), and was released under GPLv2.  All
other files are new with this release, and are released under GPLv3.
