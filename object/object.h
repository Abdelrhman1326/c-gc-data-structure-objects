#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>

typedef enum ObjectType {
    OBJ_INTEGER,
    OBJ_FLOAT,
    OBJ_STRING,
    OBJ_ARRAY,
} object_type_t;

typedef enum ArrayType {
    INTEGER_ARRAY,
    FLOAT_ARRAY,
} array_type_t;

typedef struct ObjectArray object_array_t;

typedef union ObjectData {
    int v_int;
    float v_float;
    char *v_string;
    object_array_t *v_object_array;
} object_data_t;

typedef struct Object {
    object_type_t type;
    object_data_t data;
} object_t;

struct ObjectArray {
    array_type_t type;
    void *objects;
    size_t size;
    size_t capacity;
};

object_t *object_new_int(int value);
object_t *object_new_float(float value);
object_t *object_new_string(const char *value, size_t size);
object_t *object_new_object_array(array_type_t type, size_t capacity);

void array_set(object_t *obj, int index, void *value);
void array_append(object_t *obj, void *value);
void array_add_at(object_t *obj, size_t index, const void *value);

#endif