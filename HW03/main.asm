    bits 64; машинное слово - 8 байт
    extern malloc, puts, printf, fflush, abort; Объявляем, что эти 5 функций существуют где-то снаружи — в стандартной библиотеке C
    global main; Делает метку main видимой для линковщика

    section   .data; Начало секции данных
empty_str: db 0x0
int_format: db "%ld ", 0x0

;  ┌───────────┬────────────────────┬─────────┬────────────┐
;  │ Директива │ Расшифровка        │ Размер  │ Аналог в C │
;  ├───────────┼────────────────────┼─────────┼────────────┤
;  │ db        │ define byte        │ 1 байт  │ char       │
;  │ dw        │ define word        │ 2 байта │ short      │
;  │ dd        │ define double word │ 4 байта │ int        │
;  │ dq        │ define quad word   │ 8 байт  │ long       │
;  └───────────┴────────────────────┴─────────┴────────────┘
data: dq 4, 8, 15, 16, 23, 42
data_length: equ ($-data) / 8; $ - текущий адрес, data - адрес массива, делим на 8 (ра

    section   .text
;;; print_int proc
print_int:
    push rbp
    mov rbp, rsp
    sub rsp, 16

    mov rsi, rdi
    mov rdi, int_format
    xor rax, rax
    call printf

    xor rdi, rdi
    call fflush

    mov rsp, rbp
    pop rbp
    ret

;;; p proc
p:
    mov rax, rdi
    and rax, 1
    ret

;;; add_element proc
add_element:
    push rbp
    push rbx
    push r14
    mov rbp, rsp
    sub rsp, 16; что-то непонятное (зачем?), скорее всего этот код был сгенерен?

    mov r14, rdi
    mov rbx, rsi

    mov rdi, 16; фиксируем аргумент для malloc - 16 байт
    call malloc
    test rax, rax
    jz abort

    mov [rax], r14
    mov [rax + 8], rbx

    mov rsp, rbp
    pop r14
    pop rbx
    pop rbp
    ret

;;; m proc
m:
    push rbp
    mov rbp, rsp
    sub rsp, 16

    test rdi, rdi
    jz outm

    push rbp
    push rbx

    mov rbx, rdi
    mov rbp, rsi

    mov rdi, [rdi]
    call rsi

    mov rdi, [rbx + 8]
    mov rsi, rbp
    call m

    pop rbx
    pop rbp

outm:
    mov rsp, rbp
    pop rbp
    ret

;;; f proc
f:
    mov rax, rsi; пишем 0 в ответ

    ; если первый аргумент 0, то возвращаем 0
    test rdi, rdi
    jz outf

    push rbx
    push r12
    push r13

    mov rbx, rdi
    mov r12, rsi
    mov r13, rdx

    mov rdi, [rdi]; получаем ссылку на список
    call rdx; и передаем в качестве аргумента в функцию четности
    test rax, rax; если вернулся 0, но рекурсивно вызываем функцию
    jz z

    mov rdi, [rbx]
    mov rsi, r12
    call add_element
    mov rsi, rax
    jmp ff

z:
    mov rsi, r12

ff:
    mov rdi, [rbx + 8]
    mov rdx, r13
    call f

    pop r13
    pop r12
    pop rbx

outf:
    ret

;;; main proc
main:
    mov rbp, rsp; сохраняем значение регистра

    ;По System V ABI (соглашение о вызовах) все регистры делятся на два типа:
    ;  • Caller-saved (rax, rcx, rdx, rsi, rdi, r8–r11) — вызываемая функция может их свободно портить. Если тебе нужно их значение после вызова — сам сохраняй.
    ;  • Callee-saved (rbx, rbp, r12–r15) — вызываемая функция обязана вернуть их в том же состоянии, в каком получила.
    push rbx; пушим rbx на стек. rbx - Callee-saved регистр. Мы должны сохранить его в том же состоянии, поэтому кладем на стек

    xor rax, rax; обнуляем rax регистр
    mov rbx, data_length; записываем в rbx длину массива
adding_loop:
    ; записываем значение по адресу элемента массива, здесь начинается с последнего элемента
    ; интересно, что счет идет не до нуля, а до 1 и не с 5, а с 6
    ; поэтому адрес data берется со сдвигом на 8 вверх (это компинсируется отсутствием нуля в rbx)
    mov rdi, [data - 8 + rbx * 8]

    ; На первой итерации записываем 0 в rsi, в дальнейшем это будет просто адрес из возврата add_element
    mov rsi, rax
    call add_element; нам в rax регистре должны вернуться пара значение - ноль

    ; декрементим до нуля невключительно
    dec rbx
    jnz adding_loop
    ; по итогу получилась структура [<значение>, <адрес> -> [ <значение>, <адрес> -> ... ]]

    mov rbx, rax

    mov rdi, rax
    mov rsi, print_int
    call m; рекурсивно вызывает функцию (второй аргумент) с параметром (первый аргумент) до тех пор пока указатель (сдвиг +8 не станет 0)

    mov rdi, empty_str
    call puts; печатаем пустую строку для перевода каретки

    mov rdx, p; записываем в регистр (3 арг) rdx функцию с проверкой на четность (1 - если нечетное, 0 - четное)
    xor rsi, rsi; mov rsi, 0
    mov rdi, rbx; первый аргумент ссылка на односвязный список
    call f; в итоге 1 арг: список, 2 арг: число 0, третий функция четности. Функция f фильтрует на нечетность и возвращает новый список из нечетных чисел

    ; рекурсивно до нуля вызываем print_int. Проще говоря, печатаем новый список нечетных чисел
    mov rdi, rax
    mov rsi, print_int
    call m

    ; переносим строку
    mov rdi, empty_str
    call puts

    pop rbx

    xor rax, rax
    ret
