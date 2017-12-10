COMMANDS
------------
#### -add
```
dust3d -add cube +radius 1
dust3d -add plane
```

#### -chamfer-by-vertex-index
```
dust3d -add cube -chamfer-by-vertex-index 0 +amount 0.2 -chamfer-by-vertex-index 1
```

#### -chamfer-by-plane
```
dust3d -add cube -chamfer-by-plane +origin 0,0,0 +normal 0.2,0.9,0.3
dust3d -add cube -chamfer-by-vertex-index 0 +amount 0.2 -chamfer-by-vertex-index 1 -chamfer-by-plane +origin 0,0,0 +normal 0.2,0.9,0.3
```
