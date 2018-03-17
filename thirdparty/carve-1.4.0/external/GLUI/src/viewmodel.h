/*********************************************************************

  ViewModel.h

  GLUI User Interface Toolkit (LGPL)
  Copyright (c) 1998 Paul Rademacher

  WWW:    http://sourceforge.net/projects/glui/
  Forums: http://sourceforge.net/forum/?group_id=92496

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*********************************************************************

  The ViewModel class implements a camera model, minus the perspective
  transformation.  It maintains the coordinate system of the camera
  as three vectors (forward, up, and side), which are themselves
  derived from two points (eye and lookat).

  Apart from simplifying camera translation, this class provides
  three rotation methods: yaw (rotation about the up axis),
  roll (about the forward axis), and pitch (about the side axis).
  Also, these rotations can take place about the eye (in which
  case the eye location is fixed but the lookat point changes -
  like rotating your head), or about the lookat point (in which
  case your eye rotates around the point of interest).

  There is also a routine for rotating the eye about an arbitrary
  axis, which is quite useful in conjuction with the world up axis.
  
  This class is heavily-dependent on the vec3 class in
  the file algebra3.h.

  The function update() sets the side and forward vectors based
  on the lookat, eye, and up vectors.  Remember to call this
  function if you change the side or forward directly.

  Sample use:
     ViewModel vm;
     vm.eye.set( 0.0, 0.0, 20.0 );
     vm.lookat.set( 0.0, 0.0, 0.0 );
     vm.update();  // updates the three vectors 

     vm.move( 0.0, 2.0, 0.0 );  // Moves the eye and lookat 
                                //   by 2 units in the world 
                                //   Y direction 
     vm.roll( 5.0 );            //   rolls 5 degrees about the forward axis 
     vm.lookat_yaw( -25.0 );    // Yaws about the eye (lookat is
				                //   fixed) by -25 degrees 
     vm.load_to_openGL();       // Sets OpenGL modelview matrix 
     
     .. render ...


  ---------- List of member functions -----------------------------
  set_distance()   - Sets the distance from the eye to the lookat
  set_up()         - Sets the current up vector
  set_eye()        - Sets the current eye point
  set_lookat()     - Sets the current lookat point
  roll()           - Rolls about forward axis
  eye_yaw()        - Rotates eye about up vector
  eye_yaw_abs()    - Rotates eye about arbitrary axis
  eye_pitch()      - Rotates eye about side vector
  lookat_yaw()     - Rotates lookat about up vector
  lookat_pitch()   - Rotates lookat about side vector
  reset_up_level() - Resets the up vector (after unwanted rolls), and 
                       sets the eye level with the lookat point
  move()           - Moves eye and lookat by some amount
  move_by_eye()    - Moves eye to new position, lookat follows
  move_by_lookat() - Moves lookat to new position, eye follows
  move_abs()       - Moves eye and lookat in world coordinates
  rot_about_eye()  - Rotates about the eye point by a given 4x4 matrix
  rot_about_lookat() - Rotates about the lookat point by a given 4x4 matrix  
  make_mtx()       - Constructs 4x4 matrix, used by load_to_openGL()
  load_to_openGL() - Loads current camera description in openGL
  load_to_openGL_noident() - Loads camera into OpenGL without first
                     resetting the OpenGL matrix to identity
  reset()          - Resets class values
  ViewModel()      - constructor 
  update()         - Recalculates side and forward based on eye,
                     lookat, and up
  dump()           - Prints class contents to a file


      1996, Paul Rademacher (rademach@cs.unc.edu)
  Oct 2003, Nigel Stewart - GLUI Code Cleaning

*********************************************************************/

#ifndef GLUI_VIEWMODEL_H
#define GLUI_VIEWMODEL_H

#include "algebra3.h"

class ViewModel 
{
public:
  vec3    eye, lookat;
  vec3    up, side, forward;
  mat4    mtx;
  float   distance;

  /******************************* set_distance() ***********/
  /* This readjusts the distance from the eye to the lookat */
  /* (changing the eye point in the process)                */
  /* The lookat point is unaffected                         */
  void set_distance(float new_distance);

  /******************************* set_up() ***************/
  void set_up(const vec3 &new_up);

  void set_up(float x, float y, float z);

  /******************************* set_eye() ***************/
  void set_eye(const vec3 &new_eye );

  void set_eye(float x, float y, float z);

  /******************************* set_lookat() ***************/
  void set_lookat(const vec3 &new_lookat);

  void set_lookat(float x, float y, float z);

  /******************************* roll() *****************/
  /* Rotates about the forward vector                     */
  /* eye and lookat remain unchanged                      */
  void roll(float angle);

  /******************************* eye_yaw() *********************/
  /* Rotates the eye about the lookat point, using the up vector */
  /* Lookat is unaffected                                        */
  void eye_yaw(float angle);

  /******************************* eye_yaw_abs() ******************/
  /* Rotates the eye about the lookat point, with a specific axis */
  /* Lookat is unaffected                                         */
  void eye_yaw_abs(float angle, const vec3 &axis);


  /******************************* eye_pitch() ************/
  /* Rotates the eye about the side vector                */
  /* Lookat is unaffected                                 */
  void eye_pitch(float angle);

  /******************************* lookat_yaw()************/
  /* This assumes the up vector is correct.               */
  /* Rotates the lookat about the side vector             */
  /* Eye point is unaffected                              */
  void lookat_yaw(float angle);

  /******************************* lookat_pitch() *********/
  /* Rotates the lookat point about the side vector       */
  /* This assumes the side vector is correct.             */
  /* Eye point is unaffected                              */
  void lookat_pitch(float angle);

  /******************************* reset_up() ******************/
  /* Resets the up vector to a specified axis (0=X, 1=Y, 2=Z)  */
  /* Also sets the eye point level with the lookat point,      */
  /* along the specified axis                                  */
  void reset_up(int axis_num);

  void reset_up();

  /******************************* move() ********************/
  /* Moves a specified distance in the forward, side, and up */
  /* directions.  This function does NOT move by world       */
  /* coordinates.  To move by world coords, use the move_abs */
  /* function.                                               */
  void move(float side_move, float up_move, float forw_move);

  void move(const vec3 &v);

  /******************************* move_by_eye() ***********/
  /* Sets the eye point, AND moves the lookat point by the */
  /* same amount as the eye is moved.                      */
  void move_by_eye(const vec3 &new_eye);

  /******************************* move_by_lookat() *********/
  /* Sets the lookat point, AND moves the eye point by the  */
  /* same amount as the lookat is moved.                    */
  void move_by_lookat(const vec3 &new_lookat);

  /******************************* move_abs() *****************/
  /* Move the eye and lookat in world coordinates             */
  void move_abs(const vec3 &v);

  /****************************** rot_about_eye() ************/
  /* Rotates the lookat point about the eye, based on a 4x4  */
  /* (pure) rotation matrix                                  */
  void rot_about_eye(const mat4 &rot);

  /****************************** rot_about_lookat() ************/
  /* Rotates the lookat point about the lookat, based on a 4x4  */
  /* (pure) rotation matrix                                  */
  void rot_about_lookat(const mat4 &rot);

  /******************************* make_mtx() *************/
  /* Constructs a 4x4 matrix - used by load_to_openGL()   */
  void make_mtx();

  /******************************* load_to_openGL() ********/
  /* Sets the OpenGL modelview matrix based on the current */
  /* camera coordinates                                    */
  void load_to_openGL();

  /******************************* load_to_openGL_noident() ******/
  /* Multiplies the current camera matrix by the existing openGL */
  /* modelview matrix.  This is same as above function, but      */
  /* does not set the OpenGL matrix to identity first            */
  void load_to_openGL_noident();

  /******************************* reset() ****************/
  /* Resets the parameters of this class                  */
  void reset();

  /******************************* ViewModel() ************/
  /* Constructor                                          */
  ViewModel();

  /******************************* update() ****************/
  /* updates the view params.  Call this after making      */
  /* direct changes to the vectors or points of this class */
  void update();

  /******************************* dump() *******************/
  /* Prints the contents of this class to a file, typically */
  /* stdin or stderr                                        */
  void dump(FILE *output) const;
};

#endif
