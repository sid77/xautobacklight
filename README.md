# xautobacklight
Inspired by http://www.tedunangst.com/flak/post/xautobacklight .

Pretty much the same code but with privileged ioctl functions removed in favour
of fork/exec of `xbacklight`. I'm not yet using the gamma correction function
since it causes some weird display issues on my machine.

Compile with:
```
cc -std=c99 -O2 -I /usr/X11R6/include -L /usr/X11R6/lib -lX11 -lXrandr -o xautobacklight xautobacklight.c
```
