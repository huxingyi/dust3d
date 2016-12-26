#define BIG_BALL_SIZE     0.9
#define MINI_BALL_SIZE    0.2
#define TINY_BALL_SIZE    0.1
#define SECOND_BALL_SIZE  0.5
#define THIRD_BALL_SIZE   0.4

const float bmeshTestBalls[][6] = {
  /*Head*/        {0,  0.00000,  5.39143,  0.61970, BIG_BALL_SIZE, 1},
  /*Neck*/        {1,  0.00000,  4.39990, -0.21690, THIRD_BALL_SIZE},
  /*Under Neck*/  {2,  0.00000,  2.88163,  0.00000, THIRD_BALL_SIZE, 1},
  /*Chest*/       {3,  0.00000,  1.39434,  0.61970, SECOND_BALL_SIZE},
  /*Belly*/       {4,  0.00000,  0.00000,  0.00000, BIG_BALL_SIZE, 1},
  /*Tail1*/       {5,  0.00000, -0.65069, -1.27039, MINI_BALL_SIZE},
  /*Tail2*/       {6,  0.00000, -0.06197, -2.69572, THIRD_BALL_SIZE},
  /*LKnee*/       {7, +1.82813, -1.64222,  0.30091, THIRD_BALL_SIZE}, 
  /*LAnkle*/      {8, +1.81101, -2.74796, -0.41828, MINI_BALL_SIZE},
  /*LFoot*/       {9, +2.75769, -3.34733,  0.59646, THIRD_BALL_SIZE},
  /*LShoulder*/   {10,+1.42532,  2.72670, -0.44708, THIRD_BALL_SIZE},
  /*LElbow1*/     {11,+3.12951,  1.95207, -1.07872, THIRD_BALL_SIZE},
  /*LElbow2*/     {12,+2.91261,  1.08448, -0.68593, THIRD_BALL_SIZE},
  /*LPalm1*/      {13,+1.76616,  0.53399,  0.09306, TINY_BALL_SIZE, 1},
  /*LPalm2*/      {14,+1.27039,  0.49576,  0.53458, TINY_BALL_SIZE},
  /*LPalm3*/      {15,+1.48729,  0.03099,  0.34054, TINY_BALL_SIZE},
  /*RKnee*/       {16,-1.82813, -1.64222,  0.30091, THIRD_BALL_SIZE}, 
  /*RAnkle*/      {17,-1.81101, -2.74796, -0.41828, MINI_BALL_SIZE},
  /*RFoot*/       {18,-2.75769, -3.34733,  0.59646, THIRD_BALL_SIZE},
  /*RShoulder*/   {19,-1.42532,  2.72670, -0.44708, THIRD_BALL_SIZE},
  /*RElbow1*/     {20,-3.12951,  1.95207, -1.07872, THIRD_BALL_SIZE},
  /*RElbow2*/     {21,-2.91261,  1.08448, -0.68593, THIRD_BALL_SIZE},
  /*RPalm1*/      {22,-1.76616,  0.53399,  0.09306, TINY_BALL_SIZE, 1},
  /*RPalm2*/      {23,-1.27039,  0.49576,  0.53458, TINY_BALL_SIZE},
  /*RPalm3*/      {24,-1.48729,  0.03099,  0.34054, TINY_BALL_SIZE},
  /*Mouth*/       {25, 0.00000,  4.57765,  1.92032, TINY_BALL_SIZE},
  /*BackOfHead*/  {26, 0.00000,  6.73538, -1.58601, TINY_BALL_SIZE},
};

const int bmeshTestBones[][2] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6},
  {2, 10}, {10, 11}, {11, 12}, {12, 13}, {13, 14}, {13, 15},
  {2, 19}, {19, 20}, {20, 21}, {21, 22}, {22, 23}, {22, 24},
  {4, 7}, {7, 8}, {8, 9},
  {4, 16}, {16, 17}, {17, 18},
  {0, 25}, {0, 26}
};
