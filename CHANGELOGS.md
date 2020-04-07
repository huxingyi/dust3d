Changes between 1.0.0-rc.3 and 1.0.0-rc.4:
--------------------------------------------------
- Add preference: Toon Shader  
- Add menu: Export as Image  
- Add auto saving  
- Change to single instance with multiple windows  
- Refactor stroke mesh builder  
- Add menu: Import  
- Show edge direction on canvas  
- Add example model: Backpacker  

Changes between 1.0.0-rc.1 and 1.0.0-rc.3:
--------------------------------------------------
- Fix negative skinning weight  
- Fix IBL shader  
- Add win32 release  

Changes between 1.0.0-beta.29 and 1.0.0-rc.1:
--------------------------------------------------
- Add shortcuts for menu items  
- Preserve window size between runs  
- Drop win32 support  
- Add win64 support  
- Add remesh by integrating Instant-Meshes  
- Add cloth simulation by integrating FastMassSpring  
- New rig generator  
- Add Spanish and Italian language  
- Fix glb exporting alpha mode setting  
- Add texture size preference setting  
- Add example model: Cat  

Changes between 1.0.0-beta.28 and 1.0.0-beta.29:
--------------------------------------------------
- Add marker pen: A new way of modeling from contour  
- Fix crash when nodes are extremely small  
- Fix shader for core profile  

Changes between 1.0.0-beta.27 and 1.0.0-beta.28:
--------------------------------------------------
- Add new generated mesh type: Grid mesh  
- Include non-manifold geometry in result  
- Fix smooth angle  
- Fix preview model zoom scale  

Changes between 1.0.0-beta.26 and 1.0.0-beta.27:
--------------------------------------------------
- Fix normal map generation  
- Fix transparency and solubility setting for texture generation  
- Fix shader for metallic parameter  
- Remove remote IO protocol  
- Add linear interpolation to motion timeline editor  
- Add countershading setting for texture generation  
- Update built-in example models  

Changes between 1.0.0-beta.25 and 1.0.0-beta.26:
--------------------------------------------------
- Fix undo for part tree widget  
- Add experiment auto colorizing (Thanks @enzyme69)  
- Improve mesh generation, more average edge length    
- Fix icon naming issue for icon themes (Thanks @creepertron95)  
- Add tile scale setting for texture generator  
- Add color transparency support  
- Remove built-in example model: Bob  
- Add built-in example model: Giraffe  

Changes between 1.0.0-beta.24 and 1.0.0-beta.25:
--------------------------------------------------
- Add shortcuts key F: bring part widget of hovered part to visible area
- Add shortcut key C: toggle chamfer state  
- Add shortcut key O: toggle flat shading  
- Add shortcut key R: toggle viewport rotation  
- Change behaviour of shortcut key H: toggle all nodes when there is no selection  
- Add mini buttons to quick set cut face rotation  
- Add one tool button to toggle viewport rotation  
- Increase the icon size of mini buttons  

Changes between 1.0.0-beta.23 and 1.0.0-beta.24:
--------------------------------------------------
- Support part color copy and paste  
- Add auto ragdoll animation, powered by bullet3  
- Fix texture generation  
- Fix optimized compiling options for windows  

Changes between 1.0.0-beta.22 and 1.0.0-beta.23:
--------------------------------------------------
- Fix rig bone name issue because of translation  
- Update application logo  
- Add deforming map  
- Improve UV quality  
- Speed up mesh generation by adding more caches  
- Add deforming brush  
- Remove drag mode icon  
- Add hollow modifier  

Changes between 1.0.0-beta.21 and 1.0.0-beta.22:
--------------------------------------------------
- Improve intermediate nodes generation  
- Add JavaScript language support for procedural nodes generation  
- Optimize cut face endpoints sort algorithm  
- Local average base normal to keep mesh flow  
- Support Open and Export from command line  
- Add check for updates to menu  
- Fix inconsistent base normal when shape is too straight  
- Add procedural tree example model  

Changes between 1.0.0-beta.20 and 1.0.0-beta.21:
--------------------------------------------------
- Fix build for Raspberry Pi 3 Model B+  
- Fix cut face direction for intermediate nodes  
- Fix mesh render result flashback  
- Sort cut face list by x position  
- Support customizing cut face per each node   

Changes between 1.0.0-beta.19 and 1.0.0-beta.20:
--------------------------------------------------
- Support language: Simplified Chinese  
- Fix crash on cut face nodes ordering  
- Improve quality of mesh generation  

Changes between 1.0.0-beta.18 and 1.0.0-beta.19:
--------------------------------------------------
- Improve Pose Editor UI
- Add new pose setting: yTranslationScale
- Add new base option: Average
- Fix part mesh generation variation
- Sort material, pose, and motion list by name
- Disable three nodes branch by default, could be enabled from preferences window
- Make flat shading as default

Changes between 1.0.0-beta.17 and 1.0.0-beta.18:
--------------------------------------------------
- Add preferences settings, including flat shading, default color, and default combine mode
- Fix user defined cut face flipped normal issues
- Improve shader light
- Fix motion editor issues
- Improve nodes scaling experience
- Fix rig generation
- Add more built-in example models
- Change application logo

Changes between 1.0.0-beta.16 and 1.0.0-beta.17:
--------------------------------------------------
- Remote IO Protocol
- Uncombined Mode
- User Defined Cut Face
- Base Plane Constrain
- Color Solubility
- Crash Fix

Changes between 1.0.0-beta.15 and 1.0.0-beta.16:
--------------------------------------------------
- Support onion skinning in pose editor  
- Upgrade CGAL version from 4.11.1 to 4.13  
- Replace triangulation with CGAL triangulate faces method   
- Update CGAL kernel using from exact to inexact  
- Fix invalid base normal on linux  
- Remove cut face customization  
- Make chamfer tool as a separate mini button  

Changes between 1.0.0-beta.14 and 1.0.0-beta.15:
--------------------------------------------------
- Improved mesh quality: more quads with even edge size  
- Remesh seams as better as possible and keep mesh manifold  
- Support cut face customization  
- Add chamfer tool  
- Add back wireframe view  
- Removed convex wrapping tool  
- Removed global rotation buttons  
- Removed rust language dependency: reimplemented mesh generator in C++  

Initial beta release:
------------------------
- Combinable Bmesh Modeling  
- Auto UV Unwrapping  
- Metallic/Roughness PBR Material Support  
- Auto Rigging  
- Pose Editing  
- Motion Editing  
- OBJ Export Support  
- FBX Export Support  
- glTF 2.0 Export Support  
