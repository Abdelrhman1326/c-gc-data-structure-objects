#include <stdlib.h>
#include <string.h>
#include <stdint.h>  // SIZE_MAX

#include "object.h"

static object_t *object_alloc(object_type_t type) {
    object_t *obj = malloc(sizeof *obj);
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

object_t *object_new_object_array(array_type_t type, size_t capacity) {
    if (type != INTEGER_ARRAY && type != FLOAT_ARRAY)
        return NULL;

    object_t *obj = object_alloc(OBJ_ARRAY);
    if (obj == NULL) return NULL;

    object_array_t *array = malloc(sizeof *array);
    if (array == NULL) {
        free(obj);
        return NULL;
    }

    void *objects = NULL;

    if (capacity == 0) {
        capacity = 4;
    }

    size_t element_size = type == INTEGER_ARRAY
    ? sizeof(int)
    : sizeof(float);

    objects = calloc(capacity, element_size);

    if (objects == NULL) {
        free(array);
        free(obj);
        return NULL;
    }

    array->type = type;
    array->objects = objects;
    array->size = 0;
    array->capacity = capacity;

    obj->data.v_object_array = array;

    return obj;
}

void array_add_at(const object_t *obj, size_t index, const void *value) {
    if (obj == NULL || value == NULL) return;
    if (obj->type != OBJ_ARRAY) return;

    object_array_t *array = obj->data.v_object_array;
    if (array == NULL || index > array->size) return;

    size_t element_size;

    switch (array->type) {
        case INTEGER_ARRAY:
            element_size = sizeof(int);
            break;

        case FLOAT_ARRAY:
            element_size = sizeof(float);
            break;

        default:
            return;
    }

    // Grow if necessary
    if (array->size == array->capacity) {
        size_t new_capacity = array->capacity == 0
            ? 4
            : array->capacity * 2;

        void *new_objects = realloc(
            array->objects,
            new_capacity * element_size
        );

        if (new_objects == NULL) return;

        array->objects = new_objects;
        array->capacity = new_capacity;
    }

    // Shift elements and insert
    if (array->type == INTEGER_ARRAY) {
        int *items = array->objects;

        for (size_t i = array->size; i > index; i--)
            items[i] = items[i - 1];

        items[index] = *(const int *)value;

    } else {
        float *items = array->objects;

        for (size_t i = array->size; i > index; i--)
            items[i] = items[i - 1];

        items[index] = *(const float *)value;
    }

    array->size++;
}

void array_set(const object_t *obj, size_t index, const void *value) {
    // set position index in obj to value
    if (obj == NULL || value == NULL) return;
    if (obj->type != OBJ_ARRAY) return;
    object_array_t *array = obj->data.v_object_array;
    if (array == NULL || array->objects == NULL || index >= array->size) return;

    switch (array->type) {
        case (INTEGER_ARRAY):
            ((int *)array->objects)[index] = *(const int *)value;
            break;

        case (FLOAT_ARRAY):
            ((float *)array->objects)[index] = *(const float *)value;
            break;

        default:
            return;
    }
}

void array_append(const object_t *obj, const void *value) {
    if (obj == NULL || value == NULL || obj->type != OBJ_ARRAY) return;

    object_array_t *array = obj->data.v_object_array;
    if (array->type != INTEGER_ARRAY && array->type != FLOAT_ARRAY)
        return;

    if (array->size < array->capacity) {
        switch (array->type) {
            case(INTEGER_ARRAY):
                ((int *)array->objects)[array->size] = *(const int *)value;
                break;
            case(FLOAT_ARRAY):
                ((float *)array->objects)[array->size] = *(const float *)value;
                break;
            default:
                return;
        }

        array->size++;
    } else {
        // expand the size before appending

        size_t element_size = array->type == INTEGER_ARRAY
            ? sizeof(int)
            : sizeof(float);

        if (array->capacity > SIZE_MAX / (element_size * 2))
            return;

        void *new_objects = realloc(
            array->objects,
            array->capacity * 2 * element_size
        );

        if (new_objects == NULL) {
            return;
        }

        array->objects = new_objects;
        array->capacity = array->capacity * 2;

        switch (array->type) {
            case(INTEGER_ARRAY):
                ((int *)array->objects)[array->size] = *(const int *)value;
                break;
            case(FLOAT_ARRAY):
                ((float *)array->objects)[array->size] = *(const float *)value;
                break;
            default:
                return;
        }

        array->size++;
    }
}