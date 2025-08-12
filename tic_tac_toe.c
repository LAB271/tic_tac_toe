// SPDX-License-Identifier: 0BSD
#include <stdio.h>
#include <stdlib.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 480
#define CELL_SIZE 160
#define BOARD_SIZE 3

typedef enum {
    CELL_EMPTY = 0,
    CELL_PLAYER = 1,  // X
    CELL_MACHINE = 2  // O
} CellState;

typedef enum {
    GAME_PLAYING,
    GAME_PLAYER_WIN,
    GAME_MACHINE_WIN,
    GAME_DRAW
} GameState;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    CellState board[BOARD_SIZE][BOARD_SIZE];
    GameState game_state;
    int selected_row;
    int selected_col;
    Uint64 start_time;
} AppState;

static int check_winner(AppState *app, CellState player) {
    // Check rows and columns
    for (int i = 0; i < BOARD_SIZE; i++) {
        if ((app->board[i][0] == player && app->board[i][1] == player && app->board[i][2] == player) ||
            (app->board[0][i] == player && app->board[1][i] == player && app->board[2][i] == player)) {
            return 1;
        }
    }
    // Check diagonals
    if ((app->board[0][0] == player && app->board[1][1] == player && app->board[2][2] == player) ||
        (app->board[0][2] == player && app->board[1][1] == player && app->board[2][0] == player)) {
        return 1;
    }
    return 0;
}

static int is_board_full(AppState *app) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (app->board[i][j] == CELL_EMPTY) {
                return 0;
            }
        }
    }
    return 1;
}

static void machine_move(AppState *app) {
    // Check if this is the first move (only player has moved)
    int total_moves = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (app->board[i][j] != CELL_EMPTY) {
                total_moves++;
            }
        }
    }
    
    // Add randomness to first move for variety
    if (total_moves == 1) {
        // 60% chance to take center if available, 40% chance to be random
        if (app->board[1][1] == CELL_EMPTY && (rand() % 100) < 60) {
            app->board[1][1] = CELL_MACHINE;
            return;
        }
        
        // Otherwise pick a random corner or center
        int good_positions[5][2] = {{1,1}, {0,0}, {0,2}, {2,0}, {2,2}};
        int available_positions[5][2];
        int available_count = 0;
        
        for (int i = 0; i < 5; i++) {
            int row = good_positions[i][0];
            int col = good_positions[i][1];
            if (app->board[row][col] == CELL_EMPTY) {
                available_positions[available_count][0] = row;
                available_positions[available_count][1] = col;
                available_count++;
            }
        }
        
        if (available_count > 0) {
            int random_choice = rand() % available_count;
            app->board[available_positions[random_choice][0]][available_positions[random_choice][1]] = CELL_MACHINE;
            return;
        }
    }
    
    // Improved AI strategy for subsequent moves:
    // 1. Try to win
    // 2. Block player from winning
    // 3. Take center if available
    // 4. Take corner if available
    // 5. Take any remaining spot
    
    // Check if machine can win
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (app->board[i][j] == CELL_EMPTY) {
                app->board[i][j] = CELL_MACHINE;
                if (check_winner(app, CELL_MACHINE)) {
                    return; // Win immediately
                }
                app->board[i][j] = CELL_EMPTY; // Undo test move
            }
        }
    }
    
    // Check if machine needs to block player
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (app->board[i][j] == CELL_EMPTY) {
                app->board[i][j] = CELL_PLAYER;
                if (check_winner(app, CELL_PLAYER)) {
                    app->board[i][j] = CELL_MACHINE; // Block the win
                    return;
                }
                app->board[i][j] = CELL_EMPTY; // Undo test move
            }
        }
    }
    
    // Take center if available
    if (app->board[1][1] == CELL_EMPTY) {
        app->board[1][1] = CELL_MACHINE;
        return;
    }
    
    // Take corners in preference order
    int corners[4][2] = {{0,0}, {0,2}, {2,0}, {2,2}};
    for (int i = 0; i < 4; i++) {
        int row = corners[i][0];
        int col = corners[i][1];
        if (app->board[row][col] == CELL_EMPTY) {
            app->board[row][col] = CELL_MACHINE;
            return;
        }
    }
    
    // Take any remaining spot
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (app->board[i][j] == CELL_EMPTY) {
                app->board[i][j] = CELL_MACHINE;
                return;
            }
        }
    }
}

static void make_move(AppState *app, int row, int col) {
    if (app->game_state != GAME_PLAYING || app->board[row][col] != CELL_EMPTY) {
        return;
    }
    
    // Player move
    app->board[row][col] = CELL_PLAYER;
    
    if (check_winner(app, CELL_PLAYER)) {
        app->game_state = GAME_PLAYER_WIN;
        return;
    }
    
    if (is_board_full(app)) {
        app->game_state = GAME_DRAW;
        return;
    }
    
    // Machine move
    machine_move(app);
    
    if (check_winner(app, CELL_MACHINE)) {
        app->game_state = GAME_MACHINE_WIN;
        return;
    }
    
    if (is_board_full(app)) {
        app->game_state = GAME_DRAW;
        return;
    }
}

static void draw_x(SDL_Renderer *renderer, int x, int y, int size) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE); // Red X
    int margin = size / 4;
    // Draw X with thick lines
    for (int i = 0; i < 5; i++) {
        SDL_RenderLine(renderer, x + margin + i, y + margin, x + size - margin + i, y + size - margin);
        SDL_RenderLine(renderer, x + size - margin + i, y + margin, x + margin + i, y + size - margin);
    }
}

// Simple but clean 5x7 bitmap font
static const unsigned char font_5x7[26][7] = {
    // A
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    // B
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    // C
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    // D
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C},
    // E
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    // F
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    // G
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},
    // H
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    // I
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    // J
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    // K
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    // L
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    // M
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    // N
    {0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11},
    // O
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    // P
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    // Q
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    // R
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    // S
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
    // T
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    // U
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    // V
    {0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04},
    // W
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11},
    // X
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    // Y
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    // Z
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}
};

static void draw_char(SDL_Renderer *renderer, char c, int x, int y, int scale, int r, int g, int b) {
    if (c < 'A' || c > 'Z') return;
    
    SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
    
    const unsigned char *char_data = font_5x7[c - 'A'];
    
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (char_data[row] & (1 << (4 - col))) {
                // Draw a scaled pixel block
                SDL_FRect pixel = {
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale
                };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

static void draw_clean_text(SDL_Renderer *renderer, const char *text, int x, int y, int scale, int r, int g, int b) {
    int current_x = x;
    
    for (const char *c = text; *c; c++) {
        if (*c == ' ') {
            current_x += 3 * scale; // Space width
        } else if (*c >= 'A' && *c <= 'Z') {
            draw_char(renderer, *c, current_x, y, scale, r, g, b);
            current_x += 6 * scale; // Character width + spacing
        } else if (*c >= 'a' && *c <= 'z') {
            // Convert lowercase to uppercase
            draw_char(renderer, *c - 32, current_x, y, scale, r, g, b);
            current_x += 6 * scale;
        }
    }
}

static void draw_text_with_shadow(SDL_Renderer *renderer, const char *text, int x, int y, int scale, int r, int g, int b) {
    // Draw shadow first (smaller offset to not cover main text)
    draw_clean_text(renderer, text, x + 2, y + 2, scale, 0, 0, 0);
    // Draw main text
    draw_clean_text(renderer, text, x, y, scale, r, g, b);
}

static void draw_big_text(SDL_Renderer *renderer, const char *text, int x, int y, int r, int g, int b, int scale) {
    // Just use the clean text system with shadow for big text
    draw_text_with_shadow(renderer, text, x, y, scale, r, g, b);
}

static void draw_glow_effect(SDL_Renderer *renderer, int x, int y, int width, int height, int r, int g, int b) {
    // Create a subtle glow effect around text
    for (int glow = 3; glow > 0; glow--) {
        int alpha = 30 * glow;
        SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
        
        SDL_FRect glow_rect = {
            x - glow * 2,
            y - glow * 2,
            width + glow * 4,
            height + glow * 4
        };
        SDL_RenderFillRect(renderer, &glow_rect);
    }
}

static void draw_animated_background(SDL_Renderer *renderer, GameState state, Uint64 time) {
    // Animated background based on game outcome (using integer math only)
    int phase = (time / 200) % 360; // Slow animation cycle
    
    if (state == GAME_PLAYER_WIN) {
        // Green celebration particles
        for (int i = 0; i < 50; i++) {
            int x = (i * 37 + phase * 3) % WINDOW_WIDTH;
            int y = (i * 23 + phase * 2) % WINDOW_HEIGHT;
            // Simple brightness variation without floating point
            int brightness = 100 + ((phase + i * 10) % 100) / 2;
            SDL_SetRenderDrawColor(renderer, 0, brightness, 0, 150);
            
            for (int px = 0; px < 3; px++) {
                for (int py = 0; py < 3; py++) {
                    SDL_RenderPoint(renderer, x + px, y + py);
                }
            }
        }
    } else if (state == GAME_MACHINE_WIN) {
        // Red danger effect
        for (int i = 0; i < 30; i++) {
            int x = (i * 31 + phase * 2) % WINDOW_WIDTH;
            int y = (i * 41 + phase) % WINDOW_HEIGHT;
            // Simple brightness variation without floating point
            int brightness = 80 + ((phase + i * 15) % 80) / 2;
            SDL_SetRenderDrawColor(renderer, brightness, 0, 0, 120);
            
            for (int px = 0; px < 4; px++) {
                for (int py = 0; py < 4; py++) {
                    SDL_RenderPoint(renderer, x + px, y + py);
                }
            }
        }
    } else if (state == GAME_DRAW) {
        // Yellow/orange neutral pattern
        for (int i = 0; i < 40; i++) {
            int x = (i * 29 + phase) % WINDOW_WIDTH;
            int y = (i * 17 + phase * 2) % WINDOW_HEIGHT;
            // Simple brightness variation without floating point
            int brightness = 120 + ((phase + i * 8) % 60) / 2;
            SDL_SetRenderDrawColor(renderer, brightness, brightness, 0, 100);
            
            SDL_RenderPoint(renderer, x, y);
            SDL_RenderPoint(renderer, x + 1, y);
            SDL_RenderPoint(renderer, x, y + 1);
            SDL_RenderPoint(renderer, x + 1, y + 1);
        }
    }
}

static void draw_text(SDL_Renderer *renderer, const char *text, int x, int y, int r, int g, int b) {
    // Use clean text system for regular text too
    draw_clean_text(renderer, text, x, y, 2, r, g, b);
}

static void draw_o(SDL_Renderer *renderer, int x, int y, int size) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE); // Blue O
    int center_x = x + size / 2;
    int center_y = y + size / 2;
    int radius = size / 3;
    
    // Draw circle (approximated with points)
    for (int angle = 0; angle < 360; angle += 2) {
        float rad = angle * 3.14159f / 180.0f;
        int px = center_x + (int)(radius * SDL_cosf(rad));
        int py = center_y + (int)(radius * SDL_sinf(rad));
        SDL_RenderPoint(renderer, px, py);
        SDL_RenderPoint(renderer, px + 1, py);
        SDL_RenderPoint(renderer, px, py + 1);
        SDL_RenderPoint(renderer, px + 1, py + 1);
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Seed random number generator for varied AI behavior
    srand((unsigned int)SDL_GetTicks());

    AppState *app = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!app) {
        return SDL_APP_FAILURE;
    }

    *appstate = app;

    app->window = SDL_CreateWindow("Tic Tac Toe", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!app->window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    app->renderer = SDL_CreateRenderer(app->window, NULL);
    if (!app->renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Initialize game state
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            app->board[i][j] = CELL_EMPTY;
        }
    }
    app->game_state = GAME_PLAYING;
    app->selected_row = 1;
    app->selected_col = 1;
    app->start_time = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState *app = (AppState *)appstate;
    
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
            
        case SDL_EVENT_KEY_DOWN:
            // Handle ESC key to quit
            if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
                return SDL_APP_SUCCESS;
            }
            
            if (app->game_state == GAME_PLAYING) {
                switch (event->key.scancode) {
                    case SDL_SCANCODE_UP:
                        app->selected_row = (app->selected_row - 1 + BOARD_SIZE) % BOARD_SIZE;
                        break;
                    case SDL_SCANCODE_DOWN:
                        app->selected_row = (app->selected_row + 1) % BOARD_SIZE;
                        break;
                    case SDL_SCANCODE_LEFT:
                        app->selected_col = (app->selected_col - 1 + BOARD_SIZE) % BOARD_SIZE;
                        break;
                    case SDL_SCANCODE_RIGHT:
                        app->selected_col = (app->selected_col + 1) % BOARD_SIZE;
                        break;
                    case SDL_SCANCODE_SPACE:
                    case SDL_SCANCODE_RETURN:
                        make_move(app, app->selected_row, app->selected_col);
                        break;
                    case SDL_SCANCODE_R:
                        // Reset game
                        for (int i = 0; i < BOARD_SIZE; i++) {
                            for (int j = 0; j < BOARD_SIZE; j++) {
                                app->board[i][j] = CELL_EMPTY;
                            }
                        }
                        app->game_state = GAME_PLAYING;
                        break;
                }
            } else {
                // Game over, allow reset
                if (event->key.scancode == SDL_SCANCODE_R) {
                    for (int i = 0; i < BOARD_SIZE; i++) {
                        for (int j = 0; j < BOARD_SIZE; j++) {
                            app->board[i][j] = CELL_EMPTY;
                        }
                    }
                    app->game_state = GAME_PLAYING;
                }
            }
            break;
            
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.button == SDL_BUTTON_LEFT && app->game_state == GAME_PLAYING) {
                int row = event->button.y / CELL_SIZE;
                int col = event->button.x / CELL_SIZE;
                if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
                    make_move(app, row, col);
                }
            }
            break;
    }
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState *app = (AppState *)appstate;
    Uint64 current_time = SDL_GetTicks();
    
    // Clear screen
    SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(app->renderer);
    
    // Draw grid
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    for (int i = 1; i < BOARD_SIZE; i++) {
        // Vertical lines
        SDL_RenderLine(app->renderer, i * CELL_SIZE, 0, i * CELL_SIZE, WINDOW_HEIGHT);
        // Horizontal lines
        SDL_RenderLine(app->renderer, 0, i * CELL_SIZE, WINDOW_WIDTH, i * CELL_SIZE);
    }
    
    // Draw selection highlight
    if (app->game_state == GAME_PLAYING) {
        SDL_SetRenderDrawColor(app->renderer, 255, 255, 0, 100);
        SDL_FRect highlight = {
            app->selected_col * CELL_SIZE + 2,
            app->selected_row * CELL_SIZE + 2,
            CELL_SIZE - 4,
            CELL_SIZE - 4
        };
        SDL_RenderFillRect(app->renderer, &highlight);
    }
    
    // Draw X's and O's
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            int x = col * CELL_SIZE;
            int y = row * CELL_SIZE;
            
            if (app->board[row][col] == CELL_PLAYER) {
                draw_x(app->renderer, x, y, CELL_SIZE);
            } else if (app->board[row][col] == CELL_MACHINE) {
                draw_o(app->renderer, x, y, CELL_SIZE);
            }
        }
    }
    
    // Draw spectacular game over screen
    if (app->game_state != GAME_PLAYING) {
        // Animated background effects
        draw_animated_background(app->renderer, app->game_state, current_time - app->start_time);
        
        // Dramatic semi-transparent overlay with gradient effect
        for (int i = 0; i < WINDOW_HEIGHT; i++) {
            int alpha = 180 + (i * 40 / WINDOW_HEIGHT); // Darker gradient from top to bottom
            SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, alpha);
            SDL_FRect line = {0, i, WINDOW_WIDTH, 1};
            SDL_RenderFillRect(app->renderer, &line);
        }
        
        // Main outcome text with glow and large scale
        if (app->game_state == GAME_PLAYER_WIN) {
            // Victory celebration!
            draw_glow_effect(app->renderer, 60, 120, 360, 80, 0, 255, 0);
            // Draw outline first
            draw_clean_text(app->renderer, "YOU WIN", 79, 139, 4, 255, 255, 255); // White outline
            draw_clean_text(app->renderer, "YOU WIN", 81, 139, 4, 255, 255, 255);
            draw_clean_text(app->renderer, "YOU WIN", 80, 138, 4, 255, 255, 255);
            draw_clean_text(app->renderer, "YOU WIN", 80, 142, 4, 255, 255, 255);
            // Main text
            draw_clean_text(app->renderer, "YOU WIN", 80, 140, 4, 0, 255, 0); // Bright green text
            
            // Subtitle
            draw_clean_text(app->renderer, "VICTORY", 140, 200, 2, 255, 255, 255); // White subtitle
            
        } else if (app->game_state == GAME_MACHINE_WIN) {
            // Dramatic defeat
            draw_glow_effect(app->renderer, 50, 120, 380, 80, 255, 0, 0);
            // Draw outline first
            draw_clean_text(app->renderer, "YOU LOSE", 69, 139, 4, 255, 255, 255); // White outline
            draw_clean_text(app->renderer, "YOU LOSE", 71, 139, 4, 255, 255, 255);
            draw_clean_text(app->renderer, "YOU LOSE", 70, 138, 4, 255, 255, 255);
            draw_clean_text(app->renderer, "YOU LOSE", 70, 142, 4, 255, 255, 255);
            // Main text
            draw_clean_text(app->renderer, "YOU LOSE", 70, 140, 4, 255, 0, 0); // Bright red text
            
            // Subtitle
            draw_clean_text(app->renderer, "DEFEAT", 160, 200, 2, 255, 255, 255); // White subtitle
            
        } else if (app->game_state == GAME_DRAW) {
            // Neutral but still impressive
            draw_glow_effect(app->renderer, 140, 120, 200, 80, 255, 255, 0);
            // Draw outline first
            draw_clean_text(app->renderer, "DRAW", 159, 139, 4, 255, 255, 255); // White outline
            draw_clean_text(app->renderer, "DRAW", 161, 139, 4, 255, 255, 255);
            draw_clean_text(app->renderer, "DRAW", 160, 138, 4, 255, 255, 255);
            draw_clean_text(app->renderer, "DRAW", 160, 142, 4, 255, 255, 255);
            // Main text
            draw_clean_text(app->renderer, "DRAW", 160, 140, 4, 255, 255, 0); // Bright yellow text
            
            // Subtitle
            draw_clean_text(app->renderer, "TIE GAME", 140, 200, 2, 255, 255, 255); // White subtitle
        }
        
        // Stylish restart instruction with pulsing effect (integer math only)
        int pulse_cycle = (current_time / 150) % 200; // 0-199 cycle
        int pulse;
        if (pulse_cycle < 100) {
            pulse = 150 + pulse_cycle / 2; // 150 to 200
        } else {
            pulse = 200 - (pulse_cycle - 100) / 2; // 200 to 150
        }
        draw_glow_effect(app->renderer, 30, 320, 420, 40, pulse, pulse, 255);
        draw_big_text(app->renderer, "PRESS R TO RESTART", 50, 330, 255, 255, 255, 2); // Always white text
        
        // Add some decorative border elements
        SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 200);
        for (int i = 0; i < 10; i++) {
            // Top border decoration
            SDL_RenderLine(app->renderer, i * 50, 10, i * 50 + 30, 10);
            SDL_RenderLine(app->renderer, i * 50, 11, i * 50 + 30, 11);
            
            // Bottom border decoration  
            SDL_RenderLine(app->renderer, i * 50, WINDOW_HEIGHT - 10, i * 50 + 30, WINDOW_HEIGHT - 10);
            SDL_RenderLine(app->renderer, i * 50, WINDOW_HEIGHT - 11, i * 50 + 30, WINDOW_HEIGHT - 11);
        }
    }
    
    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    AppState *app = (AppState *)appstate;
    if (app) {
        if (app->renderer) {
            SDL_DestroyRenderer(app->renderer);
        }
        if (app->window) {
            SDL_DestroyWindow(app->window);
        }
        SDL_free(app);
    }
    SDL_Quit();
}
