bits 64
section .data
buf     times 32 db 0 
global print_num
global get_num

section .text

print_num:
    push rax 
    push rbx 
    push rcx 
    push rdx 
    push rsi 
    push rdi

    mov	    rax, [rbp + 2 * 8]

    mov     rdi, buf + 31          
    mov     byte   [rdi], 10       
    lea     rsi, [rdi - 1]         
    mov     rcx, rax               

.convert_loop:                     
    xor     rdx, rdx               
    mov     rbx, 10
    div     rbx                    
    add     dl, '0'                
    mov     byte [rsi], dl
    dec     rsi
    mov     rcx, rax               
    test    rax, rax
    jnz     .convert_loop

    inc     rsi                    
    mov     rdx, buf + 32          
    sub     rdx, rsi               

    mov     rax, 1                
    mov     rdi, 1               
    syscall

    pop rdi 
    pop rsi 
    pop rdx 
    pop rcx 
    pop rbx 
    pop rax
    ret

get_num:
        push    rbp
        mov     rbp, rsp

        push    rdi
        push    rsi
        push    rdx
        push    rcx
        push    r8
        push    r11            

        sub     rsp, 32        

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
        add     rsp, 32       

        pop     r11
        pop     r8
        pop     rcx
        pop     rdx
        pop     rsi
        pop     rdi

        leave                  
        ret

