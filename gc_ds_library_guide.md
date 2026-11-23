# Garbage-Collected Data Structure Library — Implementation Guide

A roadmap for turning the current `object_t` implementation into a small C library using **mark-and-sweep garbage collection**.

## 1. Define the ownership model

Use these rules throughout the library:

- The VM owns every tracked `object_t`.
- `vm->objects` stores every allocated object managed by that VM.
- `vm->frames` stores active function frames.
- Each frame stores pointers to objects used by that invocation.
- Objects such as vectors may point to other objects.
- Frames own their reference stacks, but do not own the referenced objects.
- Vectors own their pointer arrays, but do not own the objects in those arrays.
- Only the VM/GC destroys registered objects.

```text
VM
├── frames stack ──► active frames ──► object references
└── objects stack ──────────────────► all tracked objects

Reachable object graph:
Obj A ──► Obj B ──► Obj C
```

`frames` provides the roots from which marking starts. `objects` is the registry that sweeping examines.

## 2. Complete the object model

**Files:** `object.h`, `object.c`

You already have:
- `object_t` with a type tag and a union.
- Constructors for integers, floats, strings, and object vectors.
- Heap allocation for string contents and vector storage.

### Requirements

- [ ] Declare `object_vector_t` before using it in `object_data_t`, or forward-declare it.
- [ ] Add `length` to `object_vector_t`; add `capacity` too if vectors can grow.
- [ ] Initialize every field in every constructor.
- [ ] Keep `type` consistent with the active union member.
- [ ] Document that `object_new_string(value, size)` copies exactly `size` bytes and requires that many readable bytes at `value`.
- [ ] Define vector initialization clearly. For example, `length = size` with all slots initialized to `NULL`, or `length = 0, capacity = size`. Do not mix these meanings.
- [ ] On partial allocation failure, release every allocation already made.
- [ ] Do not let callers directly `free()` objects managed by the GC.

### Important issue in the current code

Your current `object_vector_t` only stores `object_t **objects`. The collector needs to know how many entries are valid, so add a `length` field at minimum.

Also, `object_vector_t` is used in `object_data_t` before its typedef appears. One fix is:

```c
typedef struct ObjectVector object_vector_t;

typedef union ObjectData {
    int v_int;
    float v_float;
    char *v_string;
    object_vector_t *v_object_vector;
} object_data_t;

struct ObjectVector {
    object_t **objects;
    size_t length;
    size_t capacity;
};
```

This sketch assumes `object_t` is also declared before `struct ObjectVector` is defined. Alternatively, arrange the declarations with forward declarations for both structs.

## 3. Add object destruction

**Files:** `object.h`, `object.c`

Create one function that releases the memory owned by an object:

```c
void object_destroy(object_t *obj);
```

### Requirements

- [ ] Switch on `obj->type`.
- [ ] `OBJ_INTEGER` and `OBJ_FLOAT`: no separately allocated payload; free the `object_t`.
- [ ] `OBJ_STRING`: free `data.v_string`, then the `object_t`.
- [ ] `OBJ_VECTOR`: free the vector's pointer array, then its wrapper, then the `object_t`.
- [ ] Do **not** free objects referenced by a vector. They are independently managed by the VM.
- [ ] Ensure each allocation is freed exactly once.

The GC sweep phase will call this function for unreachable objects.

## 4. Implement a generic stack

**Files:** `stack.h`, `stack.c`

Use the stack for the VM's frame registry, object registry, and each frame's object references.

Suggested API:

```c
typedef struct {
    void **elements;
    size_t size;
    size_t capacity;
} stack_t;

stack_t *stack_new(size_t initial_capacity);
bool stack_push(stack_t *stack, void *value);
void *stack_pop(stack_t *stack);
void *stack_peek(const stack_t *stack);
void stack_free(stack_t *stack);
```

### Requirements

- [ ] If `stack_new()` returns `stack_t *`, allocate the stack structure dynamically.
- [ ] Allocate the backing array separately.
- [ ] Track the number of used entries (`size`) and allocated slots (`capacity`).
- [ ] Grow the array when full, checking for size/byte-count overflow.
- [ ] Handle `realloc()` failure without losing the original pointer.
- [ ] Define the behavior of `pop` and `peek` on an empty stack.
- [ ] `stack_free()` frees the backing array and then the stack structure.
- [ ] `stack_free()` does not free the pointers stored in the stack; the generic stack does not own its elements.

## 5. Implement frames and root management

**Files:** `frame.h`, `frame.c`

A frame represents one active function invocation.

```c
typedef struct {
    stack_t *references; /* stores object_t * */
} frame_t;
```

Suggested API:

```c
frame_t *frame_new(void);
bool frame_add_reference(frame_t *frame, object_t *obj);
void frame_free(frame_t *frame);
```

### Requirements

- [ ] `frame_new()` allocates the frame and creates its `references` stack.
- [ ] If stack creation fails, free the frame and return `NULL`.
- [ ] `frame_add_reference()` stores an object pointer in the frame.
- [ ] Decide how reassignment/removal of a local variable updates the references.
- [ ] `frame_free()` frees the reference stack, then the frame.
- [ ] `frame_free()` must not free the referenced objects.

For the first version, active frames can be your GC roots. Later, you may add global variables or VM-owned temporary values as additional root categories.

**Safe-point warning:** If allocation can trigger GC, root a newly created object before performing another allocation that might collect it. Otherwise, an object held only in a C local variable may be invisible to your collector.

## 6. Implement the VM/runtime

**Files:** `vm.h`, `vm.c`

```c
typedef struct {
    stack_t *frames;  /* stores frame_t * */
    stack_t *objects; /* stores object_t * */
} vm_t;
```

Suggested API:

```c
vm_t *vm_new(void);
void vm_free(vm_t *vm);

bool vm_push_frame(vm_t *vm, frame_t *frame);
frame_t *vm_pop_frame(vm_t *vm);

object_t *vm_new_int(vm_t *vm, int value);
object_t *vm_new_float(vm_t *vm, float value);
object_t *vm_new_string(vm_t *vm, const char *value, size_t size);
object_t *vm_new_object_vector(vm_t *vm, size_t size);
```

### Requirements

- [ ] `vm_new()` allocates the VM and initializes both stacks.
- [ ] If any initialization fails, free everything already allocated.
- [ ] Every successful VM-aware constructor registers its object exactly once in `vm->objects`.
- [ ] If registration fails, destroy the newly created object and return `NULL`.
- [ ] `vm_push_frame()` pushes a frame pointer.
- [ ] Define whether `vm_pop_frame()` only returns the frame or also frees it. Document the ownership contract.
- [ ] `vm_free()` frees frames, tracked objects, and the VM without double-freeing anything.
- [ ] Keep raw object construction private if possible; expose VM-aware constructors so callers cannot accidentally create untracked objects.

**Important connection to your current code:** `object_new_int()` and the other current constructors allocate objects, but they do not register them with a VM. An object absent from `vm->objects` cannot be found by that VM's sweep phase. You will need VM-aware constructors (or another explicit registration step).

## 7. Implement marking

**Files:** `gc.h`, `gc.c`

Add a mark flag to each object:

```c
typedef struct Object {
    object_type_t type;
    bool marked;
    object_data_t data;
} object_t;
```

### Requirements

- [ ] Start each collection with a well-defined mark state (for example, clear all marks before traversing roots).
- [ ] Visit every object reference in every active frame.
- [ ] Mark an object before traversing its children.
- [ ] For a vector, visit each valid, non-`NULL` entry from index `0` to `length - 1`.
- [ ] Integers, floats, and strings have no object references to traverse.
- [ ] Do not traverse an object again once it is already marked.

Marking before traversing children prevents infinite traversal through cycles.

```text
Frame ──► Obj A ──► Obj B
           ▲          │
           └──────────┘
```

If the frame stops referencing A and there are no other roots, neither object is marked and both can be swept.

## 8. Implement sweeping

**Files:** `gc.c`

### Requirements

- [ ] Iterate over every object in `vm->objects`.
- [ ] Keep marked objects.
- [ ] Destroy unmarked objects with `object_destroy()`.
- [ ] Remove freed pointers from the object registry.
- [ ] When removing an entry from an array-backed stack, do not advance the index if the next entry shifts into the current slot.
- [ ] Reset the mark flag on surviving objects for the next collection.

After sweeping, the registry must contain only surviving objects.

## 9. Connect allocation and collection safely

**Files:** `vm.c`, `gc.c`

Start with explicit collection:

```c
void gc_collect(vm_t *vm);
```

Add automatic collection later, perhaps when the tracked-object count passes a threshold.

### Requirements

- [ ] Run collection only when roots and the object graph are in a consistent state.
- [ ] Ensure temporary live values are rooted before collection can occur.
- [ ] Never register an object twice.
- [ ] Never leave a partially initialized object in the registry.
- [ ] Decide whether constructors may trigger GC. If they can, protect intermediate objects.

Explicit collection is easier to debug while the collector is being developed.

## 10. Add vector operations

**Files:** `object.c` or `vector.c`

Possible API:

```c
bool object_vector_set(object_t *vector, size_t index, object_t *value);
object_t *object_vector_get(const object_t *vector, size_t index);
```

You may later add append and resize operations.

### Requirements

- [ ] Verify that the object is `OBJ_VECTOR`.
- [ ] Check index bounds.
- [ ] Define whether `NULL` entries are allowed.
- [ ] Keep `length` and `capacity` accurate.
- [ ] Ensure every stored pointer is visited by the marker.
- [ ] Replacing an entry must not free the old object; it may be referenced elsewhere.
- [ ] Vector operations must not directly free objects.

## 11. Test each layer

### Stack tests

- [ ] Push and pop pointers in LIFO order.
- [ ] Peek without removing.
- [ ] Pop/peek an empty stack.
- [ ] Trigger capacity growth.
- [ ] Handle allocation failure.

### Object tests

- [ ] Construct each object type and verify its contents.
- [ ] Verify string copying and the trailing `'\0'`.
- [ ] Verify vector length, capacity, and initial contents.
- [ ] Destroy each object type without leaks.

### Frame and VM tests

- [ ] Create a VM and push multiple frames.
- [ ] Add object references to frames.
- [ ] Pop a frame and verify that referenced objects are not immediately destroyed.
- [ ] Verify that freeing a frame does not free its referenced objects.

### Garbage-collection tests

- [ ] A rooted object survives collection.
- [ ] An unrooted object is collected.
- [ ] An object referenced by two frames survives after only one frame is popped.
- [ ] An object referenced by a reachable vector survives even if no frame points to it directly.
- [ ] An unreachable cycle is collected.
- [ ] A reachable cycle survives.
- [ ] `NULL` references are ignored.
- [ ] No freed pointer remains in the object registry.

Use AddressSanitizer and UndefinedBehaviorSanitizer during development, plus a leak checker where available.

## 12. Suggested project layout

```text
gc_ds/
├── include/
│   ├── object.h
│   ├── stack.h
│   ├── frame.h
│   ├── vm.h
│   └── gc.h
├── src/
│   ├── object.c
│   ├── stack.c
│   ├── frame.c
│   ├── vm.c
│   └── gc.c
├── tests/
│   ├── test_stack.c
│   ├── test_object.c
│   ├── test_frame.c
│   ├── test_vm.c
│   └── test_gc.c
└── README.md
```

You do not need to create every file immediately. Implement one layer, test it, and then move to the next.

## Recommended implementation order

1. Complete `object_t` and `object_vector_t`.
2. Implement and test `object_destroy()`.
3. Implement and test `stack_t`.
4. Implement and test `frame_t`.
5. Implement `vm_t` and VM-aware object constructors.
6. Add the mark flag and implement marking.
7. Implement sweeping.
8. Add vector operations and graph traversal.
9. Add GC tests, especially shared references and cycles.
10. Consider automatic collection only after explicit collection works reliably.

## Final invariants

Keep these rules true throughout the implementation:

1. Every object managed by a VM appears exactly once in that VM's object registry.
2. Every live object is reachable from at least one GC root by following object references.
3. Frames own their reference stacks, not the objects stored in them.
4. Vectors own their pointer arrays, not the objects pointed to by those arrays.
5. Only the VM/GC destroys registered objects.
6. The sweep phase never frees a marked object.
7. No pointer to a freed object remains in a frame, a reachable object, or the VM's object registry.

If these invariants hold, the mark-and-sweep collector has a clear and testable foundation.
