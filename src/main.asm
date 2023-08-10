;NOTE
;writing 'hello world '
start:
     jmp main
     ;
 puts:
    ;save registers we will modify

    push si
    push ax 
    push bx  

loop:
   ;loads next character in al
   or al , al
   ;verify if next character is null
   jz .done 

   mov ah, 0x0E
   ;call bios interrupt 
   mov bh,0
   ;set page number to 0
   int 0x10

   jmp .loop

   .done:

       pop bx
       pop si
       ret


main: 
;setup data segments
  mov ax , 0
  ;can't set ds/es directly
      mov ds, ax 
      mov es, ax 

      ;setup stack
      mov ss, ax 
      mov sp,0x7c00
      ;stack grows downwards from where are looded in memory


      ;print hello world message 
        mov si, msg_hello
        call puts 

        hlt 
        .halt
  
  jmp .halt