#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>

typedef enum ObjectType {
    OBJ_INTEGER,
    OBJ_FLOAT,
    OBJ_STRING,
    OBJ_VECTOR,
} object_type_t;

typedef union ObjectData {
    int v_int;
    float v_float;
    char *v_string;
    object_vector_t *v_object_vector; // pointer to object vector
} object_data_t;

typedef struct Object {
    object_type_t type;
    object_data_t data;
} object_t;

typedef struct ObjectVector {
    object_t **objects; // pointer to list of object pointers
} object_vector_t;


object_t *object_new_int(int value);
object_t *object_new_float(float value);
object_t *object_new_string(const char *value, size_t size);
object_t *object_new_object_vector(size_t size);

#endif