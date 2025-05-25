#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "tinyfiledialogs.h"

#define MAX_TEXT_LEN 128
#define MAX_LINES 64
#define WINDOW_WIDTH_INITIAL 640
#define WINDOW_HEIGHT_INITIAL 480

typedef struct{
SDL_Window *window;
SDL_Renderer *renderer;
int window_width;
int window_height;
int current_render_y;
}sdlwindow;

typedef struct{
TTF_Font *font;
SDL_Color color;
char lines[MAX_LINES][MAX_TEXT_LEN];
int cursor_location_y;
int cursor_location_x;
int MAX_VISIBLE_LINES;
int text_w;
int text_h;
int line_height;
}sdltext;



// quits all SDL features if they are created and exits
int quit_all(sdlwindow *win, sdltext *txt) {
    if (win->renderer){
        SDL_DestroyRenderer(win->renderer);
    }
    if (win->window){
        SDL_DestroyWindow(win->window);
    }
    if (txt->font){
        TTF_CloseFont(txt->font);
    }
    TTF_Quit();
    SDL_Quit();
    return -1;
}


// setup SDL code
int setup_WIN_REN_TTF(sdlwindow *win, sdltext *txt) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0 || TTF_Init() != 0) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0){
            printf("SDL_Init Error: %s\n", SDL_GetError());
        } 
        if (TTF_Init() != 0){
            printf("TTF_Init Error: %s\n", TTF_GetError());
        }     
        return quit_all(&win, &txt);
    } 

    win->window = SDL_CreateWindow("Beditor", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH_INITIAL, WINDOW_HEIGHT_INITIAL, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (win->window == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return quit_all(&win, &txt);
    }

    win->renderer = SDL_CreateRenderer(win->window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (win->renderer == NULL) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        return quit_all(&win, &txt);
    }

    txt->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 32);
    if (!txt->font) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        return quit_all(&win, &txt);
    }

    return 0;    
}



//mouse input function which calculates location in file
void set_cursor_from_mouse(int mouse_x, int mouse_y, sdltext *txt) {
    // Calculate which line was clicked
    int clicked_line = (mouse_y - 20) / (txt->line_height > 0 ? txt->line_height : 32);
    if (clicked_line < 0){
        clicked_line = 0;
    }
    if (clicked_line >= MAX_LINES){
        clicked_line = MAX_LINES - 1;
        
    } 
    txt->cursor_location_y = clicked_line;
    // Find the character position in the line
    int x = 25;
    txt->cursor_location_x = 0;
    int w = 0;
    size_t len = strlen(txt->lines[txt->cursor_location_y]);
    for (size_t i = 0; i <= len; ++i) {
        char temp[MAX_TEXT_LEN];
        strncpy(temp, txt->lines[txt->cursor_location_y], i);
        temp[i] = '\0';
        TTF_SizeText(txt->font, temp, &w, NULL);
        if (x + w > mouse_x) {
            txt->cursor_location_x = i;
            break;
        }
        txt->cursor_location_x = i;
    }
}



//render function, renders all features
void render_all(sdlwindow *win, sdltext *txt){
    // Render background
        SDL_SetRenderDrawColor(win->renderer, 255, 255, 255, 255);
        SDL_RenderClear(win->renderer);
        
        // Render text lines
        
        for (int i = 0; i < MAX_LINES; ++i) { 
            if (strlen(txt->lines[i]) > 0) {
                SDL_Surface *surf = TTF_RenderText_Solid(txt->font, txt->lines[i], txt->color);
                SDL_Texture *tex = SDL_CreateTextureFromSurface(win->renderer, surf);
                SDL_Rect dst = {20, win->current_render_y, surf->w, surf->h};
                if (i == 0) txt->line_height = surf->h;
                if (txt->line_height == 0) txt->line_height = 32;
                if (win->current_render_y + surf->h > win->window_height - 20) {
                    SDL_FreeSurface(surf);
                    SDL_DestroyTexture(tex);
                    break; 
                }
                SDL_RenderCopy(win->renderer, tex, NULL, &dst);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);
                win->current_render_y += txt->line_height;
            } else {
                win->current_render_y += (txt->line_height > 0 ? txt->line_height : 32);
            }
        }

        // Draw blinking cursor at the correct position
        int cursor_x = 20, cursor_y = 20 + txt->cursor_location_y * txt->line_height;
        if (txt->cursor_location_x > 0) {
            char cursor_prefix[MAX_TEXT_LEN];
            strncpy(cursor_prefix, txt->lines[txt->cursor_location_y], txt->cursor_location_x);
            cursor_prefix[txt->cursor_location_x] = '\0';
            int w = 0, h = 0;
            TTF_SizeText(txt->font, cursor_prefix, &w, &h);
            cursor_x += w;
        }


        Uint32 ticks = SDL_GetTicks();
        if ((ticks / 500) % 2 == 0) {
            SDL_Rect cursor_rect = {cursor_x, cursor_y, 2, txt->line_height > 0 ? txt->line_height : 32};
            SDL_SetRenderDrawColor(win->renderer, 0, 0, 0, 255); // black cursor
            SDL_RenderFillRect(win->renderer, &cursor_rect);
        }

        SDL_RenderPresent(win->renderer);
        SDL_Delay(10);
    return;
}



//file saving file here basic 
void save_to_file(const char *filename, char lines[MAX_LINES][MAX_TEXT_LEN]) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Could not open file for writing");
        return;
    }
    for (int i = 0; i < MAX_LINES; ++i) {
        if (strlen(lines[i]) > 0) {
            fprintf(file, "%s\n", lines[i]);
        }
    }
    fclose(file);
}


int main(int argc, char *argv[])
{
    sdlwindow win;
    sdltext txt;

    win.window = NULL;
    win.renderer = NULL;
    txt.font = NULL;

    SDL_StartTextInput();
    SDL_Event event;
    int running = 1;

    win.window_width = WINDOW_WIDTH_INITIAL;
    win.window_height = WINDOW_HEIGHT_INITIAL;
    win.current_render_y = 0;

    memset(txt.lines, 0, sizeof(txt.lines));
    txt.line_height = 0;
    txt.color.r = 0;
    txt.color.g = 0;
    txt.color.b = 0;
    txt.color.a = 255;   
    txt.MAX_VISIBLE_LINES = 0;
    txt.text_w = 0;
    txt.text_h = 0;
    txt.cursor_location_y = 0;
    txt.cursor_location_x = 0;
    
    setup_WIN_REN_TTF(&win,&txt); // call setup 

    while (running) {
    txt.MAX_VISIBLE_LINES = (win.window_height - 95) / (txt.line_height > 0 ? txt.line_height : 32); // calcultes visible lines
    win.current_render_y = 20;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {

                running = 0;  // stop program if you exit via SDL_quit

            } else if (event.type == SDL_WINDOWEVENT) {   //resizing code
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {

                    win.window_width = event.window.data1;
                    win.window_height = event.window.data2;
                    
                }
            } else if (event.type == SDL_KEYDOWN) {  // if button is pressed
                if ((event.key.keysym.sym == SDLK_s) && (event.key.keysym.mod & KMOD_CTRL)) {   // for saving with tinyfiledialog

                    const char *filename = tinyfd_saveFileDialog("Save As", "output.txt", 0, NULL, NULL);

                if (filename) {

                save_to_file(filename, txt.lines);  // calls function above

                    }
                }
                else if (event.key.keysym.sym == SDLK_BACKSPACE && txt.cursor_location_x > 0) {

                    size_t len = strlen(txt.lines[txt.cursor_location_y]);                // backspace behavior for more than 1 word
                    memmove(&txt.lines[txt.cursor_location_y][txt.cursor_location_x - 1],&txt.lines[txt.cursor_location_y][txt.cursor_location_x],len - txt.cursor_location_x + 1);  // move location x+1 and x-1 to accomodate deletion 
                    txt.cursor_location_x--; // decrement location on backspace
                    
                } else if (event.key.keysym.sym == SDLK_BACKSPACE && txt.cursor_location_x == 0) {
                    if (txt.cursor_location_y > 0) {                //backspace behavior for going up reaching the 0th coloumn

                        txt.cursor_location_y--;
                        txt.cursor_location_x = strlen(txt.lines[txt.cursor_location_y]);
                      
                    }
                } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    // Clamp cursor_location_x to the end of the line
                    if (txt.cursor_location_x > strlen(txt.lines[txt.cursor_location_y])) {

                        txt.cursor_location_x = strlen(txt.lines[txt.cursor_location_y]);

                    }
                    if (txt.cursor_location_y < MAX_LINES - 1 && txt.cursor_location_y < txt.MAX_VISIBLE_LINES) { // Shift all lines below down by one to make space
                        for (int i = MAX_LINES - 1; i > txt.cursor_location_y + 1; --i) {

                            strncpy(txt.lines[i], txt.lines[i - 1], MAX_TEXT_LEN);
                            txt.lines[i][MAX_TEXT_LEN - 1] = '\0';

                        }
                        // Move text after cursor to new line
                        size_t len = strlen(txt.lines[txt.cursor_location_y]); // length of txt at line[y]
                        size_t tail_len = len - txt.cursor_location_x; // length of line[y] with current location in that line

                        if (tail_len > 0) {

                            strncpy(txt.lines[txt.cursor_location_y + 1], txt.lines[txt.cursor_location_y] + txt.cursor_location_x, MAX_TEXT_LEN - 1);
                            txt.lines[txt.cursor_location_y + 1][MAX_TEXT_LEN - 1] = '\0';
                            txt.lines[txt.cursor_location_y][txt.cursor_location_x] = '\0';

                        } else {

                            txt.lines[txt.cursor_location_y + 1][0] = '\0';

                        }

                        txt.cursor_location_y++;
                        txt.cursor_location_x = 0;
                    } 
                    // arrow key movement :
                } else if (event.key.keysym.sym == SDLK_UP) {
                    if (txt.cursor_location_y > 0) {
                        txt.cursor_location_y--;       //if more rows than the first then just move up
                        if (txt.cursor_location_x > strlen(txt.lines[txt.cursor_location_y])) {
                             txt.cursor_location_x = strlen(txt.lines[txt.cursor_location_y]); 
                        }
                        
                    }
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    if (txt.cursor_location_y < MAX_LINES - 1 && txt.cursor_location_y < txt.MAX_VISIBLE_LINES) {
                        txt.cursor_location_y++;                    // if less rows than the max move down
                        if (txt.cursor_location_x > strlen(txt.lines[txt.cursor_location_y]))
                            txt.cursor_location_x = strlen(txt.lines[txt.cursor_location_y]);
                    }
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    if (txt.cursor_location_x < strlen(txt.lines[txt.cursor_location_y])) {
                        txt.cursor_location_x++;
                    }
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                    if (txt.cursor_location_x > 0) {
                        txt.cursor_location_x--;
                    }
                }
                
            }else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {

                    set_cursor_from_mouse(event.button.x, event.button.y,&txt);

            }else if (event.type == SDL_TEXTINPUT) {
                size_t curr_len = strlen(txt.lines[txt.cursor_location_y]);
                size_t input_len = strlen(event.text.text);
                size_t max_copy = MAX_TEXT_LEN - 1 - curr_len;
                if (input_len > max_copy) input_len = max_copy;

                // Prepare temp with insertion
                char temp[MAX_TEXT_LEN];
                strncpy(temp, txt.lines[txt.cursor_location_y], txt.cursor_location_x);
                temp[txt.cursor_location_x] = '\0';
                strncat(temp, event.text.text, input_len);
                strncat(temp, txt.lines[txt.cursor_location_y] + txt.cursor_location_x, MAX_TEXT_LEN - strlen(temp) - 1);

                
                TTF_SizeText(txt.font, temp, &txt.text_w, &txt.text_h);
                if (txt.text_w < win.window_width - 40 && curr_len + input_len < MAX_TEXT_LEN - 1) {
                    // Insert new text at cursor_location_x
                    memmove(&txt.lines[txt.cursor_location_y][txt.cursor_location_x + input_len],
                            &txt.lines[txt.cursor_location_y][txt.cursor_location_x],
                            curr_len - txt.cursor_location_x + 1);
                    memcpy(&txt.lines[txt.cursor_location_y][txt.cursor_location_x],
                           event.text.text, input_len);
                    txt.cursor_location_x += input_len;
                }
            } 
        }

        render_all(&win,&txt);
        
        
    }
    
    // exit and destroy when loop ends
    SDL_StopTextInput();

    quit_all(&win, &txt);

    return 0;
}

