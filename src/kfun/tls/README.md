These kfuns manage a TLS session.  The current object is associated with the
OpenSSL context, which must be freed using `tls_close()` before the object is
destructed.  The configuration argument for the module specifies the
directory which contains the certificate chain and key files, both in PEM
format.

Several constraints apply.  Since the TLS state is kept inside OpenSSL
datastructures, these functions cannot be used from atomic code, and will
not work properly in Hydra where the entire task in which they are used
could be rolled back.  Furthermore, the TLS state will not persist during
a hotboot; `tls_close()` must be called before the hotboot occurs.
