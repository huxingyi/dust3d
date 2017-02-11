#ifndef GLW_H
#define GLW_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct glwSize {
  int width;
  int height;
} glwSize;

typedef struct glwVec2 {
  float x;
  float y;
} glwVec2;

typedef struct glwWin glwWin;
typedef struct glwFont glwFont;

#define GLW_LEFT_BUTTON       1
#define GLW_MIDDLE_BUTTON     2
#define GLW_RIGHT_BUTTON      3

#define GLW_DOWN              1
#define GLW_UP                2

void glwInit(void);
void glwMainLoop(void);
glwWin *glwCreateWindow(int x, int y, int width, int height);
glwWin *glwCreateSubWindow(glwWin *parent, int x, int y, int width, int height);
void glwSetUserData(glwWin *win, void *userData);
void *glwGetUserData(glwWin *win);
void glwDestroyWindow(glwWin *win);
void glwSetWindowTitle(glwWin *win, char *name);
void glwPositionWindow(glwWin *win, int x, int y);
void glwReshapeWindow(glwWin *win, int width, int height);
void glwDisplayFunc(glwWin *win, void (*func)(glwWin *win));
void glwReshapeFunc(glwWin *win, void (*func)(glwWin *win, int width,
  int height));
void glwKeyboardFunc(glwWin *win, void (*func)(glwWin *win, unsigned char key,
  int x, int y));
void glwMouseFunc(glwWin *win, void (*func)(glwWin *win, int button, int state,
  int x, int y));
void glwMotionFunc(glwWin *win, void (*func)(glwWin *win, int x, int y));
void glwPassiveMotionFunc(glwWin *win, void (*func)(glwWin *win, int x, int y));
#define glwR(rgb) ((float)(((rgb) >> 16) & 0xff) / 255)
#define glwG(rgb) ((float)(((rgb) >> 8) & 0xff) / 255)
#define glwB(rgb) ((float)(((rgb)) & 0xff) / 255)
int glwImButton(glwWin *win, int id, int x, int y, char *text);
int glwImCheck(glwWin *win, int id, int x, int y, char *text, int checked);
int glwImDropdownBox(glwWin *win, int id, int x, int y, char *text);
int glwImButtonGroup(glwWin *win, int id, int x, int y, int parentWidth,
    char **list, int sel);
int glwImPanel(glwWin *win, int id, int x, int y, int width, int height);
int glwImTabBox(glwWin *win, int id, int x, int y, int width, int height,
  char **list, int sel);
int glwImToolBar(glwWin *win, int id, int x, int y, int width, int height);
int glwImBottomBar(glwWin *win, int id, int x, int y, int width, int height);
int glwImNextX(glwWin *win);
int glwImNextY(glwWin *win);
int glwImLineHeight(glwWin *win);
int glwPointTest(int x, int y, int left, int top, int width, int height,
  int allowOffset);

#ifdef __cplusplus
}
#endif

#endif
