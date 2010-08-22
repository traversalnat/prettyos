[map symbols vidswtch.map] ; use this for ckernel.c addresses
[bits 16]
[section .text]

org 0x100

video_mode_640_480_256:
	mov ax, 0x4F02
	mov bx, 0x4101 ; video mode
	int 10h
	jmp exitvm86

video_mode_800_600_256:
	mov ax, 0x4F02
	mov bx, 0x4103 ; video mode
	int 10h
	jmp exitvm86

video_mode_1024_768_256:
	mov ax, 0x4F02
	mov bx, 0x4105 ; video mode
	int 10h
	jmp exitvm86

VgaInfoBlock:
    xor ax, ax
	mov es, ax
	mov ax, 0x3400
	mov di, ax
	mov ax, 0x4F00
	int 10h
	jmp exitvm86

ModeInfoBlock_640_480_256:
    xor ax, ax
	mov es, ax
	mov ax, 0x3600
	mov di, ax
	mov ax, 0x4F01
	mov cx, 0x4101 ; video mode
	int 10h
	mov word [0x1300], ax ; return value
	xor ax,ax
	mov ds,ax
	jmp exitvm86

ModeInfoBlock_800_600_256:
    xor ax, ax
	mov es, ax
	mov ax, 0x3600
	mov di, ax
	mov ax, 0x4F01
	mov cx, 0x4103 ; video mode
	int 10h
	mov word [0x1300], ax ; return value
	xor ax,ax
	mov ds,ax
	jmp exitvm86

ModeInfoBlock_1024_768_256:
    xor ax, ax
	mov es, ax
	mov ax, 0x3600
	mov di, ax
	mov ax, 0x4F01
	mov cx, 0x4105 ; video mode
	int 10h
	mov word [0x1300], ax ; return value
	xor ax,ax
	mov ds,ax
	jmp exitvm86

;SetBank:
;	mov ax, 0x4F05
;	mov bx, 0
;   mov dx, bank
;	jmp exitvm86

;GetBank:
;	mov ax, 0x4F05
;	mov bx, 1
;   mov dx, bank
;   jmp exitvm86

;SetScanLinePixel:
;   mov ax, 0x4F06
;   mov bl, 0
;	jmp exitvm86

;GetScanLinePixel:
;	mov ax, 0x4F06
;	mov bl, 1
;	jmp exitvm86

;SetScanLineBytes:
;   mov ax, 0x4F06
;   mov bl, 02
; 	jmp exitvm86

;GetMAXScanLine:
;	mov ax, 0x4F06
;	mov bl, 3
;	jmp exitvm86

SetDisplayStart:
	mov ax, 0x4F07
	mov bl, 0
	xor ax, ax
	mov ds, ax
	mov dx, word[0x1800] ; Set first Displayed Scan Line
	mov cx, word[0x1802] ; Set first Displayed Pixel in Scan Line
    int 10h
 	jmp exitvm86

GetDisplayStart:
	mov ax, 0x4F07
	mov bl, 1
	int 10h
	xor ax, ax
	mov ds, ax
	mov word [0x1300], dx ; First Displayed Scan Line
	mov word [0x1302], cx ; First Displayed Pixel in Scan Line
 	jmp exitvm86

SetDacPalette:
	mov ax, 0x4F08		;Set DAC Palette Format
	mov bl, 0
    mov bx, 6
    int 10h
 	jmp exitvm86

GetDacPalette:
	mov ax, 0x4F08		;Get DAC Palette Format
	mov bl, 1
    int 10h
 	jmp exitvm86

SetPalette:
	mov ax, 0x4F09
	mov bl, 0           ;=00h    Set palette data
	mov cx, 0xFF
	xor dx, dx
	xor ax, ax
	mov es, ax
	mov ax, 0x1600
	mov di, ax
	int 10h
	jmp exitvm86

GetPalette:
	xor ax, ax
	mov es, ax
	mov ax, 0x1400
	mov di, ax
	mov ax, 0x4F09
    mov bl, 1			;=01h    Get palette data
	int 10h				;=03h    Get secondary palette data
	jmp exitvm86

;GetProtectedModeInterface:
;	mov ax, 0x4F0A
;	mov bl, 0
;   jmp exitvm86

text_mode:
	mov ax, 0x0002
	int 0x10
	mov ax, 0x1112
	xor bl, bl
	int 0x10
	jmp exitvm86

; stop and leave vm86-task
exitvm86:
	hlt     ; is translated as exit() at vm86.c
	jmp $   ; endless loop
