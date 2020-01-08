A C++ implementation of *Fast Simulation of Mass-Spring Systems* [1], rendered with OpenGL.
The dynamic inverse procedure described in [2] was implemented to constrain spring deformations and prevent the "super-elastic" effect when using large time-steps.

### Dependencies
* **OpenGL, freeGLUT, GLEW, GLM** for rendering.
* **OpenMesh** for computing normals.
* **Eigen** for sparse matrix algebra.

### Demonstration
![curtain hang](https://media.giphy.com/media/5EC1drLIyBuHxRC4I8/giphy.gif)
![curtain drop](https://media.giphy.com/media/1zRbfGDmHTcn9RoUjy/giphy.gif)

### References
[1] Liu, T., Bargteil, A. W., Obrien, J. F., & Kavan, L. (2013). Fast simulation of mass-spring systems. *ACM Transactions on Graphics,32*(6), 1-7. doi:10.1145/2508363.2508406

[2] Provot, X. (1995). Deformation constraints in a mass-spring modelto describe rigid cloth behavior. *InGraphics Interface* 1995,147–154.