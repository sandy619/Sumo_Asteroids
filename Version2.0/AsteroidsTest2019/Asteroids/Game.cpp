#include "Game.h"
#include "System.h"
#include "OrthoCamera.h"
#include "Background.h"
#include "Ship.h"
#include "Asteroid.h"
#include "Explosion.h"
#include "Keyboard.h"
#include "Random.h"
#include "Maths.h"
#include "Bullet.h"
#include "Collision.h"
#include<vector>
#include <algorithm>
#include "Graphics.h"
#include "FontEngine.h"


//std::vector<Bullet*>::iterator it;

Game::Game() :
	camera_(0),
	background_(0),
	player_(0),
	bullet_(0),
	collision_(0),
	bulletDelay(0),
	lives(3),
	score(0)
{
	camera_ = new OrthoCamera();
	camera_->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	camera_->SetFrustum(800.0f, 600.0f, -100.0f, 100.0f);
	background_ = new Background(800.0f, 600.0f);
	collision_ = new Collision();
}

Game::~Game()
{
	delete camera_;
	delete background_;
	delete player_;
	DeleteAllAsteroids();
	DeleteAllBullets();
	DeleteAllExplosions();
	delete collision_;
}

void Game::Update(System *system)
{
	UpdatePlayer(system);
	UpdateAsteroids(system);
	UpdateBullet(system);
	UpdateCollisions();
    BulletDelay();
}

void Game::RenderBackgroundOnly(Graphics *graphics)
{
	camera_->SetAsView(graphics);
	background_->Render(graphics);
}

void Game::RenderEverything(Graphics *graphics)
{
	camera_->SetAsView(graphics);

	background_->Render(graphics);

	if (player_)
	{
		player_->Render(graphics);
	}

	for (AsteroidList::const_iterator asteroidIt = asteroids_.begin(),
		end = asteroids_.end();
		asteroidIt != end;
		++asteroidIt)
	{
		(*asteroidIt)->Render(graphics);
	}

	// render bullets
	for ( auto it = bullet_.begin(); it != bullet_.end();++it)
	{
		(*it)->Render(graphics);
	}
	

	for (ExplosionList::const_iterator explosionIt = explosions_.begin(),
		end = explosions_.end();
		explosionIt != end;
		++explosionIt)
	{ 
		(*explosionIt)->Render(graphics);
	}

	
	//for score
	FontEngine* fontEngine = graphics->GetFontEngine();
	char levelStartText[256];
	sprintf_s(levelStartText, "Score :  %d", score);
	int textWidth = fontEngine->CalculateTextWidth(levelStartText, FontEngine::FONT_TYPE_MEDIUM);
	int textX = 100;
	int textY = 100;
	fontEngine->DrawText(levelStartText, textX, textY, 0xff00ffff, FontEngine::FONT_TYPE_MEDIUM);

	//for lives
	char livesText[256];
	sprintf_s(livesText, "Lives :  %d", lives);
	textWidth = fontEngine->CalculateTextWidth(livesText, FontEngine::FONT_TYPE_MEDIUM);
	 textX = 600;
	 textY = 100;
	fontEngine->DrawText(livesText, textX, textY, 0xff00ffff, FontEngine::FONT_TYPE_MEDIUM);
}

void Game::InitialiseLevel(int numAsteroids)
{
	DeleteAllAsteroids();
	DeleteAllExplosions();

	SpawnPlayer();
	SpawnAsteroids(numAsteroids);
}

bool Game::IsLevelComplete() const
{
	return (asteroids_.empty() && explosions_.empty());
}

bool Game::IsGameOver() const
{
	return (player_ == 0 && explosions_.empty());
}

void Game::DoCollision(GameEntity *a, GameEntity *b)
{
	Ship *player = static_cast<Ship *>(a == player_ ? a : (b == player_ ? b : 0));
	Bullet *bullet = static_cast<Bullet *>(IsBullet(a) ? a : (IsBullet(b) ? b : 0));
	Asteroid *asteroid = static_cast<Asteroid *>(IsAsteroid(a) ? a : (IsAsteroid(b) ? b : 0));

	if (player && asteroid)
	{
		--lives;
		player->Reset();
		AsteroidHit(asteroid);
		if (lives <= 0)
		{
			DeletePlayer();
			lives = 3;
			score = 0;
		}
	}

	if (bullet && asteroid)
	{
		++score;
		AsteroidHit(asteroid);
		DeleteBullet(bullet);
	}
}

void Game::SpawnPlayer()
{
	DeletePlayer();
	player_ = new Ship();
	player_->EnableCollisions(collision_, 10.0f);
}

void Game::DeletePlayer()
{
	delete player_;
	player_ = 0;
}

void Game::UpdatePlayer(System *system)
{
	if (player_ == 0)
		return;

	Keyboard *keyboard = system->GetKeyboard();

	float acceleration = 0.0f;
	if (keyboard->IsKeyHeld(VK_UP) || keyboard->IsKeyHeld('W'))
	{
		acceleration = 1.0f;
	}
	else if (keyboard->IsKeyHeld(VK_DOWN) || keyboard->IsKeyHeld('S'))
	{
		acceleration = -1.0f;
	}

	float rotation = 0.0f;
	if (keyboard->IsKeyHeld(VK_RIGHT) || keyboard->IsKeyHeld('D'))
	{
		rotation = -1.0f;
	}
	else if (keyboard->IsKeyHeld(VK_LEFT) || keyboard->IsKeyHeld('A'))
	{
		rotation = 1.0f;
	}

	player_->SetControlInput(acceleration, rotation);
	player_->Update(system);
	WrapEntity(player_);
	

	if (keyboard->IsKeyPressed(VK_NUMPAD0))
	{   
		if (bulletDelay > 40)
		{
			/*XMVECTOR playerForward = player_->GetForwardVector();
			XMVECTOR bulletPosition = player_->GetPosition() + playerForward * 10.0f;
			SpawnBullet(bulletPosition, playerForward);*/
			

			XMFLOAT3 pos(-0.5f, 1.0f, 0.0f);
			XMVECTOR playerForward = XMLoadFloat3(&pos);
			XMVECTOR bulletPosition = player_->GetPosition() + playerForward; //*10.0f;
			SpawnBullet(bulletPosition, playerForward);

			XMFLOAT3 pos1(0.0f, 1.0f, 0.0f);
			XMVECTOR playerForward1 = XMLoadFloat3(&pos1);
			XMVECTOR bulletPosition1 = player_->GetPosition() + playerForward1; //*10.0f;
			SpawnBullet(bulletPosition1, playerForward1);

			XMFLOAT3 pos2(0.5f, 1.0f, 0.0f);
			XMVECTOR playerForward2 = XMLoadFloat3(&pos2);
			XMVECTOR bulletPosition2 = player_->GetPosition() + playerForward2; //*10.0f;
			SpawnBullet(bulletPosition2, playerForward2);
			bulletDelay = 0;
		}
	}
}

void Game::UpdateAsteroids(System *system)
{
	for (AsteroidList::const_iterator asteroidIt = asteroids_.begin(),
		end = asteroids_.end();
		asteroidIt != end;
	++asteroidIt)
	{
		(*asteroidIt)->Update(system);
		WrapEntity(*asteroidIt);
	}
}

void Game::UpdateBullet(System *system)
{
	for (BulletList::const_iterator bIt = bullet_.begin(),
		end = bullet_.end();
		bIt != end;
		++bIt)
	{
		(*bIt)->Update(system);
	}
}

void Game::WrapEntity(GameEntity *entity) const
{
	XMFLOAT3 entityPosition;
	XMStoreFloat3(&entityPosition, entity->GetPosition());
	entityPosition.x = Maths::WrapModulo(entityPosition.x, -400.0f, 400.0f);
	entityPosition.y = Maths::WrapModulo(entityPosition.y, -300.0f, 300.0f);
	entity->SetPosition(XMLoadFloat3(&entityPosition));
}

void Game::DeleteAllAsteroids()
{
	for (AsteroidList::const_iterator asteroidIt = asteroids_.begin(),
		end = asteroids_.end();
		asteroidIt != end;
		++asteroidIt)
	{
		delete (*asteroidIt);
	}

	asteroids_.clear();
}

void Game::DeleteAllExplosions()
{
	for (ExplosionList::const_iterator explosionIt = explosions_.begin(),
		end = explosions_.end();
		explosionIt != end;
	++explosionIt)
	{
		delete (*explosionIt);
	}

	explosions_.clear();
}

void Game::DeleteAllBullets()
{
	explosions_.clear();
}

void Game::SpawnBullet(XMVECTOR position, XMVECTOR direction)
{
	//DeleteBullet();
	//bullet_ = new Bullet(position, direction);
	Bullet* bullet = new Bullet(position, direction);
	bullet->EnableCollisions(collision_, 3.0f);
	bullet_.push_back(bullet);
}

void Game::DeleteBullet(Bullet *bullet)
{
	bullet_.remove(bullet);
	delete bullet;
}

void Game::SpawnAsteroids(int numAsteroids)
{
	float halfWidth = 800.0f * 0.5f;
	float halfHeight = 600.0f * 0.5f;
	for (int i = 0; i < numAsteroids; i++)
	{
		float x = Random::GetFloat(-halfWidth, halfWidth);
		float y = Random::GetFloat(-halfHeight, halfHeight);
		XMVECTOR position = XMVectorSet(x, y, 0.0f, 0.0f);
		SpawnAsteroidAt(position, 3);
	}
}

void Game::SpawnAsteroidAt(XMVECTOR position, int size)
{
	const float MAX_ASTEROID_SPEED = 1.0f;

	float angle = Random::GetFloat(Maths::TWO_PI);
	XMMATRIX randomRotation = XMMatrixRotationZ(angle);
	XMVECTOR velocity = XMVectorSet(0.0f, Random::GetFloat(MAX_ASTEROID_SPEED), 0.0f, 0.0f);
	velocity = XMVector3TransformNormal(velocity, randomRotation);

	Asteroid *asteroid = new Asteroid(position, velocity, size);
	asteroid->EnableCollisions(collision_, size * 5.0f);
	asteroids_.push_back(asteroid);
}

bool Game::IsAsteroid(GameEntity *entity) const
{
	return (std::find(asteroids_.begin(),
		asteroids_.end(), entity) != asteroids_.end()); 
}

bool Game::IsBullet(GameEntity* entity) const
{
	return (std::find(bullet_.begin(),
		bullet_.end(), entity) != bullet_.end());
}

void Game::AsteroidHit(Asteroid *asteroid)
{
	int oldSize = asteroid->GetSize();
	if (oldSize > 1)
	{
		int smallerSize = oldSize -1;
		XMVECTOR diff = XMVectorSet(20.0f, 20.0f, 0.0f,0.0f);
		XMVECTOR position = asteroid->GetPosition();
		SpawnAsteroidAt(position, smallerSize);
		SpawnAsteroidAt(position+diff, smallerSize);
	}
	DeleteAsteroid(asteroid);
}

void Game::DeleteAsteroid(Asteroid *asteroid)
{
	asteroids_.remove(asteroid);
	delete asteroid;
}

void Game::UpdateCollisions()
{
	collision_->DoCollisions(this);
}

void Game::BulletDelay()
{
	++bulletDelay;
}
