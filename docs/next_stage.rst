The Next Stage of Dust3D: for Monster Animation Clips Making
-----------------------------------------------------------------
This post is my plan for the coarse motion capture feature for my 3D modeling software, or maybe as a separate tool.
my initial target for Dust3D_ has been achieved, I can use it by myself to finish a 3d model in minutes which could took me half day while using traditional 3d softwares, as a newbie modeler.

.. _Dust3D: https://github.com/huxingyi/dust3d

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/master/docs/examples/modeling-camel/modeling-camel-dust3d-screenshot.png

I made this software mostly for myself to speed up the game assets making. Now, I can easily make model and colorize it, and got the model auto-rigged, the next step is to speed up the short animation clips making, such as walk, run, dead, attack... and so on.

Just as the modeling process, I want make this animation progress’s workload as small scale as possible, then I can finish the whole progress without outsourcing it, say, as a indie game developer.

Currently, I have some initial thoughts,
    1. Based on some physics simulation or other algorithms, auto generate skeleton animations.
    2. Use webcam to capture video, and analyze the motion from video by using OpenCV or Tensorflow.

I prefer the second method, use webcam, because I think can easily do it myself, if I want some weird animation, especially for monster characters, and maybe it is hard to represent as a simulation or algorithm.

This animation making should be very simple, no facial or muscles motion capture, just coarse motion, walk, run, dead, attack, for low poly model.

Here are some details I am going to implement:
    First capture one side video like this, just move my hands or body in front of the camera, and use OpenCV or Tensorflow to track the movement, automatically concludes the recognized shape as bones, match the bones's movement with the Model's skeleton. May do a part match, not a full skeleton match. Such as use my hands to mimic the snake head's movement.
    This is for one side, then do the same movement again, but with face forward to the side way of camera, so I can get another profile recorded, do the same analyze and match coords with the first record.

This sounds very simple, although I don't know if it would work. I have made a post on reddit's programming board, I hope I can get more ideas on this, just as I post my modeling software initial plan in 2016_ from scratch,

.. _2016: https://www.reddit.com/r/gamedev/comments/5iuf3h/i_am_writting_a_3d_monster_model_generate_tool/

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/331889ece938c463f449a226fc353e26ed39d6bc/screenshot/dust3d_sphere_cylinder.png

and got it first dev release shipped in 2018_, thanks for the wonderful Redditors,

.. _2018: https://www.reddit.com/r/gamedev/comments/8dfihy/dust3d_a_brand_new_3d_modeling_software_for_game/

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/master/docs/examples/modeling-ant/modeling-ant-dust3d-screenshot.png

I have shared my full progress here_, including how I back and forth in Qt/C++ choice, and how the Rust language is been used as the core algorithm implementation,

.. _here: https://dust3d.readthedocs.io/en/latest/origin-and-future.html

I wish this post is my new start of the game making journey, **please leave a comment on reddit website to share your idea, what's the best way to achieve the simple monster animation task, thanks**, and here is the reddit_ post link.

.. _reddit: https://www.reddit.com/r/programming/comments/8g6i8l/i_am_writing_a_coarse_motion_capture_software_to/
