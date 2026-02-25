#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define WIDTH 48 //48
#define HEIGHT 27 //27

#define KEYS_BUFFER 10
#define GRAVITY -.00004

#define LIGTH_EMPTY_COLOR 47
#define LIGTH_FILL_COLOR 40
#define DARK_EMPTY_COLOR 40
#define DARK_FILL_COLOR 47

#define MAX_ITERATION 100000
#define MAX_ANIM_SPEED 500
#define MIN_ANIM_SPEED 100
#define MAX_SCORE 10000
#define HIGHT_SCORE 100
#define MAX_RANGE_CACTUS_SPAWN 50

typedef struct {
	float x, y;
} Vector2;

typedef struct {
	Vector2 position;
	Vector2 size;
	int anim_speed;
	char* texture;
	char* run_anim[2];
	char* death_texture;
	float vel_y;
	float jump_power;
	int is_gorund;
} Player;

typedef struct {
	Vector2 position;
	Vector2 size;
	char* texture;
} Cactus;

typedef struct {
	int game_over;
	int running;
	int iteration;
	char keys_buffer[KEYS_BUFFER];
	char map[WIDTH*HEIGHT];
	char* cactus_types_texture[5];
	Vector2 cactus_types_size[5];
	Player player;
	Cactus cactus;
	int score;
	int best_score;
	int theme;
} GameState;

int randint(int min, int max);

void init_terminal();
void restore_terminal();

Player CreatePlayer();
Cactus CreateCactus();

void AnimatePlayer(Player* player, int* iteration);
void PlayerGravity(Player* player);
void PlayerJump(Player* player);
int PlayerCollision(Player* player, Cactus* cactus);
void ChangeCactus(Cactus* c, char* texture, Vector2 size);

void RestartGame(GameState* state);
void KeyHandler(GameState* state);
void Update(GameState* state);
void Draw(char (*map)[WIDTH*HEIGHT], int* score, int* best_score, int* game_over, int* theme);

void GameLoop(GameState* state) {
	while (state->running) {
		KeyHandler(state);
		if (state->game_over) continue;
		Update(state);
		Draw(&state->map, &state->score, &state->best_score, &state->game_over, &state->theme);
		state->iteration++;
		if (state->iteration>=MAX_ITERATION) state->iteration=0;
	}
}

void main(int argc, char** argv) {
	init_terminal();
	GameState state;
	RestartGame(&state);
	if (argc>1) state.theme = atoi(argv[1]);
	GameLoop(&state);
	restore_terminal();
}

int randint(int min, int max) {
	return min+rand()%(max-min+1);
}

void init_terminal() {
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~(ICANON | ECHO);
	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	printf("\e[?25l");
	for (int i=0; i<HEIGHT+2; i++) printf("\n");
}

void restore_terminal() {
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag |= (ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	printf("\e[?25h\n");
}

Player CreatePlayer() {
	Player p = {
		.position=(Vector2){.x=0, .y=0},
		.size=(Vector2){.x=10, .y=10},
		.anim_speed=MIN_ANIM_SPEED,
		.texture = "0000001111000001011100000111110000011110100011100011011111001111111000011111100000111100000000010000",
		.death_texture = "0000001111000001001100000100110000011110100011100011011111001111111000011111100000111100000000010000",
		.run_anim = {
			"0000001111000001011100000111110000011110100011100011011111001111111000011111100000111100000000010000",
			"0000001111000001011100000111110000011110100011100011011111001111111000011111100000111100000001000000"
		},
		.vel_y = .0,
		.jump_power = .035,
		.is_gorund = 0
	};
	return p;
}

Cactus CreateCactus() {
	Cactus c = {
		.position=(Vector2){.x=WIDTH-10, .y=0},
		.size=(Vector2){.x=3, .y=4},
		.texture = "010111011010",
	};
	return c;
}

void ChangeCactus(Cactus* c, char* texture, Vector2 size) {
	c->texture = texture;
	c->position = (Vector2) {.x=randint(WIDTH, WIDTH+MAX_RANGE_CACTUS_SPAWN), .y=0};
	c->size = size;
}

void AnimatePlayer(Player* player, int* iteration) {
	player->texture = player->run_anim[(*iteration)%(MAX_ITERATION/player->anim_speed)/(MAX_ITERATION/2/player->anim_speed)];
}

void RestartGame(GameState* state) {
	*state = (GameState){
		.game_over = 0,
		.running = 1,
    .iteration = 0,
    .player = CreatePlayer(),
    .cactus = CreateCactus(),
    .cactus_types_texture = {
	    "010110111111010",
      "010011111111010",
      "010011111111110010",
      "010110111111011010",
      "010111011010"
		},
	  .cactus_types_size = {
  		(Vector2){.x=3, .y=5},
      (Vector2){.x=3, .y=5},
      (Vector2){.x=3, .y=6},
      (Vector2){.x=3, .y=6},
      (Vector2){.x=3, .y=4},
    },
    .score=0,
    .best_score=HIGHT_SCORE,
		.theme=0
  };
}

void KeyHandler(GameState* state) {
	ssize_t bytes_read = read(STDIN_FILENO, state->keys_buffer, sizeof(state->keys_buffer) - 1);
	if (bytes_read<=0) return;
	(state->keys_buffer)[bytes_read] = '\0';
	if (bytes_read==1) {
		char key = (state->keys_buffer)[0];
		if (key=='q' || key==27) state->running = 0;
		if (key==' ') {
			if (state->game_over) RestartGame(state);
			else PlayerJump(&state->player);
		}
	}
}

void UpdateSprite(char (*map)[WIDTH*HEIGHT], Vector2* position, Vector2* size, const char* texture) {
	for (int i = 0; i < size->x * size->y; i++) {
		if (texture[i] == '1') {
			int tex_x = i%(int)size->x;
			int tex_y = i/(int)size->x;
	
			Vector2 draw_pos = {.x=(position->x+tex_x), .y=((HEIGHT - 1 - position->y - (size->y - 1))+tex_y)};
			if (draw_pos.x >= 0 && draw_pos.x < WIDTH && draw_pos.y >= 0 && draw_pos.y < HEIGHT)
				(*map)[(int)draw_pos.x + (int)draw_pos.y * WIDTH] = 1;
		}
	}
}

void PlayerGravity(Player* player) {
	player->vel_y+=GRAVITY;
	player->position.y += player->vel_y;
	if (player->position.y <= 0) {
		player->position.y=0;
		player->vel_y=0;
		player->is_gorund=1;
	}
}

void PlayerJump(Player* player) {
	if (player->is_gorund) {
		player->vel_y = player->jump_power;
		player->is_gorund=0;
	}
}

int PlayerCollision(Player* player, Cactus* cactus) {
	if (player->position.x + player->size.x < cactus->position.x ||
  		cactus->position.x + cactus->size.x < player->position.x ||
    	player->position.y + player->size.y < cactus->position.y ||
    	cactus->position.y + cactus->size.y < player->position.y) {
    return 0;
	}
    
  for (int ip = 0; ip < (int)player->size.x * (int)player->size.y; ip++) {
  	if (player->texture[ip] == '0') continue;
        
    int px = (int)player->position.x + (ip % (int)player->size.x);
    int py = (HEIGHT - 1 - (int)player->position.y - ((int)player->size.y - 1)) + (ip / (int)player->size.x);
        
    for (int ic = 0; ic < (int)cactus->size.x * (int)cactus->size.y; ic++) {
    	if (cactus->texture[ic] == '0') continue;
            
      int cx = (int)cactus->position.x + (ic % (int)cactus->size.x);
      int cy = (HEIGHT - 1 - (int)cactus->position.y - ((int)cactus->size.y - 1)) + (ic / (int)cactus->size.x);
          
      if (px == cx && py == cy) return 1;
    }
	}
    
  return 0;
}

void Update(GameState* state) {
	if (state->player.is_gorund) AnimatePlayer(&state->player, &state->iteration);
	PlayerGravity(&state->player);
	if (state->iteration%100 == 0) state->cactus.position.x-=(state->score+50)/50.;
	if (state->cactus.position.x+state->cactus.size.x < 0) {
		int new_cactus = randint(0, 4);
		state->score++;
		state->player.anim_speed = (state->score*(MAX_ANIM_SPEED-MIN_ANIM_SPEED)/MAX_SCORE)+MIN_ANIM_SPEED;
		ChangeCactus(&state->cactus, state->cactus_types_texture[new_cactus], state->cactus_types_size[new_cactus]);
	}
	for (int i=0; i<WIDTH*HEIGHT; i++) state->map[i] = 0;
	if (state->score == MAX_SCORE) state->game_over = 1; 
	if (PlayerCollision(&state->player, &state->cactus)) {
		state->player.texture = state->player.death_texture;
		state->game_over = 1;
	}
	UpdateSprite(&state->map, &state->player.position, &state->player.size, state->player.texture);
	UpdateSprite(&state->map, &state->cactus.position, &state->cactus.size, state->cactus.texture);
}

void Draw(char (*map)[WIDTH*HEIGHT], int* score, int* best_score, int* game_over, int* theme) {
	printf("\e[%dD\e[%dA", WIDTH+2, HEIGHT+2);
	for (int yd = 0; yd < HEIGHT+2; yd++) {
		int yw=yd-1;
		for (int xd = 0; xd < WIDTH+2; xd++) {
			int xw=xd-1;
			if (xw>=0 && xw<WIDTH && yw>=0 && yw<HEIGHT) {
				if ((*map)[xw+yw*WIDTH]==0) printf("\e[%dm  ", *theme ? LIGTH_EMPTY_COLOR : DARK_EMPTY_COLOR);
				else printf("\e[%dm  ", *theme ? LIGTH_FILL_COLOR : DARK_FILL_COLOR);
			} else {
				if (xd==0 && yd==0) printf("\e[%dm┌", 40);
				else if (xd==WIDTH+1 && yd==0) printf("\e[%dm┐",40);
				else if (xd==0 && yd==HEIGHT+1) printf("\e[%dm└",40);
				else if (xd==WIDTH+1 && yd==HEIGHT+1) printf("\e[%dm┘",40);
				else if (yd==0 || yd==HEIGHT+1) printf("\e[%dm──",40);
				else if (xd==0 || xd==WIDTH+1) printf("\e[%dm│",40);
			}
		}
		if (yd==1) printf("\tSCORE: %d", *score);
		if (yd==2) printf("\tBEST SCORE: %d", *best_score);
		if (yd==4) printf("\t[Q] or [ESCAPE] - QUIT");
		if (yd==5) printf("\t[SPACE] - %s", *game_over ? "RESTART":"JUMP   ");
		if (yd==7) printf("\tENGLISH LAYOUT - \e[1;32mSUPPORTED\e[0m");
		if (yd==8) printf("\tRUSSIAN LAYOUT - \e[1;31mNOT_SUPPORTED\e[0m");

		printf("\e[%dm\n", 40);
	}
	fflush(stdout);
}
