#ifndef GLW_INTERNAL_H
#define GLW_INTERNAL_H
#include "glw.h"
#include "glw_style.h"

#define GLW_SYSTEM_FONT_BEGIN_CHAR  ' '
#define GLW_SYSTEM_FONT_END_CHAR    '~'
#define GLW_SYSTEM_FONT_CHAR_NUM    (GLW_SYSTEM_FONT_END_CHAR - GLW_SYSTEM_FONT_BEGIN_CHAR + 1)

#define GLW_TEXT_ALIGN_LEFT   0
#define GLW_TEXT_ALIGN_CENTER 1
#define GLW_TEXT_ALIGN_RIGHT  2

typedef enum glwCtrlState {
  GLW_CTRL_STATE_NORMAL,
  GLW_CTRL_STATE_PRESS,
} glwCtrlState;

typedef struct glwSystemFontTexture {
  int texId;
  glwSize size;
  glwSize originSize;
  float lefts[GLW_SYSTEM_FONT_CHAR_NUM];
  int widths[GLW_SYSTEM_FONT_CHAR_NUM];
} glwSystemFontTexture;

typedef struct glwImGui {
  int mouseButton;
  int mouseState;
  int mouseX;
  int mouseY;
  int activeId;
  int nextX;
  int nextY;
} glwImGui;

glwSize glwMeasureText(char *text, glwFont *font);
unsigned char *glwRenderTextToRGBA(char *text, glwFont *font, glwSize size,
  int *pixelsWide, int *pixelsHigh);
void glwDrawText(int x, int y, char *text, glwFont *font);
void glwDrawSystemText(glwWin *win, int x, int y, char *text);
glwSize glwMeasureSystemText(glwWin *win, char *text);
void glwInitSystemFontTexture(glwWin *win);
void glwInitPrimitives(void);
glwSystemFontTexture *glwGetSystemFontTexture(glwWin *win);
void glwDestroyFont(glwFont *font);
glwFont *glwCreateFont(char *name, int weight, int size, int bold);
void glwDestroyFont(glwFont *font);
void glwDrawButtonBackground(float x, float y, float width, float height,
    glwCtrlState state);
void glwMouseEvent(glwWin *win, int button, int state, int x, int y);
glwImGui *glwGetImGUI(glwWin *win);

#endif
