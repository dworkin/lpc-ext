# define alloc(type, count)	((type *) jit_alloc(sizeof(type) * (count)))

extern void	*jit_alloc	(size_t);
extern void	 fatal		(const char*, ...);
