#ifndef DUST3D_EDITOR_H
#define DUST3D_EDITOR_H
#include "halfedge.h"
#include "rbtree.h"
#include "array.h"

#define EDITOR_MAX_OBJECT_NAME_LENGTH   300

enum editorActionType {
    EDITOR_ACTION_ADD_BOX,
};

enum editorObjectType {
    EDITOR_OBJECT_MESH,
};

typedef struct editorAction {
    enum editorActionType type;
    void *param;
} editorAction;

typedef struct editorObject {
    rbtreeNode nameNode;
    char *name;
    int type;
    int staged;
    void *data;
    struct editorObject *previous;
    struct editorObject *next;
} editorObject;

typedef struct editor {
    array *actionArray;
    rbtree objectMap;
    int currentActionIndex;
    editorObject *firstObject;
    editorObject *lastObject;
    mesh *currentMesh;
} editor;

int editorInit(editor *ed);
editorAction *editorInsertAction(editor *ed, int index);
editorAction *editorAppendAction(editor *ed);
void editorRemoveAction(editor *ed, int index);
int editorAddObject(editor *ed, editorObject *object);
editorObject *editorFindObject(editor *ed, const char *name);
int editorResetObjects(editor *ed);
int editorSubmitAction(editor *ed, editorAction *action);
mesh *editorGetCurrentMesh(editor *ed);
void editorSetCurrentMesh(editor *ed, mesh *m);
int editorAddMesh(editor *ed, const char *name, mesh *m);
void editorRemoveMesh(editor *ed, const char *name);
mesh *editorFindMesh(editor *ed, const char *name);
editorAction *editorAddStandardBox(editor *ed);

#endif
