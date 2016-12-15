#include <QtGui>
#include <QtOpenGL>
#include <assert.h>

#include "glwidget.h"

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
}

GLWidget::~GLWidget()
{
}

void GLWidget::initializeGL()
{
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 50.0 };
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
    GLfloat light[] = { 1.0, 0.2, 0.2 };
    GLfloat lmodel_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel (GL_SMOOTH);

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light );
    glLightfv(GL_LIGHT0, GL_SPECULAR, light );
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
}

void normalize(float v[3]) 
{    
    GLfloat d = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); 
    if (d == 0.0) 
    {  
        return;
    }
    v[0] /= d; v[1] /= d; v[2] /= d; 
}

void drawtriangle(float *v1, float *v2, float *v3) 
{ 
   glBegin(GL_TRIANGLES); 
      glNormal3fv(v1); glVertex3fv(v1);    
      glNormal3fv(v2); glVertex3fv(v2);    
      glNormal3fv(v3); glVertex3fv(v3);    
   glEnd(); 
}

void subdivide(float *v1, float *v2, float *v3) 
{
    GLfloat v12[3], v23[3], v31[3];    
    GLint i;

    for (i = 0; i < 3; i++) 
    { 
        v12[i] = v1[i]+v2[i]; 
        v23[i] = v2[i]+v3[i];     
        v31[i] = v3[i]+v1[i];    
    } 
    normalize(v12);    
    normalize(v23); 
    normalize(v31); 
    drawtriangle(v1, v12, v31);    
    drawtriangle(v2, v23, v12);    
    drawtriangle(v3, v31, v23);    
    drawtriangle(v12, v23, v31); 
}

// An example come from http://www.glprogramming.com/red/chapter02.html#name8

#define X .525731112119133606 
#define Z .850650808352039932

static GLfloat vdata[12][3] = {    
    {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},    
    {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},    
    {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0} 
};
static GLuint tindices[20][3] = { 
    {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},    
    {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},    
    {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6}, 
    {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} 
};

void GLWidget::paintGL()
{
    GLint i;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (i = 0; i < 20; i++) 
    { 
        subdivide(&vdata[tindices[i][0]][0],       
                 &vdata[tindices[i][1]][0],       
                 &vdata[tindices[i][2]][0]); 
    }
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h)
    {
        glOrtho(-1.5, 1.5, -1.5*(GLfloat)h/(GLfloat)w,
           1.5*(GLfloat)h/(GLfloat)w, -10.0, 10.0);
    }
    else
    {
        glOrtho(-1.5*(GLfloat)w/(GLfloat)h,
           1.5*(GLfloat)w/(GLfloat)h, -1.5, 1.5, -10.0, 10.0);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
}
