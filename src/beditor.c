#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "tinyfiledialogs.h"
// done XD
#define MAX_TEXT_LEN 128
#define MAX_LINES 64
#define WINDOW_WIDTH_INITIAL 640
#define WINDOW_HEIGHT_INITIAL 480

// quits all SDL features if they are created and exits
int quit_all(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font) {
    if (renderer){
        SDL_DestroyRenderer(renderer);
    }
    if (window){
        SDL_DestroyWindow(window);
    }
    if (font){
        TTF_CloseFont(font);
    }
    TTF_Quit();
    SDL_Quit();
    return -1;
}


// setup SDL code
int setup_WIN_REN_TTF(SDL_Window **window, SDL_Renderer **renderer, TTF_Font **font) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0 || TTF_Init() != 0) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0){
            printf("SDL_Init Error: %s\n", SDL_GetError());
        } 
        if (TTF_Init() != 0){
            printf("TTF_Init Error: %s\n", TTF_GetError());
        }     
        return quit_all(NULL, NULL, NULL);
    } 

    *window = SDL_CreateWindow("Beditor", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH_INITIAL, WINDOW_HEIGHT_INITIAL, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (*window == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return quit_all(*window, NULL, NULL);
    }

    *renderer = SDL_CreateRenderer(*window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (*renderer == NULL) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        return quit_all(*window, *renderer,NULL);
    }

    *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 32);
    if (!*font) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        return quit_all(*window, *renderer,*font);
    }

    return 0;    
}


//mouse input function which calculates location in file
void set_cursor_from_mouse( int mouse_x, int mouse_y,char lines[MAX_LINES][MAX_TEXT_LEN],
int *cursor_location_y, int *cursor_location_x,int line_height, TTF_Font *font
) {
    // Calculate which line was clicked
    int clicked_line = (mouse_y - 20) / (line_height > 0 ? line_height : 32);
    if (clicked_line < 0){
        clicked_line = 0;
    }
    if (clicked_line >= MAX_LINES){
        clicked_line = MAX_LINES - 1;
        
    } 
    *cursor_location_y = clicked_line;
    // Find the character position in the line
    int x = 25;
    *cursor_location_x = 0;
    int w = 0;
    size_t len = strlen(lines[*cursor_location_y]);
    for (size_t i = 0; i <= len; ++i) {
        char temp[MAX_TEXT_LEN];
        strncpy(temp, lines[*cursor_location_y], i);
        temp[i] = '\0';
        TTF_SizeText(font, temp, &w, NULL);
        if (x + w > mouse_x) {
            *cursor_location_x = i;
            break;
        }
        *cursor_location_x = i;
    }
}


//render function, renders all features
void render_all(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font,char lines[MAX_LINES][MAX_TEXT_LEN],
int line_height, int window_height, SDL_Color color, int cursor_location_y,int cursor_location_x, int y){
    // Render background
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        // Render text lines
        
        for (int i = 0; i < MAX_LINES; ++i) { 
            if (strlen(lines[i]) > 0) {
                SDL_Surface *surf = TTF_RenderText_Solid(font, lines[i], color);
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect dst = {20, y, surf->w, surf->h};
                if (i == 0) line_height = surf->h;
                if (line_height == 0) line_height = 32;
                if (y + surf->h > window_height - 20) {
                    SDL_FreeSurface(surf);
                    SDL_DestroyTexture(tex);
                    break; 
                }
                SDL_RenderCopy(renderer, tex, NULL, &dst);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);
                y += line_height;
            } else {
                y += (line_height > 0 ? line_height : 32);
            }
        }

        // Draw blinking cursor at the correct position
        int cursor_x = 20, cursor_y = 20 + cursor_location_y * line_height;
        if (cursor_location_x > 0) {
            char cursor_prefix[MAX_TEXT_LEN];
            strncpy(cursor_prefix, lines[cursor_location_y], cursor_location_x);
            cursor_prefix[cursor_location_x] = '\0';
            int w = 0, h = 0;
            TTF_SizeText(font, cursor_prefix, &w, &h);
            cursor_x += w;
        }

        Uint32 ticks = SDL_GetTicks();
        if ((ticks / 500) % 2 == 0) {
            SDL_Rect cursor_rect = {cursor_x, cursor_y, 2, line_height > 0 ? line_height : 32};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black cursor
            SDL_RenderFillRect(renderer, &cursor_rect);
        }

        SDL_RenderPresent(renderer);
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
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    TTF_Font *font = NULL;
    char lines[MAX_LINES][MAX_TEXT_LEN] = {""};
    int cursor_location_y = 0;
    int cursor_location_x = 0;
    SDL_StartTextInput();
    SDL_Event event;
    int running = 1;
    int window_width = WINDOW_WIDTH_INITIAL;
    int window_height = WINDOW_HEIGHT_INITIAL;
    int line_height = 0;
    SDL_Color color = {0, 0, 0, 255};
    int current_render_y = 0;
    int MAX_VISIBLE_LINES = 0;
    int text_w = 0, text_h = 0;
    
    setup_WIN_REN_TTF(&window, &renderer, &font); // call setup 

    while (running) {
        MAX_VISIBLE_LINES = (window_height - 95) / (line_height > 0 ? line_height : 32); // calcultes visible lines
        current_render_y = 20;
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;  // stop program if you exit via SDL_quit
            } else if (event.type == SDL_WINDOWEVENT) {   //resizing code
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    window_width = event.window.data1;
                    window_height = event.window.data2;
                }
            } else if (event.type == SDL_KEYDOWN) {  // if button is pressed
                if ((event.key.keysym.sym == SDLK_s) && (event.key.keysym.mod & KMOD_CTRL)) {   // for saving with tinyfiledialog
                    const char *filename = tinyfd_saveFileDialog("Save As", "output.txt", 0, NULL, NULL);
                if (filename) {
                save_to_file(filename, lines);  // calls function above
                    }
                }
                else if (event.key.keysym.sym == SDLK_BACKSPACE && cursor_location_x > 0) {
                    size_t len = strlen(lines[cursor_location_y]);                // backspace behavior for more than 1 word
                    memmove(&lines[cursor_location_y][cursor_location_x - 1],&lines[cursor_location_y][cursor_location_x],len - cursor_location_x + 1);  // move location x+1 and x-1 to accomodate deletion 
                    cursor_location_x--; // decrement location on backspace
                } else if (event.key.keysym.sym == SDLK_BACKSPACE && cursor_location_x == 0) {
                    if (cursor_location_y > 0) {                //backspace behavior for going up reaching the 0th coloumn
                        cursor_location_y--;
                        cursor_location_x = strlen(lines[cursor_location_y]);
                      
                    }
                } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    // Clamp cursor_location_x to the end of the line
                    if (cursor_location_x > strlen(lines[cursor_location_y])) {
                        cursor_location_x = strlen(lines[cursor_location_y]);
                    }
                    if (cursor_location_y < MAX_LINES - 1 && cursor_location_y < MAX_VISIBLE_LINES) {
                        // Shift all lines below down by one to make space
                        for (int i = MAX_LINES - 1; i > cursor_location_y + 1; --i) {
                            strncpy(lines[i], lines[i - 1], MAX_TEXT_LEN);
                            lines[i][MAX_TEXT_LEN - 1] = '\0';
                        }
                        // Move text after cursor to new line
                        size_t len = strlen(lines[cursor_location_y]); // length of txt at line[y]
                        size_t tail_len = len - cursor_location_x; // length of line[y] with current location in that line
                        if (tail_len > 0) {
                            strncpy(lines[cursor_location_y + 1], lines[cursor_location_y] + cursor_location_x, MAX_TEXT_LEN - 1);
                            lines[cursor_location_y + 1][MAX_TEXT_LEN - 1] = '\0';
                            lines[cursor_location_y][cursor_location_x] = '\0';
                        } else {
                            lines[cursor_location_y + 1][0] = '\0';
                        }
                        cursor_location_y++;
                        cursor_location_x = 0;
                    } 
                    // arrow key movement :
                } else if (event.key.keysym.sym == SDLK_UP) {
                    if (cursor_location_y > 0) {
                        cursor_location_y--;       //if more rows than the first then just move up
                        if (cursor_location_x > strlen(lines[cursor_location_y])) {
                             cursor_location_x = strlen(lines[cursor_location_y]); 
                        }
                        
                    }
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    if (cursor_location_y < MAX_LINES - 1 && cursor_location_y < MAX_VISIBLE_LINES) {
                        cursor_location_y++;                    // if less rows than the max move down
                        if (cursor_location_x > strlen(lines[cursor_location_y]))
                            cursor_location_x = strlen(lines[cursor_location_y]);
                    }
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    if (cursor_location_x < strlen(lines[cursor_location_y])) {
                        cursor_location_x++;
                    }
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                    if (cursor_location_x > 0) {
                        cursor_location_x--;
                    }
                }
                
            }else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    set_cursor_from_mouse(event.button.x, event.button.y,
                    lines, &cursor_location_y, &cursor_location_x,
                    line_height, font);
            }else if (event.type == SDL_TEXTINPUT) {
                size_t curr_len = strlen(lines[cursor_location_y]);
                size_t input_len = strlen(event.text.text);
                size_t max_copy = MAX_TEXT_LEN - 1 - curr_len;
                if (input_len > max_copy) input_len = max_copy;

                // Prepare temp with insertion
                char temp[MAX_TEXT_LEN];
                strncpy(temp, lines[cursor_location_y], cursor_location_x);
                temp[cursor_location_x] = '\0';
                strncat(temp, event.text.text, input_len);
                strncat(temp, lines[cursor_location_y] + cursor_location_x, MAX_TEXT_LEN - strlen(temp) - 1);

                
                TTF_SizeText(font, temp, &text_w, &text_h);
                if (text_w < window_width - 40 && curr_len + input_len < MAX_TEXT_LEN - 1) {
                    // Insert new text at cursor_location_x
                    memmove(&lines[cursor_location_y][cursor_location_x + input_len],
                            &lines[cursor_location_y][cursor_location_x],
                            curr_len - cursor_location_x + 1);
                    memcpy(&lines[cursor_location_y][cursor_location_x],
                           event.text.text, input_len);
                    cursor_location_x += input_len;
                }
            }
        }

        render_all(window,renderer,font,lines, line_height,  window_height, color, 
            cursor_location_y, cursor_location_x, current_render_y );
        
        
    }
    // exit and destroy when loop ends
    SDL_StopTextInput();
    quit_all(window,renderer,font);
    return 0;
}

