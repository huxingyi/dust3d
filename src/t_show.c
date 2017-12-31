#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dust3d.h"
#include "color.h"

typedef struct cameraView {
    float angleX;
    float angleY;
    float distance;
    float offsetH;
    dust3dState *state;
} cameraView;

typedef struct mouseContext {
    int x;
    int y;
    int button;
    int state;
} mouseContext;

typedef struct showOptions {
    int showFaceNumber;
    int showHalfedgeDebugInfo;
} showOptions;

static cameraView camera;
static mouseContext mouse;
static showOptions options;

static void initCamera(dust3dState *state) {
    camera.angleX = 30;
    camera.angleY = -312;
    camera.distance = 5.4;
    camera.offsetH = 0;
    camera.state = state;
}

static void showText(float x, float y, float z, int color, const char *text) {
    glColor3fv(colors[color]);
    glRasterPos3f(x, y, z);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *text);
        text++;
    }
}

static void onDisplay(void) {
    dust3dState *state = camera.state;
    
    glClearColor(0.8, 0.8, 0.8, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glPushMatrix();
    glTranslatef(camera.offsetH, 0, -camera.distance);
    glRotatef(camera.angleX, 0, 1, 0);
    glRotatef(camera.angleY, 1, 0, 0);
    {
        face *f;
        mesh *m = dust3dGetCurrentMesh(state);
        for (f = m->firstFace; f; f = f->next) {
            halfedge *it = f->handle;
            halfedge *stop = it;
            vector3d faceNormal;
            //int vertexIndex = 0;
            halfedgeFaceNormal(m, f, &faceNormal);
            glColor3fv(colors[f->color]);
            glBegin(GL_POLYGON);
            do {
                glNormal3f(faceNormal.x, faceNormal.y, faceNormal.z);
                //glColor3fv(colors[(f->color + vertexIndex * 2) % MAX_COLOR]);
                glVertex3f(it->start->position.x,
                    it->start->position.y,
                    it->start->position.z);
                //vertexIndex++;
                it = it->next;
            } while (it != stop);
            glEnd();
        }
    }
    if (options.showFaceNumber)
    {
        face *f;
        mesh *m = dust3dGetCurrentMesh(state);
        int faceIndex = 0;
        for (f = m->firstFace; f; f = f->next) {
            point3d center;
            vector3d normal;
            char number[10];
            halfedgeFaceCenter(m, f, &center);
            halfedgeFaceNormal(m, f, &normal);
            vector3dNormalize(&normal);
            vector3dMultiply(&normal, 0.1);
            vector3dAdd(&center, &normal);
            snprintf(number, sizeof(number), "%d", faceIndex);
            showText(center.x, center.y, center.z, BLACK, number);
            faceIndex++;
        }
    }
    if (options.showHalfedgeDebugInfo)
    {
        face *f;
        mesh *m = dust3dGetCurrentMesh(state);
        for (f = m->firstFace; f; f = f->next) {
            vector3d normal;
            vector3d begin;
            vector3d end;
            halfedge *h = f->handle;
            halfedge *stop = h;
            vector3d offset;
            halfedgeFaceNormal(m, f, &normal);
            vector3dNormalize(&normal);
            offset = normal;
            vector3dMultiply(&offset, 0.1);
            do {
                //halfedgeVector3d(m, f->handle, &edge);
                //vector3dCrossProduct(&edge, &normal, &perp);
                //vector3dNormalize(&perp);
                //vector3dMultiply(&perp, 0.1);
                begin = h->start->position;
                end = h->next->start->position;
                vector3dAdd(&begin, &offset);
                //vector3dAdd(&begin, &perp);
                vector3dAdd(&end, &offset);
                //vector3dAdd(&end, &perp);
                glBegin(GL_QUADS);
                    glColor3fv(colors[h == f->handle ? BLACK : RED]);
                    glVertex3f(h->start->position.x,
                        h->start->position.y,
                        h->start->position.z);
                    glVertex3f(begin.x, begin.y, begin.z);
                    glColor3fv(colors[WHITE]);
                    glVertex3f(end.x, end.y, end.z);
                    glVertex3f(h->next->start->position.x,
                        h->next->start->position.y,
                        h->next->start->position.z);
                glEnd();
                h = h->next;
            } while (h != stop);
        }
    }
    {
        face *f;
        mesh *m = dust3dGetCurrentMesh(state);
        for (f = m->firstFace; f; f = f->next) {
            halfedge *it = f->handle;
            halfedge *stop = it;
            glBegin(GL_LINES);
            do {
                if (halfedgeMakeSureOnlyOnce(it)) {
                    if (it->opposite) {
                        glColor3fv(colors[it->color]);
                    } else {
                        glColor3fv(colors[RED]);
                    }
                    glVertex3f(it->start->position.x,
                        it->start->position.y,
                        it->start->position.z);
                    glVertex3f(it->next->start->position.x,
                        it->next->start->position.y,
                        it->next->start->position.z);
                }
                it = it->next;
            } while (it != stop);
            glEnd();
        }
    }
    glPopMatrix();
    
    glutSwapBuffers();
}

static void onPressSpecialKeys(int key, int x, int y) {
    float fraction = 10;
    switch (key) {
    case GLUT_KEY_LEFT:
        camera.angleX -= fraction;
        break;
    case GLUT_KEY_RIGHT:
        camera.angleX += fraction;
        break;
    case GLUT_KEY_UP:
        camera.angleY -= fraction;
        break;
    case GLUT_KEY_DOWN:
        camera.angleY += fraction;
        break;
    }
}

static void onReleaseSpecialKeys(int key, int x, int y) {
    
}

static void onResize(int w, int h) {
    float ratio;
    ratio = w * 1.0 / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(60.0f, ratio, 1, 1000);
    glMatrixMode(GL_MODELVIEW);
}

static void onPressNormalKeys(unsigned char key, int x, int y) {
    float fraction = 1.0f;
    if (key == 27) {
        exit(0);
    }
    switch (key) {
    case 'w':
        camera.distance -= fraction;
        break;
    case 's':
        camera.distance += fraction;
        break;
    case 'a':
        camera.offsetH -= fraction;
        break;
    case 'd':
        camera.offsetH += fraction;
        break;
    case 'z':
        options.showFaceNumber = !options.showFaceNumber;
        options.showHalfedgeDebugInfo = !options.showHalfedgeDebugInfo;
        break;
    }
}

static void onReleaseNormalKeys(unsigned char key, int x, int y) {
}

static void onMotion(int x, int y) {
    switch (mouse.button) {
    case GLUT_LEFT_BUTTON:
        camera.offsetH += (mouse.x - x) * 0.03;
        camera.distance -= (mouse.y - y) * 0.03;
        break;
    case GLUT_RIGHT_BUTTON:
        camera.angleX -= (mouse.x - x) * 1;
        camera.angleY -= (mouse.y - y) * 1;
        break;
    }
    mouse.x = x;
    mouse.y = y;
}

static void onMouse(int button, int state, int x, int y) {
    //printf("button:%d state:%d x:%d y:%d\n", button, state, x, y);
    mouse.button = button;
    mouse.state = state;
    mouse.x = x;
    mouse.y = y;
}

int theShow(dust3dState *state) {
    int argc = 0;
    char **argv = 0;
    GLfloat globalAmbient[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat ambientLight[] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat diffuseLight[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat specularLight[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat shininess = 50.0;
    GLfloat position[] = {1.0, 1.0, -1.0, 0.0};
    
    initCamera(state);
    memset(&mouse, 0, sizeof(mouse));
    memset(&options, 0, sizeof(options));
    
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
    
    glutCreateWindow("dust3d");
    glutReshapeFunc(onResize);
    glutDisplayFunc(onDisplay);
    glutIdleFunc(onDisplay);
    glutKeyboardFunc(onPressNormalKeys);
    glutKeyboardUpFunc(onReleaseNormalKeys);
    glutSpecialFunc(onPressSpecialKeys);
    glutSpecialUpFunc(onReleaseSpecialKeys);
    glutMouseFunc(onMouse);
    glutMotionFunc(onMotion);

    glShadeModel(GL_SMOOTH);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightf(GL_LIGHT0, GL_SHININESS, shininess);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);

    glutMainLoop();
    return 0;
}

