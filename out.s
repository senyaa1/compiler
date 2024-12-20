call main
hlt
; program begin


0_print:
; args
;(ident)  a
pop bx 
out bx 
ret 




1_input:
; args
in 
pop dx
ret




main:
; args
push 0
pop ax
mov [2_i], ax
4_whilestart:
;(ident)  i
push [2_i]
push 10
cmp
push FLAGS
push 2
and
push 0
cmp
jle 3_whileend
; body
;(ident)  i
push [2_i]
call 0_print
push dx
;(ident)  i
push [2_i]
push 1
add
;(ident)  i
pop ax
mov [2_i], ax
jmp 4_whilestart
3_whileend:
push 0
pop dx
ret


; program end
; data
2_i:
	 dq 0
