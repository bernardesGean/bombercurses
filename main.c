/***************************************************************************
 * Autor: Geancarlo Bernardes de Jesus                                     *
 * R.A.: 1581228                                                           *
 * UTFPR-MD Universidade Tecnologica Federal do Parana - Campus Medianeira *
 *                                                                         *
 * Projeto: Desenvolver uma estrutura basica do famoso game Bomberman,     *
 * em caracteres ASCII.                                                    *
 *                                                                         *
 * file:                                                                   *
 *    bomberman.c                                                          *
 **************************************************************************/

#include <assert.h>
#include <curses.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define KEY_ESCAPE          0x1B    // Valor Hexadecimal da tecla ESC
#define KEY_SPACE           0x20    // Valor Hexadecimal da tecla SPACE

#define ROW                 13      // Total de linhas no mapa (com as bordas)
#define COL                 43      // Total de colunas no mapa (com as bordas)
#define N_MAX_BALLOONS      50      // Quantidade maxima de baloes no jogo
#define N_MAX_BOMBS         20      // Quantidade maxima de bombas no jogo

#define WALL_COLOR          1
#define BOX_COLOR           2
#define SPACE_COLOR         3
#define BOMBERMAN_COLOR     4
#define BALLOON_COLOR       5
#define BOMB_COLOR          6
#define BEXPLOSION_COLOR    7

// transforma as informações em um valor inteiro sem sinal
const chtype chEXPLOSION = '~' | COLOR_PAIR(BOMB_COLOR) | A_BOLD | A_REVERSE;
const chtype chBALLOON = '$' | COLOR_PAIR(BALLOON_COLOR) | A_BOLD;
const chtype chBOMB = '*' | COLOR_PAIR(BOMB_COLOR) | A_BOLD;
const chtype chBOMBERMAN = '@' | COLOR_PAIR(BOMBERMAN_COLOR) | A_BOLD;
const chtype chSPACE = ' ' | COLOR_PAIR(SPACE_COLOR) | A_BOLD;

typedef enum {
	eSTOP,
	eRIGHT,
	eUP,
	eLEFT,
	eDOWN
} direction_t;


typedef struct {
	int line;
	int column;
	int stepsSameDir; // quadros deslocados na mesma direção
	double delayMov; // intervalo de tempo entre os movimentos
	bool alive;    // esta vivo?
	bool dying;    // esta morrendo?
	struct timeval tic;
	struct timeval tac;
	direction_t dir; // sentido do deslocamento
} balloon_t;

typedef struct {
	int line;
	int column;
	int powerExplosion; // poder de destruição
	int currentWaveSize; // tamanho da explosão
	double delayExplosion; // tempo para a bomba explodir
	double speedExp; // tempo para a onda de explosão percorrer o mapa
	bool activated; // esta no mapa?
	bool blowingUp; // esta detonando ?
	bool dirExplosion[5]; // direção possível de deslocamento da explosão
	bool reserved; // na reserva?
	struct timeval tic;
	struct timeval tac;
} bomb_t;

typedef struct {
	int line;
	int column;
	int score; // pontuação
	int lives; // vidas
	double delayMov; // tempo para o personagem poder se mover
	struct timeval tic;
	struct timeval tac;
	direction_t dir; // direção de deslocamento
} bomberman_t;

int qtBalloon;
int qtBombs = 1;
balloon_t balloon[N_MAX_BALLOONS];
bomb_t bomb[N_MAX_BOMBS];
bomberman_t bomberman;

char map[ROW + 1][COL + 1] =
		{
				{"A-----------------------------------------B"},
				{"|  %     %    %          %    %    %      |"},
				{"| X X X X X X X X X X X X X X X X X X X X |"},
				{"| %       %  %  %       %    %       % %  |"},
				{"|%X X X X X X X X X X%X X X X X X%X X X X |"},
				{"|  %         %   %       %  %    %  %     |"},
				{"| X X X X X X X X X X X X X X X X X X X X%|"},
				{"|    %    %    %    %%%    %              |"},
				{"| X X X X X X X X X X X X X X X X X X X X |"},
				{"|   %     %%%      %     %           %    |"},
				{"| X X X X X X X X X X X X X X X X X X X X |"},
				{"|   %%%    %    %     %     %   %        %|"},
				{"D-----------------------------------------C"}
		};

void startScreenSettings(void);

void startBomberman(void);

void startBalloon(int i, int line, int column, direction_t dir);

void showMap(void);

void balloonMove(balloon_t *balloon);

void bombermanMove(direction_t dir);

bool authorizeBalloonMovement(int line, int column, direction_t dir);

bool authorizeBombermanMovement(int line, int column, direction_t dir);

double timeLapsed(struct timeval tic, struct timeval tac);

int probabilityMassFunction(const int n, const double weight[n]);

void updateScoreboard(void);

void updateHighScore(void);

void helpBoard(void);

void gameOver(void);

void startExplosion(bomb_t *B);

void finishExplosion(bomb_t *B);

int main(void) {
	int ch, line, column, dir, i;
	double dTime;
	bool exitGame = false;
	direction_t nextDir = eSTOP;


	initscr();    // inicia o modo ncurses
	startScreenSettings();
	startBomberman();
	showMap();
	helpBoard();

	qtBalloon = 10;
	for (i = 0; i < qtBalloon; i++) {
		do {
			line = (int) ((double) rand() * ROW / RAND_MAX);
			column = (int) ((double) rand() * COL / RAND_MAX);
		} while (map[line][column] != ' ');
		dir = (int) ((double) rand() * 4 / RAND_MAX) + 1;
		startBalloon(i, line, column, (direction_t) dir);
	}

	qtBombs = 3;
	for (i = 0; i < qtBombs; i++)
		bomb[i].reserved = true;

	gettimeofday(&bomberman.tic, NULL); // primeiro boot do personagem

	// inicia o boot dos balões
	for (i = 0; i < qtBalloon; i++)
		gettimeofday(&(balloon[i].tic), NULL);

	while ((bomberman.lives > 0) && !exitGame) {
		// imprime o caractere do personagem na tela
		mvaddch(bomberman.line, bomberman.column, chBOMBERMAN);
		updateScoreboard(); // imprime o placar
		gettimeofday(&bomberman.tac, NULL); // encerra o cronometro do pers.
		dTime = timeLapsed(bomberman.tic, bomberman.tac); // calc. o tempo

		if (dTime > bomberman.delayMov) {
			// se o tempo decorrido for maior que o especificado para o delay
			// então o personagem pode se mover
			bombermanMove(nextDir);
			nextDir = eSTOP;
			gettimeofday(&bomberman.tic, NULL); // recomeça a contagem
		}

		for (i = 0; i < qtBalloon; i++) {
			// ignora balões vivos
			if (!balloon[i].alive)
				continue;

			gettimeofday(&(balloon[i].tac), NULL);
			dTime = timeLapsed(balloon[i].tic, balloon[i].tac);
			if (dTime > balloon[i].delayMov) {
				balloonMove(&balloon[i]);

				// se o balão foi atingido por uma bomba, ele ira ficar
				// certo tempo aparecendo na tela
				if (!balloon[i].dying)
					gettimeofday(&(balloon[i].tic), NULL);
			}
		}

		for (i = 0; i < qtBombs; i++) {
			if (bomb[i].reserved)
				continue;

			if (bomb[i].activated) {
				// imprime o caractere da bomba na tela
				mvaddch(bomb[i].line, bomb[i].column, chBOMB);

				gettimeofday(&(bomb[i].tac), NULL);
				dTime = timeLapsed(bomb[i].tic, bomb[i].tac);

				if (dTime > bomb[i].delayExplosion) {
					bomb[i].activated = false;
					bomb[i].blowingUp = true;
					bomb[i].currentWaveSize = 1;
					startExplosion(&bomb[i]);

					gettimeofday(&(bomb[i].tic), NULL);
				}
				continue;
			}

			if (bomb[i].blowingUp) {
				gettimeofday(&(bomb[i].tac), NULL);
				dTime = timeLapsed(bomb[i].tic, bomb[i].tac);

				if (dTime > (1.0 / bomb[i].speedExp)) {
					bomb[i].currentWaveSize++;
					startExplosion(&bomb[i]);
					gettimeofday(&(bomb[i].tic), NULL);

					if (bomb[i].currentWaveSize == bomb[i].powerExplosion)
						bomb[i].blowingUp = false;
				}
				continue;
			}

			if (bomb[i].currentWaveSize > 0) {
				gettimeofday(&(bomb[i].tac), NULL);
				dTime = timeLapsed(bomb[i].tic, bomb[i].tac);

				if (dTime > (1.0 / bomb[i].speedExp)) {
					finishExplosion(&bomb[i]);
					bomb[i].currentWaveSize--;
					gettimeofday(&(bomb[i].tic), NULL);
				}

				if (bomb[i].currentWaveSize == 0)
					bomb[i].reserved = true;
			}
		}

		flushinp();

		ch = getch();
		switch (ch) {
			case KEY_RIGHT:
				nextDir = eRIGHT;
				break;
			case KEY_UP:
				nextDir = eUP;
				break;
			case KEY_LEFT:
				nextDir = eLEFT;
				break;
			case KEY_DOWN:
				nextDir = eDOWN;
				break;
			case KEY_SPACE:
				for (i = 0; i < qtBombs; i++) {
					if (!bomb[i].reserved)
						continue;

					bomb[i].line = bomberman.line;
					bomb[i].column = bomberman.column;
					bomb[i].activated = true;
					bomb[i].blowingUp = false;
					bomb[i].powerExplosion = 3;
					bomb[i].currentWaveSize = 0;
					bomb[i].delayExplosion = 1.0;
					bomb[i].speedExp = 25;
					bomb[i].reserved = false;

					gettimeofday(&(bomb[i].tic), NULL);
					break;
				}
				break;
			case 'p':
			case 'P': {
				WINDOW *pauseWindow;
				char pauseMsg[] = "Pressione 'P' para continuar";
				//28 = tamanho da mensagem exibida
				pauseWindow = newwin(7, (28 + 4), ROW / 2 - 3,
									 (COL - 28 - 4) / 2);
				box(pauseWindow, 0, 0);
				wattron(pauseWindow, A_BOLD);
				mvwprintw(pauseWindow, 2, 2, "   *** Jogo Pausado ***   ");
				wattroff(pauseWindow, A_BOLD);
				mvwprintw(pauseWindow, 4, 2, pauseMsg);
				wrefresh(pauseWindow);
				flushinp();
				do { } while (toupper(wgetch(pauseWindow)) != 'P');
				flushinp();
				delwin(pauseWindow);
				touchwin(stdscr);
				break;
			}
			case KEY_ESCAPE:
				exitGame = true;
				break;
			default:
				continue;
		}

		refresh();
	}

	updateHighScore();
	refresh();
	endwin();
	return EXIT_SUCCESS;
}

void startScreenSettings(void) {

	if (has_colors() == false) {
		endwin();
		perror("O terminal não suporta cores.");
		exit(EXIT_FAILURE);
	}

	resize_term(20, 70);
	curs_set(0);
	cbreak();
	keypad(stdscr, true);
	timeout(1);
	noecho();
	srand((unsigned) time(NULL));
	start_color();


	init_pair(WALL_COLOR, COLOR_YELLOW, COLOR_BLUE);
	init_pair(BOX_COLOR, COLOR_RED, COLOR_BLUE);
	init_pair(SPACE_COLOR, COLOR_BLUE, COLOR_BLUE);
	init_pair(BOMBERMAN_COLOR, COLOR_GREEN, COLOR_BLUE);
	init_pair(BALLOON_COLOR, COLOR_CYAN, COLOR_BLUE);
	init_pair(BOMB_COLOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(BEXPLOSION_COLOR, COLOR_WHITE, COLOR_RED);
}

void startBomberman(void) {
	bomberman.line = 1;
	bomberman.column = 1;
	bomberman.score = 0;
	bomberman.dir = eSTOP;
	bomberman.lives = 10;
	bomberman.delayMov = 1.0 / 12;
}

void startBalloon(int i, int line, int column, direction_t dir) {
	assert(i < qtBalloon);

	balloon[i].line = line;
	balloon[i].column = column;
	balloon[i].dir = dir;
	balloon[i].alive = true;
	balloon[i].dying = false;
	balloon[i].stepsSameDir = 0;
	balloon[i].delayMov = 1.0 / 3.5;
}

void showMap(void) {
	int chColor, line, column;
	size_t ch;

	for (line = 0; line < ROW; line++)
		for (column = 0; column < COL; column++) {
			switch (map[line][column]) {
				case '|': // Paredes verticais
					chColor = WALL_COLOR;
					ch = ACS_VLINE | A_BOLD;
					break;
				case '-': // Paredes horizontais
					chColor = WALL_COLOR;
					ch = ACS_HLINE | A_BOLD;
					break;
				case 'A': // Canto superior esquerdo
					chColor = WALL_COLOR;
					ch = ACS_ULCORNER | A_BOLD;
					break;
				case 'B': // Canto superior direito
					chColor = WALL_COLOR;
					ch = ACS_URCORNER | A_BOLD;
					break;
				case 'C': // Canto inferior direito
					chColor = WALL_COLOR;
					ch = ACS_LRCORNER | A_BOLD;
					break;
				case 'D': // Canto superior esquerdo
					chColor = WALL_COLOR;
					ch = ACS_LLCORNER | A_BOLD;
					break;
				case 'X': // Muros internos
					chColor = WALL_COLOR;
					ch = 'X' | A_BOLD;
					break;
				case ' ': // Espaço vazio
					chColor = SPACE_COLOR;
					ch = ' ';
					break;
				case '%': // Caixa destrutiva
					chColor = BOX_COLOR;
					ch = '%' | A_BOLD;
					break;
				default: // Se houver algum erro...
					getch();
					endwin();
					perror("Erro na especificação do mapa.");
					exit(EXIT_FAILURE);
			}

			// mostra o caractere na tela
			mvaddch(line, column, ch | COLOR_PAIR(chColor));
		}
	refresh();
}

void balloonMove(balloon_t *balloon) {
	int stepsCanWalk = 0;
	int line, column;
	double FMPDir[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
	direction_t dir, newDir, opoDir;

	if (balloon->dying) {
		gettimeofday(&balloon->tac, NULL);

		// apos ser atingido por uma explosão, o balão fica parado por 1 seg.
		if (timeLapsed(balloon->tic, balloon->tac) > 1.0) {
			balloon->alive = false;
			// ao final deste tempo, insere um espaço na tela
			mvaddch(balloon->line, balloon->column, chSPACE);
		}
		else
			// inverte as cores do balão, indicando sua morte
			mvaddch(balloon->line, balloon->column, chBALLOON | A_REVERSE);

		return;
	}


	for (dir = eRIGHT; dir <= eDOWN; dir++) {
		if (authorizeBalloonMovement(balloon->line, balloon->column, dir)) {
			stepsCanWalk++;
			FMPDir[dir] = 1.0;
		}
	}


	newDir = eSTOP;
	if (stepsCanWalk != 0) {
		opoDir = eSTOP;
		if (balloon->dir == eRIGHT)
			opoDir = eLEFT;
		else if (balloon->dir == eLEFT)
			opoDir = eRIGHT;
		else if (balloon->dir == eUP)
			opoDir = eDOWN;
		else if (balloon->dir == eDOWN)
			opoDir = eUP;


		// Incrementa em 8 vezes a possibildade de se mover para frente
		if (FMPDir[balloon->dir])
			FMPDir[balloon->dir] *= 8;

		// Incrementa em 8 vezes a possibilidade de se movers para tras,
		// se ja tiver movido mais de 10 vezes consecutivas na mesma direção
		if (authorizeBalloonMovement(balloon->line, balloon->column, opoDir) &&
			(balloon->stepsSameDir > 10))
			FMPDir[opoDir] *= 8;

		// calcula a probabilidade de movimento
		newDir = (direction_t) probabilityMassFunction(5, FMPDir);

		if (newDir == balloon->dir)
			balloon->stepsSameDir++;
		else
			balloon->stepsSameDir = 0;
	}

	balloon->dir = newDir;

	line = balloon->line;
	column = balloon->column;


	switch (balloon->dir) {
		case eSTOP:
			break;
		case eRIGHT:
			if (column < (COL - 1))
				column++;
			break;
		case eUP:
			if (line > 0)
				line--;
			break;
		case eLEFT:
			if (column > 0)
				column--;
			break;
		case eDOWN:
			if (line < (ROW - 1))
				line++;
			break;
	}

	mvaddch(balloon->line, balloon->column, chSPACE);

	if(mvinch(line, column) == chBOMBERMAN)
		bomberman.lives--;

	balloon->column = column;
	balloon->line = line;

	mvaddch(balloon->line, balloon->column, chBALLOON);
}

void bombermanMove(direction_t dir) {
	int line, column;
	int prevLine = line = bomberman.line;
	int prevColumn = column = bomberman.column;


	if (!authorizeBombermanMovement(line, column, dir))
		return;


	switch (dir) {
		case eSTOP:
			return;
		case eRIGHT:
			if (column < (COL - 1))
				column++;
			break;
		case eUP:
			if (line > 0)
				line--;
			break;
		case eLEFT:
			if (column > 0)
				column--;
			break;
		case eDOWN:
			if (line < (ROW - 1))
				line++;
			break;
	}


	if (map[line][column] == ' ') {
		bomberman.column = column;
		bomberman.line = line;
		bomberman.dir = dir;


		mvaddch(prevLine, prevColumn, chSPACE);
	}


	mvaddch(bomberman.line, bomberman.column, chBOMBERMAN);
}

bool authorizeBalloonMovement(int line, int column, direction_t dir) {

	switch (dir) {
		case eSTOP:
			break;
		case eRIGHT:
			if (column < (COL - 1))
				column++;
			break;
		case eUP:
			if (line > 0)
				line--;
			break;
		case eLEFT:
			if (column > 0)
				column--;
			break;
		case eDOWN:
			if (line < (ROW - 1))
				line++;
			break;
	}

	// Balao não é suicida
	if (mvinch(line, column) == chEXPLOSION)
		return false;

	// Balao não é suicida
	if (mvinch(line, column) == chBOMB)
		return false;

	if (map[line][column] == ' ')
		return true;

	return false;
}

bool authorizeBombermanMovement(int line, int column, direction_t dir) {

	switch (dir) {
		case eSTOP:
			return false;
		case eRIGHT:
			if (column < (COL - 1))
				column++;
			break;
		case eUP:
			if (line > 0)
				line--;
			break;
		case eLEFT:
			if (column > 0)
				column--;
			break;
		case eDOWN:
			if (line < (ROW - 1))
				line++;
			break;
	}


	if (mvinch(line, column) == chBOMB)
		return false;


	if (map[line][column] == ' ')
		return true;


	return false;
}

double timeLapsed(struct timeval tic, struct timeval tac) {
	return ((tac.tv_sec - tic.tv_sec) + ((tac.tv_usec - tic.tv_usec) / 1E6));
}

int probabilityMassFunction(const int n, const double weight[n]) {
	int i;
	double random = ((double) rand()) / RAND_MAX; // Retorna um valor de 0 a 1
	double sumWeight;
	double acumSumWeight;

	sumWeight = acumSumWeight = 0.0;

	for (i = 0; i < n; i++)
		sumWeight += weight[i];

	for (i = 0; i < n; i++) {
		acumSumWeight += weight[i] / sumWeight;
		if (random <= acumSumWeight)
			return i;
	}

	return 0;
}

void updateScoreboard(void) {
	mvprintw(0, 50, "Marcador = %03d", bomberman.score);
	mvprintw(1, 50, "Vidas    = %3d", bomberman.lives);
}

void updateHighScore(void) {
	FILE *fscore = NULL;
	
	if (!(fscore = fopen("data/score.txt", "w")))
	{
		fprintf(stderr, "Erro na criação do arquivo.");
		exit(0);
	}
	fprintf(fscore, "%d\n", bomberman.score);
	fclose(fscore);
}

void helpBoard(void) {
	mvprintw(6, 50, "Setas : Movimenta");
	mvprintw(7, 50, "SPACE : Bomba");
	mvprintw(8, 50, "P     : Pause");
	mvprintw(9, 50, "ESC   : Quit");
}

void startExplosion(bomb_t *B) {
	assert(B->currentWaveSize >= 1);

	int i, t;
	int line, column;
	direction_t dir;

	if (B->currentWaveSize == 1) {
		mvaddch(B->line, B->column, chEXPLOSION);


		B->dirExplosion[eSTOP] = false;
		B->dirExplosion[eRIGHT] = true;
		B->dirExplosion[eUP] = true;
		B->dirExplosion[eLEFT] = true;
		B->dirExplosion[eDOWN] = true;
		return;
	}

	t = B->currentWaveSize - 1;

	for (dir = eRIGHT; dir <= eDOWN; dir++) {
		line = B->line;
		column = B->column;


		switch (dir) {
			case eSTOP:
				break;
			case eRIGHT:
				if ((column + t) < (COL - 1))
					column += t;
				else
					B->dirExplosion[dir] = false;
				break;
			case eUP:
				if ((line - t) > 0)
					line -= t;
				else
					B->dirExplosion[dir] = false;
				break;
			case eLEFT:
				if ((column - t) > 0)
					column -= t;
				else
					B->dirExplosion[dir] = false;
				break;
			case eDOWN:
				if ((line + t) < (ROW - 1))
					line += t;
				else
					B->dirExplosion[dir] = false;
				break;
		}


		if (map[line][column] == 'X')
			B->dirExplosion[dir] = false;


		if (B->dirExplosion[dir]) {
			mvaddch(line, column, chEXPLOSION);


			if (map[line][column] == '%') {
				map[line][column] = ' ';
				B->dirExplosion[dir] = false;
				bomberman.score++;
			}

			if ((line == bomberman.line) && (column == bomberman.column))
				bomberman.lives--;

			for (i = 0; i < qtBalloon; i++)
				if ((line == balloon[i].line) &&
					(column == balloon[i].column) &&
					balloon[i].alive) {
					balloon[i].dying = true;
					bomberman.score += 10;
				}
		}
	}
}

void finishExplosion(bomb_t *B) {
	assert(B->currentWaveSize > 0);

	int t;
	int line, column;
	direction_t dir;

	if (B->currentWaveSize == 1) {

		mvaddch(B->line, B->column, chSPACE);
		return;
	}

	t = B->currentWaveSize - 1;

	for (dir = eRIGHT; dir <= eDOWN; dir++) {
		line = B->line;
		column = B->column;


		switch (dir) {
			case eSTOP:
				break;
			case eRIGHT:
				if ((column + t) < (COL - 1)) {
					column += t;

					if (mvinch(line, column) == chEXPLOSION)
						mvaddch(line, column, chSPACE);
				}
				break;
			case eUP:
				if ((line - t) > 0) {
					line -= t;
					if (mvinch(line, column) == chEXPLOSION)
						mvaddch(line, column, chSPACE);
				}
				break;
			case eLEFT:
				if ((column - t) > 0) {
					column -= t;
					if (mvinch(line, column) == chEXPLOSION)
						mvaddch(line, column, chSPACE);
				}
				break;
			case eDOWN:
				if ((line + t) < (ROW - 1)) {
					line += t;
					if (mvinch(line, column) == chEXPLOSION)
						mvaddch(line, column, chSPACE);
				}
				break;
		}
	}
}
