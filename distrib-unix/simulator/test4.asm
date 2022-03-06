;       Example code which might be created by the program
;       PROGRAM TEST;
;       VAR X;
;       BEGIN
;           X := 5;
;           WHILE X >= 0 DO BEGIN
;               WRITE( X );
;               X := X - 1;
;           END;
;       END.
;
0   Inc  1      ; create local variable space for X
1   Load #5     ; store 5 in it
2   Store 0
3   Load 0      ; do comparison with 0
4   Load #0
5   Sub
6   Bl 14
7   Load 0      ; load X for write
8   Write
9   load 0      ; X := X - 1;
10  load #1
11  sub
12  store 0
13  br    3
14  halt
