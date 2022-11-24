#include<SDL.h>
#include<sdl_ttf.h>
#include<iostream>
#include<algorithm>
#include<vector>
#include<list>
#include<cmath>
#include<fstream>
#include<sstream>
#include<time.h>
#include "game.h"
#include "json.hpp"
#include <chrono>
#include <random>

class Rnd {
public:
	std::mt19937 rng;

	Rnd()
	{
		std::mt19937 prng(std::chrono::steady_clock::now().time_since_epoch().count());
		rng = prng;
	}

	int getRndInt(int min, int max)
	{
		std::uniform_int_distribution<int> distribution(min, max);
		return distribution(rng);
	}

	double getRndDouble(double min, double max)
	{
		std::uniform_real_distribution<double> distribution(min, max);
		return distribution(rng);
	}
} rnd;

//la clase juego:
Game* Game::s_pInstance = 0;

Game::Game()
{
	m_pRenderer = NULL;
	m_pWindow = NULL;
}

Game::~Game()
{

}

SDL_Window* g_pWindow = 0;
SDL_Renderer* g_pRenderer = 0;

bool Game::init(const char* title, int xpos, int ypos, int width,
	int height, bool fullscreen)
{
	// almacenar el alto y ancho del juego.
	m_gameWidth = width;
	m_gameHeight = height;

	// attempt to initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		int flags = 0;
		if (fullscreen)
		{
			flags = SDL_WINDOW_FULLSCREEN;
		}

		std::cout << "SDL init success\n";
		// init the window
		m_pWindow = SDL_CreateWindow(title, xpos, ypos,
			width, height, flags);
		if (m_pWindow != 0) // window init success
		{
			std::cout << "window creation success\n";
			m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
			if (m_pRenderer != 0) // renderer init success
			{
				std::cout << "renderer creation success\n";
				SDL_SetRenderDrawColor(m_pRenderer,
					255, 255, 255, 255);
			}
			else
			{
				std::cout << "renderer init fail\n";
				return false; // renderer init fail
			}
		}
		else
		{
			std::cout << "window init fail\n";
			return false; // window init fail
		}
	}
	else
	{
		std::cout << "SDL init fail\n";
		return false; // SDL init fail
	}
	if (TTF_Init() == 0)
	{
		std::cout << "sdl font initialization success\n";
	}
	else
	{
		std::cout << "sdl font init fail\n";
		return false;
	}

	std::cout << "init success\n";
	m_bRunning = true; // everything inited successfully, start the main loop

	//Joysticks
	InputHandler::Instance()->initialiseJoysticks();

	//load images, sounds, music and fonts
	//AssetsManager::Instance()->loadAssets();
	AssetsManager::Instance()->loadAssetsJson(); //ahora con formato json
	Mix_Volume(-1, 16); //adjust sound/music volume for all channels

	//ReadHiScores();

	state = GAME;
	subState = LINES;

	//SDL_RenderSetScale(Game::Instance()->getRenderer(), 2, 2);

	p = new player();
	p->settings("fang", { 100,100 }, { 0,0 }, 40, 49, 6, 5, 0, 0.0, 1);

	return true;
}

void Game::render()
{
	SDL_SetRenderDrawColor(Game::Instance()->getRenderer(), 255, 255, 255, 0);
	SDL_RenderClear(m_pRenderer);


	for (int i = 0; i < H; i++)
		for (int j = 0; j < W; j++)
		{
			Uint8 *r = 0;
			Uint8 *g = 0;
			Uint8 *b = 0;
			Uint8 *a = 0;
			SDL_GetRenderDrawColor(Game::Instance()->getRenderer(), r, g, b, a);
			//SDL_RenderFillRect(SDL_Renderer * renderer,const SDL_Rect * rect);
			SDL_Rect* rct = new SDL_Rect();
			rct->x = j * 32;
			rct->y = i * 32;
			rct->h = rct->w = 32;

			if (TileMap[i][j] == 'B') SDL_SetRenderDrawColor(Game::Instance()->getRenderer(), 0, 0, 0, 0);

			if (TileMap[i][j] == '0')  SDL_SetRenderDrawColor(Game::Instance()->getRenderer(), 0, 255, 0, 0);

			if (TileMap[i][j] == ' ') continue;

			SDL_RenderFillRect(Game::Instance()->getRenderer(), rct);
			SDL_SetRenderDrawColor(Game::Instance()->getRenderer(), 255, 255, 255, 0);
		}

	p->draw();
	AssetsManager::Instance()->Text(to_string(p->m_onGround), "font", 0, 0, { 255,255,255,255 }, Game::Instance()->getRenderer());

	SDL_RenderPresent(m_pRenderer); // draw to the screen
}

void Game::quit()
{
	m_bRunning = false;
}

void Game::clean()
{
	//WriteHiScores();
	std::cout << "cleaning game\n";
	InputHandler::Instance()->clean();
	AssetsManager::Instance()->clearFonts();
	TTF_Quit();
	SDL_DestroyWindow(m_pWindow);
	SDL_DestroyRenderer(m_pRenderer);
	Game::Instance()->m_bRunning = false;
	SDL_Quit();
}

void Game::handleEvents()
{
	InputHandler::Instance()->update();

	//HandleKeys
	if (state == MENU)
	{
	}

	if (state == GAME)
	{
		p->m_isMoving = false;
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_LEFT))
		{
			p->m_isMoving = true;
			p->m_position.m_x -= 2;
			p->m_heading = false;
			isCollide(p, 0);
		}
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_RIGHT))
		{
			p->m_isMoving = true;
			p->m_position.m_x += 2;
			p->m_heading = true;
			isCollide(p, 0);
		}
		if (InputHandler::Instance()->isKeyDown(SDL_SCANCODE_UP))
		{
			p->m_isMoving = true;
			if (p->m_onGround && !p->m_isJumping) { p->m_position.m_y -= 2; p->m_onGround = false; p->m_isJumping = true;  }
		}
		if (p->m_isJumping)
		{
			p->m_isMoving = true;
			p->m_position.m_y -= 12;
			p->m_jumpHigh++;
			if (p->m_jumpHigh >= 12)
			{
				p->m_jumpHigh = 0;
				p->m_isJumping = false;
			}
		}

		if (!p->m_onGround)
		{
			p->m_position.m_y += 2;
			isCollide(p,1);
		}
		p->m_position.m_y++;
		p->m_onGround = false;
		isCollide(p, 1);
	}

	if (state == END_GAME)
	{
	}

}

void Game::isCollide(player* pl, int dir)
{
	for (int i = pl->m_position.m_y / 32; i < (pl->m_position.m_y + pl->m_height) / 32; i++)
		for (int j = pl->m_position.m_x / 32; j < (pl->m_position.m_x + pl->m_width) / 32; j++)
		{
			if (TileMap[i][j] == 'B')
			{
				if (pl->m_heading == true && dir == 0) pl->m_position.m_x = j * 32 - pl->m_width;
				if (pl->m_heading == false && dir == 0) pl->m_position.m_x = j * 32 + 32;
				if (!pl->m_isJumping && dir == 1) { pl->m_position.m_y = i * 32 - pl->m_height;  pl->m_onGround = true; }
				if (pl->m_isJumping && (dir == 1)) { pl->m_position.m_y = i * 32 + 32; }
			}

			if (TileMap[i][j] == '0')
			{
				TileMap[i][j] = ' ';
			}

		}
}

bool Game::isCollide(Entity *a, Entity *b)
{
	return (b->m_position.m_x - a->m_position.m_x)*(b->m_position.m_x - a->m_position.m_x) +
		(b->m_position.m_y - a->m_position.m_y)*(b->m_position.m_y - a->m_position.m_y) <
		(a->m_radius + b->m_radius)*(a->m_radius + b->m_radius);
}

bool Game::isCollideRect(Entity *a, Entity * b) {
	if (a->m_position.m_x < b->m_position.m_x + b->m_width &&
		a->m_position.m_x + a->m_width > b->m_position.m_x &&
		a->m_position.m_y < b->m_position.m_y + b->m_height &&
		a->m_height + a->m_position.m_y > b->m_position.m_y) {
		return true;
	}
	return false;
}

void Game::update()
{
	if (state == GAME)
	{
		for (auto i = entities.begin(); i != entities.end(); i++)
		{
			Entity *e = *i;

			e->update();
		}
		p->update();
	}

}

void Game::UpdateHiScores(int newscore)
{
	//new score to the end
	vhiscores.push_back(newscore);
	//sort
	sort(vhiscores.rbegin(), vhiscores.rend());
	//remove the last
	vhiscores.pop_back();
}

void Game::ReadHiScores()
{
	std::ifstream in("hiscores.dat");
	if (in.good())
	{
		std::string str;
		getline(in, str);
		std::stringstream ss(str);
		int n;
		for (int i = 0; i < 5; i++)
		{
			ss >> n;
			vhiscores.push_back(n);
		}
		in.close();
	}
	else
	{
		//if file does not exist fill with 5 scores
		for (int i = 0; i < 5; i++)
		{
			vhiscores.push_back(0);
		}
	}
}

void Game::WriteHiScores()
{
	std::ofstream out("hiscores.dat");
	for (int i = 0; i < 5; i++)
	{
		out << vhiscores[i] << " ";
	}
	out.close();
}

const int FPS = 60;
const int DELAY_TIME = 1000.0f / FPS;

int main(int argc, char* args[])
{
	srand(time(NULL));

	Uint32 frameStart, frameTime;

	std::cout << "game init attempt...\n";
	if (Game::Instance()->init("Platformer Fang", 100, 100, 600, 400,
		false))
	{
		std::cout << "game init success!\n";
		while (Game::Instance()->running())
		{
			frameStart = SDL_GetTicks(); //tiempo inicial

			Game::Instance()->handleEvents();
			Game::Instance()->update();
			Game::Instance()->render();

			frameTime = SDL_GetTicks() - frameStart; //tiempo final - tiempo inicial

			if (frameTime < DELAY_TIME)
			{
				//con tiempo fijo el retraso es 1000 / 60 = 16,66
				//procesar handleEvents, update y render tarda 1, y hay que esperar 15
				//cout << "frameTime : " << frameTime << "  delay : " << (int)(DELAY_TIME - frameTime) << endl;
				SDL_Delay((int)(DELAY_TIME - frameTime)); //esperamos hasta completar los 60 fps
			}
		}
	}
	else
	{
		std::cout << "game init failure - " << SDL_GetError() << "\n";
		return -1;
	}
	std::cout << "game closing...\n";
	Game::Instance()->clean();
	return 0;
}