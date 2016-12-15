Dust3D
=========
Dust3D is aim to be a quick modeling tool for game development. Currently the development is in the very early stage.  

Idea
===========
I was inspired by Jimmy Gunawan's blogs of monster generation, here is the first one [INSPIRATION / Pixar Monster Factory Part One](http://blendersushi.blogspot.com.au/2013/06/inspiration-pixar-monster-factory-part.html), and by search the Skin Modifier of Blender, I found the theory of Skin Modifier:    
[B-Mesh: A Fast Modeling System for Base Meshes
of 3D Articulated Shapes](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.357.7134&rep=rep1&type=pdf)(Authors: Zhongping Ji, Ligang Liu2, Yigang Wang1). I started to think of monster model generation for game development from years ago, thanks for this paper, Dust3D is achievable now.  
From my initial thought, Dust3D should be a tool like [Makehuman](http://www.makehuman.org), with more versatile features, not only can make human, but also be able to **generate monsters automatically**.

Progress
==============
Drawing Sphere
-----------
I don't want the whole project have any unnecessary dependent, like glu library.
Let's start with [drawing a sphere without gluSphere]( http://stackoverflow.com/questions/7687148/drawing-sphere-in-opengl-without-using-glusphere), because I want implement the same balls which presented in the [B-Mesh paper](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.357.7134&rep=rep1&type=pdf).  
