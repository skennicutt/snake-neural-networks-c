#include <SDL3/SDL.h>
#include <cassert>
#include <iostream>

#define WINDOW_X 0
#define WINDOW_Y 0
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define GRID_SIZE 20
#define GRID_DIM 800

#define GRID_START_X ((WINDOW_WIDTH / 2) - (GRID_DIM / 2))
#define GRID_START_Y ((WINDOW_HEIGHT / 2) - (GRID_DIM / 2))

int uniform_random(int lower, int upper) {
  if (lower >= upper)
    return lower; // Handle invalid range

  int range = upper - lower + 1;
  int limit =
      (RAND_MAX / range) * range; // Largest multiple of 'range' <= RAND_MAX

  int r;
  do {
    r = rand();
  } while (r >= limit);

  return lower + (r / (RAND_MAX / range));
}

// Core structures
typedef struct game_state {
  bool quit;
  bool reset;
  bool game_over;
} GameState;

// Snake Structures
enum snake_dir {
  SNAKE_UP,
  SNAKE_DOWN,
  SNAKE_LEFT,
  SNAKE_RIGHT,
};

struct snake {
  int x;
  int y;
  snake_dir dir;

  struct snake *next;
};

typedef struct snake Snake;
Snake *head;
Snake *tail;

void init_snake() {
  const int starting_cell_margin = 6;
  Snake *new_snake = (Snake *)malloc(sizeof(Snake));
  new_snake->x =
      uniform_random(starting_cell_margin, GRID_SIZE - starting_cell_margin);
  new_snake->y =
      uniform_random(starting_cell_margin, GRID_SIZE - starting_cell_margin);
  new_snake->dir = SNAKE_UP;
  new_snake->next = NULL;

  head = new_snake;
  tail = new_snake;
}

void reset_snake() {
  assert(head);
  Snake *current = head;
  Snake *next;

  while (current != NULL) {
    next = current->next;
    free(current);
    current = next;
  }

  init_snake();
}

void increase_snake() {
  Snake *new_snake = (Snake *)malloc(sizeof(Snake));

  new_snake->x = tail->x;
  new_snake->y = tail->y;

  // Offset based on tail direction
  switch (new_snake->dir) {
  case SNAKE_UP:
    new_snake->y++;
    break;
  case SNAKE_DOWN:
    new_snake->y--;
    break;
  case SNAKE_LEFT:
    new_snake->x++;
    break;
  case SNAKE_RIGHT:
    new_snake->x--;
    break;
  }

  new_snake->dir = tail->dir;

  new_snake->next = NULL;
  tail->next = new_snake;

  tail = new_snake;
}

void move_snake() {
  int prev_x = head->x;
  int prev_y = head->y;

  switch (head->dir) {
  case SNAKE_UP:
    head->y--;
    break;
  case SNAKE_DOWN:
    head->y++;
    break;
  case SNAKE_LEFT:
    head->x--;
    break;
  case SNAKE_RIGHT:
    head->x++;
    break;
  default:
    break;
  }

  Snake *track = head;

  if (track->next != NULL) {
    track = track->next;
  }

  while (track != NULL) {
    int swap_x = track->x;
    int swap_y = track->y;

    track->x = prev_x;
    track->y = prev_y;

    track = track->next;

    prev_x = swap_x;
    prev_y = swap_y;
  }
}

// Apple structures
struct apple {
  int x;
  int y;
};

typedef struct apple Apple;
Apple *current_apple;

void init_apple() { current_apple = (Apple *)malloc(sizeof(Apple)); }

void move_apple() {
  current_apple->x = uniform_random(0, GRID_SIZE - 1);
  current_apple->y = uniform_random(0, GRID_SIZE - 1);
}

// Collision detection
bool is_snake_colliding_with_apple() {
  return current_apple->x == head->x && current_apple->y == head->y;
}

bool is_snake_colliding_with_self() {
  assert(head);

  Snake *tracking = head->next;
  bool is_colliding = false;

  while (tracking != NULL) {
    if (tracking->x == head->x && tracking->y == head->y) {
      is_colliding = true;
      break;
    }
    tracking = tracking->next;
  }

  return is_colliding;
}

bool is_snake_out_of_bounds() {
  return head->x < 0 || head->x >= GRID_SIZE - 1 || head->y < 0 ||
         head->y >= GRID_SIZE - 1;
}

bool is_game_over() {
  return is_snake_colliding_with_self() || is_snake_out_of_bounds();
}

// Input handling
void handle_user_key_down(game_state *state, SDL_Event e) {
  switch (e.key.key) {
  case SDLK_ESCAPE:
    state->quit = true;
    break;
  case SDLK_UP:
  case SDLK_W:
    // Prevent snake from trying to reverse direction into itself.
    if (head->dir == SNAKE_DOWN) {
      break;
    }
    head->dir = SNAKE_UP;
    break;
  case SDLK_DOWN:
  case SDLK_S:
    if (head->dir == SNAKE_UP) {
      break;
    }
    head->dir = SNAKE_DOWN;
    break;
  case SDLK_LEFT:
  case SDLK_A:
    if (head->dir == SNAKE_RIGHT) {
      break;
    }
    head->dir = SNAKE_LEFT;
    break;
  case SDLK_RIGHT:
  case SDLK_D:
    if (head->dir == SNAKE_LEFT) {
      break;
    }
    head->dir = SNAKE_RIGHT;
    break;
  case SDLK_R:
    state->reset = true;
    break;
  default:
    break;
  }
}

void handle_user_key_input(game_state *state, SDL_Event e) {
  switch (e.type) {
  case SDL_EVENT_QUIT:
    state->quit = true;
    break;
  case SDL_EVENT_KEY_UP:
    break;
  case SDL_EVENT_KEY_DOWN:
    handle_user_key_down(state, e);
    break;
  default:
    break;
  }
}

// Input Handling
void handle_user_input(game_state *state) {
  SDL_Event e;

  while (SDL_PollEvent(&e)) {
    handle_user_key_input(state, e);
  }
}

// Render Loop
void render_snake(SDL_Renderer *renderer, int x, int y) {
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  int snake_cell_size = GRID_DIM / GRID_SIZE;
  SDL_FRect snake_cell_rect;

  snake_cell_rect.h = (float)snake_cell_size;
  snake_cell_rect.w = (float)snake_cell_size;

  Snake *current_cell = head;

  while (current_cell != NULL) {
    snake_cell_rect.x = (float)(x + current_cell->x * snake_cell_size);
    snake_cell_rect.y = (float)(y + current_cell->y * snake_cell_size);
    SDL_RenderFillRect(renderer, &snake_cell_rect);

    current_cell = current_cell->next;
  }
}

void render_apple(SDL_Renderer *renderer, int x, int y) {
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  int apple_size = GRID_DIM / GRID_SIZE;
  SDL_FRect apple_rect;

  apple_rect.h = (float)apple_size;
  apple_rect.w = (float)apple_size;

  apple_rect.x = (float)(x + current_apple->x * apple_size);
  apple_rect.y = (float)(y + current_apple->y * apple_size);

  SDL_RenderFillRect(renderer, &apple_rect);
}

void render_grid(SDL_Renderer *renderer, int x, int y) {
  if (!renderer) {
    fprintf(stderr,
            "ERROR: SDL_CreateRenderer is required to render the grid.");
    return;
  }

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  int cell_size = GRID_DIM / GRID_SIZE;
  SDL_FRect cell_rect;

  cell_rect.h = (float)cell_size;
  cell_rect.w = (float)cell_size;

  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      cell_rect.x = (float)(x + (i * cell_size));
      cell_rect.y = (float)(y + (j * cell_size));
      SDL_RenderRect(renderer, &cell_rect);
    }
  }
}

SDL_Window *initialize_window() {
  SDL_Window *window = SDL_CreateWindow("Snake", WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_BORDERLESS);

  if (!window) {
    fprintf(stderr, "ERROR: SDL_CreateWindow");
  }
  return window;
}

SDL_Renderer *initialize_renderer(SDL_Window *window) {
  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

  if (!renderer) {
    fprintf(stderr, "ERROR: SDL_CreateRenderer");
  }
  return renderer;
}

game_state initialize_game_state() {
  return game_state{
      false,
      false,
      false,
  };
}

void execute_main_gameplay_loop(SDL_Renderer *renderer, game_state &state) {
  if (state.reset) {
    reset_snake();
    increase_snake();
    increase_snake();
    increase_snake();

    move_apple();

    state.reset = false;
  }

  if (is_snake_colliding_with_apple()) {
    increase_snake();
    move_apple();
  }

  if (is_game_over()) {
    state.game_over = true;
    state.quit = true;
    return;
  }

  handle_user_input(&state);
  move_snake();

  SDL_RenderClear(renderer);
  // RENDER LOOP START

  render_grid(renderer, GRID_START_X, GRID_START_Y);
  render_snake(renderer, GRID_START_X, GRID_START_Y);
  render_apple(renderer, GRID_START_X, GRID_START_Y);
  // RENDER LOOP END
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderPresent(renderer);

  SDL_Delay(300); // This is a horrible way to delay, it interferes with input
                  // handling and causes delays
}

int main() {
  if (SDL_INIT_VIDEO < 0) {
    fprintf(stderr, "ERROR: SDL_INIT_VIDEO");
  }

  init_snake();
  increase_snake();
  increase_snake();
  increase_snake();

  init_apple();
  move_apple();

  SDL_Window *window = initialize_window();
  SDL_Renderer *renderer = initialize_renderer(window);

  // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Hello World", "This
  // is a test of the message box", window);

  game_state state = initialize_game_state();

  while (state.quit != true) {
    execute_main_gameplay_loop(renderer, state);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
