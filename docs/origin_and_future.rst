Write a 3D modeling software from scratch
----------------------------------------------
*The origin and the future of Dust3D*


Origin
======================

I want to write a 3D modeling software since 2015, back to that time, I was trying to make a 2.5D MMORPG game. I self-learned blender for a while from youtube, the blender is quite good actually, however, the workload made me realized that it is impossible for one person to make tons of models, texture them, rigidify them, animate them, then use them in game, because built a simple dinosaur model took me half day. 

I watched lots of tutorials about how to quickly make a game ready model, tried to figure out a unified way, a repeatable mode, which can be simplified in programming language. I summarized most usual steps of making a model: firstly, setup a turnaround reference sheet for front view, side view, and back view, secondly, make a plane then subdivide to hexagon, extrude this hexagon by following the reference sheet, adjust the size of the face, fine-tune in different angle, back and forth, finally get the base model.

Initial Experiment
======================

Looks like I can write a software to automatically do these steps for me, I fed it reference sheet, it comes out a model. Let’s do it, I made a very rough test program, to recognize each view in the image, extract the boundaries, extrude faces according to the boundaries, it works if you zoom out the final result to a teeny tiny size. But it’s too small to be used in a game.

New Idea
======================

Some day, I googled some monster generation keywords, and found Jimmy Gunawan’s blog, I was shocked by his article_, this is what I am looking for, this is the answer, I was very excited and when dug into the technonigies behind blender’s Skin Modifier which described in Jimmy’s blog, I found the paper: <B-Mesh: A Fast Modeling System for Base Meshes of 3D Articulated Shapes>, at that point, I thought it’s time to finalize the idea to a real working software.

.. _article: https://blendersushi.blogspot.com.au/2013/06/inspiration-pixar-monster-factory-part.html

Launch
======================

I launched the Dust3D project, and post my plan on reddit_ even I haven’t done much things. I did this because, as a newbie in game industry, I don’t want miss somethings in the very beginning. Thanks the amazing Redditors, I learned lots of new software names and modeling terms, such as Meshmixer, CGAL, and so on.

.. _reddit: https://www.reddit.com/r/gamedev/comments/5iuf3h/i_am_writting_a_3d_monster_model_generate_tool/

Reinventing the wheel
======================

Reinventing the wheel is fun, so I didn’t strictly follow the advise by using the existing libraries, I want make a 3D software from scratch. I want draw the 3D world vertex by vertex, line by line. This is the first screenshot of Dust3D, using raw OpenGL without any dependency except OpenGL environment:

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/331889ece938c463f449a226fc353e26ed39d6bc/screenshot/dust3d_sphere_cylinder.png

Very quickly, I found it took so much time to debug the drawing issues, so I carefully introduced GLU library, this is what it looks like:

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/331889ece938c463f449a226fc353e26ed39d6bc/screenshot/dust3d_node_edge_with_glu.png

After some time, I thought Qt is much easy to use, so I carefully introduced Qt. And then the Bmesh algorithm implemented:

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/331889ece938c463f449a226fc353e26ed39d6bc/screenshot/dust3d_bmesh_test_2.png

Catmull-Clark subdivision:

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/331889ece938c463f449a226fc353e26ed39d6bc/screenshot/dust3d_bmesh_subdivide_2.png

Now, it’s time to make a more formal UI.

Reinventing the wheel Again
=============================
You can see, the repository start from zero dependency, and then introduced some things inevitably. This way goes well if continues. But there is something happened.
Because of no complex UI, I use blender to build the relationships of Bmesh balls, and I found a bug of blender in the Callada exporter,  I tried to fix it by myself, so I downloaded blender source, fix it then submitted a patch_. In this progress, I was sick of different C++ version problems, So I decided to remove all the C++ code from my Dust3D codebase.

.. _patch: https://developer.blender.org/D2489

Qt is C++, so Qt removed. I tried to find some UI library instead, Dear ImGui_ was promising, but because it’s C++, so abandoned. I started the UI from zero again, this is what it looks like:

.. _ImGui: https://github.com/ocornut/imgui

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/331889ece938c463f449a226fc353e26ed39d6bc/screenshot/dust3d_glw_preview_dark_2.png

Aha! 

Gap
======================

I started Dust3D project in Australia when I was on a work and holiday visa. There are lots of things stopped me from coding on this project, it’s quite busy. This made me to rethink the decisions I made. Remove all the dependencies are not good, I am making a 3D modeling software, not a GUI library. I also started to think about some details of the modeling progress. In the Bmesh paper, the author expressed some limitations that it is not suitable for making sharp edges. We all known, when we make a model for game, inevitably, there will be a cloth something, definitely makes some sharp shapes.

Reinvestigate
======================

I checked almost all the modeling software through youtube tutorials videos, tried to find how they do the job. these software including Houdini, I was shocked by the node based modeling technologies. I thought this is what I want, this is the answer, Looks familiar, right? :-)

Restart
======================

I created a new branch named poc to do the proof of concept. Not by exactly implementing the node based modeling, I tried to define a new modeling script language, it can be easily embed in command line. At that time, I built many fundamental mesh operating algorithms, such as chamfer a mesh, mesh booleans.

Rust
======================
I can’t remember the exact reason, maybe the project name? anyway, I was distracted by the rust language. I tried rewrite all the fundamental mesh algorithms in Rust to practice the language skills. This is how the meshlite_ library came out.

.. _meshlite: https://github.com/huxingyi/meshlite

Finalize
======================
Now, I had far much better understanding of mesh, and know how to generate the mesh I want, no matter it is smooth or sharp.
After finish the meshlite library, I tried to build the UI again. There are not so much choice of UI framework in the Rust world. And I did some investigation, and played many GUI solutions, such as bgfx_, I even fixed a trivial issue_ of bgfx and got it merged. But finally, I still decided to use Qt. This time, the whole coding progress is very smooth, Qt for UI, Rust for algorithm, worked like a charm, and Rust never crash on right use case, what I mean by saying that, rust have some built-in difficulty to build a double-linked like data structure, so I need some unsafe code or index based system to support the multiple linked data, such as the famous half-edge structure in mesh processing, because the index based system are not protected by the Rust language, sometimes, it crashes on some logic error. I found I am happy with Rust and C++11 and the new Qt signal slots, I also happily introduced Carve and CGAL libraries to do the mesh union.

.. _bgfx: https://github.com/bkaradzic/bgfx
.. _issue: https://github.com/bkaradzic/bgfx/pull/1311

Current
======================
Today, I decide to share my story, I have finished the stage one of Dust3D. It’s not perfect, but it is what I thought it should like in many years ago. This is what I want, this is the answer for my past years.

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/master/docs/examples/modeling-ant/modeling-ant-dust3d-screenshot.png

Future
======================
Currently, there is no auto unwrap texture, no auto rigidify, no auto animation generated. There is a far way to go, and I am looking forward to it. 
Thanks for reading.

PS: Auto-unwrap texture and auto-rigidify already landed, auto-animation is under development.  
(May 30, 2018)


