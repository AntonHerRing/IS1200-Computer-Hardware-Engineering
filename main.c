#include <pic32mx.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//g++ -o MprojV1.exe MprojV1.c

#define XLIM 128	//40
#define YLIM 32		//20

#define EDGE 1


#define BALLX 2		//size in x
#define BALLY 2		//size in y

//char field[XLIM * YLIM];
uint8_t displaybuff[512];

//binary calculator guessing
int prevPos[2] = {0 , 1};

//game specific
int btns = 0;
int player1Score = 0;
int player2Score = 0;

char playername[3] = "AAA"; 		//initials and place

char playerarray[10][3];
int arrayspot = 0;

int limit = 3;

int highscores[11] = {0,0,0,0,0,0,0,0,0,0,0};					//read character values to get array spot

//ball 2x2 pixels
int x[2][2] = {{63 , 64}, {63, 64}};
int y[2][2] = {{15 , 15}, {16, 16}};

//copy of ball start values
int copyX[2][2] = {{63 , 64}, {63, 64}};
int copyY[2][2] = {{15 , 15}, {16, 16}};

//ball movement
float moveX = 1;
float moveY = 1;

//movement increments
float sumMoveX = 0;
float sumMoveY = 0;

//player 1 pad 8x2 alltid 3 från kant
int xP1[8][2] = {{3 , 4}, {3 , 4},{3 , 4},{3 , 4},{3 , 4},{3 , 4},{3 , 4},{3 , 4}};
int yP1[8][2] = {{7 , 7}, {8, 8},{9 , 9}, {10, 10},{11 , 11}, {12, 12},{13 , 13}, {14, 14}};

//player 2 pad 8x2 alltid 3 från kant
//int xP2[8][2] = {{26 , 27}, {26 , 27},{26 , 27},{26 , 27},{26 , 27},{26 , 27},{26 , 27},{26 , 27}};
int xP2[8][2] = {{(XLIM - 4) , (XLIM - 3)}, {(XLIM - 4) , (XLIM - 3)},{(XLIM - 4) , (XLIM - 3)},{(XLIM - 4) , (XLIM - 3)},{(XLIM - 4) , (XLIM - 3)},
				{(XLIM - 4) , (XLIM - 3)},{(XLIM - 4) , (XLIM - 3)},{(XLIM - 4) , (XLIM - 3)}};
int yP2[8][2] = {{7 , 7}, {8, 8},{9 , 9}, {10, 10},{11 , 11}, {12, 12},{13 , 13}, {14, 14}};

//screens for use in game
const uint8_t const startScreen[512];
const uint8_t const selectMode[512];
const uint8_t const player1Wins[512];
const uint8_t const player2Wins[512];
const uint8_t const AIWins[512];
const uint8_t const AIlose[512];
const uint8_t const difficultyModes[512];

//function deklaration
void usr_isr(void);
void change_direction(int collisionObject);
int find_ball(int xrow, int yrow);
void player_points(int playernum);
int find_Player(int xrow, int yrow, int playerX[8][2], int playerY[8][2]);
int find_PlayerY(int yrow, int playerY[8][2]);
int find_PlayerX(int xrow, int playerX[8][2]);
void Player_move(int playerY[8][2], int updown);
int Player_hit(int ballX[2][2], int ballY[2][2], int playerX[8][2], int playerY[8][2], int playerbit);
int randn();
void initials();

int currentdirection = 2; 		// 1, 2 , 3, 4
int count = 0;
int upp = 9;
int countrand = 0;
int globalscore = 0;

//****************************************
//Ai test
//tracks ball y movment, has randomness, only moves when ball reaches halfway (only right side player)
void AI_test1(int playerX[8][2], int playerY[8][2], int factor)
{
	int difficulty = randn(5) % (factor * pow((-1), (randn(1) % 7)));
	int position = (playerY[4][0] - y[0][0]) + difficulty;
	if(x[0][0] > (XLIM/2))
		if(playerY[4][0] != y[0][0])
		{
			if(position < 0)
				Player_move(playerY, 1); 	//down
			else if(position > 0)
				Player_move(playerY, -1); 	//up
		}
}

void AI_test2(int playerX[8][2], int playerY[8][2])
{
	int position = y[0][0] - 10 ; //(rand() % 20);
	if(position < 0)
		Player_move(playerY, 1); 	//down
	else if(position > 0)
		Player_move(playerY, -1); 	//up
}
//****************************************

int pow(int a, int b)
{
	int answer = 1;
	int i;
	if(b != 0)
		for(i = 0; i < b; i++)
			answer *= a;
	return answer;
}

int binary_converter(char n[8], int page, int column)
{
	int i;
	int num = 0;
	int power = 7;
	for(i = 0; i < 8; i++)
		num += (int)n[i] * pow(2, i);
return num;
}

void update_screen(void)		//prints out the Field and ball
{
	int zero = 0;
	char byte[8];
	int i;
	int j = EDGE;
	int kant = EDGE;
	int b = 0;
	int d = 0;
	while(d < 512)
	{
		if( j > XLIM )		//if j == 128 set to edge value
		{
			j = EDGE;
			kant = i;
		}
		for(i = kant; i <= kant + 7; i++)
		{
			if(find_ball(j, i) == 1 || find_Player(j, i, xP1, yP1) == 1 || find_Player(j, i, xP2, yP2) == 1 || i == YLIM || i == EDGE)
			{
				byte[b++] = 1;
				zero = 1;
			}
			else
				byte[b++] = 0;
		}
		if( zero != 0 )		//if byte is 0 skip calculation
		{
			displaybuff[d++] = binary_converter(byte, i , j);
			zero = 0;
		}
		else
			displaybuff[d++] = 0;
		b = 0;
		j++;
	}		//(alla nollor)0 = släkt, (alla ettor)255 = tänt
	OledUpdate(displaybuff);
}

int find_ball(int xrow, int yrow)		//finds if ball is at position x and y
{
	if(xrow >= x[0][0] && xrow <= x[0][1] && yrow >= y[0][0] && yrow <= y[1][0])
		return 1;
	else
		return 0;
}

int find_ballX(int edge, int xrow)		//finds if ball is at position x
{
	if(xrow >= x[0][0] && xrow <= x[0][1])
		return 1;
	else if(edge >= x[0][0] && edge <= x[0][1])
		return 2;
	else
		return 0;
}

int find_ballY(int edge, int yrow)		//finds if ball is at position y
{
	if(yrow >= y[0][0] && yrow <= y[1][0])
		return 1;
	else if(edge >= y[0][0] && edge <= y[1][0])
		return 2;
	else
		return 0;
}

int find_Player(int xrow, int yrow, int playerX[8][2], int playerY[8][2])		//finds player at position x and y
{
	if(yrow >= playerY[0][0] && yrow <= playerY[7][0] && xrow >= playerX[0][0] && xrow <= playerX[0][1])
		return 1;
	else
		return 0;
}

int find_PlayerY(int yrow, int playerY[8][2])		//finds pad at position y
{
	if(yrow >= playerY[0][0] && yrow <= playerY[7][0])
	return 1;
	else
		return 0;
}

int find_PlayerX(int xrow, int playerX[8][2])		//finds player at position x and y
{
	if(xrow >= playerX[0][0] && xrow <= playerX[7][1])
		return 1;
	else
		return 0;
}

int Player_hit(int ballX[2][2], int ballY[2][2], int playerX[8][2], int playerY[8][2], int playerbit)	//finds if ball comes in contact with player 1 = P1, -1 = P2
{
	int yaxis;
	if(playerbit < 0)
		yaxis = playerbit * (-1);
	else
		yaxis = playerbit;
	if(ballY[0][0] >= (playerY[0][0] - 1) && ballY[0][1] <= (playerY[7][0] + 1) && ballX[0][1] >= (playerX[0][0] + playerbit) && ballX[0][1] <= (playerX[0][1] + playerbit))
		return 1;
	else
		return 0;
}

void game_delay(int ms)			//adds delay
{
	int i;
	while(ms > 0)
	{
		ms--;
		for(i = 0; i < 10000; i++){}
	}
}

/***************************************************
	Bug när boll hamnar i mitt i hörnet. den kommer då
	följa längs med väggen istället för att studsa
**************************************************/

void update_position()		//updates position of ball
{
	int j;
	int player1 = Player_hit(x, y, xP1, yP1, 2);
	int player2 = Player_hit(x, y, xP2, yP2, -2);
	int xball = find_ballX(EDGE, XLIM);
	int yball = find_ballY(EDGE, YLIM);

	int updateX;
	int updateY;

	//finding points
	if(xball == 1)				//left edge
	{
		player_points(1);		//go out if points update
		return;
	}
	else if(xball == 2)			//right edge
	{
		player_points(2);
		return;
	}

	//ball hits wall/floor
	if(xball != 0  || yball != 0 || yball == 2)		//if any is out of bounds
		change_direction(0);

	//ball hits player pad
	if(player1 == 1)
	{
		change_direction(1);
		globalscore++;
	}

	if(player2 == 1)
		change_direction(2);


	sumMoveX += moveX;
	sumMoveY += moveY;

	updateX = (int) sumMoveX;
	updateY = (int) sumMoveY;

	sumMoveX -= updateX;
	sumMoveY -= updateY;
	for(j = 0; j < 4; j++)
	{
		x[j / 2][j % 2] += updateX;
		y[j / 2][j % 2] += updateY;
	}

	if(x[0][0] < 5 && x[1][1] > 3 || x[1][1] > XLIM-4 && x[0][0] < XLIM-3)
    {
        if(y[1][1] >= yP1[0][0]-1 && y[0][0] <= yP1[7][0]+1)
        {
            if(moveY > 0)
            {
                for(j = 0; j < 4; j++)
                {
                    y[0][j % 2] = yP1[0][0]-2;
                    y[1][j % 2] = yP1[0][0]-1;
                }
            }
            else if(moveY < 0)
            {
                for(j = 0; j < 4; j++)
                {
                    y[0][j % 2] = yP1[7][0]+1;
                    y[1][j % 2] = yP1[7][0]+2;
                }
           	}
            change_direction(3);
        }
        if(y[1][1] >= yP2[0][0]-1 && y[0][0] <= yP2[7][0]+1)
        {
            if(moveY > 0)
            {
                for(j = 0; j < 4; j++)
                {
                    y[0][j % 2] = yP2[0][0]-2;
                    y[1][j % 2] = yP2[0][0]-1;
                }
            }
            else if(moveY < 0)
            {
                for(j = 0; j < 4; j++)
                {
                    y[0][j % 2] = yP2[7][0]+1;
                    y[1][j % 2] = yP2[7][0]+2;
                }
           }
            change_direction(3);
		}
    }

	if(y[0][0] < EDGE)
	{
		for(j = 0; j < 4; j++)
		{
			y[0][j % 2] = EDGE + 1;
			y[1][j % 2] = EDGE + 2;
		}
	}
	else if(y[1][0] > YLIM)
	{
		for(j = 0; j < 4; j++)
		{
			y[0][j % 2] = YLIM - 2;
			y[1][j % 2] = YLIM - 1;
		}
	}
}
//only find lowest player y and higest player y no searching
void Player_move(int playerY[8][2], int updown)	//-1 = up, 1 = down
{
	int j;
	//only moves if not at top or bottom
	if((playerY[7][0] != YLIM && !(updown < 1)) || (playerY[0][0] != EDGE && !(updown > 0)))
		for(j = 0; j < 16; j++)
		{
			playerY[j / 2][j % 2] += updown;
		}
}

void change_direction(int collisionObject)			//changes the direction of the ball according to which object was collided with 0 for walls, 1 for player1 and 2 for player 2
{
	float deviation;
	switch(collisionObject)
	{
		case 0:
			moveY = moveY * (-1.0);
			if(moveY < 0)
				sumMoveY = (-1) - moveY;
			else
				sumMoveY = 1 - moveY;
			break;
		case 1:
			moveX *= -1;
			deviation = (yP1[3][0] - y[0][0]);
			moveY += deviation* 0.2;
			break;
		case 2:
			moveX *= -1;
			deviation = (yP2[3][0] - y[0][0]);
			moveY += deviation * 0.2;
			break;
		case 3:
            moveX *= -1;
            moveY *= -1.5;
			if(moveY < 0)
				sumMoveY = (-1) - moveY;
			else
				sumMoveY = 1 - moveY;
            break;

	}
}

int randn(int n)
{
	int randnum = 0;
	char ran[1];
	upp += n;
	if((int)ran[upp] > 0)
		randnum = (int)ran[upp] + 3;
	else
		randnum = (int)ran[upp] * (-1) + 5;

	if(upp > 100)
		upp = 0;
		return randnum;
	//return rand();
}

void player_points(int playernum)
{
	int j;
	int randoffset = currentdirection = (randn(3) % 10) + 1;
	if(playernum == 2)		//player 2
	{
		player1Score += 1;
		PORTE |= (0x0f >> (4 - player1Score));
	}
	else if(playernum == 1)
	{
		player2Score += 1;
		PORTE |= (0xf0 << ( 4 - player2Score));
	}
	for(j = 0; j < 4; j++)	//player 1
	{
		x[j / 2][j % 2] = copyX[j / 2][j % 2] + randoffset;
		y[j / 2][j % 2] = copyY[j / 2][j % 2] + randoffset;
	}
	//currentdirection = (randn(randn(3) % 6) % 4) + 1;
	//change_direction(0, 0, 0, 0);
	moveY = 1;
	moveX = 1;
	sumMoveY = 0;
	sumMoveX = 0;
}

//vs player2 mode
void VsPL2()
{
	PORTECLR = 0xff;
	currentdirection = (randn(7) % 4) + 1;
	count = 0;
	while(player1Score != 4 && player2Score != 4 && (TRISGCLR & 0x1) != 1)
	{
		btns = getbtns();
		update_screen();
		update_position();

		//player1 move btns
		if((btns & 0x4) == 4)
			Player_move(yP1, -1);
		else if((btns & 0x8) == 8)
			Player_move(yP1, 1);

		//player2 moves btns
		if((btns & 0x1) == 1)
			Player_move(yP2, -1);
		else if((btns & 0x2) == 2)
			Player_move(yP2, 1);
	}
	if(player1Score < player2Score)	//player1 wins
		OledUpdate(player1Wins);
	else if(player1Score == player2Score)
		OledUpdate(selectMode);
	else					//player2 wins
		OledUpdate(player2Wins);
	DelayMs(1000);
	while(1)
	{
		btns = getbtns();
		if((btns & 0x1) == 1)
			break;
		else if((btns & 0x2) == 2)
			break;
		else if((btns & 0x4) == 4)
			break;
		else if((btns & 0x8) == 8)
			break;
	}
	OledUpdate(selectMode);
	DelayMs(1000);

	player1Score = 0;
	player2Score = 0;
	btns = 0;
	TRISGCLR = 0x1;
}

//vs AI mode
void VsAI(int difficulty)
{
	if(difficulty == 1)
		initials();		//initiates playerscore
	PORTECLR = 0xff;
	currentdirection = (randn(7) % 4) + 1;
	count = 0;
	while(player1Score != 4 && player2Score != 4)
	{
		btns = getbtns();
		update_screen();

		//AI delay
		if(IFS(0) & 0x100)
		{
			count++;
			if(count == difficulty)
			{
				AI_test1(xP2, yP2, difficulty);
				count = 0;
			}
			IFSCLR(0) = 0x100;
		}
		update_position();

		//player1 move btns
		if((btns & 0x4) == 4)
			Player_move(yP1, -1);
		else if((btns & 0x8) == 8)
			Player_move(yP1, 1);
	}
	if(player1Score < player2Score)	//player wins against AI
	{
		OledUpdate(AIlose);
	}
	else if(difficulty == 1)
	{
		OledUpdate(AIWins);
		int correctname = 0;
		int index = 0;
		int i,j;
		for(i = 0; i < 10; i++)
		{
			for(j = 0; j < 3; j++)
			{
				if(playerarray[i][j] == playername[j])
				{
					correctname++;
				}
				else
				{
					correctname = 0;
				}
			}
			if(correctname == limit)
			{
				index = i;
				break;
			}
			correctname = 0;
		}
		highscores[index] = globalscore;
	}
	else					//player loses against AI
		OledUpdate(AIWins);
	DelayMs(1000);
	while(1)
	{
		btns = getbtns();
		if((btns & 0x1) == 1)
			break;
		else if((btns & 0x2) == 2)
			break;
		else if((btns & 0x4) == 4)
			break;
		else if((btns & 0x8) == 8)
			break;
	}
	OledUpdate(selectMode);
	DelayMs(1000);

	player1Score = 0;
	player2Score = 0;
	btns = 0;
	TRISGCLR = 0x1;
}

//high score screen
void HISC()
{
	int i,j;
	int correctname = 0;
	int index;
	initials();

	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 3; j++)
		{
			if(playerarray[i][j] == playername[j])
			{
				correctname++;
			}
			else
				correctname = 0;

		}
		if(correctname == limit)
		{
			index = i;
			break;
		}
		correctname = 0;
	}
	//highscores[i] the i found previosly
	display_string(0, "Score");
	display_string(1, "           ");
	display_update();
	char word[] = "000";
	if(correctname == limit)
	{
		word[0] = '0' + (char)(highscores[index] / 100);
		word[1] = '0' + (char)((highscores[index] % 100) / 10);
		word[2] = '0' + (char)(highscores[index] % 10);
	}

	while(1)
	{
		display_string(1, word);
		display_update();

		btns = getbtns();
		if((btns & 0x1) == 1)
			break;
		else if((btns & 0x2) == 2)
			break;
		else if((btns & 0x4) == 4)
			break;
		else if((btns & 0x8) == 8)
			break;
	}
	OledUpdate(selectMode);
	DelayMs(1000);
}
//int highscores[270];
//alphabet[26]
//playername[3]
//display_string(char[])

void initials()
{
	char alphabet[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	arrayspot = 0;
	display_string(0, "           ");
	display_string(1, "           ");
	display_update();
	char letter;
	int i = 0;
	int b = 0;
	display_string(0, "Write 3 initials");
	display_update();
	while(b < 3)
	{
		letter = alphabet[i];
		display_string(1, playername);
		display_update();

		btns = getbtns();
		if((btns & 0x1) == 1)				//select letter
		{
			playername[b++] = alphabet[i];
			DelayMs(200);
		}
		else if((btns & 0x4) == 4 && i < 26) //scroll upp
		{
			i++;
			playername[b] = alphabet[i];
			DelayMs(200);
		}
		else if((btns & 0x8) == 8 && i > 0)	//scroll down
		{
			i--;
			playername[b] = alphabet[i];
			DelayMs(200);
		}
	}
	int index = 0;
	int correctname = 0;
	int j;

	for(i = 0; i < 3; i++)
	{
		playerarray[arrayspot][i] = playername[i];
	}
	arrayspot++;
}

//difficulty for Ai
void AIdifficulty()
{
	OledUpdate(difficultyModes);
	DelayMs(1000);
	while(1)
	{
		btns = getbtns();
		if((btns & 0x1) == 1){			//super hard mode
			VsAI(1);
			break;
		}
		else if((btns & 0x2) == 2){		//hard mode
			VsAI(2);
			break;
		}
		else if((btns & 0x4) == 4){		//Normal
			VsAI(4);
			break;
		}
		else if((btns & 0x8) == 8){		//baby mode
			VsAI(6);
			break;
		}
	}
}

//the main menu
void menu()
{
	OledUpdate(selectMode);
	DelayMs(1000);
	while(1)
	{
		//display screen
		btns = getbtns();
		if((btns & 0x1) == 1)
			AIdifficulty();
		else if((btns & 0x2) == 2)
			HISC();
		else if((btns & 0x8) == 8)
			VsPL2();

		if(IFS(0) & 0x100)
		{
			count++;
			if(count == 100)
			{
				countrand++;
				count = 0;
			}
			IFSCLR(0) = 0x100;
		}
	}
}

int main()
{
	//initializations
	ioinnit();
	timerinnit();
	dispinnit();

	//start screen
	OledUpdate(startScreen);
	DelayMs(1000);

	//meny
	while(1)
	{
		btns = getbtns();
		if((btns & 0x1) == 1)
			menu();
		else if((btns & 0x2) == 2)
			menu();
		else if((btns & 0x4) == 4)
			menu();
		else if((btns & 0x8) == 8)
			menu();
	}
	return 0;
}

/**********************
interupt switch to end game early
*********************/
void user_isr()
{
	if(IFS(0) & 0x800)
	{
		TRISGSET = 0x1;
		IFSCLR(0) = 0x800;
	}
}
