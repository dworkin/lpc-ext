typedef uint16_t StackSize;

enum StackPointer {
    STACK_EMPTY = 0,
    STACK_INVALID = 0xffff
};

template <class T> class Stack {
public:
    Stack(StackSize size) {
	elements = new Elem[size];
	nElems = STACK_EMPTY;
    }

    virtual ~Stack() {
	delete[] elements;
    }

    StackSize push(StackSize pointer, T item) {
	elements[++nElems].item = item;
	elements[nElems].next = pointer;
	return nElems;
    }

    StackSize pop(StackSize pointer) {
	return elements[pointer].next;
    }

    void set(StackSize pointer, T item) {
	elements[pointer].item = item;
    }

    T get(StackSize pointer) {
	return elements[pointer].item;
    }

private:
    struct Elem {
	T item;
	StackSize next;
    };

    Elem *elements;
    StackSize nElems;
};
