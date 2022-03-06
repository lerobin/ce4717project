;
;       test7.asm
;
;       code to test the Ldp and Rdp instructions
;
;       first reserve 4 words of memory, 0, 1, 2, 3, as display registers.
;       some fiddly code is needed to do this. Because there is no way of
;       directly modifying the frame pointer, it is set up by "faking"
;       a stack frame and using rsf to remove it, "restoring" the desired
;       value into FP. SP is then adjusted to match.
;
  0 load #0     ; establish fake stack frame conatining new value for FP
  1 load #4     ; i.e., 4
  2 rsf         ; load FP using a Remove Stack Frame instruction
  3 inc   5     ; adjust the frame pointer
  4 load #-1    ; set the dispaly register values to a marker "invalid"
  5 store 0
  6 load #-1
  7 store 1
  8 load #-1
  9 store 2
 10 load #-1
 11 store 3
 12 ldp   0     ; set the display register for static scope 0
 13 inc   1     ; make 1 word of local storage at address 6
 14 push  fp    ; build a stack frame with a dummy value in [FP]
 15 bsf
 16 call  21
 17 rsf
 18 load FP+1   ; load value in local variable to tos
 19 write       ; display it
 20 halt
 21 ldp   1     ; set the display register for static scope 1
 22 read        ; get a value from the keyboard
 23 load  0     ; load display register contents for static scope 0
 24 store [SP]+1  ; store tos at address of local variable in that frame
 25 rdp   1     ; restore display register
 26 ret

