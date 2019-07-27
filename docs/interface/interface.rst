Dust3D Interface Overview
-------------------------

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/master/docs/images/dust3d-ui-main.png

1. Tool Box
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Tool Box contains tools that you use to switch the document edit mode, toggle the XYZ-axis and radius lock, indicate the mesh generating status, and trigger regenerate.

2. Document Title
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can have multiple windows, the document title shows what version of Dust3D you are using and the full path of the opened Dust3D document, if the document have unsaved changes, there will be a star(*) right after the path.

3. Dock Panel
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Dock panel contains multiple tabs, including `Parts Tree`_, Material List, Rig Weight View, Pose List, Motion List, and `Script Panel`_.

.. _Parts Tree: http://docs.dust3d.org/en/latest/interface/parts_panel.html
.. _Script Panel: http://docs.dust3d.org/en/latest/interface/script_panel.html

4. Rendered Model
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Rendered Model shows current generated mesh. Anytime, if there is a change would affect the mesh generation, the mesh would be generated automatically in realtime, and get rendered to Rendered Model after generated.
You can use mouse Middle Button plus Shift key to rotate and move the Rendered Model. It's floating above the canvas.

5. Node Front View
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Every time you add a new node to canvas, there will be two items shows, the first one is the item you places, the second one is the automatically generated item for the same node but with different profile.
If you add a separate node, which means it doesn't connect with other nodes, the node is in front view, otherwise it depends on the view of the node you add from.

6. Node Side View
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Side view is a profile heading left, see node front view for more details.
