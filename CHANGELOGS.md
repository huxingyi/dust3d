Changes between 1.0.0-beta.18 and 1.0.0-beta.19:
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
