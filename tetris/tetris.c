#include <stdio.h>
#include <time.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define WIDTH 12
#define HEIGHT 16

#define EMPTY 40
#define SCORE 43

struct object {
	char type;
	int color;
};

struct player {
	char mask[17];
	int color;
	int x,
	    y,
		speed;
};

struct object map[WIDTH*HEIGHT];
char buffer[WIDTH*HEIGHT];

char next_mask[17];
const char number_masks[10][36] = {
	"01110100011000110001100011000101110",
	"00100011000010000100001000010001110",
	"01110100010000100010001000100011111",
	"11111000100010000010000011000101110",
	"00010001100101010010111110001000010",
	"11111100001111000001000011000101110",
	"00110010001000011110100011000101110",
	"11111000010001000100010000100001000",
	"01110100011000101110100011000101110",
	"01110100011000101111000010001001100"
};
const char pause_mask[36] = "000000000001010010100101000000000000";

int next_color;
int pause_game = 0;
int score = 0;

int randint(int min, int max) {
	return min+rand()%(max-min+1);
}

void draw_map(struct object (*buffer)[WIDTH*HEIGHT]) {
	char tmp_score[12];
	sprintf(tmp_score, "%d", score);
	for (int y=-1; y<HEIGHT; y++) {
		// DRAW MAP
		for (int x=0;x<WIDTH; x++) {
			if (x==0 && y==-1) printf("\e[%dm┌", EMPTY);
			else if (x==WIDTH-1 && y==-1) printf("\e[%dm┐",EMPTY);
			else if (x==0 && y==HEIGHT-1) printf("\e[%dm└",EMPTY);
			else if (x==WIDTH-1 && y==HEIGHT-1) printf("\e[%dm┘",EMPTY);
			else if ((*buffer)[x+y*WIDTH].type=='w') printf("\e[%dm│",EMPTY);
			else if ((*buffer)[x+y*WIDTH].type=='f' || y==-1) printf("\e[%dm──",EMPTY);
			else printf("\e[%dm  ", (*buffer)[x+y*WIDTH].color);
		}
		printf("  ");
		
		// DRAW_SCORE
		if (y >= 0 && y < 7) {
			int ym = y;
			if (pause_game) {
				for (int xm = 0; xm < 5; xm++) {
					if (pause_mask[xm+ym*5]=='1') printf("\e[41m  \e[0m");
					else printf("\e[%dm  \e[0m", EMPTY);
				}
			}
			else {
				for (int i = 0; i < strlen(tmp_score); i++) {
					for (int xm = 0; xm < 5; xm++) {
						if (number_masks[tmp_score[i] - '0'][xm + ym * 5] == '1') printf("\e[%dm  \e[0m", SCORE);
						else printf("\e[%dm  \e[0m", EMPTY);
					}
					printf("\e[%dm  \e[0m", EMPTY);
				}
			}
		}
		// DRAW_MASK
		else if (y >=HEIGHT-7 && y <= HEIGHT-2) {
			int yo = y-HEIGHT+7;
			if (yo == 0)printf("\e[%dm┌────────┐", EMPTY);
			else if (yo == 5)printf("\e[%dm└────────┘", EMPTY);
			else {
				printf("\e[%dm│", EMPTY);
				int ym = yo-1;
				for (int xm=0; xm<4; xm++) {
					if (next_mask[xm+ym*4]=='1') printf("\e[%dm  \e[0m", next_color);
					else printf("\e[%dm  \e[0m", EMPTY);
				}
				printf("\e[%dm│  ", EMPTY);
				if (yo == 1) printf("\e[0mWASD - \e[32mCONTROL\e[0m");
				else if (yo == 2) printf("\e[0mP - \e[33mPAUSE\e[0m");
				else if (yo == 3) printf("\e[0mQ - \e[31mEXIT\e[0m");
			}
		}

		printf("\n");
	}
}

void draw_control(int x, int y) {
	printf("\e[%d;%dH\e[33mQ - \e[31mEXIT\t\e[33mWASD - \e[32mCONTROL\e[0m", y, x);
}

void setPlayerData(struct player *p, struct object (*buffer)[WIDTH*HEIGHT], char data) {
	int color=p->color;
	if (data=='e') color=EMPTY;
	for (int i=0; i<16; i++) {
		if (p->mask[(i%4)+(i/4)*4]=='1') {
				(*buffer)[(p->x+(i%4))+(p->y+(i/4))*WIDTH].color = color;
				(*buffer)[(p->x+(i%4))+(p->y+(i/4))*WIDTH].type = data;
		}
	}
}

void generate_mask(char (*_mask)[17]) {
	static char _masks[7][17] = {
		"0000000011110000",
		"0000010001110000",
		"0000000101110000",
		"0000011001100000",
		"0000001101100000",
		"0000001001110000",
		"0000011000110000"
	};
	int mask_index = randint(0,5);
	next_color=40+randint(1,6);
	for(int i=0;i<16;i++) (*_mask)[i]=_masks[mask_index][i];
	(*_mask)[16]='\0';
}

void create_player(int x, int y, struct player *p, struct object (*buffer)[WIDTH*HEIGHT]) {
	for (int i=0; i<16; i++)p->mask[i]=next_mask[i];
	p->mask[16]='\0';
	p->color=next_color;
	generate_mask(&next_mask);
	p->x=x;
	p->y=y;
	p->speed=1;
	setPlayerData(p, buffer, 'p');
}

int is_collision(int dx, int dy, struct player *p, struct object (*buffer)[WIDTH*HEIGHT]) {
	for (int i=0; i<16; i++) {
		if (p->mask[i] == '0') continue;
		int world_x = p->x + (i % 4) + dx;
		int world_y = p->y + (i / 4) + dy;
		if ((*buffer)[world_x+world_y*WIDTH].type!='e' && (*buffer)[world_x+world_y*WIDTH].type!='p') return 1;
	}
	return 0;
}

int move_player(int dx, int dy, struct player *p, struct object (*buffer)[WIDTH*HEIGHT]) {
	if (is_collision(dx,dy,p,buffer)) return 1;
	setPlayerData(p, buffer, 'e');
	p->x+=dx;
	p->y+=dy;
	setPlayerData(p, buffer, 'p');
	return 0;
}

void rotate_player(struct player *p, struct object (*buffer)[WIDTH*HEIGHT]) {
	char old_mask[17];
	for (int i=0;i<17;i++) old_mask[i]=p->mask[i];
	setPlayerData(p,buffer,'e');
	for (int i=0;i<17;i++) p->mask[i]=old_mask[(3-(i%4))*4+(i/4)];
	if (is_collision(0,0,p,buffer)) for (int i=0;i<16;i++) p->mask[i]=old_mask[i];
	setPlayerData(p,buffer,'p');
}

void init_terminal() {
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag &= ~(ICANON | ECHO);
	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void restore_terminal() {
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag |= (ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}


int line_is_full(struct object (*buffer)[WIDTH*HEIGHT], int y) {
	for (int x=0; x<WIDTH; x++) {
		if ((*buffer)[x+y*WIDTH].type=='e') return 0;
	}
	return 1;
}

void clear_line(struct object (*buffer)[WIDTH*HEIGHT], int y) {
	for (int x=0; x<WIDTH; x++) {
		if ((*buffer)[x+y*WIDTH].type=='w') continue;
		(*buffer)[x+y*WIDTH].type='e';
		(*buffer)[x+y*WIDTH].color=EMPTY;
	}
}

void shift_line_down(struct object (*buffer)[WIDTH*HEIGHT], int sy) {
	for (int y=sy-1; y>=0; y--) {
		for (int x=0; x<WIDTH; x++) {
			if ((*buffer)[x+(y+1)*WIDTH].type == 'w') continue;
			(*buffer)[x+(y+1)*WIDTH].type = (*buffer)[x+y*WIDTH].type;
			(*buffer)[x+(y+1)*WIDTH].color = (*buffer)[x+y*WIDTH].color;
		}
	}
}

void check_line(struct object (*buffer)[WIDTH*HEIGHT]) {
	int tmp_score = 0;
	for (int y=HEIGHT-2; y>=0; y--) {
		if (line_is_full(buffer, y)) {
			clear_line(buffer, y);
			shift_line_down(buffer, y);
			y++;
			tmp_score++;
		}
	}
	score+=tmp_score*100;
}

int main(int argc, char *argv[]) {
	int speed = 2;
	if (argc>1) speed = atoi(argv[1]);

	srand(time(0));
	for (int y=0; y<HEIGHT; y++) {
		for (int x=0;x<WIDTH; x++) {
			map[x+y*WIDTH].color=EMPTY;
			map[x+y*WIDTH].type='e';
			if (x==0 || x==WIDTH-1) map[x+y*WIDTH].type='w';
			else if (y==HEIGHT-1) map[x+y*WIDTH].type='f';
		}
	}
	
	generate_mask(&next_mask);
	struct player Player;
	create_player(WIDTH/2-2, -2, &Player, &map);

	init_terminal();
	char buffer[10];
	
	int iterations=0;

	for (int i=0;i<HEIGHT+1; i++) printf("\n");
	printf("\e[?25l");
	while (1) {
		Player.speed=speed;
		ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
		if (bytes_read>0) {
			buffer[bytes_read] = '\0';
			
			if (bytes_read==1 && buffer[0]=='q') break;
			if (bytes_read==1 && buffer[0]=='p') pause_game=!pause_game;
			else if (bytes_read==1 && buffer[0]=='w' && !pause_game) rotate_player(&Player, &map);
			else if (bytes_read==1 && buffer[0]=='a' && !pause_game) move_player(-1,0,&Player, &map);
			else if (bytes_read==1 && buffer[0]=='s' && !pause_game) Player.speed=10*speed;
			else if (bytes_read==1 && buffer[0]=='d' && !pause_game) move_player(1,0,&Player, &map);
		}

		if (!pause_game && iterations>8000/Player.speed) {
			if (move_player(0,1,&Player, &map)) {
				setPlayerData(&Player, &map, 'b');
				if (Player.y<=-2) break;
				check_line(&map);
				create_player(WIDTH/2-2, -2, &Player, &map);
				score+=10;
			}
			iterations=0;
		}
		printf("\e[%dA\e[%dD", HEIGHT+1, WIDTH);
		draw_map(&map);
		iterations++;
	}
	restore_terminal();
	printf("\e[?25h");
}
