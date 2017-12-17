#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <stdlib.h>
#include "dust3d.h"

typedef struct cameraView {
    float angleX;
    float angleY;
    float distance;
    dust3dState *state;
} cameraView;

static cameraView camera;

static void initCamera(dust3dState *state) {
    camera.angleX = 30;
    camera.angleY = -312;
    camera.distance = 5.4;
    camera.state = state;
}

void onDisplay(void) {
    dust3dState *state = camera.state;
    
    glClearColor(0.14, 0.14, 0.14, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glPushMatrix();
    glTranslatef(0, 0, -camera.distance);
    glRotatef(camera.angleX, 0, 1, 0);
    glRotatef(camera.angleY, 1, 0, 0);
    {
        face *f;
        mesh *m = dust3dGetCurrentMesh(state);
        glColor3f(1.0f, 1.0f, 1.0f);
        for (f = m->firstFace; f; f = f->next) {
            halfedge *it = f->handle;
            halfedge *stop = it;
            glBegin(GL_POLYGON);
            do {
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
                glVertex3f(it->start->position.x,
                    it->start->position.z,
                    it->start->position.y);
                it = it->next;
            } while (it != stop);
            glEnd();
        }
    }
    glPopMatrix();
    
    glutSwapBuffers();
}

void onProcessSpecialKeys(int key, int x, int y) {
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

void onResize(int w, int h) {
    float ratio;
    ratio = w * 1.0 / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(60.0f, ratio, 1, 1000);
    glMatrixMode(GL_MODELVIEW);
}

void onProcessNormalKeys(unsigned char key, int x, int y) {
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
        break;
    case 'd':
        break;
    }
}

int theShow(dust3dState *state) {
    int argc = 0;
    char **argv = 0;
    GLfloat globalAmbient[] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat ambientLight[] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat diffuseLight[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat specularLight[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat shininess = 128.0f;
    
    initCamera(state);
    
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

    glShadeModel(GL_SMOOTH);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightf(GL_LIGHT0, GL_SHININESS, shininess);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);

    glutMainLoop();
    return 0;
}

