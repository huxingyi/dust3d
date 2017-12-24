#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include "dust3d.h"

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

static cameraView camera;
static mouseContext mouse;

static void initCamera(dust3dState *state) {
    camera.angleX = 30;
    camera.angleY = -312;
    camera.distance = 5.4;
    camera.offsetH = 0;
    camera.state = state;
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
        glColor3f(1.0f, 1.0f, 1.0f);
        for (f = m->firstFace; f; f = f->next) {
            halfedge *it = f->handle;
            halfedge *stop = it;
            vector3d faceNormal;
            halfedgeFaceNormal(m, f, &faceNormal);
            glBegin(GL_POLYGON);
            do {
                glNormal3f(faceNormal.x, faceNormal.y, faceNormal.z);
                glVertex3f(it->start->position.x,
                    it->start->position.z,
                    it->start->position.y);
                it = it->next;
            } while (it != stop);
            glEnd();
        }
    }
    {
        face *f;
        mesh *m = dust3dGetCurrentMesh(state);
        glColor3f(0.07f, 0.07f, 0.07f);
        for (f = m->firstFace; f; f = f->next) {
            halfedge *it = f->handle;
            halfedge *stop = it;
            glBegin(GL_LINE_STRIP);
            do {
                if (it->opposite) {
                    glColor3f(0.07f, 0.07f, 0.07f);
                } else {
                    glColor3f(1.00f, 0.00f, 0.00f);
                }
                glVertex3f(it->start->position.x,
                    it->start->position.z,
                    it->start->position.y);
                it = it->next;
            } while (it != stop);
            if (it->opposite) {
                glColor3f(0.07f, 0.07f, 0.07f);
            } else {
                glColor3f(1.00f, 0.00f, 0.00f);
            }
            glVertex3f(f->handle->start->position.x,
                f->handle->start->position.z,
                f->handle->start->position.y);
            glEnd();
        }
    }
    glPopMatrix();
    
    glutSwapBuffers();
}

static void onProcessSpecialKeys(int key, int x, int y) {
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

static void onResize(int w, int h) {
    float ratio;
    ratio = w * 1.0 / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(60.0f, ratio, 1, 1000);
    glMatrixMode(GL_MODELVIEW);
}

static void onProcessNormalKeys(unsigned char key, int x, int y) {
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
    }
}

static void onMotion(int x, int y) {
    switch (mouse.button) {
    case GLUT_LEFT_BUTTON:
        camera.offsetH += (mouse.x - x) * 0.5;
        camera.distance -= (mouse.y - y) * 0.5;
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
    printf("button:%d state:%d x:%d y:%d\n", button, state, x, y);
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
    GLfloat position[] = {1.0, 1.0, 1.0, 0.0};
    
    initCamera(state);
    memset(&mouse, 0, sizeof(mouse));
    
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(320, 320);
    
    glutCreateWindow("dust3d");
    glutReshapeFunc(onResize);
    glutDisplayFunc(onDisplay);
    glutIdleFunc(onDisplay);
    glutKeyboardFunc(onProcessNormalKeys);
    glutSpecialFunc(onProcessSpecialKeys);
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

