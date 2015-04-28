
#include <cmath>
#include <stack>
#include <list>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <soil/SOIL.h>
#include <glbinding/gl/gl.h>
#include <fmod/fmod_studio.hpp>
#include <fmod/fmod.hpp>
#include <fmod/fmod_errors.h>
#include "gameloop.h"
#include "utils/textureloader.h"
#include "entity/entity.h"
#include "terrain/terraindata.h"
#include "render/terrainrenderer.h"
#include "graphics/rendersettingshelper.h"
#include "utils/timehelper.h"
#include "terrain/flatterrain.h"
#include "graphics/gluhelper.h"
#include "render/render.h"
#include "terrain/grass.h"
#include "graphics/gluhelper.h"
#include "utils/objparser.h"
#include "render/glfont.h"
#include "render/ui.h"
#include "entity/projectile.h"
#include "render/sphere.h"
#include "graphics/tree.h"
#include "math/gamemath.h"
#include "utils/fileutils.h"
#include "utils/random.h"
#include "entity/enemy.h"
#include "entity/player.h"
#include "render/menu.h"

///***********************************************************************
///***********************************************************************
/// Start internal API declaration
///***********************************************************************
///***********************************************************************
///
/// Initialization functions
///
void initializeEngine();
///
/// Input functions
///
void processKeyboardInput();
void processMouseInput();
///
/// Game update/rendering functions
///
void gameUpdateTick();
///
/// Define the KeyManager class. Because GLUT is a C library, unfortunately we have to write this is a fairly C-like style.
///
/// Special Usage Note: KeyManager class has some problems with the isShiftDown, isControlDown, and isAltDown fields not updating. The state of these keys cannot be queried until another
/// key event happens [this is an OS restriction - there might be a workaround?]. Their state only updates when another key is pressed or released.
///
void keyManagerKeyPressed(unsigned char key, int x, int y);
void keyManagerKeyUp (unsigned char key, int x, int y);
void keyManagerKeySpecial(int key, int x, int y);
void keyManagerKeySpecialUp(int key, int x, int y);
void updateModifierState();
///
/// Define the MouseManager class. Similarly to KeyManager this is in a fairly C-like style because GLUT is a C library.
///
void mouseManagerHandleMouseClick(int button, int state, int x, int y);
void mouseManagerHandleMouseMovementWhileClicked(int x,int y);
void mouseManagerHandleMouseMovementWhileNotClicked(int x, int y);
///
/// Level class
///

class Level
{
public:
	std::vector<std::shared_ptr<Enemy>> enemies;
	AABB worldBounds;
	std::shared_ptr<TerrainRenderer> terrainRenderer;
	Level();
	virtual void createLevel() = 0;
	virtual void update(float deltaTime) = 0;
	virtual void draw(Camera *cam, float deltaTime) = 0;
	virtual void drawTerrain(Camera *cam) = 0;
};

class ForestLevel : public Level
{
public: 
	std::list<Tree> trees;
	std::shared_ptr<Grass> grass;
	ForestLevel();
	~ForestLevel();
	void createLevel() override;
	void update(float deltaTime) override;
	void draw(Camera* cam, float deltaTime) override;
	void drawTerrain(Camera *cam);
};

class DesertLevel : public Level
{
public:
	DesertLevel();
	void createLevel() override;
	void update(float deltaTime) override;
	void draw(Camera* cam, float deltaTime) override;
	void drawTerrain(Camera *cam);
};

///
/// Define the GameLoop class.
///
class GameLoop
{
public:
    const int GAME_TICKS_PER_SECOND = 60;
    bool gameIsRunning;
    Player player;
    std::shared_ptr<TerrainData> terrain;
    long startTime;
    
    KeyManager keyManager;
    MouseManager mouseManager;
  
    std::shared_ptr<Model> treeModel;
	std::shared_ptr<Model> gunModel;
	std::shared_ptr<Model> zombieModel;
	std::shared_ptr<Model> zombieModel2;
	std::shared_ptr<GLFont> fontRenderer;
	unsigned long long previousFrameTime;
	float deltaTime;
	std::shared_ptr<Texture> ammoTexture;
	std::shared_ptr<Texture> medkitTexture;
	std::shared_ptr<Texture> gunTexture;
	std::shared_ptr<Texture> logo;
	std::shared_ptr<Texture> sliderTexture;
	std::vector<std::shared_ptr<Projectile>> projectiles;
	FMOD::Studio::System* system = NULL;
	FMOD::Sound *music;
	FMOD::Channel* musicChannel;
	FMOD::Studio::EventInstance* eventInstance;
	FMOD::Studio::EventInstance* bgmInstance;
	FMOD::Studio::EventInstance* hurtInstance;
	bool hasStartedBGM = false;
	std::shared_ptr<Texture> skyboxTextureForest;
	std::shared_ptr<Texture> skyboxTextureDesert;
	std::stack<std::shared_ptr<Menu>> menus;
	std::shared_ptr<Texture> startDesertButtonTexture;
	std::shared_ptr<Texture> startForestButtonTexture;
	std::shared_ptr<Texture> helpButtonTexture;
	std::shared_ptr<Texture> optionsButtonTexture;
	std::shared_ptr<Texture> backButtonTexture;
	std::shared_ptr<Texture> helpTexture;
	std::shared_ptr<Texture> gameOverTexture;
	std::shared_ptr<Menu> mainMenu;
	std::shared_ptr<Texture> terrainTextureGrass;
	std::shared_ptr<Texture> terrainTextureSand;	
	std::shared_ptr<Level> activeLevel;
	float volume;
	std::shared_ptr<Shader> genericTextureShader;

    GameLoop();
	~GameLoop();
	void loadWithGLContext();
	void update();
	void endOfTick();
	void collisionCheck();
	void loadModels();
	float getDeltaTime();
	///
    /// Draws a basic text string.
    /// \param val - the string of text to draw
    /// \param x - the x coordinate of the text
    /// \param y - the y coordinate of the text
    /// \param z - the z coordinate of the text. Leave 0 when rendering in 2D.
    /// \param colour - the colour the text will be rendered.
    ///
    bool drawString(std::string val, float x, float y, float z, Colour colour);
};

class GLState
{
public:
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 model; 

	/// Loads the identity matrix into the model matrix
	void loadIdentity();
	void translate(float x, float y, float z);
	void rotate(float angle, float x, float y, float z);
	void update();
};

///***********************************************************************
///***********************************************************************
/// End internal API declaration
///***********************************************************************
///***********************************************************************

///***********************************************************************
///***********************************************************************
/// GameLoop "global" object for shared state between these functions.
///***********************************************************************
///***********************************************************************
GameLoop gameLoopObject;
GLState glState;
///***********************************************************************
///***********************************************************************
/// Define initialization functions
///***********************************************************************
///***********************************************************************

///
/// this is an error handling function for FMOD errors
/// 
void ERRCHECK(FMOD_RESULT result)        
{
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s", result, FMOD_ErrorString(result));
		exit(-1);
	}
}

void initializeEngine()
{
    using namespace gl;    
    initializeViewport();
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    //Create font
	GLuint textureName;
	gl::glGenTextures(1, &textureName);
	gameLoopObject.fontRenderer = std::shared_ptr<GLFont>(new GLFont());
	try
	{
		gameLoopObject.fontRenderer->Create(getTexture(buildPath("res/font.png")));
		gameLoopObject.ammoTexture = getTexture(buildPath("res/ammo_icon.png"));
		gameLoopObject.medkitTexture = getTexture(buildPath("res/medkit.png"));
	}
	catch(GLFontError::InvalidFile)
	{
		std::cout << "Cannot load font" << std::endl;
		exit(1);
	}    

	// Init the sound engine (FMOD)
	FMOD::Studio::System* system = NULL;
	ERRCHECK(FMOD::Studio::System::create(&system));
	gameLoopObject.system = system;

	// The example Studio project is authored for 5.1 sound, so set up the system output mode to match
	FMOD::System* lowLevelSystem = NULL;
	ERRCHECK(system->getLowLevelSystem(&lowLevelSystem));
	ERRCHECK(lowLevelSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));
	ERRCHECK(system->initialize(32, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, NULL));

	FMOD::Studio::Bank* masterBank = NULL;
	ERRCHECK(system->loadBankFile(buildPath("res/audio/Master Bank.bank").c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank));
	FMOD::Studio::Bank* stringsBank = NULL;
	ERRCHECK(system->loadBankFile(buildPath("res/audio/Master Bank.strings.bank").c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank));
	masterBank->loadSampleData();
	stringsBank->loadSampleData();

	bool done = false;
	while (!done)
	{		
		FMOD_STUDIO_LOADING_STATE val1;
		FMOD_STUDIO_LOADING_STATE val2;
		masterBank->getLoadingState(&val1);
		stringsBank->getLoadingState(&val2);
		if (val1 == FMOD_STUDIO_LOADING_STATE_LOADED && val2 == FMOD_STUDIO_LOADING_STATE_LOADED)
		{
			done = true;
		}
	}


	FMOD::Studio::EventDescription* eventDescription = NULL;
	ERRCHECK(system->getEvent("event:/pistol-01", &eventDescription));
	gameLoopObject.eventInstance = NULL;
	ERRCHECK(eventDescription->createInstance(&gameLoopObject.eventInstance));	
	
	FMOD::Studio::EventDescription* bgmDescription = NULL;
	ERRCHECK(system->getEvent("event:/bgm-battle-01", &bgmDescription));
	gameLoopObject.bgmInstance = NULL;
	ERRCHECK(bgmDescription->createInstance(&gameLoopObject.bgmInstance));

	FMOD::Studio::EventDescription* hurtDescription = NULL;
	ERRCHECK(system->getEvent("event:/hurt1", &hurtDescription));
	gameLoopObject.hurtInstance = NULL;
	ERRCHECK(hurtDescription->createInstance(&gameLoopObject.hurtInstance));

	// Position the listener at the origin
	FMOD_3D_ATTRIBUTES attributes = { { 0 } };
	attributes.forward.z = 1.0f;
	attributes.up.y = 1.0f;
	ERRCHECK(system->setListenerAttributes(&attributes));

	// Position the event 2 units in front of the listener
	attributes.position.z = 2.0f;
	ERRCHECK(gameLoopObject.eventInstance->set3DAttributes(&attributes));

	// Build the terrain
	gameLoopObject.loadWithGLContext();
}

///
/// GLState declarations
///
void GLState::update()
{
	Camera *camera = gameLoopObject.player.getCamera();
	proj = buildProjectionMatrix(53.13f, getAspectRatio(), 0.1f, 1000.0f);
	view = createLookAtMatrix(
		camera->position,
		//Reference point
		glm::vec3(
			camera->position.x + sin(camera->rotation.y),
			camera->position.y - sin(camera->rotation.x),
			camera->position.z - cos(camera->rotation.y)
		),
		//Up Vector
		glm::vec3(
			0,
			cos(camera->rotation.x),
			0
		)
	);
	model = MATRIX_IDENTITY_4D;
}

void GLState::translate(float x, float y, float z)
{
	model = construct3DTranslationMatrix(x, y, z) * model;
}

void GLState::loadIdentity()
{
	model = MATRIX_IDENTITY_4D;
}

void GLState::rotate(float angle, float x, float y, float z)
{
	if (approximatelyEqual(x, 1.0f))
	{
		model = construct3dRotationMatrixOnX(rad(angle)) * model;
	}
	else if (approximatelyEqual(y, 1.0f))
	{
		model = model * construct3dRotationMatrixOnY(rad(angle));
	}
	else if (approximatelyEqual(z, 1.0f))
	{
		model = construct3dRotationMatrixOnZ(rad(angle)) * model;
	}
	else
	{
		throw std::invalid_argument("Rotation not supported");
	}
}

///***********************************************************************
///***********************************************************************
/// Define the GameLoop class methods.
///***********************************************************************
///***********************************************************************
GameLoop::GameLoop() : gameIsRunning(true), player(Player(Camera(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)))), startTime(getCurrentTimeMillis()),
	previousFrameTime(getCurrentTimeMillis()), volume(0.5f)
{
	// Important usage note: a GL Context is not bound when this constructor is called. Using any gl functions with cause a segfault or crash.
	player.setCamera(Camera(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)));
	player.boundingBox = AABB(0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f);
}

GameLoop::~GameLoop()
{
	if (system)
	{
		ERRCHECK(system->release());
	}
}

void GameLoop::loadModels()
{
	// Load the tree model
	ObjParser parser(
		buildPath("res/models/pine_tree1/"), buildPath("res/models/pine_tree1/Tree.obj"),
		"Branches0018_1_S.png", false);
	treeModel = parser.exportModel();
	auto treeTexture = getTexture(buildPath("res/models/pine_tree1/BarkDecidious0107_M.jpg"));
	auto branchTexture = getTexture(buildPath("res/models/pine_tree1/Branches0018_1_S.png"));
	std::map<std::string, std::shared_ptr<Texture>> textures;
	textures["tree"] = treeTexture;
	textures["leaves"] = branchTexture;
	treeModel->createVBOs(genericTextureShader, textures);

	// Load the gun model
	parser = ObjParser(
		buildPath("res/models/gun/"), buildPath("res/models/gun/M9.obj"),
		"", true);
	gunModel = parser.exportModel();
	auto Handgun_D = getTexture(buildPath("res/models/gun/Tex_0009_1.jpg"));
	gunTexture = Handgun_D;
	textures = std::map<std::string, std::shared_ptr<Texture>>();
	textures["Tex_0009_1"] = Handgun_D;
	gunModel->createVBOs(genericTextureShader, textures);
	for (std::shared_ptr<TexturedNormalColouredVAO> &vao : gunModel->vaos)
	{
		vao->tex = Handgun_D;
	}
	gunModel->generateAABB();

	// Load the zombie
	parser = ObjParser(buildPath("res/models/zombie/"), buildPath("res/models/zombie/Lambent_Male.obj"), "", true);
	zombieModel = parser.exportModel();
	auto _D = getTexture(buildPath("res/models/zombie/Lambent_Male_D.png"));
	auto _E = getTexture(buildPath("res/models/zombie/Lambent_Male_E.tga"));
	auto _N = getTexture(buildPath("res/models/zombie/Lambent_Male_N.tga"));
	auto _S = getTexture(buildPath("res/models/zombie/Lambent_Male_S.tga"));
	textures = std::map<std::string, std::shared_ptr<Texture>>();
	textures["Lambent_Male_D.tga"] = _D;
	textures["Lambent_Male_E.tga"] = _E;
	textures["Lambent_Male_N.tga"] = _N;
	textures["Lambent_Male_S.tga"] = _S;
	zombieModel->createVBOs(genericTextureShader, textures);
	for (std::shared_ptr<TexturedNormalColouredVAO> &vao : zombieModel->vaos)
	{
		vao->tex = _D;
	}
	zombieModel->generateAABB();

	// Load the second zombie
	parser = ObjParser(buildPath("res/models/zombie2/"), buildPath("res/models/zombie2/Lambent_Female.obj"), "", true);
	zombieModel2 = parser.exportModel();
	auto __D = getTexture(buildPath("res/models/zombie2/Lambent_Female_D.png"));
	textures = std::map<std::string, std::shared_ptr<Texture>>();
	textures["Lambent_Female_D.tga"] = __D;
	zombieModel2->createVBOs(genericTextureShader, textures);
	for (std::shared_ptr<TexturedNormalColouredVAO> &vao : zombieModel2->vaos)
	{
		vao->tex = __D;
	}
	zombieModel2->generateAABB();
}

void GameLoop::loadWithGLContext()
{
	// Generic texture shader.
	std::string vertexShaderPath = buildPath("res/shaders/3d_standard_shader.vert");
	std::string fragmentShaderPath = buildPath("res/shaders/3d_standard_shader.frag");
	genericTextureShader = createShader(&vertexShaderPath, &fragmentShaderPath);

	loadModels();
	int i = 0;
	skyboxTextureDesert = getTexture(buildPath("res/skybox_desert.png"));
	skyboxTextureForest = getTexture(buildPath("res/skybox_texture.jpg"));
	terrainTextureGrass = getTexture(buildPath("res/grass1.png"));
	terrainTextureSand = getTexture(buildPath("res/sand1.png"));	
	logo = getTexture(buildPath("res/logo.png"));
	gameOverTexture = getTexture(buildPath("res/game_over.png"));
	sliderTexture = getTexture(buildPath("res/volume.png"));
	// Create the menu(s)
	startDesertButtonTexture = getTexture(buildPath("res/button_start.png"));
	startForestButtonTexture = getTexture(buildPath("res/button_start2.png"));
	helpButtonTexture = getTexture(buildPath("res/button_help.png"));
	optionsButtonTexture = getTexture(buildPath("res/button_options.png"));
	backButtonTexture = getTexture(buildPath("res/button_back.png"));
	helpTexture = getTexture(buildPath("res/help.png"));
	auto backButtonTexture = this->backButtonTexture;
	auto helpTexture = this->helpTexture;	
	auto sliderTexture = this->sliderTexture;
	float *volume = &this->volume;
	std::shared_ptr<Level> *activeLevel = &this->activeLevel;
	mainMenu = std::shared_ptr<Menu>(new MainMenu(startDesertButtonTexture, startForestButtonTexture, helpButtonTexture, optionsButtonTexture,
		[activeLevel]()
		{
			// DesertEvt
			if (!(*activeLevel))
			{
				*activeLevel = std::shared_ptr<Level>(new DesertLevel());
				(*activeLevel)->createLevel();
			}
		},
		[activeLevel]()
		{
			// forestEvt
			if (!(*activeLevel))
			{
				*activeLevel = std::shared_ptr<Level>(new ForestLevel());
				(*activeLevel)->createLevel();
			}
		},
		[backButtonTexture, helpTexture]()
		{
			// helpEvn
			gameLoopObject.menus.push(std::shared_ptr<Menu>(new HelpMenu(backButtonTexture, helpTexture)));
		},
		[backButtonTexture, sliderTexture, volume]()
		{
			// optionsEvn
			gameLoopObject.menus.push(std::shared_ptr<Menu>(new OptionsMenu(backButtonTexture, sliderTexture, *volume,
				[volume](float value){
					*volume = value;
				}
			)));
		}, 
		logo
	));
	menus.push(mainMenu);

	

}

bool GameLoop::drawString(std::string val, float x, float y, float z, Colour colour)
{
    using namespace gl;
    try
    {
        glEnable(GL_TEXTURE_2D);
        gl::glColor4f(colour.r, colour.g, colour.b, colour.a);
        glDisable(GL_BLEND);
        glAlphaFunc(GL_GREATER, 0.1f);
        glEnable(GL_ALPHA_TEST);
		std::stringstream ss;
		ss << "score:" << player.score;
        fontRenderer->TextOut(ss.str(), 20, 20, 0);
        return true;
    }
    catch(GLFontError::InvalidFont)
    {
        return false;
    }
    return false;
}

float GameLoop::getDeltaTime()
{
	return deltaTime;
}

void GameLoop::update()
{
	system->update();
	unsigned long long currentTime = getCurrentTimeMillis();
	unsigned long long deltaTimeMillis = currentTime - previousFrameTime;
	this->deltaTime = static_cast<float>(deltaTimeMillis) / 1000.0f;
	previousFrameTime = currentTime;
	keyManager.update();
	// We need to grab the mouse during the game to enable mouse based turning, but free it if the menu is open so the user can click things
	// So, if there's a menu just free the mouse at the start of the frame, otherwise grab it.
	if (menus.size() > 0)
	{
		gameLoopObject.mouseManager.setGrabbed(false);		
	}
	else
	{
		gameLoopObject.mouseManager.setGrabbed(true);
	}

	if (player.isDead() && activeLevel)
	{
		activeLevel = nullptr;
		menus.push(mainMenu);
		menus.push(std::shared_ptr<Menu>(new GameOverMenu(backButtonTexture, gameOverTexture, player.score, fontRenderer)));
		bgmInstance->stop(FMOD_STUDIO_STOP_MODE::FMOD_STUDIO_STOP_IMMEDIATE);
		activeLevel = nullptr;
	}
	if (activeLevel && menus.size() > 0)
	{
		while (menus.size() > 0)
		{
			menus.pop();
		}
	}

	if (activeLevel)
	{
		if (!hasStartedBGM)
		{
			hasStartedBGM = true;
			ERRCHECK(bgmInstance->start());
		}
		else
		{
			FMOD_STUDIO_PLAYBACK_STATE bgmState;
			bgmInstance->getPlaybackState(&bgmState);
			if (bgmState == FMOD_STUDIO_PLAYBACK_STOPPED)
			{
				ERRCHECK(bgmInstance->start());
			}
		}
		bgmInstance->setVolume(volume);
		eventInstance->setVolume(volume);
		hurtInstance->setVolume(volume);
		// Check collisions
		activeLevel->update(this->deltaTime);
		player.update(activeLevel->worldBounds, deltaTime);
		collisionCheck();
	}
}

void GameLoop::endOfTick()
{
	mouseManager.update();
}

void GameLoop::collisionCheck()
{
	std::vector<std::shared_ptr<Enemy>> &enemies = activeLevel->enemies;
	// Player - monster collision
	for (int i = 0; i < enemies.size(); i++)
	{
		std::shared_ptr<Enemy> enemy = enemies[i];
		if (intersects(enemy->getAABB(), player.getAABB()))
		{
			if (!player.isInvincible())
			{
				player.hurtPlayer(20);
				gameLoopObject.hurtInstance->start();
			}
		}
	}	

	// Player's bullet vs enemy collision test. 
	for (int j = 0; j < projectiles.size(); j++)
	{
		std::vector<std::shared_ptr<Enemy>> hits;
		for (int i = 0; i < enemies.size(); i++)
		{
			// Capsule variables
			LineSegment3 seg = projectiles[j]->getMovement();
			float radius = projectiles[j]->size;
			Capsule3D capsule(seg.point1, seg.point2, radius);
			// Check for overlap with a lazy overlap test that misses some [in this case trivial] cases.	
			if (brokenIntersection(enemies[i]->boundingBox, capsule))
			{
				hits.push_back(enemies[i]);
			}
		}
		if (hits.size() == 1)
		{
			hits[0]->hurt(20);
			projectiles.erase(projectiles.begin() + j);
			j--;
			continue;
		}
		else if (hits.size() > 1&&false)
		{
			int closest = 0;
			float closestLength = 10000;
			glm::vec3 head = player.boundingBox.center();
			for (int i = 0; i < hits.size(); i++)
			{
				glm::vec3 tail = enemies[i]->boundingBox.center();
				glm::vec3 result = head - tail;
				float length = glm::length(result);
				if (length < closestLength)
				{
					closest = i;
				}
			}
			hits[closest]->hurt(20);

			projectiles.erase(projectiles.begin() + j);
			j--;
			continue;
		}
	}

	for (int i = 0; i < enemies.size(); i++)
	{
		if (enemies[i]->isDead())
		{
			player.score += 1;
			player.ammoCount += 5;
			// 5% chance to get a healing item
			float chance = 0.05f;
			float f = getRandomFloat();
			if (f < chance)
			{
				player.healingItemCount += 1;
			}

			enemies.erase(enemies.begin() + i);
			i--;
			continue;
		}
	}
}

///
/// Define methods to load the levels
///

Level::Level() : worldBounds(AABB(0, 0, 0, 0, 0, 0))
{
}

ForestLevel::ForestLevel() : Level()
{
}

ForestLevel::~ForestLevel()
{
}

void ForestLevel::createLevel()
{
	// Generate a forest level
	// Create the terrain	
	std::shared_ptr<Terrain> terrain(new FlatTerrain(200));
	worldBounds = AABB(-100, 0, -100, 60, 50, 60);
	auto tex = getTexture(buildPath("res/grass1.png"));
	auto terrainExp = terrain->exportToTerrainData();
	this->terrainRenderer = std::shared_ptr<TerrainRenderer>(new TerrainRenderer(gameLoopObject.genericTextureShader, terrainExp, tex));
	// Create the grass
	auto grassTexture = getTexture(buildPath("res/grass_1.png"));
	int grassDensity = (getRandomInt(1000) + 300) * 7;
	this->grass = std::shared_ptr<Grass>(new Grass(grassDensity, glm::vec3(-20, 0, -20), glm::vec3(2.0f, 0, 2.0f), 80, grassTexture));
	//Generate some trees.
	for (int i = 0; i < 15; i++)
	{
		int x = getRandomInt(70) - 35;
		int y = 0;
		int z = getRandomInt(70) - 35;

		int retryCounter = 0;
		while (retryCounter < 20)
		{
			bool success = true;
			for (Tree &tree : trees)
			{
				float distanceSquared = square(tree.x - x) + square(tree.y - y);
				if (distanceSquared < 25)
				{
					success = false;
					break;
				}
			}
			retryCounter++;
			if (success)
			{
				trees.push_back(Tree(gameLoopObject.treeModel, x, y, z));
				retryCounter += 1;
			}
		}
	}
	gameLoopObject.projectiles.clear();
	gameLoopObject.player.reset();
}

void ForestLevel::update(float deltaTime)
{
	for (std::shared_ptr<Enemy> enemy : enemies)
	{
		enemy->onGameTick(gameLoopObject.player, deltaTime, worldBounds);
	}
	grass->update();

	double chance = 0.30 * static_cast<double>(deltaTime);
	double f = static_cast<double>(getRandomFloat());
	if (f < chance)
	{
		// Try to spawn an enemy
		std::shared_ptr<Enemy> enemy(new Enemy(
			gameLoopObject.zombieModel,
			Camera(glm::vec3(static_cast<float>(getRandomInt(80) - 60), 0, static_cast<float>(getRandomInt(80) - 60)),
			glm::vec3(0, 0, 0)))
			);
		enemy->boundingBox = AABB(0, 0, 0, 1, 1, 1);
		enemy->speedModifier *= 1.25f;
		enemies.push_back(enemy);
	}
}

void ForestLevel::drawTerrain(Camera *cam)
{
	drawSkybox(gameLoopObject.genericTextureShader, gameLoopObject.skyboxTextureForest, cam);
	terrainRenderer->draw();
}

void ForestLevel::draw(Camera* cam, float deltaTime)
{
	using namespace gl;
	/// tree
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisable(GL_BLEND);
	glAlphaFunc(GL_GREATER, 0.1f);
	glEnable(GL_ALPHA_TEST);
	gl::glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	setLookAt(cam);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	for (Tree &tree : trees)
	{
		tree.draw(cam);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	/// end tree
		
	grass->draw(cam);

	// draw enemies
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisable(GL_BLEND);
	glAlphaFunc(GL_GREATER, 0.1f);
	glEnable(GL_ALPHA_TEST);
	gl::glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	setLookAt(cam);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	for (std::shared_ptr<Enemy> enemy : enemies)
	{
		enemy->draw(cam);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	// End draw enemies
}

DesertLevel::DesertLevel() : Level()
{
}

void DesertLevel::drawTerrain(Camera *cam)
{
	drawSkybox(gameLoopObject.genericTextureShader, gameLoopObject.skyboxTextureDesert, cam);
	terrainRenderer->draw();
}

void DesertLevel::createLevel()
{
	std::shared_ptr<Terrain> terrain(new FlatTerrain(200));
	worldBounds = AABB(-100, 0, -100, 60, 50, 60);
	auto tex = getTexture(buildPath("res/sand1.png"));
	auto terrainExp = terrain->exportToTerrainData();
	this->terrainRenderer = std::shared_ptr<TerrainRenderer>(new TerrainRenderer(gameLoopObject.genericTextureShader, terrainExp, tex));
	gameLoopObject.projectiles.clear();
	gameLoopObject.player.reset();
}

void DesertLevel::update(float deltaTime)
{
	for (std::shared_ptr<Enemy> enemy : enemies)
	{
		enemy->onGameTick(gameLoopObject.player, deltaTime, worldBounds);
	}
	double chance = 0.225 * static_cast<double>(deltaTime);
	double f = static_cast<double>(getRandomFloat());
	if (f < chance)
	{
		// Try to spawn an enemy
		std::shared_ptr<Enemy> enemy(new Enemy(
			gameLoopObject.zombieModel2,
			Camera(glm::vec3(static_cast<float>(getRandomInt(80) - 60), 0, static_cast<float>(getRandomInt(80) - 60)),
			glm::vec3(0, 0, 0)))
			);
		enemy->boundingBox = AABB(0, 0, 0, 1, 1, 1);
		enemies.push_back(enemy);
	}
}

void DesertLevel::draw(Camera* cam, float deltaTime)
{
	using namespace gl;
	// draw enemies
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	for (std::shared_ptr<Enemy> enemy : enemies)
	{
		enemy->draw(cam);
	}
}

///***********************************************************************
///***********************************************************************
/// Define Gameloop/render functions.
///***********************************************************************
///***********************************************************************

void drawLaserSight()
{
	/*
	Camera *cam = gameLoopObject.player.getCamera();
	glm::vec3 forward(
		cam->position.x + sin(cam->rotation.y),
		cam->position.y - sin(cam->rotation.x),
		cam->position.z - cos(cam->rotation.y)
		);
	forward = glm::normalize(forward);
	glm::vec3 up = glm::vec3(0, 1, 0);
	glm::vec3 left = glm::normalize(glm::cross(forward, up));

	glm::vec3 lookAt = glm::normalize(glm::vec3(
		sin(cam->rotation.y * -1.0f),
		0,
		cos(cam->rotation.y * -1.0f)
		)) * -1.0f;
	AABB gunbox = gameLoopObject.gunModel->getAABB();
	float yDelta = gunbox.yMax - gunbox.yMin;
	lookAt = lookAt + lookAt * yDelta;
	
	glm::vec3 start = (gameLoopObject.player.getPosition() + lookAt + glm::vec3(0.025f, -0.05f, 0.0f));
	glm::vec3 end = start + (lookAt * 200.0f);




	using namespace gl;
	gl::glPushMatrix();
	gl::glLoadIdentity();
	setLookAt(cam);
	glDisable(GL_TEXTURE_2D);

	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	gl::glVertex3f(start.x, start.y, start.z);
	gl::glVertex3f(end.x, end.y, end.z);
	glEnd();

	glPopMatrix();
	*/
}

void gameUpdateTick()
{
    using namespace gl;	
	float deltaTime = gameLoopObject.getDeltaTime();
	gameLoopObject.update();
	glState.update();
    if (gameLoopObject.menus.size() > 0)
	{
		// Draw the menu
		startRenderCycle();
		start2DRenderCycle();

		std::shared_ptr<Menu> m = gameLoopObject.menus.top();
		m->update(&gameLoopObject.mouseManager, deltaTime);
		m->draw(deltaTime);
		if (m->shouldPopThisMenu())
		{
			gameLoopObject.menus.pop();
		}

		gameLoopObject.genericTextureShader->releaseShader();
		end2DRenderCycle();
		endRenderCycle();
		gameLoopObject.endOfTick();
		return;
	}

	processKeyboardInput();
	processMouseInput();

    //Draw here
    startRenderCycle();
	gl::glClearDepth(1.0f);
//	glEnable(GL_DEPTH_TEST);
//	glDepthFunc(GL_LEQUAL);

    Camera *cam = gameLoopObject.player.getCamera();
    startRenderCycle();
    start3DRenderCycle();
	//renderAxes(cam);
	
	
	glState.loadIdentity();
	gameLoopObject.genericTextureShader->bindShader();
	gameLoopObject.genericTextureShader->glUniform1("texture1", 0);
	gameLoopObject.genericTextureShader->glUniformMatrix4("modelMatrix", gl::GL_FALSE, glState.model);
	gameLoopObject.genericTextureShader->glUniformMatrix4("projMatrix", gl::GL_FALSE, glState.proj);
	gameLoopObject.genericTextureShader->glUniformMatrix4("viewMatrix", gl::GL_FALSE, glState.view);
	
	gameLoopObject.activeLevel->drawTerrain(cam);
	
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glDisable(GL_TEXTURE_2D);
	for (int i = 0; i < gameLoopObject.projectiles.size(); )
	{
		std::shared_ptr<Projectile> p = gameLoopObject.projectiles.at(i);
		p->onGameTick(deltaTime);
			
		if (p->getY() < -p->size)
		{
			gameLoopObject.projectiles.erase(gameLoopObject.projectiles.begin() + i);
		}
		else
		{
			p->draw();
			i++;
		}
	}

	//glPopMatrix();
	glEnable(GL_TEXTURE_2D);

	//gameLoopObject.activeLevel->draw(cam, deltaTime);


	// Draw the player's gun
	glDisable(GL_CULL_FACE);

	//glMatrixMode(GL_MODELVIEW);
	//glPushMatrix();
	glState.loadIdentity();
	//setLookAt(cam);
	
	glm::vec3 lookAt = glm::normalize(glm::vec3(
		+ sin(cam->rotation.y),
		- sin(cam->rotation.x),
		- cos(cam->rotation.y)
	));
	glm::vec3 offset = lookAt;
//	glState.translate(cam->position.x + offset.x, cam->position.y + offset.y - 0.2f, cam->position.z + offset.z);
	glState.translate(1.0f * (cam->position.x + offset.x), 1.0f * (cam->position.y + offset.y), 1.0f * (cam->position.z + offset.z));
	glm::vec3 forward(
		cam->position.x + sin(cam->rotation.y),
		cam->position.y - sin(cam->rotation.x),
		cam->position.z - cos(cam->rotation.y)
		);
	glm::vec3 up(0, cos(cam->rotation.x), 0);
	glm::vec3 left = glm::cross(up, forward);
	//glState.rotate(deg(-cam->rotation.y), 0, 1, 0);
	glEnable(GL_TEXTURE_2D);
	gameLoopObject.genericTextureShader->glUniformMatrix4("modelMatrix", gl::GL_FALSE, glState.model);
	gameLoopObject.gunModel->draw(cam);	
	//glPopMatrix();
	// End gun draw
	
	end3DRenderCycle();

    start2DRenderCycle();
	drawUI(gameLoopObject.player, gameLoopObject.mouseManager, gameLoopObject.fontRenderer, gameLoopObject.ammoTexture, gameLoopObject.medkitTexture);
    end2DRenderCycle();
    endRenderCycle();
	gameLoopObject.endOfTick();
}

///***********************************************************************
///***********************************************************************
/// Define Core Input Functions
///***********************************************************************
///***********************************************************************
#include <GL/freeglut.h>
#include "graphics/windowhelper.h"
void processKeyboardInput()
{
	float deltaTime = gameLoopObject.getDeltaTime();
    // to check for a key modifier use:
    //       int glutGetModifiers(void);
    // The return value for this function is either one of three predefined constants or any bitwise OR combination of them. The constants are:
    //       GLUT_ACTIVE_SHIFT - Set if either you press the SHIFT key, or Caps Lock is on. Note that if they are both on then the constant is not set.
    //       GLUT_ACTIVE_CTRL - Set if you press the CTRL key.
    //       GLUT_ACTIVE_ALT - Set if you press the ALT key.
    Camera *camera = gameLoopObject.player.getCamera();
    KeyManager *manager = &gameLoopObject.keyManager;
   
    if (manager->getKeyState('=') == KeyManager::PRESSED) 
	{
		exit(0);
	}

    if(manager->getKeyState('w') == KeyManager::PRESSED)
    {
		gameLoopObject.player.accel(
			glm::vec3(
				sin(gameLoopObject.player.getCamera()->rotation.y),
				0,
				-cos(gameLoopObject.player.getCamera()->rotation.y)
			) * deltaTime * 3.8f * -1.0f
		);
    }
    if(manager->getKeyState('s') == KeyManager::PRESSED)
    {
        gameLoopObject.player.accel(
			glm::vec3(
				-sin(gameLoopObject.player.getCamera()->rotation.y),
                0,
                cos(gameLoopObject.player.getCamera()->rotation.y)
			) * deltaTime * 2.6f * -1.0f
		);
    }
    if(manager->getKeyState('a') == KeyManager::PRESSED)
    {
        gameLoopObject.player.accel(
			glm::vec3(-cos(gameLoopObject.player.getCamera()->rotation.y),
                0,
                -sin(gameLoopObject.player.getCamera()->rotation.y)
				) * deltaTime * 3.8f * -1.0f
		);
    }
    if(manager->getKeyState('d') == KeyManager::PRESSED)
    {
        gameLoopObject.player.accel(
			glm::vec3(cos(gameLoopObject.player.getCamera()->rotation.y),
                0,
                sin(gameLoopObject.player.getCamera()->rotation.y)
				) * deltaTime * 3.8f * -1.0f
		);
    }
	/*
    if(manager->isKeyDown('1'))
    {
        camera->rotate(glm::vec3(0.01f, 0, 0));
    }
    if(manager->isKeyDown('2'))
    {
        camera->rotate(glm::vec3(-.01f, 0, 0));
    }
    if(manager->isKeyDown('3'))
    {
        camera->rotate(glm::vec3(0, 0.01f, 0));
    }
    if(manager->isKeyDown('4'))
    {
        camera->rotate(glm::vec3(0, -.01f, 0));
    }
    if(manager->isKeyDown('5'))
    {
        camera->rotate(glm::vec3(0, 0, 0.01f));
    }
    if(manager->isKeyDown('6'))
    {
        camera->rotate(glm::vec3(0, 0, -.01f));
    }
	*/
	// Use a healthkit if the player has less than their maximum health. 
	static bool healLock = false;
	if (manager->isKeyDown('h') && !healLock)
	{
		if (gameLoopObject.player.health < gameLoopObject.player.maxHealth && gameLoopObject.player.healingItemCount > 0)
		{
			gameLoopObject.player.health = gameLoopObject.player.maxHealth;
			gameLoopObject.player.healingItemCount -= 1;
		}
	}
	if (!manager->isKeyDown('h'))
	{
		healLock = false;
	}

	if (manager->isKeyDown('-'))
	{
		gameLoopObject.player.health = 0;
	}

	/*
    static bool keylockTab = false;
    if(manager->getKeyState('\t') == KeyManager::PRESSED && !keylockTab)
    {
        keylockTab = true;
        gameLoopObject.mouseManager.setGrabbed(!gameLoopObject.mouseManager.grabbed);
    }
    if(manager->getKeyState('\t') == KeyManager::RELEASED)
    {
        keylockTab = false;
    }
	*/
	/*
    if(manager->getKeyState(' ') == KeyManager::PRESSED)
    {
        gameLoopObject.player.accel(glm::vec3(0, 1, 0) * deltaTime);
    }
    if(manager->getKeyState('x') == KeyManager::PRESSED)
    {
		gameLoopObject.player.accel(glm::vec3(0, -1, 0) * deltaTime);
    }
	*/
}

void processMouseInput()
{
    MouseManager *manager = &gameLoopObject.mouseManager;
    Camera *cam = gameLoopObject.player.getCamera();
	float deltaTime = gameLoopObject.getDeltaTime();

    if(manager->grabbed)
    {
        glm::vec3 *grabDir = &manager->relativeGrabDirection;
        if (grabDir->x < -2)
        {
			cam->rotate(glm::vec3(0, -1.2, 0) * deltaTime);
        }
        else if (grabDir->x > 2)
        {
            cam->rotate(glm::vec3(0, 1.2, 0) * deltaTime);
        }
        if (grabDir->y < -2)
        {
			cam->rotate(glm::vec3(1.2, 0, 0) * deltaTime);
        }
        else if (grabDir->y > 2)
        {
			cam->rotate(glm::vec3(-1.2, 0, 0) * deltaTime);
        }
    }

	// FIRE!
	static bool leftClickLock = false;
	if (manager->leftMouseButtonState == MouseManager::MOUSE_PRESSED && !leftClickLock)
	{
		leftClickLock = true;
		if (gameLoopObject.player.ammoCount > 0)
		{
			// Figure out the bullet's offset based on the lookAt vector
			glm::vec3 lookAt = glm::normalize(glm::vec3(
				sin(cam->rotation.y * -1.0f),
				0,
				cos(cam->rotation.y * -1.0f)
				)) * -1.0f;
			AABB gunbox = gameLoopObject.gunModel->getAABB();
			float yDelta = gunbox.yMax - gunbox.yMin;
			lookAt = lookAt + lookAt * yDelta;
			// Note the bullet is still a little bit off
			//lookAt.x += 0.025f;
			//lookAt.y -= 0.1f; // 0.1f is a magic number. we'll eventually have to solve for this based on where the barrel in the gun model is.
			// Create the projectile.
			Camera camera((gameLoopObject.player.getPosition() + lookAt + glm::vec3(0.025f, -0.1f, 0.0f)), glm::vec3(0, 0, 0));
			std::shared_ptr<Projectile> projectile(new Projectile(camera, .029f));

			glm::vec3 acceleration = (lookAt)* 40.0f;
			acceleration.y -= 1.8f;
			projectile->accel(acceleration);
			gameLoopObject.projectiles.push_back(projectile);

			ERRCHECK(gameLoopObject.eventInstance->start());
			gameLoopObject.player.ammoCount -= 1;
		}
	}
	if (manager->leftMouseButtonState != MouseManager::MOUSE_PRESSED)
	{
		leftClickLock = false;
	}
}

void entryCall(int argc, char **argv)
{
    // init GLUT and create window
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(1280,720);
	glutCreateWindow("Monster Hunter");
	// register callbacks
	glutDisplayFunc(gameUpdateTick);
	glutReshapeFunc(changeSize);
	glutIdleFunc(gameUpdateTick);
	
	glutKeyboardFunc(keyManagerKeyPressed);
	glutKeyboardUpFunc(keyManagerKeyUp);
	glutSpecialFunc(keyManagerKeySpecial);
	glutSpecialUpFunc(keyManagerKeySpecialUp);
//	glutSetKeyRepeat(GLUT_KEY_REPEAT_ON);
	//glutSetKeyRepeat(GLUT_KEY_REPEAT_ON);
	//glutKeyboardFunc(processNormalKeys);
	//glutSpecialFunc(processSpecialKeys);
    glutMouseFunc(mouseManagerHandleMouseClick);
    glutMotionFunc(mouseManagerHandleMouseMovementWhileClicked);
    glutPassiveMotionFunc(mouseManagerHandleMouseMovementWhileNotClicked);
    // load opengl functions beyond version 1.1
    glbinding::Binding::initialize(true);
    // Initialize the engine.
    initializeEngine();
	// enter GLUT event processing loop
	glutMainLoop();
}

///
/// Define the KeyManager functions and methods.
///
KeyManager::KeyManager() : isShiftDown(false), isControlDown(false), isAltDown(false)
{
}

void KeyManager::update()
{
}

void KeyManager::updateModifierState()
{
    int mod = glutGetModifiers();
    isShiftDown = false;
    isControlDown = false;
    isAltDown = false;

    if(mod == GLUT_ACTIVE_SHIFT)
    {
        isShiftDown = true;
    }
    if(mod == GLUT_ACTIVE_CTRL)
    {
        isControlDown = true;
    }
    if(mod == GLUT_ACTIVE_ALT)
    {
        isAltDown = true;
    }
    if(mod == (GLUT_ACTIVE_ALT|GLUT_ACTIVE_CTRL))
    {
        isControlDown = true;
        isAltDown = true;
    }
    if(mod == (GLUT_ACTIVE_SHIFT|GLUT_ACTIVE_CTRL))
    {
        isShiftDown = true;
        isControlDown = true;
    }
    if(mod == (GLUT_ACTIVE_SHIFT|GLUT_ACTIVE_ALT))
    {
        isShiftDown = true;
        isAltDown = true;
    }
    if(mod == (GLUT_ACTIVE_SHIFT|GLUT_ACTIVE_CTRL|GLUT_ACTIVE_ALT))
    {
        isShiftDown = true;
        isControlDown = true;
        isAltDown = true;
    }
}

void keyManagerKeyPressed(unsigned char key, int x, int y)
{
    gameLoopObject.keyManager.updateModifierState();
    if(key >= KeyManager::VALID_NUMBER_OF_CHARS || key < 0)
    {
        return;
    }
    gameLoopObject.keyManager.keystates[key] = KeyManager::PRESSED;
}

void keyManagerKeyUp(unsigned char key, int x, int y)
{
    gameLoopObject.keyManager.updateModifierState();

    if(key >= KeyManager::VALID_NUMBER_OF_CHARS || key < 0)
    {
        return;
    }
    gameLoopObject.keyManager.keystates[key] = KeyManager::RELEASED;
}

void keyManagerKeySpecial(int key, int x, int y)
{
    gameLoopObject.keyManager.updateModifierState();

    if(key >= KeyManager::VALID_NUMBER_OF_SPECIALS || key < 0)
    {
        return;
    }
    gameLoopObject.keyManager.specialKeystates[key] = KeyManager::PRESSED;
}

void keyManagerKeySpecialUp(int key, int x, int y)
{
    gameLoopObject.keyManager.updateModifierState();
    if(key >= KeyManager::VALID_NUMBER_OF_SPECIALS || key < 0)
    {
        return;
    }
    gameLoopObject.keyManager.specialKeystates[key] = KeyManager::RELEASED;
}

unsigned char KeyManager::getKeyState(unsigned char key)
{
    if(key >= KeyManager::VALID_NUMBER_OF_CHARS || key < 0)
    {
        return RELEASED;
    }
    return gameLoopObject.keyManager.keystates[key];
}

unsigned char KeyManager::getSpecialState(int key)
{
    if(key >= KeyManager::VALID_NUMBER_OF_CHARS || key < 0)
    {
        return RELEASED;
    }
    return gameLoopObject.keyManager.specialKeystates[key];

}

unsigned char KeyManager::isKeyDown(unsigned char key)
{
    return getKeyState(key) == PRESSED;
}

///
/// Define MouseManager functions/methods
///
MouseManager::MouseManager() : leftMouseButtonState(MOUSE_RELEASED), middleMouseButtonState(MOUSE_RELEASED), rightMouseButtonState(MOUSE_RELEASED), x(0), y(0), grabbed(false)
{
}

void mouseManagerHandleMouseClick(int button, int state, int x, int y)
{
/*
The first relates to which button was pressed, or released. This argument can have one of three values:
    GLUT_LEFT_BUTTON
    GLUT_MIDDLE_BUTTON
    GLUT_RIGHT_BUTTON
The second argument relates to the state of the button when the callback was generated, i.e. pressed or released. The possible values are:
    GLUT_DOWN
    GLUT_UP
*/
    if(button == GLUT_LEFT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
			if (gameLoopObject.mouseManager.leftMouseButtonState == MouseManager::MOUSE_RELEASED)
			{
				gameLoopObject.mouseManager.leftMouseButtonState = MouseManager::MOUSE_JUST_PRESSED;
			}
			else
			{
				gameLoopObject.mouseManager.leftMouseButtonState = MouseManager::MOUSE_PRESSED;
			}
        }
        if(state == GLUT_UP)
        {
            gameLoopObject.mouseManager.leftMouseButtonState = MouseManager::MOUSE_RELEASED;
        }
    }
    if(button == GLUT_MIDDLE_BUTTON)
    {
		if (state == GLUT_DOWN)
		{
			if (gameLoopObject.mouseManager.middleMouseButtonState == MouseManager::MOUSE_RELEASED)
			{
				gameLoopObject.mouseManager.middleMouseButtonState = MouseManager::MOUSE_JUST_PRESSED;
			}
			else
			{
				gameLoopObject.mouseManager.middleMouseButtonState = MouseManager::MOUSE_PRESSED;
			}
		}
        if(state == GLUT_UP)
        {
            gameLoopObject.mouseManager.middleMouseButtonState = MouseManager::MOUSE_RELEASED;
        }
    }
    if(button == GLUT_RIGHT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
			if (gameLoopObject.mouseManager.rightMouseButtonState == MouseManager::MOUSE_RELEASED)
			{
				gameLoopObject.mouseManager.rightMouseButtonState = MouseManager::MOUSE_JUST_PRESSED;
			}
			else
			{
				gameLoopObject.mouseManager.rightMouseButtonState = MouseManager::MOUSE_PRESSED;
			}
		}
        if(state == GLUT_UP)
        {
            gameLoopObject.mouseManager.rightMouseButtonState = MouseManager::MOUSE_RELEASED;
        }
    }
    gameLoopObject.mouseManager.x = x;
    gameLoopObject.mouseManager.y = y;
}

// active mouse movement: active motion occurs when the mouse is moved and a button is pressed.
void mouseManagerHandleMouseMovementWhileClicked(int x, int y)
{
	std::cout << "M:" << x << " " << y << std::endl;
    // Do stuff with the mouse clicked then dragged
    static bool warped = false;
    if(warped)
    {
        warped = false;
        return;
    }
    if(gameLoopObject.mouseManager.grabbed)
    {
        warped = true;
        int width = getWindowWidth();
        int height = getWindowHeight();
        int centerX = (float)width / 2.0;
        int centerY = (float)height / 2.0;
        int deltaX = (x - centerX);
        int deltaY = (y - centerY);
        glm::vec3 dir(deltaX , deltaY , 0);
        gameLoopObject.mouseManager.relativeGrabDirection = dir;
        glutWarpPointer( glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2 );
        gameLoopObject.mouseManager.x = x;
        gameLoopObject.mouseManager.y = y;
    }
    else
    {
        gameLoopObject.mouseManager.x = x;
        gameLoopObject.mouseManager.y = y;
    }
}

// passive mouse movement: movement that occurs when the mouse is not clicked.
void mouseManagerHandleMouseMovementWhileNotClicked(int x, int y)
{
    // Do stuff while the mouse isn't clicked, but dragged
    static bool warped = false;
    if(warped)
    {
        warped = false;
        return;
    }
    if(gameLoopObject.mouseManager.grabbed)
    {
        warped = true;
        int width = getWindowWidth();
        int height = getWindowHeight();
        int centerX = (float)width / 2.0;
        int centerY = (float)height / 2.0;
        int deltaX = (x - centerX);
        int deltaY = (y - centerY);
        glm::vec3 dir(deltaX , deltaY , 0);
        gameLoopObject.mouseManager.relativeGrabDirection = dir;
        glutWarpPointer( glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2 );
        gameLoopObject.mouseManager.x = x;
        gameLoopObject.mouseManager.y = y;
    }
    else
    {
        gameLoopObject.mouseManager.x = x;
        gameLoopObject.mouseManager.y = y;
    }
}

void MouseManager::setGrabbed(bool grabbed)
{
    this->grabbed = grabbed;
    if(grabbed)
    {
        glutSetCursor(GLUT_CURSOR_NONE);
    }
    else
    {
        glutSetCursor(GLUT_CURSOR_LEFT_SIDE);
    }
}

glm::vec3 MouseManager::getRelativeGrabDirection()
{
    if(grabbed)
    {
        return this->relativeGrabDirection;
    }
    else
    {
        return glm::vec3(0, 0, 0);
    }
}

void MouseManager::update()
{
	if (leftMouseButtonState == MOUSE_JUST_PRESSED)
	{
		leftMouseButtonState = MOUSE_PRESSED;
	}
	if (middleMouseButtonState == MOUSE_JUST_PRESSED)
	{
		middleMouseButtonState = MOUSE_PRESSED;
	}
	if (rightMouseButtonState == MOUSE_JUST_PRESSED)
	{
		rightMouseButtonState = MOUSE_PRESSED;
	}
}

