global get_num
global print_num

section .bss
buf     resb 22              ; '-' + 20 digits + '\n'  = 22 bytes max

section .text


print_num:
        mov	    rax, [rbp + 2 * 8]

        mov     rbx, buf + 21        
        mov     byte [rbx], 10       
        dec     rbx                  

        mov     rcx, rax             
        cmp     rax, 0
        jge     .digits
        neg     rax                  
        mov     rdi, 1               
        jmp     .digits_start
.digits:
        xor     rdi, rdi             
.digits_start:

.digit_loop:
        xor     rdx, rdx             
        mov     r8, 10
        div     r8                   
        add     dl, '0'              
        mov     [rbx], dl
        dec     rbx
        test    rax, rax
        jne     .digit_loop

        test    rdi, rdi
        jz      .adjust_ptr
        mov     byte [rbx], '-'
        dec     rbx
.adjust_ptr:
        inc     rbx                  

        mov     rdx, buf + 22        
        sub     rdx, rbx             

        mov     rax, 1               
        mov     rdi, 1               
        mov     rsi, rbx             
        syscall
        ret


get_num:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 32        

        push    rdi
        push    rsi
        push    rdx
        push    rcx
        push    r8
        push    r11            


        xor     edi, edi       
        lea     rsi, [rsp]     
        mov     edx, 32        
        xor     eax, eax       
        syscall                

        mov     rcx, rax       
        lea     rsi, [rsp]     
        xor     rax, rax       
        xor     r8,  r8        

        test    rcx, rcx
        jz      .done_parse
        cmp     byte [rsi], '-'
        jne     .parse_loop
        inc     rsi
        dec     rcx
        mov     r8b, 1

.parse_loop:
        test    rcx, rcx
        jz      .done_parse

        movzx   rdx, byte [rsi]
        cmp     rdx, '0'
        jb      .done_parse
        cmp     rdx, '9'
        ja      .done_parse

        imul    rax, rax, 10   
        sub     rdx, '0'
        add     rax, rdx

        inc     rsi
        dec     rcx
        jmp     .parse_loop

.done_parse:
        test    r8b, r8b
        jz      .cleanup
        neg     rax

.cleanup:
        pop     r11
        pop     r8
        pop     rcx
        pop     rdx
        pop     rsi
        pop     rdi

        mov     rsp, rbp
        pop     rbp
        ret

