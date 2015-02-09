#include "gc.h"

// Linked list node
struct node {
	void *ptr;			// Pointer to be freed
	int protected;		// Scope
	struct node *next;	// Link
};

// Linked list head
struct node *first = NULL;

// Protected array
int *_protected = NULL;
size_t _max_protected = 0;

// First call
int should_srand = 1;

// Generator seed
#define SEED 0x32ff2523

/*
* Appends @new_node to @dest.
*/
void __gc_register_node(struct node *dest, struct node *new_node)
{
	// Iterator
	struct node *it;

	// First node
	if (dest == NULL)
		dest = new_node;

	// Append to the tail
	else {
		for (it = dest; it->next != NULL; it = it->next);
		it->next = new_node;
	}	
}

/*
* Generates node and appends it to first.
*/
void __gc_register_internal(int code, void *ptr)
{
	struct node *new_node = (struct node *) malloc(sizeof(struct node));
	new_node->ptr = ptr;
	new_node->protected = code;
	new_node->next = NULL;
	__gc_register_node(first, new_node);

}

/*
* Zeroes _protected array
*/
void __gc_zero_protected()
{
	size_t i;
	for (i = 0; i < _max_protected; i++)
		_protected[i] = 0;
}

/*
* Checks if @code is in _protected
*/
int __gc_check_protected(int code)
{
	size_t i;
	for (i = 0; i < _max_protected; i++)
		if (_protected[i] == code)
			return code;

	return 0;
}

/*
* Registers @code in first 0 space in _protected.
* __gc_check_protected should be called first
* since this function does not verify existance.
* __gc_protected_full should be called first since
* this function does not verify capacity and @code
* could not be registered.
*/
int __gc_register_code(int code)
{
	size_t i;
	for (i = 0; i < _max_protected; i++)
		if (_protected[i] == 0)
			_protected[i] = code;
}

/*
* Looks for @code and it is replaced with 0 if found.
*/
void __gc_unregister_code(int code)
{
	size_t i;
	for (i = 0; i < _max_protected; i++)
		if (_protected[i] == code)
			_protected[i] = 0;
}

/*
* Checks if there is room for more codes.
*/
int __gc_protected_full()
{
	size_t i;
	for (i = 0; i < _max_protected; i++)
		if (_protected[i] == 0)
			return 0;

	return 1;
}

void gc_enable_protected(size_t max_protected)
{
	if (max_protected <= 0)
		return;

	if (_max_protected > 0)
		return;

	if (should_srand) {
		srand(SEED);
		should_srand = 0;
	}

	_protected = (int *) malloc(sizeof(int) * max_protected);
	_max_protected = max_protected;
	__gc_zero_protected();
}

void gc_release_protected()
{
	if (_protected == NULL)
		return;

	free(_protected);
	_max_protected = 0;
}

int gc_request_protected()
{
	// Protected was not enabled
	if (_max_protected == 0)
		return 0;

	// There is no room for more protected
	if (__gc_protected_full())
		return 0;

	int code = 0;

	// Generate unique protected code
	do code = rand() % RAND_MAX + 1;
	while (__gc_check_protected(code));

	// Register valid code
	__gc_register_code(code);
	return code;
}

void gc_register_protected(int code, void *ptr)
{
	if (code == 0)
		return;

	if (!__gc_check_protected(code))
		return;

	__gc_register_internal(code, ptr);
}

void gc_register(void *ptr)
{
	__gc_register_internal(0, ptr);
}

void gc_execute_protected(int code)
{
	if (code != 0 && !__gc_check_protected(code))
		return;

	struct node *next, *it = first, *head = NULL;

	while (it != NULL) {
		// Save next
		next = it->next;

		// Clean if not protected
		if (it->protected == code) {
			free(it->ptr);
			free(it);
		} else {
			it->next = NULL;
			__gc_register_node(head, it);
		}

		// Next
		it = next;
	}

	// Reset
	first = head;

	if (code != 0) __gc_unregister_code(code);
}

void gc_execute()
{
	gc_execute_protected(0);
}

void gc_force_execute()
{
	struct node *next, *it = first;

	while (it != NULL) {
		// Save next
		next = it->next;

		// Clean
		free(it->ptr);
		free(it);			

		// Next
		it = next;
	}

	// Reset
	first = NULL;

	// Just in case
	gc_release_protected();
}