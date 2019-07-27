Script Panel
------------------------

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/master/docs/images/dust3d-ui-script.png

Dust3D supports JavaScript language to generate the source document, which then been fed into the mesh generator to construct the final model. Please check `Dust3D Script Reference`_ for details.

.. _Dust3D Script Reference: http://docs.dust3d.org/en/latest/script_reference.html

1. Rendered Model
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Result model generated from the script.

2. Script Editor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Paste your JavaScript code here, or type in the code directly. The script runner will be invoked automatically once there is any change detected from the script editor.
If there is an error or console output from the script, the output will show under the editor.

3. Variables Panel
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can create input control from script and receive the user input variable which can be used to adjust the settings for the procedural generation.
