Dust3D Interface Overview
-------------------------

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/master/docs/interface/interface.png

1. Tool Box
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Tool Box contains tools that you use to undo steps, switch the document edit mode, and toggle the XYZ-axis lock.

2. Document Title
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can have multiple windows, the document title shows what version of Dust3D you are using and the full path of the openned Dust3D document, if the document have unsaved changes, there will be a star(*) right after the path.

3. Parts List
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Part is a very important edit unit of Dust3D. A part consists of connected nodes in the canvas. If you cut the edge of two nodes, the one part would be divided to two seperate parts.
Parts List shows all the parts's rendered thumbnail, and current edit states. You can click the mini buttons around the parts thumbnail to toggle the settings.

4. Rendered Model
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Rendered Model shows current generated mesh. Anytime, if there is a change would affect the mesh generation, the mesh would be generated automatically in realtime, and get rendered to Rendered Model after generated.
You can use mouse Middle Button plus Shift key to rotate and move the Rendered Model. It's floating above the canvas.

5. Main Profile
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Every time you add a new node to canvas, there will be two items shows, the first one is the item you places, the second one is the automatically generated item for the same node but with different profile.
If you add a seperate node, which means it dosen't connect with other nodes, the node is in main profile, otherwise it depends on the profile of the node you add from.

6. Side Profile
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See Main Profile for explaining.
