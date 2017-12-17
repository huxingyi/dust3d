#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <stdlib.h>
#include "dust3d.h"

typedef struct cameraView {
    float angle;
    float directX;
    float directZ;
    float x;
    float z;
    dust3dState *state;
} cameraView;

static cameraView camera;

static void initCamera(dust3dState *state) {
    camera.angle = 0.0;
    camera.directX = 0.0f;
    camera.directZ = -1.0f;
    camera.x = 0.0f;
    camera.z = 10.0f;
    camera.state = state;
}

void onDisplay(void) {
    dust3dState *state = camera.state;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(camera.x, 1.0f, camera.z,
        camera.x + camera.directX, 1.0f,  camera.z + camera.directZ,
        0.0f, 1.0f,  0.0f);

    glColor3f(1.0f, 1.0f, 1.0f);
    {
        face *f;
        mesh *m = dust3dGetCurrentMesh(state);
        for (f = m->firstFace; f; f = f->next) {
            halfedge *it = f->handle;
            halfedge *stop = it;
            glBegin(GL_POLYGON);
            do {
                glVertex3f(it->start->position.x,
                    it->start->position.y,
                    it->start->position.z);
                it = it->next;
            } while (it != stop);
            glEnd();
        }
    }
    
    glutSwapBuffers();
}

void onProcessSpecialKeys(int key, int x, int y) {
    float fraction = 0.1f;
    switch (key) {
    case GLUT_KEY_LEFT:
        camera.angle -= 0.01f;
        camera.directX = sin(camera.angle);
        camera.directZ = -cos(camera.angle);
        break;
    case GLUT_KEY_RIGHT:
        camera.angle += 0.01f;
        camera.directX = sin(camera.angle);
        camera.directZ = -cos(camera.angle);
        break;
    case GLUT_KEY_UP:
        camera.x += camera.directX * fraction;
        camera.z += camera.directZ * fraction;
        break;
    case GLUT_KEY_DOWN:
        camera.x -= camera.directX * fraction;
        camera.z -= camera.directZ * fraction;
        break;
    }
}

void onResize(int w, int h) {
    float ratio;
    ratio = w * 1.0 / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void onProcessNormalKeys(unsigned char key, int x, int y) {
    if (key == 27) {
        exit(0);
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

