#ifdef __APPLE__
#include <OpenGL/glu.h>
#elif defined(_WIN32)
#include <windows.h>
#include <GL/glu.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "glw.h"
#include "glw_style.h"
#include "draw.h"
#include "skeleton.h"
#include "bmesh.h"
#include "icons.h"

#define GEN_ID __LINE__

#define ED_MODE_EDIT            0
#define ED_MODE_ANIMATION       1

#define ED_BACKGROUND_COLOR     0x141414

#define ED_SIDEBAR_WIDTH        400
#define ED_SPACING              10

#define ED_TOPBAR_HEIGHT        100
#define ED_TOOLBAR_HEIGHT       (glwImLineHeight(win) * 1.2)

#define ED_GL_BACKGROUND_COLOR  GLW_BACKGROUND_COLOR

#define ED_BONE_COLOR           0xffff00

typedef struct editor {
  glwWin *win;
  float cameraAngleX;
  float cameraAngleY;
  float cameraDistance;
  int newImId;
  int showBoneChecked;
  int showBallChecked;
  int showMeshChecked;
  int width;
  int height;
  int mode;
  int renderLeft;
  int renderTop;
  int renderWidth;
  int renderHeight;
  int moveMouseX;
  int moveMouseY;
  bmesh *bm;
  skeleton *skl;
} editor;

#include "../data/bmesh_test_2.h"

static int mouseInRenderRect(glwWin *win) {
  editor *ed = glwGetUserData(win);
  return glwPointTest(glwMouseX(win), glwMouseY(win),
    ed->renderLeft, ed->renderTop,
    ed->renderWidth, ed->renderHeight, 0);
}

static int drawBmeshBone(bmesh *bm, bmeshBone *bone) {
  glColor3f(glwR(ED_BONE_COLOR), glwG(ED_BONE_COLOR), glwB(ED_BONE_COLOR));
  bmeshBall *firstBall = bmeshGetBall(bm, bone->firstBallIndex);
  bmeshBall *secondBall = bmeshGetBall(bm, bone->secondBallIndex);
  drawCylinder(&firstBall->position, &secondBall->position, 0.1, 36, 24);
  return 0;
}

#define ED_CANVAS_BORDER_COLOR      0
#define ED_CANVAS_OUTER_COLOR       0
#define ED_CANVAS_BACKGROUND_COLOR  0

static void drawCanvas(int x, int y, int width, int height) {
  int left = x;
  int top = y;
  int right = x + width - 1;
  int bottom = y + height - 1;
  glColor3f(glwR(ED_CANVAS_BORDER_COLOR),
    glwG(ED_CANVAS_BORDER_COLOR),
    glwB(ED_CANVAS_BORDER_COLOR));
  glBegin(GL_QUADS);
    glVertex2f(left, top);
    glVertex2f(right, top);
    glVertex2f(right, bottom);
    glVertex2f(left, bottom);
  glEnd();
}

static void display(glwWin *win) {
  editor *ed = glwGetUserData(win);
  
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, ed->width, ed->height, 0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  {
    char *list[] = {"Open..", "New..", "Save As..", 0};
    glwImButtonGroup(win, GEN_ID, 100, 30, 300, list, -1);
  }
  
  {
    char *titles[] = {"Tool", 0};
    int icons[] = {ICON_TOOL, 0};
    glwImTabBox(win, GEN_ID, ED_SPACING / 2, ED_TOPBAR_HEIGHT, ED_SIDEBAR_WIDTH,
      ed->height - ED_TOPBAR_HEIGHT - ED_SPACING / 2,
      titles, icons, 0);
  }
  
  {
    int x = ED_SPACING / 2 + ED_SIDEBAR_WIDTH + ED_SPACING;
    int y = ED_TOPBAR_HEIGHT;
    int width = ed->width - ED_SIDEBAR_WIDTH - ED_SPACING / 2 - ED_SPACING - ED_SPACING / 2;
    int height = ed->height - ED_TOPBAR_HEIGHT - ED_SPACING / 2;
    char *titles[] = {"Skeleton", "Animation", "Config", 0};
    int icons[] = {ICON_MONITOR, ICON_PENCIL, ICON_EYE, 0};
    ed->mode = glwImTabBox(win, GEN_ID,
      x, y, width, height,
      titles, icons, ed->mode);
    if (ED_MODE_EDIT == ed->mode) {
      int toolBarY = glwImNextY(win) - 1;
      int glWinY = toolBarY + ED_TOOLBAR_HEIGHT;
      glwImToolBar(win, GEN_ID, x, toolBarY, width, ED_TOOLBAR_HEIGHT);
      ed->showBallChecked = glwImCheck(win, GEN_ID, x + ED_SPACING + ED_SPACING,
        toolBarY + (ED_TOOLBAR_HEIGHT - glwImLineHeight(win)) / 2 + 2,
        "Ball",
        ed->showBallChecked);
      ed->showBoneChecked = glwImCheck(win, GEN_ID,
        glwImNextX(win) + ED_SPACING,
        toolBarY + (ED_TOOLBAR_HEIGHT - glwImLineHeight(win)) / 2 + 2,
        "Bone",
        ed->showBoneChecked);
      ed->showMeshChecked = glwImCheck(win, GEN_ID,
        glwImNextX(win) + ED_SPACING,
        toolBarY + (ED_TOOLBAR_HEIGHT - glwImLineHeight(win)) / 2 + 2,
        "Mesh",
        ed->showMeshChecked);
      
      ed->renderLeft = x + 1;
      ed->renderTop = glWinY;
      ed->renderWidth = width - 3;
      ed->renderHeight = height - ED_TOOLBAR_HEIGHT;
      
      if (0 == ed->skl) {
        ed->skl = skeletonCreate();
      }

      if (0 == ed->bm) {
        bmeshBall ball;
        bmeshBone bone;
        unsigned int i;
        ed->bm = bmeshCreate();

        for (i = 0; i < sizeof(bmeshTestBalls) / sizeof(bmeshTestBalls[0]); ++i) {
          memset(&ball, 0, sizeof(ball));
          ball.position.x = bmeshTestBalls[i][1];
          ball.position.y = bmeshTestBalls[i][2];
          ball.position.z = bmeshTestBalls[i][3];
          ball.radius = bmeshTestBalls[i][4];
          ball.type = bmeshTestBalls[i][5];
          bmeshAddBall(ed->bm, &ball);
        }

        for (i = 0; i < sizeof(bmeshTestBones) / sizeof(bmeshTestBones[0]); ++i) {
          memset(&bone, 0, sizeof(bone));
          bone.firstBallIndex = bmeshTestBones[i][0];
          bone.secondBallIndex = bmeshTestBones[i][1];
          bmeshAddBone(ed->bm, &bone);
        }

        bmeshGenerate(ed->bm);
      }
      
      glEnable(GL_SCISSOR_TEST);
      glScissor(ed->renderLeft, ED_SPACING / 2 + 1,
        ed->renderWidth, ed->renderHeight);
      glPushMatrix();
      glTranslatef(ed->renderLeft, ed->renderTop, 0);
      
      /*
      glColor3f(glwR(ED_GL_BACKGROUND_COLOR),
        glwG(ED_GL_BACKGROUND_COLOR),
        glwB(ED_GL_BACKGROUND_COLOR));
      glRecti(0, 0, ed->renderWidth, ed->renderHeight);*/
      
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      gluPerspective(60.0f, (float)ed->renderWidth / ed->renderHeight, 1, 1000);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glPushMatrix();
      glTranslatef(0, 0, -ed->cameraDistance);
      glRotatef(ed->cameraAngleX, 1, 0, 0);
      glRotatef(ed->cameraAngleY, 0, 1, 0);
      
      //drawGrid(10, 1);
      
      glEnable(GL_LIGHTING);
      glEnable(GL_CULL_FACE);
      
      if (ed->showBallChecked) {
        
      }
      
      if (ed->showBoneChecked) {
        int index;
        for (index = 0; index < bmeshGetBoneNum(ed->bm); ++index) {
        bmeshBone *bone = bmeshGetBone(ed->bm, index);
          drawBmeshBone(ed->bm, bone);
        }
      }
      
      if (ed->showMeshChecked) {
        bmeshDraw(ed->bm);
      }
      
      glDisable(GL_CULL_FACE);
      glDisable(GL_LIGHTING);
      
      glPopMatrix();
      
      glPopMatrix();
      glDisable(GL_SCISSOR_TEST);
    }
  }
}

static void reshape(glwWin *win, int width, int height) {
  editor *ed = glwGetUserData(win);
  
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHT0);
  
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  
  ed->width = width;
  ed->height = height;
  
  glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_ALPHA_TEST);
  //glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  glClearColor(glwR(ED_BACKGROUND_COLOR),
    glwG(ED_BACKGROUND_COLOR),
    glwB(ED_BACKGROUND_COLOR), 1);
  
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

static void mouse(glwWin *win, int button, int state,
    int x, int y){
  editor *ed = glwGetUserData(win);
  if (!mouseInRenderRect(win)) {
    return;
  }
  if (GLW_DOWN == state) {
    ed->moveMouseX = x;
    ed->moveMouseY = y;
  }
}

static void motion(glwWin *win, int x, int y) {
  editor *ed = glwGetUserData(win);
  if (!mouseInRenderRect(win)) {
    return;
  }
  ed->cameraAngleY += (x - ed->moveMouseX);
  ed->cameraAngleX += (y - ed->moveMouseY);
  ed->moveMouseX = x;
  ed->moveMouseY = y;
}

static void wheel(glwWin *win, float delta) {
  editor *ed = glwGetUserData(win);
  if (!mouseInRenderRect(win)) {
    return;
  }
  ed->cameraDistance -= delta * 0.01f;
}

int main(int argc, char *argv[]) {
  editor ed;
  glwInit();
  drawInit();
  
  memset(&ed, 0, sizeof(ed));
  ed.cameraAngleX = 30;
  ed.cameraAngleY = -312;
  ed.cameraDistance = 14.4;
  ed.showMeshChecked = 1;
  
  ed.win = glwCreateWindow(0, 0, 600, 480);
  glwSetUserData(ed.win, &ed);
  glwReshapeFunc(ed.win, reshape);
  glwDisplayFunc(ed.win, display);
  glwMouseFunc(ed.win, mouse);
  glwMotionFunc(ed.win, motion);
  glwWheelFunc(ed.win, wheel);
  glwMainLoop();
  return 0;
}
