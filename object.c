#include <stdlib.h>
#include <string.h>

#include "object.h"

static object_t *object_alloc(object_type_t type) {
    object_t *obj = malloc(sizeof(object_t));
    if (obj == NULL) {
        return NULL;
    }

    obj->type = type;
    return obj;
}

object_t *object_new_int(int value) {
    object_t *obj = object_alloc(OBJ_INTEGER);
    if (obj == NULL) {
        return NULL;
    }

    obj->data.v_int = value;
    return obj;
}

object_t *object_new_float(float value) {
    object_t *obj = object_alloc(OBJ_FLOAT);
    if (obj == NULL) {
        return NULL;
    }

    obj->data.v_float = value;
    return obj;
}

object_t *object_new_string(const char *value, size_t size) {
    object_t *obj = object_alloc(OBJ_STRING);
    if (obj == NULL) {
        return NULL;
    }

    char *copy = malloc(size + 1);
    if (copy == NULL) {
        free(obj);
        return NULL;
    }

    memcpy(copy, value, size);
    copy[size] = '\0';
    obj->data.v_string = copy;
    return obj;
}

object_t *object_new_object_vector(size_t size) {
    // allocate new object
    object_t *obj = object_alloc(OBJ_VECTOR);
    if (obj == NULL) {
        return NULL;
    }

    // allocate object vector
    object_vector_t *objects_vector = malloc(sizeof(object_vector_t));
    if (objects_vector == NULL) {
        free(obj);
        return NULL;
    }

    // allocate the array on the heap
    object_t **objects_array = calloc(size, sizeof(object_t*));
    if (objects_array == NULL) {
        free(objects_vector);
        free(obj);
        return NULL;
    }

    objects_vector->objects = objects_array;
    obj->data.v_object_vector = objects_vector;    

    return obj;
}