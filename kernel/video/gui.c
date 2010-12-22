/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "kheap.h"
#include "util.h"
#include "mouse.h"
#include "keyboard.h"
#include "vbe.h"
#include "gui_window.h"
#include "gui_button.h"

extern char mouse_lm;
extern int32_t mouse_x;
extern int32_t mouse_y;

// cursor
extern BMPInfo_t cursor_start;
extern BMPInfo_t cursor_end;

extern volatile window_t* window_list[MAX_WINDOWS];
extern window_t current_window;

void StartGUI()
{
    init_window_manager();
    button_t button = CreateButton(250, 220, 80, 20, "close");

    CreateWindow("Window 1", 10, 10, 340, 250, 0);
    CreateWindow("Window 2", 400, 10, 340, 250, 0);
    CreateWindow("Window 3", 10, 300, 340, 250, 0);
    CreateWindow("Window 4", 400, 300, 340, 250, 0);

    memcpy(window_list[1]->data, &cursor_start, (uintptr_t)&cursor_end - (uintptr_t)&cursor_start);
    // memcpy(current_window.data, &bmp_start, (uintptr_t)&bmp_end - (uintptr_t)&bmp_start);
    memcpy(window_list[4]->data, &cursor_start, (uintptr_t)&cursor_end - (uintptr_t)&cursor_start);    

    while(!keyPressed(VK_ESCAPE))
    {
        if(mouse_lm == 1)
        {
            vbe_drawString("left Mouse Button Pressed", 10, 2);

            if(mouse_x > 250 && mouse_x < 320 && mouse_y > 220 && mouse_y < 240)
            {
                DestroyWindow(1);
                vbe_clearScreen();
            }
        }

        if(window_list[1])
        {
            DrawWindow(1);
            DrawButton(&button);
        }

        DrawWindow(2);
        DrawWindow(3);
        DrawWindow(4);

        vbe_drawString("Press ESC to Exit!", 10, 2);
    }
}

void EndGUI()
{
    // DestroyWindow(window);
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/