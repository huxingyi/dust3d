#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "editor.h"
#include "util.h"
#include "standard.h"

int editorInit(editor *ed) {
    memset(ed, 0, sizeof(editor));
    rbtreeInit(&ed->objectMap, editorObject, nameNode, name, rbtreeStringComparator);
    ed->actionArray = arrayCreate(sizeof(editorAction));
    return 0;
}

editorAction *editorInsertAction(editor *ed, int index) {
    return arrayNewItemAt(ed->actionArray, index);
}

editorAction *editorAppendAction(editor *ed) {
    return editorInsertAction(ed, -1);
}

void editorRemoveAction(editor *ed, int index) {
    return arrayRemoveItem(ed->actionArray, index);
}

int editorAddObject(editor *ed, editorObject *object) {
    rbtreeInsert(&ed->objectMap, object);
    return 0;
}

editorObject *editorFindObject(editor *ed, const char *name) {
    return rbtreeFind(&ed->objectMap, &name);
}

mesh *editorGetCurrentMesh(editor *ed) {
    return ed->currentMesh;
}

void editorSetCurrentMesh(editor *ed, mesh *m) {
    ed->currentMesh = m;
}

int editorAddMesh(editor *ed, const char *name, mesh *m) {
    editorObject *object;
    if (editorFindObject(ed, name)) {
        fprintf(stderr, "Mesh name \"%s\" already exists\n", name);
        return -1;
    }
    object = (editorObject *)calloc(1, sizeof(editorObject));
    object->name = strdup(name);
    object->type = EDITOR_OBJECT_MESH;
    object->data = m;
    object->staged = 1;
    rbtreeInsert(&ed->objectMap, object);
    addToDoubleLinkedListHead(object, ed->firstObject, ed->lastObject);
    return 0;
}

void editorRemoveMesh(editor *ed, const char *name) {
    editorObject *object;
    if (!(object=editorFindObject(ed, name))) {
        fprintf(stderr, "Mesh name \"%s\" not found\n", name);
        return;
    }
    rbtreeDelete(&ed->objectMap, object);
    removeFromDoubleLinkedList(object, ed->firstObject, ed->lastObject);
    switch (object->type) {
    case EDITOR_OBJECT_MESH:
        halfedgeDestroyMesh((mesh *)object->data);
        object->data = 0;
        break;
    default:
        assert(0 && "Unknown object type found, must be memory corruption\n");
    }
    assert(0 == object->data);
    free(object->name);
    free(object);
}

mesh *editorFindMesh(editor *ed, const char *name) {
    editorObject *object;
    mesh *m;
    if (!(object=editorFindObject(ed, name))) {
        return 0;
    }
    assert(EDITOR_OBJECT_MESH == object->type);
    m = (mesh *)object->data;
    return m;
}

int editorGenerateUniqueObjectName(editor *ed, const char *prefix, char *buffer, int bufferSize) {
    long long repeat = 0;
    int len = 0;
    if (!editorFindObject(ed, prefix)) {
        return snprintf(buffer, bufferSize, "%s", prefix);
    }
    for (;;) {
        len = snprintf(buffer, bufferSize, "%s(%lld)", prefix, ++repeat);
        if (!editorFindObject(ed, buffer)) {
            return len;
        }
    }
}

editorAction *editorAddStandardBox(editor *ed) {
    standardBox box = STANDARD_BOX_DEFAULT;
    editorAction *action = editorAppendAction(ed);
    mesh *m = standardCreateBoxMesh(&box);
    char name[EDITOR_MAX_OBJECT_NAME_LENGTH + 1];
    action->type = EDITOR_ACTION_ADD_BOX;
    editorGenerateUniqueObjectName(ed, "box", name, sizeof(name));
    editorAddMesh(ed, name, m);
    return action;
}

