**Icons are not made by me, so if you want use these icons please check the listed license details inside current folder.**  

The following command line is using [squeezer](https://github.com/huxingyi/squeezer) to pack all the icons to one png.  
```sh
./squeezerw files --verbose --width 128 --height 128 --border 0 --allowRotations 0 --outputTexture ../res/icons.png --outputInfo ../src/icons.c --infoHeader "//THIS FILE IS GENERATED AUTOMATICALLY BY https://github.com/huxingyi/squeezer, PLEASE DONT MODIFY IT MANUALLY.\n\n#include \"icons.h\"\n\nconst int iconTable[][ICON_ITEM_MAX] = {\n" --infoFooter "\n};\n" --infoBody "  {%w, %h, %x, %y, %l, %t, %c, %r} /*\"%n\"*/" --infoSplit ",\n"
```
