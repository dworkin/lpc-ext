The compression functionality offered by zlib allocates internal buffers.
Zlib supports user-supplied allocation functions, allowing the kfun module
to store these buffers in LPC strings.  However, zlib does not support
relocation of buffers, which is needed by the kfun module.

Therefore, the kfun module does its own reallocation, adjusting pointers when
needed.  This means that the kfun module has to know about zlib internal
datastructures, which can change between zlib versions.  Currently, the
kfun module supports zlib versions 1.2.11 and 1.2.8.

Thanks to buffer reallocation, the compression kfuns can be used in atomic
functions (an error will roll back the compression state too) and across
hotboots.  Note that it is not possible to preserve compression state while
hotbooting to a different CPU architecture or to a different zlib version.
