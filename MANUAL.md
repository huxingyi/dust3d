COMMANDS(DRAFT1)
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

COMMANDS(DRAFT2)
---------------
stone.dus
```
-define n:5 r:1
-add cube r:@r
-foreach @n
    -chamfer v:1
-end
```
```
-function curve n:6
    -add stone r:5 n:10
    -chamfer v:2 edge:2
    -chamfer f.n:1,0,1 f.p:0,0,0
    -chamfer loop:[v:1,v:2,v:3]
-end
```
