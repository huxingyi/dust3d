/*

  viewmodel.cpp

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

/***********************************************************************

      1996, Paul Rademacher (rademach@cs.unc.edu)
  Oct 2003, Nigel Stewart - GLUI Code Cleaning
  
************************************************************************/

#include "viewmodel.h" 

#include "GL/glui.h"

#include <cstdio>
#include <cstdlib>

void 
ViewModel::set_distance(const float new_distance) 
{
    if ( new_distance <= 0.0 )  /* Distance has to be positive */
        return;

    /* We find the current forward vector */
    forward = lookat - eye;
    forward.normalize();

    /* Set distance */
    distance = new_distance;

    /* Find new eye point */
    eye = lookat - forward * distance;
}

void 
ViewModel::set_up(const vec3 &new_up) 
{
    up = new_up;
    update();
}

void 
ViewModel::set_up(const float x, const float y, const float z) 
{
    set_up(vec3(x,y,z));
}

void 
ViewModel::set_eye(const vec3 &new_eye) 
{
    eye = new_eye;
    update();
}

void 
ViewModel::set_eye(const float x, const float y, const float z) 
{
    set_eye(vec3(x,y,z));
}

void 
ViewModel::set_lookat(const vec3 &new_lookat) 
{
    lookat = new_lookat;
    update();
}

void 
ViewModel::set_lookat(const float x, const float y, const float z) 
{
    set_lookat(vec3(x,y,z));
}


void 
ViewModel::roll(const float angle) 
{
    mat4 rot = rotation3D( forward, angle );
    up = rot * up;
    update();
}

void 
ViewModel::eye_yaw(const float angle)
{
    vec3 eye_pt = eye - lookat; /* eye w/lookat at center */
    mat4 rot    = rotation3D( up, angle );

    eye_pt = rot * eye_pt;
    eye    = lookat + eye_pt;
 
    update();
}

void 
ViewModel::eye_yaw_abs(const float angle, const vec3 &axis) 
{
    vec3 eye_pt   = eye  - lookat;   /* eye w/lookat at center */
    mat4 rot      = rotation3D( axis, angle );

    eye_pt  = rot * eye_pt;
    eye     = lookat + eye_pt;

    up      = rot * up;

    update();
}


void 
ViewModel::eye_pitch(const float angle) 
{
    vec3 eye_pt = eye - lookat; /* eye w/lookat at center */
    mat4 rot    = rotation3D( side, angle );

    eye_pt = rot * eye_pt;
    eye    = lookat + eye_pt;

    up = rot * up;
 
    update();
}

void 
ViewModel::lookat_yaw(const float angle) 
{
    vec3 lookat_pt = lookat - eye;   /* lookat w/eye at center */
    mat4 rot = rotation3D( up, -angle );

    lookat_pt = rot * lookat_pt;
    lookat = eye + lookat_pt;

    update();
}

void 
ViewModel::lookat_pitch(const float angle) 
{
    vec3 lookat_pt = lookat - eye;   /* lookat w/eye at center */
    mat4 rot = rotation3D( side, -angle );
    
    lookat_pt = rot * lookat_pt;
    lookat = eye + lookat_pt;

    up = rot * up;

    update();
}

void 
ViewModel::reset_up(const int axis_num) 
{
    float eye_distance = (lookat - eye).length();
    eye[axis_num] = lookat[axis_num]; 
    /* Bring eye to same level as lookat */

    vec3 vector = eye - lookat;
    vector.normalize();
    vector *= eye_distance;

    eye = lookat + vector;
    up.set( 0.0, 0.0, 0.0 );
    up[axis_num] = 1.0;

    update();
}

void 
ViewModel::reset_up() 
{
    reset_up( VY ); /* Resets to the Y axis */
}

void 
ViewModel::move(const float side_move, const float up_move, const float forw_move) 
{
    eye    += forward * forw_move;
    eye    += side    * side_move;
    eye    += up      * up_move;
    lookat += forward * forw_move;
    lookat += side    * side_move;
    lookat += up      * up_move;
    update();
}

void 
ViewModel::move(const vec3 &v) /* A vector version of the above command */
{ 
    move( v[VX], v[VY], v[VZ] );
}

void 
ViewModel::move_by_eye(const vec3 &new_eye) 
{
    vec3 diff = new_eye - eye;

    eye    += diff;
    lookat += diff;

    update();
}

void 
ViewModel::move_by_lookat(const vec3 &new_lookat) 
{
    vec3 diff = new_lookat - lookat;

    lookat += diff;
    eye    += diff;

    update();
}

void 
ViewModel::move_abs(const vec3 &v) 
{
    eye    += v;
    lookat += v;

    update();
}

void 
ViewModel::rot_about_eye(const mat4 &rot) 
{
    vec3  view = lookat - eye;

    view = rot * view;
    up   = rot * up;

    lookat = eye + view;

    update();
}

void 
ViewModel::rot_about_lookat(const mat4 &rot) 
{
    // NOT QUITE RIGHT YET

    vec3 view = eye - lookat;

    view = rot * view;
    up   = rot * up;

    eye = lookat + view;

    update();
}

void 
ViewModel::make_mtx() 
{
    update();

    mtx[0][0]=side[VX]; mtx[0][1]=up[VX]; mtx[0][2]=forward[VX]; mtx[0][3]=0.0;
    mtx[1][0]=side[VY]; mtx[1][1]=up[VY]; mtx[1][2]=forward[VY]; mtx[1][3]=0.0;
    mtx[2][0]=side[VZ]; mtx[2][1]=up[VZ]; mtx[2][2]=forward[VZ]; mtx[2][3]=0.0;
    mtx[3][0]=0.0;      mtx[3][1]=0.0;    mtx[3][2]= 0.0;        mtx[3][3]=1.0;
}

void 
ViewModel::load_to_openGL() 
{
    mat4  m;

    make_mtx();

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glMultMatrixf( (float*) &mtx[0][0]);
    glTranslatef( -eye[VX], -eye[VY], -eye[VZ] );
}

void 
ViewModel::load_to_openGL_noident() 
{
    mat4  m;

    make_mtx();

    glMatrixMode( GL_MODELVIEW );
    glMultMatrixf( (float*) &mtx[0][0]);
    glTranslatef( -eye[VX], -eye[VY], -eye[VZ] );
}

void 
ViewModel::reset() 
{
    up.set( 0.0, 1.0, 0.0 );

    eye.set(0.0,0.0,10.0);
    lookat.set(0.0,0.0,0.0);

    mtx = identity3D();

    update();
}

ViewModel::ViewModel() 
{
    reset();
}

void 
ViewModel::update() 
{
    /* get proper side and forward vectors, and distance  */
    forward  = -(lookat - eye);
    distance = forward.length();
    forward /= distance;

    side     = up ^ forward;
    up       = forward ^ side;

    forward.normalize();
    up.normalize();
    side.normalize();
}


void 
ViewModel::dump(FILE *output) const 
{
    fprintf( output, "Viewmodel: \n" );
    eye.print(    output, "  eye"    );
    lookat.print( output, "  lookat" );
    up.print(     output, "  up"     );
    side.print(   output, "  side"   );
    forward.print(output, "  forward");
    mtx.print(    output, "  mtx"    );
}
