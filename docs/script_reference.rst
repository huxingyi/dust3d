Dust3D Script Reference
---------------------------------------
Introduction
===============
| The most important function of Dust3D is to generate mesh from the source document, which contains the position, radius and connectivity informations of all the nodes.
|
| Normally, the nodes are drawn by user on the canvas. The scripting system, which we introduce in this document, is to provide a way to procedurally generate nodes, and then the mesh can be generated from these nodes.
|
| Using nodes to generate mesh, already speed up the whole modeling progress. Scripting is to help speed up the speed up.
|
| The following is the screenshot of the `Script Panel`_, demonstrating how a tree been procedurally generated, you can find it from the Software Open Example Menu as Procedural Tree example.

.. _Script Panel: http://docs.dust3d.org/en/latest/interface/script_panel.html

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/master/docs/images/dust3d-script-reference.png


Document Structure
=====================

The following two pictures shows the structure of Dust3D Document from different perspective.

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/master/docs/images/dust3d-document-structure-ui.png

.. image:: https://raw.githubusercontent.com/huxingyi/dust3d/master/docs/images/dust3d-document-structure-xml.png

*You can select the nodes, chose copy from context menu and paste to a text editor, then you can see the xml content just like the second screenshot*

1. Document

2. Canvas

3. Nodes and Edges

4. Part

5. Component

Document API
==================

document.**createComponent**(*parentComponent*)

    | Create component and return the created component.
    |
    | If *parentComponent* is omit, then component will be created as top level.

document.**createPart**(*component*)

    | Create part as child of *component* and return the created part.
    |
    | The *component* is not optional.

document.**createNode**(*part*)

    | Create node as child of *part* and return the created node.
    |
    | The *part* is not optional.

document.**connect**(*firstNode*, *secondNode*)

    | Connect two nodes by create a new edge between *firstNode* and *secondNode*.
    |
    | *firstNode* and *secondNode* must come from the same part.

document.**setAttribute**(*element*, *attributeName*, *attributeValue*)

    | *element* can be component, part, node, and canvas.

document.**attribute**(*element*, *attributeName*)

    | Fetch the attribute value of *element*, name specified by *attributeName*

document.**createCheckInput**(*controlName*, *defaultChecked*)

    | Create a check input control, and return the value specified by user.
    |
    | *defaultChecked* can be *true* or *false*
    | Return *true* when user checked the input box otherwise *false*.

document.**createFloatInput**(*controlName*, *defaultValue*, *minValue*, *maxValue*)

    | Create a float number input control, and return the value specified by user.

document.**createIntInput**(*controlName*, *defaultValue*, *minValue*, *maxValue*)

    | Create a int number input control, and return the value specified by user.

document.**createColorInput**(*controlName*, *defaultColor*)

    | Create a color picker, and return the color picked by user.
    |
    | Returned value is a hex string represent the color,
    | for example,
    |
    | var leafColor = document.createColorInput("Leaf Color", "#58834b");
    | document.setAttribute(part, "color", leafColor);

document.**createSelectInput**(*controlName*, *defaultSelectedIndex*, *optionsArray*)

    | Create a select control, and return the selected index by user.
    |
    | *optionsArray* is a array list all the options,
    | for example,
    |
    |   var modeList = ["Mode1", "Mode2", "Mode3"];
    |   var selectedModeIndex = document.createSelectInput("Mode", 0, modeList);
    |   console.log(modeList[selectedModeIndex]);

3D Math API
==================
Dust3D introduces 3D vector math from `three.js`_, the following API are supported:
`Vector3`_, `Quaternion`_.

.. _three.js: https://threejs.org
.. _Vector3: https://threejs.org/docs/#api/en/math/Vector3
.. _Quaternion: https://threejs.org/docs/#api/en/math/Quaternion

Document Element Attributes
=============================

Following are all the attributes grouped by element type, which can be used as the second parameter of *document.setAttribute* and *document.attribute*.
Both attribute name and value are string format, even for boolean type, for example,

    document.setAttribute(component, "expanded", "false")

canvas
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There is only one canvas for a document, and you don't need to create it, use *document.canvas* to fetch it.

+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Name                        | Description                                                                                                                                                                                           |
+=============================+=======================================================================================================================================================================================================+
| | originX                   | The center anchor position.                                                                                                                                                                           |
| | originY                   |                                                                                                                                                                                                       |
| | originZ                   |                                                                                                                                                                                                       |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| rigType                     | The only option currently supported is **Animal**                                                                                                                                                     |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

component
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can have multiple depth of components. Use *document.createComponent* to create it.

+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Name                        | Description                                                                                                                                                                                                             |
+=============================+=========================================================================================================================================================================================================================+
| name                        | This name will be displayed on Parts Tree Widget                                                                                                                                                                        |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| expanded                    | All components show as a tree widget. This attribute control if the tree item expand or not.                                                                                                                            |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| combineMode                 | | Options:                                                                                                                                                                                                              |
|                             | |     `**Normal**` This component's generated mesh will combine with previous components' generated mesh by union algorithm.                                                                                            |
|                             | |                                                                                                                                                                                                                       |
|                             | |     `**Inversion**` This component's generated mesh will subtract the previous components' generated mesh.                                                                                                            |
|                             | |                                                                                                                                                                                                                       |
|                             | |     `**Uncombined**` This component's generated mesh will merge with previous components's generated mesh directly, without boolean algorithm. Use this option will cause the final generated mesh not watertight.    |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

part
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Part can't be created directly from document, it must belong to a component. Use *document.createPart* to create it.

+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Name                        | Description                                                                                                                                                                                           |
+=============================+=======================================================================================================================================================================================================+
| visible                     | Show nodes and edges on canvas if this attribute is **true**.                                                                                                                                         |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| locked                      | Lock nodes and edges on canvas if this attribute is **true**.                                                                                                                                         |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| subdived                    | The generated mesh will be subdivided if this attribute is **true**.                                                                                                                                  |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| disabled                    | This part's mesh will not be included in the final mesh result if this attribute is *true*, but still shows preview on Parts Tree Widget.                                                             |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| xMirrored                   | This part's mesh will be mirrored on X axis if this attribute is **true**.                                                                                                                            |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| base                        | | Options:                                                                                                                                                                                            |
|                             | |     `**XYZ**` This value is actually showed as Dynamic on the UI.                                                                                                                                   |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Average**`                                                                                                                                                                                   |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**YZ**` This value is actually showed as Side Plane on the UI.                                                                                                                                 |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**XY**` This value is actually showed as Front Plan on the UI.                                                                                                                                 |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**ZX**` This value is actually showed as Top Plane on the UI.                                                                                                                                  |
|                             | |                                                                                                                                                                                                     |
|                             | |     Please check the `Script Panel`_ for more introduction about *Base*.                                                                                                                            |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| rounded                     | An extra small sized face compared to the original one at the end effector will be generated if this attribute is **true**.                                                                           |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| chamfered                   | The edge of the generated mesh will be beveled if this attribute is **true**.                                                                                                                         |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| target                      | | Options:                                                                                                                                                                                            |
|                             | |     `**Model**` The generated mesh of this part will be included in the final mesh result.                                                                                                          |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**CutFace**` The generated mesh of this part will be excluded in the final mesh result. The nodes coordinations of this part on the XY plane will be used as other parts' Cut Face             |
|                             | |                                                                                                                                                                                                     |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| cutRotation                 | Range: -1.0 ~ 1.0, represents the Rotation of Cut Face from -180 to 180 degrees.                                                                                                                      |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| cutFace                     | | Options:                                                                                                                                                                                            |
|                             | |     `**Quad**`                                                                                                                                                                                      |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Pentagon**`                                                                                                                                                                                  |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Hexagon**`                                                                                                                                                                                   |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Triangle**`                                                                                                                                                                                  |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**{partId}**` You can specified other part's id as Cut Face. For example, document.setAttribute(part, "cutFace", document.attribute(leafTemplatePart, "id"));                                  |
|                             | |                                                                                                                                                                                                     |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| color                       |                                                                                                                                                                                                       |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| colorSolubility             | Range: 0.0 ~ 1.0, If set this attribute, the generated color texture seam between the neighbor parts will be gradient filled using this part’s color with the specified solubility.                   |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| deformThickness             | Range: 0.0 ~ 2.0                                                                                                                                                                                      |
| deformWidth                 |                                                                                                                                                                                                       |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. _Script Panel: http://docs.dust3d.org/en/latest/interface/script_panel.html

node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Node can't be created directly from document, it must belong to a part. Use *document.createNode* to create it.

+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Name                        | Description                                                                                                                                                                                           |
+=============================+=======================================================================================================================================================================================================+
| radius                      |                                                                                                                                                                                                       |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| | x                         | The node position on the canvas. The range of y is 0.0(Top) ~ 1.0(Bottom), range of x and z is 0.0(Left) ~ 1.0(Right)                                                                                 |
| | y                         |                                                                                                                                                                                                       |
| | z                         |                                                                                                                                                                                                       |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| boneMark                    | | Options: Used for rig generation.                                                                                                                                                                   |
|                             | |     `**None**`                                                                                                                                                                                      |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Limb**`                                                                                                                                                                                      |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Tail**`                                                                                                                                                                                      |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Joint**`                                                                                                                                                                                     |
|                             | |                                                                                                                                                                                                     |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| cutRotation                 | Range: -1.0 ~ 1.0, represents the Rotation of Cut Face from -180 to 180 degrees.                                                                                                                      |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| cutFace                     | | Options:                                                                                                                                                                                            |
|                             | |     `**Quad**`                                                                                                                                                                                      |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Pentagon**`                                                                                                                                                                                  |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Hexagon**`                                                                                                                                                                                   |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**Triangle**`                                                                                                                                                                                  |
|                             | |                                                                                                                                                                                                     |
|                             | |     `**{partId}**` You can specified other part's id as Cut Face. For example, document.setAttribute(node, "cutFace", document.attribute(leafTemplatePart, "id"));                                  |
|                             | |                                                                                                                                                                                                     |
+-----------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

Procedural Tree Example
==========================

*You can copy the following code to the script editor of Dust3D software to run it, scripting function requires 1.0.0-beta.22 and above*.

.. code-block:: none

    var SUBDIVED = document.createCheckInput("Subdived", false);
    var MAX_RADIUS_OFFSET = document.createFloatInput("Max Radius Offset", 0.10, -1.0, 1.0);
    var MAX_ORIGIN_OFFSET = document.createFloatInput("Max Origin Offset", 0.47, -1.0, 1.0);
    var TREE_HEIGHT = document.createFloatInput("Height", 0.85, 0.1, 1.0);
    var TRUNK_RADIUS = document.createFloatInput("Trunk Radius", 0.02, 0.01, 0.2);
    var TRUNK_SEGMENTS = document.createIntInput("Trunk Segments", 15, 2, 30);
    var TRUNK_COLOR = document.createColorInput("Trunk Color", "#939297");
    var BRANCH_MIN_ANGLE = document.createFloatInput("Branch Min Angle", 10.72, 8, 30);
    var BRANCH_MAX_ANGLE = document.createFloatInput("Branch Max Angle", 58.81, 50, 75);
    var MAX_BRANCHES_PER_GROWTH = document.createIntInput("Max Branches", 7, 1, 7);
    var BRANCH_LENGTH_MIN_RATIO = document.createFloatInput("Branch Length Min Ratio", 0.24, 0.2, 0.9);
    var BRANCH_LENGTH_MAX_RATIO = document.createFloatInput("Branch Length Max Ratio", 0.78, 0.2, 0.9);
    var LEAF_MIN_LENGTH = document.createFloatInput("Leaf Min Length", 0.04, 0.005, 0.1);
    var LEAF_MAX_LENGTH = document.createFloatInput("Leaf Max Length", 0.08, 0.005, 0.2);
    var LEAF_SEGMENTS = document.createIntInput("Leaf Segments", 5, 3, 6);
    var LEAF_COLOR = document.createColorInput("Leaf Color", "#58834b");

    var X = new THREE.Vector3( 1, 0, 0 );
    var Y = new THREE.Vector3( 0, 1, 0 );

    function createNode(part, origin, radius)
    {
        var node = document.createNode(part);
        document.setAttribute(node, "x", origin.getComponent(0));
        document.setAttribute(node, "y", origin.getComponent(1));
        document.setAttribute(node, "z", origin.getComponent(2));
        document.setAttribute(node, "radius", radius);
        return node;
    }

    THREE.Vector3.prototype.random = function (maxOffset)
    {
        for (var i = 0; i < 3; ++i)
            this.setComponent(i, this.getComponent(i) + maxOffset * Math.random());
        return this;
    };

    function randInRange(min, max)
    {
        if (min > max) {
            var tmp = min;
            min = max;
            max = tmp;
        }
        return (min + (max - min) * Math.random());
    }

    function createLeafCutFace()
    {
        var component = document.createComponent();
        var part = document.createPart(component);
        document.setAttribute(part, "target", "CutFace");
        var predefinesCutFace = [
            {radius:"0.005", x:"0.314637", y:"0.336525", z:"0.713406"},
            {radius:"0.005", x:"0.343365", y:"0.340629", z:"0.713406"},
            {radius:"0.005", x:"0.377565", y:"0.340629", z:"0.713406"},
            {radius:"0.0159439", x:"0.402189", y:"0.341997", z:"0.713406"},
            {radius:"0.005", x:"0.440492", y:"0.329685", z:"0.713406"},
            {radius:"0.005", x:"0.473324", y:"0.317373", z:"0.713406"},
        ];
        var previousNode = undefined;
        for (var i = 0; i < predefinesCutFace.length; ++i) {
            var node = createNode(part,
                new THREE.Vector3(parseFloat(predefinesCutFace[i]["x"]),
                    parseFloat(predefinesCutFace[i]["y"]),
                    parseFloat(predefinesCutFace[i]["z"])),
                parseFloat(predefinesCutFace[i]["radius"]));
            if (undefined != previousNode)
                document.connect(previousNode, node);
            previousNode = node;
        }
        return part;
    }

    function getRandChildDirection(parentDirection)
    {
        var rotationAxis = parentDirection.clone().cross(X);
        var degree = randInRange(BRANCH_MIN_ANGLE, BRANCH_MAX_ANGLE);
        var branchRotateQuaternion = new THREE.Quaternion();
        branchRotateQuaternion.setFromAxisAngle(rotationAxis, degree * (Math.PI / 180));
        var rotatedDirection = parentDirection.clone().applyQuaternion(branchRotateQuaternion);

        var distributeQuaternion = new THREE.Quaternion();
        distributeQuaternion.setFromAxisAngle(Y, 360 * Math.random() * (Math.PI / 180));
        rotatedDirection = rotatedDirection.applyQuaternion(distributeQuaternion);

        return rotatedDirection;
    }

    var CUTFACE_PART_ID = document.attribute(createLeafCutFace(), "id");

    function createLeaf(leafRootPosition, parentDirection)
    {
        var direction = getRandChildDirection(parentDirection);
        var component = document.createComponent();
        var part = document.createPart(component);
        document.setAttribute(part, "color", LEAF_COLOR);
        document.setAttribute(part, "cutFace", CUTFACE_PART_ID);
        var length = randInRange(LEAF_MIN_LENGTH, LEAF_MAX_LENGTH);
        var segments = LEAF_SEGMENTS;
        var maxRadius = length / 3;
        var toPosition = leafRootPosition.clone().add(direction.clone().multiplyScalar(length));
        var previousNode = undefined;
        for (var i = 0; i < segments; ++i) {
            var alpha = (i + 0.0) / segments;
            var origin = leafRootPosition.clone().lerp(toPosition, alpha);
            var radiusFactor = 1.0 - 2 * Math.abs(alpha - 0.3);
            var radius = maxRadius * radiusFactor;
            var node = createNode(part, origin, radius);
            if (undefined != previousNode)
                document.connect(previousNode, node);
            previousNode = node;
        }
    }

    function createBranch(branchRootPosition, radius, length, segments, parentDirection, maxRadius)
    {
        var rotatedDirection = getRandChildDirection(parentDirection);
        var branchEndPosition = branchRootPosition.clone().add(rotatedDirection.multiplyScalar(length));
        createTrunk(branchRootPosition, radius, branchEndPosition, segments, maxRadius);
    }

    function shouldGrowBranch(alpha, alreadyGrowedNum)
    {
        var factor = alpha * Math.random();
        if (factor > 0.5 && factor < 0.8) {
            if (alreadyGrowedNum > 0)
                return true;
            if (Math.random() > 0.5)
                return true;
        }
        return false;
    }

    function shouldGrowLeaf(alreadyGrowedNum)
    {
        var factor = Math.random();
        if (factor > 0.5) {
            if (alreadyGrowedNum > 0)
                return true;
            if (Math.random() > 0.5)
                return true;
        }
        return false;
    }

    function createTrunk(fromPosition, fromRadius, toPosition, segments, maxRadius)
    {
        var component = document.createComponent();
        var part = document.createPart(component);
        if (SUBDIVED)
            document.setAttribute(part, "subdived", "true");
        document.setAttribute(part, "color", TRUNK_COLOR);
        var previousNode = undefined;
        var toRadius = fromRadius * 0.2;
        var direction = toPosition.clone().sub(fromPosition).normalize();
        var length = fromPosition.distanceTo(toPosition);
        for (var i = 0; i < segments; ++i) {
            var alpha = (i + 0.0) / segments;
            var origin = fromPosition.clone().lerp(toPosition, alpha);
            var radius = fromRadius * (1.0 - alpha) + toRadius * alpha;
            radius += radius * MAX_RADIUS_OFFSET * Math.random();
            if (undefined != maxRadius && radius > maxRadius)
                radius = maxRadius;
            var oldY = origin.y;
            origin.random(length * 0.1 * MAX_ORIGIN_OFFSET);
            origin.setY(oldY + radius * 0.5 * Math.random());
            if (undefined != maxRadius && i == 0)
                origin = fromPosition;
            var node = createNode(part, origin, radius);
            if (undefined != previousNode)
                document.connect(previousNode, node);

            var maxBranches = MAX_BRANCHES_PER_GROWTH * Math.random();
            var alreadyGrowedBranchNum = 0;
            var alreadyGrowedLeafNum = 0;
            for (var j = 0; j < maxBranches; ++j) {
                if (shouldGrowBranch(alpha, alreadyGrowedBranchNum)) {
                    var branchLength = length * randInRange(BRANCH_LENGTH_MIN_RATIO, BRANCH_LENGTH_MAX_RATIO);
                    var branchRadius = radius * 0.5;
                    var branchSegments = segments * (branchLength / length);
                    if (branchSegments >= 3 && branchRadius > TRUNK_RADIUS / 20) {
                        createBranch(origin, branchRadius, branchLength, branchSegments, direction, radius);
                        ++alreadyGrowedBranchNum;
                    }
                }
                if (undefined != maxRadius && shouldGrowLeaf(alreadyGrowedLeafNum)) {
                    createLeaf(origin, direction);
                    ++alreadyGrowedLeafNum;
                }
            }
            previousNode = node;
        }
    }

    var treeRootPosition = new THREE.Vector3(0.5, 1.0, 1.0);
    var treeTopPosition = treeRootPosition.clone();
    treeTopPosition.add((new THREE.Vector3(0, -1, 0)).multiplyScalar(TREE_HEIGHT));

    document.setAttribute(document.canvas, "originX", 0.506767);
    document.setAttribute(document.canvas, "originY", 0.615943);
    document.setAttribute(document.canvas, "originZ", 1.08543);

    createTrunk(treeRootPosition, TRUNK_RADIUS, treeTopPosition, TRUNK_SEGMENTS);
