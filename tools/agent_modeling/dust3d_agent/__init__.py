"""Dust3D agent modeling toolkit.

A small, dependency-light pipeline that lets an AI agent (or a script) create
Dust3D models automatically:

    spec (JSON)  --compile-->  .ds3  --dust3d -o-->  .glb  --inspect/render-->  report + PNG

The .ds3 produced is a normal Dust3D document, so every generated model can be
opened and refined by hand in the Dust3D editor afterwards.
"""

__version__ = "0.1.0"
