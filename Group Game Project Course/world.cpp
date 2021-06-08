// Header
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "building.hpp"
#include "background.hpp"
#include "render_components.hpp"
#include "bone.hpp"
#include "seagull.hpp"
#include "rabbit.hpp"
#include "ball.hpp"
#include "enemy.hpp"
#include "dirt.hpp"
#include "conversation.hpp"
#include "litter_quest_girl.hpp"
#include "dumpster.hpp"
#include "litter.hpp"
#include "unicyclist.hpp"
#include "socks.hpp"
#include "cloud.hpp"
#include "water_bowl.hpp"
#include "informant_squirrel.hpp"
#include "poor_squirrel.hpp"
#include "waves.hpp"
#include "particle.hpp"

// stlib
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>

const float CONVERSATION_DISTANCE = 400.0f;
const float PLAYER_SPEED = 300.0f;
const size_t MAX_SEAGULLS = 6;
bool currentlyWalking = false;
bool quests_completed[NUM_QUESTS];
entt::entity questEntities[NUM_QUESTS];
std::string scriptPaths[NUM_QUESTS] = {
	scripts_path("intro.txt"),        scripts_path("ball_tutorial.txt"),
	scripts_path("frank_attack.txt"), scripts_path("litter.txt"),
									   scripts_path("unicyclist.txt"), scripts_path("night_tutorial.txt"), 
	scripts_path("informant_squirrel.txt"), scripts_path("poor_squirrel.txt")};

bool currentlyConversing = false;
Quest currentConversation = NUM_QUESTS;
vec2 window_size_gu; // not ideal to have this, but needed to pass
					 // window_size_in_game_units to Conversation::converse()

// This enum stores which channels which sounds are played on
// You don't need to add a member to this enum for all different types of
// sounds, only add one if you need to refer back to that channel again. To play
// a sound on the next free channel, use channel -1
enum Channel
{
	ALL = -1,
	SEAGULL = 0,
	WALK = 1
};

// Note, this has a lot of OpenGL specific things, could be moved to the
// renderer; but it also defines the callbacks to the mouse and keyboard. That
// is why it is called here.
WorldSystem::WorldSystem(ivec2 window_size_px, vec2 window_size_in_game_units) :
	points(0)
{
	window_size_gu = window_size_in_game_units;
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());

	///////////////////////////////////////
	// Initialize GLFW
	auto glfw_err_callback = [](int error, const char* desc) {
		std::cerr << "OpenGL:" << error << desc << std::endl;
	};
	glfwSetErrorCallback(glfw_err_callback);
	if (!glfwInit())
		throw std::runtime_error("Failed to initialize GLFW");

	//-------------------------------------------------------------------------
	// GLFW / OGL Initialization, needs to be set before glfwCreateWindow
	// Core Opengl 3.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(window_size_px.x, window_size_px.y, "Rebuild UBC",
							  nullptr, nullptr);
	if (window == nullptr)
		throw std::runtime_error("Failed to glfwCreateWindow");

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) {
		((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3);
	};
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) {
		((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({_0, _1});
	};
	auto mouse_button_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) {
		((WorldSystem*)glfwGetWindowUserPointer(wnd))
			->on_mouse_button(_0, _1, _2);
	};
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_button_redirect);

	init_audio();
}

WorldSystem::~WorldSystem()
{
	// Destroy music components
	// TODO

	// Destroy all created components
	_ecs.clear();

	// Close the window
	glfwDestroyWindow(window);
}

void chunkLoadError(char* path)
{
	throw std::runtime_error(
		"Failed to load sounds make sure the data directory is present: " +
		audio_path(path));
}

void WorldSystem::init_audio()
{
	//////////////////////////////////////
	// Loading music and sounds with SDL

	// Uncomment these lines to start the game without sound
	// fx_volume = 0;
	// music_volume = 0;

	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		throw std::runtime_error("Failed to initialize SDL Audio");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
		throw std::runtime_error("Failed to open audio device");

	background_music = Mix_LoadMUS(audio_path("background-music2.wav").c_str());
	walk_sound = Mix_LoadWAV(audio_path("beep-walk.wav").c_str());
	bone_sound = Mix_LoadWAV(audio_path("happy-barks.wav").c_str());
	seagull_sound1 = Mix_LoadWAV(audio_path("seagull1.wav").c_str());
	seagull_sound2 = Mix_LoadWAV(audio_path("seagull2.wav").c_str());
	seagull_sound3 = Mix_LoadWAV(audio_path("seagull3.wav").c_str());
	seagull_sound4 = Mix_LoadWAV(audio_path("seagull4.wav").c_str());

	if (walk_sound == nullptr)
		chunkLoadError("beep-walk.wav");

	if (background_music == nullptr)
		chunkLoadError("background-music2.wav");

	if (bone_sound == nullptr)
		chunkLoadError("happy-barks.wav");

	if (seagull_sound1 == nullptr)
		chunkLoadError("seagull1.wav");

	if (seagull_sound2 == nullptr)
		chunkLoadError("seagull2.wav");

	if (seagull_sound3 == nullptr)
		chunkLoadError("seagull3.wav");

	if (seagull_sound4 == nullptr)
		chunkLoadError("seagull4.wav");

	Mix_Volume(-1, fx_volume);

	Mix_FadeInMusic(background_music, -1, 2000);
	Mix_VolumeMusic(music_volume);
	std::cout << "Loaded music\n";
}

void WorldSystem::playRandomSeagullSound(void)
{
	if (!Mix_Playing(SEAGULL))
	{
		float x = uniform_dist(rng);
		if (x < 0.25)
		{
			Mix_PlayChannel(SEAGULL, seagull_sound1, 0);
		}
		else if (x < 0.5)
		{
			Mix_PlayChannel(SEAGULL, seagull_sound2, 0);
		}
		else if (x < 0.75)
		{
			Mix_PlayChannel(SEAGULL, seagull_sound3, 0);
		}
		else
		{
			Mix_PlayChannel(SEAGULL, seagull_sound4, 0);
		}
	}
}

bool isWithinDistance(const Motion& motion1, const Motion& motion2,
					  float distance)
{
	return (sqrt(dot(motion1.position - motion2.position,
					 motion1.position - motion2.position)) <= distance);
}

void WorldSystem::converse(Conversation& conversation)
{
	if (!currentlyConversing)
	{
		currentlyConversing = true;
		if (currentlyWalking)
		{
			currentlyWalking = false;
			Mix_HaltChannel(WALK);
		}
		currentConversation = conversation.getQuest();
	}
	if (currentlyConversing && currentConversation == conversation.getQuest())
	{
		Motion& player_motion = _ecs.get<Motion>(main_player);
		player_motion.velocity = vec2(0, 0);
		input_up = input_down = input_left = input_right = false;
		currentlyConversing =
			conversation.converse(quests_completed, window_size_gu);
	}
}

void WorldSystem::setColorTimer(entt::entity panel, bool gained)
{
	if (!_ecs.has<ColorTimer>(panel))
	{
		ColorTimer& ct = _ecs.emplace<ColorTimer>(panel);
		if (gained)
		{
			_ecs.get<UIPanel>(panel).color = {
				0, 1, 0,
				uiPanelOpacity}; // green because we gained a bone / ball etc
		}
		else
		{
			_ecs.get<UIPanel>(panel).color = {
				1, 0, 0,
				uiPanelOpacity}; // red because we lost a bone / ball etc
		}
	}
}

void WorldSystem::load_scripts(void)
{
	for (int q = INTRO; q != NUM_QUESTS; q++)
	{
		Quest quest = static_cast<Quest>(q);
		std::string path = scriptPaths[quest];
		std::cout << "Loading script for quest " << quest << ", with path "
				  << path << std::endl;
		auto map_file = std::ifstream{path};
		if (!map_file)
		{
			throw std::runtime_error("Could not open script file " + path);
		}

		auto line = std::string{};
		std::vector<Line> lines[NUM_CONVERSATION_STATES];
		for (int i = 0; i < NUM_CONVERSATION_STATES - 1; i++)
		{
			lines[i] = std::vector<Line>();
		}
		while (std::getline(map_file, line))
		{
			auto ss = std::istringstream{line};

			auto type = std::string{};
			ss >> type;

			if (!ss)
			{
				continue;
			}

			if (type == "state")
			{
				auto conversationState = int{};
				ss >> conversationState;

				while (std::getline(map_file, line))
				{
					ss = std::istringstream{line};
					auto str = std::string{};
					bool isMyLine;
					ss >> str;
					if (str == "end")
					{
						break;
					}
					else if (str == "true")
					{
						isMyLine = true;
					}
					else if (str == "false")
					{
						isMyLine = false;
					}
					else
					{
						std::cout
							<< "Error, should be true false or end, but we got "
							<< str << std::endl;
					}
					auto text = std::string{};
					std::getline(ss, text, '\n');
					text.erase(0, 1); // remove the whitespace character from
									  // the beginning
					lines[conversationState].push_back(Line{isMyLine, text});
				}
			}
			else
			{
				std::cout << "Error, should be state but we got " << type
						  << std::endl;
			}
		}
		entt::entity entity = questEntities[quest];
		_ecs.emplace<Conversation>(entity, entity, quest, lines, gameFont);
		if (quest == FRANK_ATTACK || quest == NIGHT_TUTORIAL)
		{
			_ecs.get<Conversation>(entity).setOneTimeOnly(true);
		}
	}
	std::cout << "Finished loading scripts\n";
}

void WorldSystem::load_level(std::string path)
{
	auto map_file = std::ifstream{path};
	if (!map_file)
	{
		throw std::runtime_error("Could not open map file " + path);
	}

	// Reset the bone count
	total_bone_count = num_bones_from_quests;

	auto line = std::string{};
	while (std::getline(map_file, line))
	{
		auto ss = std::istringstream{line};

		auto type = std::string{};
		ss >> type;

		if (!ss)
		{
			continue;
		}

		if (type == "building")
		{
			auto name = std::string{};
			auto position = vec2{};
			ss >> name >> position.x >> position.y;
			entt::entity building = Building::createBuilding(position, name);
			if (name == "beaty")
			{
				questEntities[INTRO] = building;
			}
			if (name == "bonetree")
			{
				bonetree = building;
			}
		}
		else if (type == "background")
		{
			auto name = std::string{};
			auto position = vec2{};
			ss >> name >> position.x >> position.y;
			Background::createBackground(position, name);
		}
		else if (type == "player")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			main_player = Player::createPlayer(position);
			questEntities[NIGHT_TUTORIAL] = main_player;
		}
		else if (type == "bone")
		{
			auto bonetype = int{};
			auto position = vec2{};
			ss >> bonetype >> position.x >> position.y;
			// this is the bone stuck in the tree
			// add physics component to the bone for when it drops from the tree
			if (bonetype == 6)
			{
				flipper_in_tree = Bone::createBone(position, (BoneAsset)bonetype);
				total_bone_count++;
				auto& bone_physics = _ecs.emplace<Physics>(flipper_in_tree);
				auto& bone_motion = _ecs.get<Motion>(flipper_in_tree);
				auto& tree_motion = _ecs.get<Motion>(bonetree);
                bone_motion.depth = 2.f;
				bone_physics.mass = 2.f;
				vec2 gravity = {0.f, 980.f * bone_physics.mass};
				bone_physics.forces += gravity;
				bone_physics.groundLevel = tree_motion.position.y + tree_motion.scale.y;
				bone_physics.restitution = {0.6f, 0.3f};
                bone_physics.originalScale = bone_motion.scale;
			}
			else
			{
				Bone::createBone(position, (BoneAsset)bonetype);
				total_bone_count++;
			}
		}
		else if (type == "enemy")
		{
			auto position = vec2{};
			auto sprite = std::string{};
			ss >> position.x >> position.y >> sprite;
			// create enemy dog here
			entt::entity enemy = Enemy::createEnemy(position, sprite);
			questEntities[FRANK_ATTACK] = enemy;
		}
		else if (type == "rabbit")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			auto rabbit = Rabbit::createRabbit(position);
			auto& rabbitPathFinder = _ecs.get<PathFinder>(rabbit);
			rabbitPathFinder.pathIndex = 0;
			rabbitPathFinder.pathInitialized = false;
		}
		else if (type == "ball")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			Ball::createBall(position);
			playerHasBall = 0;
			std::cout << "ball " << position.x << position.y;
		}
		else if (type == "dirt")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			Dirt::createDirt(position);
		}
		else if (type == "litterquestgirl")
		{
			auto position = vec2{};
			
			ss >> position.x >> position.y;
			auto litter_quest_girl =
				LitterQuestGirl::createLitterQuestGirl(position);
			questEntities[LITTER] = litter_quest_girl;
		}
		else if (type == "dumpster")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			Dumpster::createDumpster(position);
		}
		else if (type == "litter")
		{
			auto position = vec2{};
			auto index = int{};
			ss >> index >> position.x >> position.y;
			litter[index] = position;
		}
		else if (type == "unicyclist")
		{
			auto position = vec2{};
			
			ss >> position.x >> position.y;
			auto unicyclist = Unicyclist::createUnicyclist(position);
			questEntities[UNICYCLIST] = unicyclist;
		}
		else if (type == "socks")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			socks_pos = position;
		}
		else if (type == "waterbowl")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			entt::entity water_bowl = WaterBowl::createWaterBowl(position);
			questEntities[BALL_TUTORIAL] = water_bowl;
			std::cout << "Created water bowl id: "
					  << static_cast<int>(water_bowl) << std::endl;
		}
		else if (type == "informantsquirrel")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			auto informant_squirrel =
				InformantSquirrel::createInformantSquirrel(position);
			questEntities[INFORMANT_SQUIRREL] = informant_squirrel;
		}
		else if (type == "poorsquirrel")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			auto poor_squirrel = PoorSquirrel::createPoorSquirrel(position);
			questEntities[POOR_SQUIRREL] = poor_squirrel;
        }
		else if (type == "waves")
		{
			auto position = vec2{};
			ss >> position.x >> position.y;
			Waves::createWaves(position);
		}
	}

	const vec2 world_size_in_game_units = vec2{10000.0f, 6000.0f};

	// Generate clouds
	const size_t NumClouds = 10;
	for (size_t i = 0; i < NumClouds; ++i)
	{
		float height = 0.1f + 0.2f * uniform_dist(rng);
		Cloud::createCloud(vec2{uniform_dist(rng) * world_size_in_game_units.x,
								uniform_dist(rng) * world_size_in_game_units.y},
						   vec2{1 + height, 1 + height}, -height);
	}

	// Generate grass
	const size_t NumGrass = 500;
	for (size_t i = 0; i < NumGrass; ++i)
	{
		auto pos = vec2{uniform_dist(rng) * world_size_in_game_units.x,
						uniform_dist(rng) * world_size_in_game_units.y};
		float depth = 0.2 * (-pos.y / world_size_in_game_units.y) + 0.8;
		FoliageParticle::create(pos, vec2{60, 60}, 999);
	}
	// Uncomment the below line to test the end text
	//total_bone_count = 2;
}

void WorldSystem::load_save_state(std::string path)
{
	auto map_file = std::ifstream{path};
	if (!map_file)
	{
		throw std::runtime_error("Could not open save file " + path);
	}
	std::vector<vec2> bones;
	std::vector<vec2> dirts;
	std::vector<vec2> balls;

	auto line = std::string{};
	while (std::getline(map_file, line))
	{
		auto ss = std::istringstream{line};

		auto type = std::string{};
		ss >> type;

		if (!ss)
		{
			continue;
		}

		else if (type == "player")
		{
			// done
			auto position = vec2{};
			ss >> position.x >> position.y;
			auto& player_motion = _ecs.get<Motion>(main_player);
			player_motion.position = position;
		}
		else if (type == "bone")
		{
			// done
			auto questbone = int{};
			auto bonetype = int{};
			auto position = vec2{};
			ss >> bonetype >> position.x >> position.y;

			auto& unicyclist_entity = _ecs.get<Motion>(questEntities[UNICYCLIST]);
			vec2 unicyclist_pos = {unicyclist_entity.position.x -
									   abs(unicyclist_entity.scale.x),
								   unicyclist_entity.position.y +
									   abs(unicyclist_entity.scale.y / 2.f)};

			auto& littergirl_entity = _ecs.get<Motion>(questEntities[LITTER]);
			vec2 litter_pos = {littergirl_entity.position.x -
								   abs(littergirl_entity.scale.x),
							   littergirl_entity.position.y +
								   abs(littergirl_entity.scale.y / 1.5f)};
			if (position == unicyclist_pos || position == litter_pos)
			{
				std::cout << bonetype << "quest boneeeeeee\n";
				Bone::createBone(position, (BoneAsset)bonetype);
			}
			bones.push_back(position);
		}
		else if (type == "enemy")
		{
			// done
			auto position = vec2{};
			ss >> position.x >> position.y;
			auto& enemy_motion = _ecs.get<Motion>(questEntities[FRANK_ATTACK]);
			enemy_motion.position = position;
		}
		else if (type == "rabbit")
		{
			// done
			auto position = vec2{};
			ss >> position.x >> position.y;
			auto rabbit_view = _ecs.view<Rabbit>();
			for (auto& rabbit : rabbit_view)
			{
				auto& rabbit_motion = _ecs.get<Motion>(rabbit);
				rabbit_motion.position = position;
			}
		}
		else if (type == "ball")
		{
			// done
			auto position = vec2{};
			ss >> position.x >> position.y;
			auto ball_view = _ecs.view<Ball>();
			if (!balls.empty())
			{
				Ball::createBall(position);
				std::cout << "Create ball " << position.x << " " << position.y;
			}
			else
			{
				for (auto& ball : ball_view)
				{
					auto& ball_motion = _ecs.get<Motion>(ball);
					ball_motion.position = position;
					std::cout << "Move ball " << position.x << " "
							  << position.y;
				}
				balls.push_back(position);
			}
		}
		else if (type == "dirt")
		{
			// done
			auto position = vec2{};
			ss >> position.x >> position.y;
			dirts.push_back(position);
		}
		else if (type == "litter")
		{
			// done
			auto position = vec2{};
			auto index = int{};
			ss >> index >> position.x >> position.y;
			Litter::createLitter(position, (LitterAsset)index);
		}
		else if (type == "socks")
		{
			// done
			auto position = vec2{};
			ss >> position.x >> position.y;
			Socks::createSocks(position);
		}
		else if (type == "seagull")
		{
			// done
			auto position = vec2{};
			ss >> position.x >> position.y;
			Seagull::createSeagull(position);
		}
		if (type == "pickedupbones")
		{
			// done
			int collected = int{};
			ss >> collected;
			points = collected;
		}
		else if (type == "pickedupballs")
		{
			// done
			int balls = int{};
			ss >> balls;
			playerHasBall = balls;
		}
		else if (type == "ballrespawn")
		{
			// done
			int doRespawn = int{};
			int timer = int{};
			ss >> doRespawn >> timer;
			ballRespawn = doRespawn;
			ballRespawnTimer = timer;
		}
		else if (type == "firstnight")
		{
			// done
			auto& playerNight = _ecs.get<Night>(main_player);
			int firstnight = int{};
			ss >> firstnight;
			playerNight.firstNightHasHappened = firstnight;
		}
		else if (type == "night")
		{
			// done
			int isNight = int{};
			int timer = int{};
			ss >> isNight >> timer;
			auto& playerNight = _ecs.get<Night>(main_player);
			playerNight.is_night = isNight;
			if (isNight)
			{
				playerNight.night_timer = timer;
			}
			else
			{
				playerNight.day_timer = timer;
			}
		}
		else if (type == "intro")
		{
			int completed = int{};
			int first = int{};
			int state = int{};
			int linecount = int{};

			ss >> completed >> first >> state >> linecount;
			quests_completed[INTRO] = completed;
			auto& intro = _ecs.get<Conversation>(questEntities[INTRO]);
			intro.setFirstEncounter(first);
			intro.setState(static_cast<ConversationState>(state));
			intro.setCurrentLine(linecount);

			//std::cout << intro.conversationInProgress()
			//		  << intro.getFirstEncounter()
			//		  << intro.getState() << intro.getCurrentLine();
		}
		else if (type == "ballintro")
		{
			int completed = int{};
			int first = int{};
			int state = int{};
			int linecount = int{};

			ss >> completed >> first >> state >> linecount;
			quests_completed[BALL_TUTORIAL] = completed;
			auto& ballintro = _ecs.get<Conversation>(questEntities[BALL_TUTORIAL]);
			ballintro.setFirstEncounter(first);
			ballintro.setState(static_cast<ConversationState>(state));
			ballintro.setCurrentLine(linecount);

			//std::cout << ballintro.conversationInProgress()
			//		  << ballintro.getFirstEncounter() << ballintro.getState()
			//		  << ballintro.getCurrentLine();
		}
		else if (type == "frankintro")
		{
			int completed = int{};
			int first = int{};
			int state = int{};
			int linecount = int{};

			ss >> completed >> first >> state >> linecount;
			quests_completed[FRANK_ATTACK] = completed;
			auto& frankintro = _ecs.get<Conversation>(questEntities[FRANK_ATTACK]);
			frankintro.setFirstEncounter(first);
			frankintro.setState(static_cast<ConversationState>(state));
			frankintro.setCurrentLine(linecount);

			//std::cout << frankintro.conversationInProgress()
			//		  << frankintro.getFirstEncounter() << frankintro.getState()
			//		  << frankintro.getCurrentLine();
		}
		else if (type == "littergirl")
		{
			int completed = int{};
			int inprogress = int{};
			int reward = int{};
			int first = int{};
			int state = int{};
			int linecount = int{};

			ss >> completed >> inprogress >> reward >> first >> state >> linecount;
			quests_completed[LITTER] = completed;
			auto& littergirl = _ecs.get<Conversation>(questEntities[LITTER]);
			littergirl.setFirstEncounter(first);
			littergirl.setState(static_cast<ConversationState>(state));
			littergirl.setCurrentLine(linecount);
			rendered_litter = inprogress;
			rendered_litter_reward = reward;

			std::cout << littergirl.getFirstEncounter() << " "
					  << littergirl.state << " "
					  << littergirl.getCurrentLine() << "\n";
		}
		else if (type == "unicycle")
		{
			int completed = int{};
			int inprogress = int{};
			int reward = int{};
			int first = int{};
			int state = int{};
			int linecount = int{};

			ss >> completed >> inprogress >> reward >> first >> state >>
				linecount;
			quests_completed[UNICYCLIST] = completed;
			auto& unicycle = _ecs.get<Conversation>(questEntities[UNICYCLIST]);
			unicycle.setFirstEncounter(first);
			unicycle.setState(static_cast<ConversationState>(state));
			unicycle.setCurrentLine(linecount);
			rendered_socks = inprogress;
			rendered_unicyclist_reward = reward;

			/*std::cout << unicycle.conversationInProgress()
					  << unicycle.getFirstEncounter() << unicycle.getState()
					  << unicycle.getCurrentLine();*/
		}
		else if (type == "informantsquirrel")
		{
			int completed = int{};
			int reward = int{};
			int first = int{};
			int state = int{};
			int linecount = int{};

			ss >> completed >> reward >> first >> state >>
				linecount;
			quests_completed[INFORMANT_SQUIRREL] = completed;
			auto& informantsquirrel =
				_ecs.get<Conversation>(questEntities[INFORMANT_SQUIRREL]);
			informantsquirrel.setFirstEncounter(first);
			informantsquirrel.setState(static_cast<ConversationState>(state));
			informantsquirrel.setCurrentLine(linecount);
			rendered_ball = reward;

			std::cout << informantsquirrel.getFirstEncounter() << " "
					  << informantsquirrel.state << " "
					  << informantsquirrel.getCurrentLine()
					  << "\n";
		}
		else if (type == "poorsquirrel")
		{
			int completed = int{};
			int first = int{};
			int state = int{};
			int linecount = int{};

			ss >> completed >> first >> state >> linecount;
			auto& poorsquirrel =
				_ecs.get<Conversation>(questEntities[POOR_SQUIRREL]);
			poorsquirrel.setState(static_cast<ConversationState>(state));
			quests_completed[POOR_SQUIRREL] = completed;
			poorsquirrel.setFirstEncounter(false);
			poorsquirrel.setCurrentLine(linecount);

			// std::cout << poorsquirrel.conversationInProgress()
			//		  << poorsquirrel.getFirstEncounter() << poorsquirrel.getState()
			//		  << poorsquirrel.getCurrentLine();
		}
		else if (type == "nightintro")
		{
			int completed = int{};
			int first = int{};
			int state = int{};
			int linecount = int{};

			ss >> completed >> first >> state >> linecount;
			auto& nightintro =
				_ecs.get<Conversation>(questEntities[NIGHT_TUTORIAL]);
			nightintro.setState(static_cast<ConversationState>(state));
			quests_completed[NIGHT_TUTORIAL] = completed;
			nightintro.setFirstEncounter(false);
			nightintro.setCurrentLine(linecount);

			//std::cout << nightintro.conversationInProgress()
			//		  << nightintro.getFirstEncounter() << nightintro.getState()
			//		  << nightintro.getCurrentLine();
		}
	}
	auto ball_view = _ecs.view<Ball>();
	for (auto& ball : ball_view)
	{
		auto& ball_motion = _ecs.get<Motion>(ball);
		if (std::count(balls.begin(), balls.end(), ball_motion.position) == 0)
		{
			_ecs.destroy(ball);
		}
	}

	auto bone_view = _ecs.view<Bone>();
	for (auto& bone : bone_view)
	{
		auto& bone_motion = _ecs.get<Motion>(bone);
		if (std::count(bones.begin(), bones.end(), bone_motion.position) == 0)
		{
			_ecs.destroy(bone);
		}
	}
	if (float(points) / float(total_bone_count) == 1.0)
	{
		_ecs.destroy(_ecs.view<Enemy>()[0]);
	}
	auto dirt_view = _ecs.view<Dirt>();
	for (auto& dirt : dirt_view)
	{
		auto& dirt_motion = _ecs.get<Motion>(dirt);
		if (std::count(dirts.begin(), dirts.end(), dirt_motion.position) == 0)
		{
			_ecs.destroy(dirt);
		}
	}
}

std::string WorldSystem::generateSaveData(entt::entity entity, vec2 position)
{
	std::stringstream ss;
	if (_ecs.has<Player>(entity))
	{
		// done
		ss << "player " << position.x << " " <<
			   position.y << "\n";
	}
	else if (_ecs.has<Bone>(entity))
	{
		// done
		auto& bone = _ecs.get<Bone>(entity);
		ss << "bone " << bone.type << " " << position.x << " "
		   << position.y << "\n";
	}
	else if (_ecs.has<Enemy>(entity))
	{
		// done
		auto& enemy = _ecs.get<Enemy>(entity);
		ss << "enemy " << position.x << " " << 
			   position.y << " " << enemy.name << "\n";
	}
	else if (_ecs.has<Rabbit>(entity))
	{
		// done
		ss << "rabbit " << position.x << " " << 
			   position.y << "\n";
	}
	else if (_ecs.has<Ball>(entity))
	{
		// done
		ss << "ball " << position.x << " " << 
			   position.y << "\n";
	}
	else if (_ecs.has<Dirt>(entity))
	{
		// done
		ss << "dirt " << position.x << " " << 
			   position.y << "\n";
	}
	else if (_ecs.has<Seagull>(entity))
	{
		// done
		ss << "seagull " << position.x << " " << 
			   position.y << "\n";
	}
	else if (_ecs.has<Litter>(entity))
	{
		// done
		auto& litter = _ecs.get<Litter>(entity);
		ss << "litter " << std::to_string(litter.asset) << " " << position.x << " "
		   << position.y << "\n";
	}
	else if (_ecs.has<Socks>(entity))
	{
		// done
		ss << "socks " << " " << position.x << " "
		   << position.y << "\n";
	}
	else
	{
		// done
		ss << "";
	}
	std::string result = ss.str();
	return result;
}

void WorldSystem::write_save()
{
	std::cout << "Saving game\n";

	std::string savestate = levels_path("savestate.txt");

	std::cout << "Removing previous save\n";
	std::filesystem::remove(savestate);

	auto save_state = std::ofstream{savestate};

	auto motionView = _ecs.view<Motion>();

	for (auto& entity : motionView)
	{
		// done
		auto& motion = _ecs.get<Motion>(entity);
		std::cout << generateSaveData(entity, motion.position) << "\n";
		save_state << generateSaveData(entity, motion.position);
	}

	// done
	save_state << "pickedupbones " << points << "\n";

	// done
	save_state << "pickedupballs " << playerHasBall << "\n";

	// done
	save_state << "ballrespawn " << ballRespawn << " " << ballRespawnTimer
			   << "\n";

	// done
	auto& playerNight = _ecs.get<Night>(main_player);
	save_state << "firstnight " << playerNight.firstNightHasHappened << "\n";

	// done
	save_state << "night " << playerNight.is_night << " ";
	
	if (playerNight.is_night)
	{
		save_state << playerNight.night_timer << "\n";
	}
	else
	{
		save_state << playerNight.day_timer << "\n";
	}

	// done
	auto& intro = _ecs.get<Conversation>(questEntities[INTRO]);
	bool completed = quests_completed[INTRO];
	bool first = intro.getFirstEncounter();
	ConversationState state = intro.getState();
	int linecount = intro.getCurrentLine();
	save_state << "intro " << completed << " " << first
			   << " " << state << " " << linecount << "\n";

	// done
	auto& ballintro = _ecs.get<Conversation>(questEntities[BALL_TUTORIAL]);
	completed = quests_completed[BALL_TUTORIAL];
	first = ballintro.getFirstEncounter();
	state = ballintro.getState();
	linecount = ballintro.getCurrentLine();
	save_state << "ballintro " << completed << " " << first
			   << " " << state << " " << linecount << "\n";

	// done
	if (_ecs.valid(questEntities[FRANK_ATTACK]))
	{
		auto& frankintro = _ecs.get<Conversation>(questEntities[FRANK_ATTACK]);
		completed = quests_completed[FRANK_ATTACK];
		first = frankintro.getFirstEncounter();
		state = frankintro.getState();
		linecount = frankintro.getCurrentLine();
		save_state << "frankintro " << completed << " " << first << " " << state
				   << " " << linecount << "\n";
	}
	
	// done
	auto& littergirl = _ecs.get<Conversation>(questEntities[LITTER]);
	completed = quests_completed[LITTER];
	first = littergirl.getFirstEncounter();
	state = littergirl.state;
	linecount = littergirl.getCurrentLine();
	bool inprogress = rendered_litter;
	bool reward = rendered_litter_reward;
	save_state << "littergirl " << completed << " " << inprogress << " " << reward << " " << first
			   << " " << state << " " << linecount << "\n";

	// done
	auto& informantsquirrel =
		_ecs.get<Conversation>(questEntities[INFORMANT_SQUIRREL]);
	completed = quests_completed[INFORMANT_SQUIRREL];
	first = informantsquirrel.getFirstEncounter();
	state = informantsquirrel.state;
	linecount = informantsquirrel.getCurrentLine();
	reward = rendered_ball;
	save_state << "informantsquirrel " << completed << " " << reward << " " << first << " " << state << " " << linecount
			   << "\n";

	// done
	auto& unicycle = _ecs.get<Conversation>(questEntities[UNICYCLIST]);
	completed = quests_completed[UNICYCLIST];
	first = unicycle.getFirstEncounter();
	state = unicycle.getState();
	linecount = unicycle.getCurrentLine();
	inprogress = rendered_socks;
	reward = rendered_unicyclist_reward;
	save_state << "unicycle " << completed << " " << inprogress << " " << reward
			   << " " << first << " " << state << " " << linecount << "\n";

	// done
	auto& nightintro = _ecs.get<Conversation>(questEntities[NIGHT_TUTORIAL]);
	completed = quests_completed[NIGHT_TUTORIAL];
	first = nightintro.getFirstEncounter();
	state = nightintro.getState();
	linecount = nightintro.getCurrentLine();
	save_state << "nightintro " << completed << " " << first
			   << " " << state << " " << linecount << "\n";

	// done
	auto& poorsquirrel = _ecs.get<Conversation>(questEntities[POOR_SQUIRREL]);
	completed = quests_completed[POOR_SQUIRREL];
	first = poorsquirrel.getFirstEncounter();
	state = poorsquirrel.getState();
	linecount = poorsquirrel.getCurrentLine();
	save_state << "poorsquirrel " << completed << " " << first << " " << state
			   << " " << linecount << "\n";
}

// Update our game world
void WorldSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	// Update mouse game position
	auto cameraPos = _ecs.get<Motion>(main_player).position;
	mouse_world_position = pixelToGameUnits(mouse_screen_position) + cameraPos -
						   window_size_in_game_units / 2.0f;

	// Updating window title with points
	std::stringstream title_ss;
	// If you remove the space at the end of the following line, the entire game
	// breaks down into uncontrollable chaos.
	title_ss << points << " / " << total_bone_count << " ";
	// glfwSetWindowTitle(window, title_ss.str().c_str());
	_ecs.get<UILabel>(uiBonePanel).text = title_ss.str();

	std::stringstream title_ball;
	title_ball << playerHasBall << " balls";
	// glfwSetWindowTitle(window, title_ball.str().c_str());
	_ecs.get<UILabel>(uiBallPanel).text = title_ball.str();

	// Removing out of screen entities
	auto motionView = _ecs.view<Motion>();
	const auto numMotionComponents = motionView.size();

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without interfering with the next
	// object to visit (the containers exchange the last element with the
	// current upon delete)
	for (int i = static_cast<int>(numMotionComponents) - 1; i >= 0; --i)
	{
		auto entity = motionView[i];
		auto& motion = _ecs.get<Motion>(entity);
		if (motion.position.x + abs(motion.scale.x) < 0.f)
		{
			_ecs.destroy(entity);
		}
	}

	// Move litter when playing is holding it
	if (isGrabbing)
	{
		// update position of the litter
		auto& motion = _ecs.get<Motion>(grabObject);
		motion.position = mouse_world_position;
		std::cout << "Grabbing at " << mouse_world_position.x << " "
				  << mouse_world_position.y << "\n";
	}

	// Spawning new seagulls
	if (_ecs.view<Seagull>().size() < MAX_SEAGULLS)
	{
		entt::entity entity = Seagull::createSeagull({0, 0});
		auto& motion = _ecs.get<Motion>(entity);
		motion.position =
			vec2(uniform_dist(rng) * 3000.0f, uniform_dist(rng) * 3000.0f);
	}

	// update the player's movements based on key inputs
	// only added the trivial movements (WASD)
	vec2 keyInput = {(input_right ? 1.0 : 0.0) - (input_left ? 1.0 : 0.0),
					 (input_down ? 1.0 : 0.0) - (input_up ? 1.0 : 0.0)};
	auto& playerTransform = _ecs.get<Motion>(main_player);
	// arbitrary velocity change, modify when finalized
	if (socksEffect == false)
		playerTransform.velocity = keyInput * PLAYER_SPEED * current_speed;

	if (socksEffect == true)
	{
		socksMovementEffectTimer -= elapsed_ms * current_speed;
		float changeVelocity = socksMovementEffect ? 1.0 : -1.0;
		playerTransform.velocity = keyInput * PLAYER_SPEED * current_speed * changeVelocity;
	}
		
	if (socksMovementEffectTimer < 0.f)
	{
		socksMovementEffect = !socksMovementEffect;
		// Temp to hold old velocity
		float randomTimer = rand() % 4 + 4;
		socksMovementEffectTimer = randomTimer * 1000;
	}
	
	if (ballRespawn)
	{
		ballRespawnTimer -= elapsed_ms * current_speed;
		if (ballRespawnTimer < 0.f)
		{
			respawnBall();
		}
	}

	// Check if dog is moving and if so, change animation to walking
	auto& playerAnimator = _ecs.get<Animator>(main_player);
	if (playerTransform.velocity != vec2(0.f, 0.f))
		playerAnimator.curr_animation = 2;
	else
		playerAnimator.curr_animation = 0;

	// Check if enemy is moving and if so, change animation to walking
	if (_ecs.view<Enemy>().size() > 0)
	{
		entt::entity enemy = _ecs.view<Enemy>()[0];
		auto& enemyTransform = _ecs.get<Motion>(enemy);
		auto& enemyAnimator = _ecs.get<Animator>(enemy);
		if (enemyTransform.velocity != vec2(0.f, 0.f))
			enemyAnimator.curr_animation = 1;
		else
			enemyAnimator.curr_animation = 0;
	}

	// Check if rabbit is moving and if so, change animation to walking
	auto rabbit = _ecs.view<Rabbit>()[0];
	auto& rabbitMotion = _ecs.get<Motion>(rabbit);
	auto& rabbitAnimator = _ecs.get<Animator>(rabbit);
	if (rabbitMotion.velocity != vec2(0.f, 0.f))
		rabbitAnimator.curr_animation = 2;
	else
		rabbitAnimator.curr_animation = 0;

	// Check if seagulls are moving and if so, change animation to walking
	auto seagullView = _ecs.view<Seagull>();
	for (auto seagull : seagullView)
	{
		auto& seagullTransform = _ecs.get<Motion>(seagull);
		auto& seagullAnimator = _ecs.get<Animator>(seagull);
		if (seagullTransform.velocity != vec2(0.f, 0.f))
			seagullAnimator.curr_animation = 1;
		else
			seagullAnimator.curr_animation = 0;
	}

	// Check if dog is near something interactable and if so, change animation
	// to wagging
	auto interactionView = _ecs.view<Interaction>();
	const auto numInteractions = interactionView.size();
	near_interaction = false;
	if (numInteractions == 0)
		near_interaction = false;
	for (auto interaction : interactionView)
	{
		if (!_ecs.has<Player>(interaction)) // the player has an Interaction component, but should ignore the player
		{
			const auto& interactionTransform = _ecs.get<Motion>(interaction);
			if (isWithinDistance(interactionTransform, playerTransform, 200.f))
				near_interaction = true;
		}
	}

	auto conversationView = _ecs.view<Conversation>();
	near_conversation = false;
	for (auto conversation : conversationView)
	{
        if (conversation != main_player)    // ignore Conversation component on player
        {
            const auto& conversationTransform = _ecs.get<Motion>(conversation);
            if (isWithinDistance(conversationTransform, playerTransform, CONVERSATION_DISTANCE))
                near_conversation = true;
        }
	}

	// Enable interaction with flipper stuck in tree if it has fallen off the
	// tree and is at rest
	if (_ecs.valid(flipper_in_tree))
	{
		auto& bone_motion = _ecs.get<Motion>(flipper_in_tree);
		auto& tree_motion = _ecs.get<Motion>(bonetree);
		auto& bone_interaction = _ecs.get<Interaction>(flipper_in_tree);
		bool stuck_in_tree = true;
		if (abs(bone_motion.position.y -
				(tree_motion.position.y + tree_motion.scale.y / 2.f)) < 300.f)
		{
			stuck_in_tree = false;
		}
		bone_interaction.canInteract = !stuck_in_tree;
	}

	if ((near_interaction || near_conversation) && playerTransform.velocity == vec2(0.f, 0.f))
		playerAnimator.curr_animation = 1; // wag
	else if (near_interaction || near_conversation)
		playerAnimator.curr_animation = 3; // wag and walk


	// Check if dirt mound is over a bone and if so, make it non-interactable
	auto boneView = _ecs.view<Bone>();
	auto dirtView = _ecs.view<Dirt>();
	for (auto bone : boneView)
	{
		auto& boneTransform = _ecs.get<Motion>(bone);
		auto& boneInteraction = _ecs.get<Interaction>(bone);
		boneInteraction.timer += elapsed_ms;
		if (bone != flipper_in_tree)
		{
			bool under_dirt = false;
			for (auto dirt : dirtView)
			{
				auto& dirtTransform = _ecs.get<Motion>(dirt);
				if (isWithinDistance(boneTransform, dirtTransform, 100.f))
					under_dirt = true;
			}
			boneInteraction.canInteract = !under_dirt;
		}
	}

	// Check distance between the player and poor squirrel
	// start the conversation if they are near and the quest 
	// is in initial state
	auto& playerMotion = _ecs.get<Motion>(main_player);
	auto& squirrelMotion = _ecs.get<Motion>(questEntities[POOR_SQUIRREL]);
	auto& squirrelQuest = _ecs.get<Conversation>(questEntities[POOR_SQUIRREL]);
	if (isWithinDistance(playerMotion, squirrelMotion, CONVERSATION_DISTANCE) 
		&& squirrelQuest.getState() == INITIAL)
	{
		converse(_ecs.get<Conversation>(questEntities[POOR_SQUIRREL]));
	}

	// Update night timer
	if (lightEffectsOn)
	{
		auto& playerNight = _ecs.get<Night>(main_player);

		if (playerNight.is_night == 0)
			playerNight.day_timer += elapsed_ms;
		else
			playerNight.night_timer += elapsed_ms;

		if (playerNight.day_timer >= playerNight.day_duration)
		{
			if (!playerNight.firstNightHasHappened && !currentlyConversing)
			{
				playerNight.firstNightHasHappened = true;
				quests_completed[NIGHT_TUTORIAL] = true;
				converse(_ecs.get<Conversation>(questEntities[NIGHT_TUTORIAL]));
			}

			if (playerNight.firstNightHasHappened)
			{
				playerNight.is_night = 1;
				playerNight.day_timer = 0.f;
			}
		}

		if (playerNight.night_timer >= playerNight.night_duration)
		{
			playerNight.is_night = 0;
			playerNight.night_timer = 0.f;
			playerNight.flashlight_on = 0;
		}
	}

	// Update UI
	UI::update(mouse_screen_position /*mouseScreenPos*/,
			   input_mouse_click /*isMouseClicking*/);
	// Check button state
	if (button.isClicked())
	{
		buttonOnOffState = !buttonOnOffState;
		vec4 color = buttonOnOffState ? Colors::Teal : Colors::Yellow;
		color.a = 0.9f;
		_ecs.get<UIPanel>(uiBonePanel).color = color;
	}
	if (tooltip.isClicked())
	{
		tooltipOnOffState = !tooltipOnOffState;
		if (tooltipOnOffState)
		{
			_ecs.get<UIPanel>(uiToolTipWordsPanel).color[3] = 1;
			_ecs.get<UILabel>(uiToolTipWordsPanel).color[3] = 1;
		}
		else
		{
			_ecs.get<UIPanel>(uiToolTipWordsPanel).color[3] = 0;
			_ecs.get<UILabel>(uiToolTipWordsPanel).color[3] = 0;
		}
	}
	if (playButton.isClicked())
	{
		controls = true;
		_ecs.get<UIPanel>(uiLoadingScreenPanel).color[3] = 0;
		_ecs.get<UIPanel>(uiLoadingScreenPanel).position[2] = 0;
		_ecs.get<UIImage>(uiLoadingScreenPanel).color[3] = 0;
		_ecs.get<UIImage>(uiLoadingScreenPanel).position[2] = 0;
		_ecs.get<UILabel>(uiLoadingScreenPanel).color[3] = 0;
		_ecs.get<UILabel>(uiLoadingScreenPanel).position[2] = 0;
		
	}

	for (auto& e : _ecs.view<ColorTimer>())
	{
		ColorTimer& ct = _ecs.get<ColorTimer>(e);
		ct.timer -= elapsed_ms;
		if (ct.timer < 0)
		{
			if (_ecs.has<UIPanel>(e))
			{
				UIPanel& panel = _ecs.get<UIPanel>(e);
				panel.color = uiPanelColor;
			}
			else
			{
				std::cout << "Error: this color timer entity doesn't have a "
							 "UIPanel\n";
			}
			_ecs.remove<ColorTimer>(e);
		}
	}

	// Update conditional rendering of items based on if player has talked to
	// certain people
	auto unicyclist = _ecs.view<Unicyclist>()[0];
	if (_ecs.has<Conversation>(unicyclist))
	{
		auto& unicyclistQuest = _ecs.get<Conversation>(unicyclist);
		// only render socks if the unicyclist quest has begun
		if (unicyclistQuest.getState() == QUEST_INCOMPLETE && !rendered_socks)
		{
			Socks::createSocks(socks_pos);
			std::cout << "created socks at pos " << socks_pos.x << ", "
					  << socks_pos.y << std::endl;
			rendered_socks = true;
			playerHasSocks = 0;
		}
		// only render a scapula bone when the unicyclist quest has ended
		if (unicyclistQuest.getState() == END && !rendered_unicyclist_reward)
		{
			auto& transform = _ecs.get<Motion>(unicyclist);
			vec2 bone_pos = {transform.position.x - abs(transform.scale.x),
							 transform.position.y +
								 abs(transform.scale.y / 2.f)};
			Bone::createBone(bone_pos, (BoneAsset)1);
			std::cout << "created scapula bone at pos " << bone_pos.x << ", "
					  << bone_pos.y << std::endl;
			rendered_unicyclist_reward = true;
			socksEffect = false;
		}
	}
	auto litter_quest_girl = _ecs.view<LitterQuestGirl>()[0];
	if (_ecs.has<Conversation>(litter_quest_girl))
	{
		auto& litterQuest = _ecs.get<Conversation>(litter_quest_girl);
		// only render the litter if the litter quest has begun
		if (litterQuest.getState() == QUEST_INCOMPLETE && !rendered_litter)
		{
			for (int i = 0; i < num_litter; i++)
			{
				Litter::createLitter(litter[i], (LitterAsset)i);
				std::cout << "created litter at pos " << litter[i].x << ", "
						  << litter[i].y << std::endl;
			}
			rendered_litter = true;
		}
		// only render a flipper bone if the litter quest has ended
		if (litterQuest.getState() == END && !rendered_litter_reward)
		{
			auto& transform = _ecs.get<Motion>(litter_quest_girl);
			vec2 bone_pos = {transform.position.x - abs(transform.scale.x),
							 transform.position.y +
								 abs(transform.scale.y / 1.5f)};
			Bone::createBone(bone_pos, (BoneAsset)6);
			std::cout << "created flipper bone at pos " << bone_pos.x << ", "
					  << bone_pos.y << std::endl;
			rendered_litter_reward = true;
		}
	}

	// check the state of the informant squirrel quest and render a ball if the convo is complete
	auto informant_squirrel = _ecs.view<InformantSquirrel>()[0];
	if (_ecs.has<Conversation>(informant_squirrel))
	{
		auto& informant_squirrel_quest =
			_ecs.get<Conversation>(informant_squirrel);
		if (informant_squirrel_quest.getState() == QUEST_INCOMPLETE && !rendered_ball)
		{
			quests_completed[INFORMANT_SQUIRREL] = true;
		}
		if (informant_squirrel_quest.getState() == QUEST_COMPLETE && !rendered_ball)
		{
			auto& motion = _ecs.get<Motion>(informant_squirrel);
			Ball::createBall(
				{motion.position.x - 350.f, motion.position.y + 200.f});
			rendered_ball = true;
		}
	}
	// check if all the litter is in dumpster, if yes, update litter quest state
	if (!isGrabbing)
	{
		auto& dumpster = _ecs.get<Motion>(_ecs.view<Dumpster>()[0]);
		for (auto litter : _ecs.view<Litter>())
		{
			auto& litterTransform = _ecs.get<Motion>(litter);

			if ((litterTransform.position.x >
					 (dumpster.position.x - abs(dumpster.scale.x / 2.f)) &&
				 litterTransform.position.x <
					 (dumpster.position.x + abs(dumpster.scale.x / 2.f))) &&
				(litterTransform.position.y >
					 (dumpster.position.y - abs(dumpster.scale.y / 2.f)) &&
				 litterTransform.position.y <
					 (dumpster.position.y + abs(dumpster.scale.y / 2.f))))
			{
				// destroy litter that is dragged inside the dumpster
				_ecs.destroy(litter);
				if (_ecs.view<Litter>().size() == 0)
					quests_completed[LITTER] = true;
			}
		}
	}

	// Update particles
	updateDirtParticles(elapsed_ms / 1000.f);
}

// Reset the world state to its initial state
void WorldSystem::restart(bool loadSave)
{
	// Debugging for memory/component leaks
	printECSState(_ecs);
	std::cout << "Restarting\n";

	// Reset points
	points = 0;

	// Reset the game speed
	current_speed = 1.f;

	// reset sock effect
	socksEffect = false;

	// reset ball respawn
	ballRespawn = false;
	ballRespawnTimer = 15000;

	// reset congratsPanel booleans
	currentlyDisplayingCongrats = false;
	haveDisplayedHalfway = false;

	auto conversationView = _ecs.view<Conversation>();
	for (auto entity : conversationView)
	{
		_ecs.destroy(entity);
	}

	// Remove all entities that we created
	auto motionView = _ecs.view<Motion>();
	for (auto entity : motionView)
		_ecs.destroy(entity);

	auto pathfinderView = _ecs.view<PathFinder>();
	for (auto entity : pathfinderView)
	{
		auto& pathfinder = _ecs.get<PathFinder>(entity);
		pathfinder.path.clear();
		pathfinder.pathInitialized = false;
		pathfinder.pathIndex = 0;
	}

	// Remove all particles
	auto foliage = _ecs.view<FoliageParticle>();
	for (auto e : foliage)
		_ecs.destroy(e);

	// Debugging for memory/component leaks
	printECSState(_ecs);

	// Create the player
	// tested the rendering and controls with a placeholder texture.
	// Initialize the starting map
	load_level(levels_path("mainmall.txt"));	
	load_scripts();

	for (int i = 0; i < NUM_QUESTS; i++)
	{
		quests_completed[i] = false;
	}

	rendered_litter = false;
	rendered_litter_reward = false;
	rendered_socks = false;
	rendered_unicyclist_reward = false;
	rendered_ball = false;
	currentlyConversing = false;

	// start with the tutorial conversation (player should be within range of Beaty)
	if (!loadSave)
	{
		auto newConversationView = _ecs.view<Conversation>();
		for (auto c : newConversationView)
		{
			auto& player_motion = _ecs.get<Motion>(main_player);
			const auto& motion = _ecs.get<Motion>(c);
			if (isWithinDistance(
					motion, player_motion,
					CONVERSATION_DISTANCE)) // might need to increase the
											// distance for beaty building
			{
				converse(_ecs.get<Conversation>(c));
			}
		}
	}

	if (loadSave)
	{
		load_save_state(levels_path("savestate.txt"));	
	}
	// Initialize the starting map
	std::cout << "Loaded map\n";
}

// Compute collisions between entities

void WorldSystem::handle_collisions()
{
	// Loop over all collisions detected by the physics system
	auto collisionsView = _ecs.view<PhysicsSystem::Collision>();
	const auto numCollisions = collisionsView.size();
	for (unsigned int i = 0; i < numCollisions; i++)
	{
		// The entity and its collider
		auto& collision = _ecs.get<PhysicsSystem::Collision>(collisionsView[i]);
		auto entity = collision.firstEntity;
		auto entity_other = collision.secondEntity;

		// Check to see if either entity has been destroyed
		if (!_ecs.valid(entity) || !_ecs.valid(entity_other))
			continue;

		if (_ecs.has<Player>(entity) || _ecs.has<Seagull>(entity) ||
			_ecs.has<Enemy>(entity))
		{
			if (_ecs.has<Building>(entity_other) ||
				((_ecs.has<Seagull>(entity) || _ecs.has<Enemy>(entity)) &&
				 _ecs.has<Seagull>(entity_other)))
			{
				auto& motion = _ecs.get<Motion>(entity);
				if (fabs(collision.uncollideMotion.x) >
					fabs(collision.uncollideMotion.y))
				{
					motion.position.y += collision.uncollideMotion.y;
				}
				else
				{
					motion.position.x += collision.uncollideMotion.x;
				}
			}
			else if (_ecs.has<Seagull>(entity_other))
			{
				playRandomSeagullSound();
			}
		}
		// Observer pattern for Bone
		if (_ecs.has<Bone>(entity_other) && _ecs.has<Player>(entity))
		{
			auto& playerInteraction = _ecs.get<Interaction>(entity);
			auto& boneInteraction = _ecs.get<Interaction>(entity_other);
			if (playerInteraction.canInteract && boneInteraction.canInteract &&
				boneInteraction.timer >= boneInteraction.interactionDelay)
			{
				setColorTimer(uiBonePanel, true);
				Bone bone = _ecs.get<Bone>(entity_other);
				if (bone.type == SKULL && !quests_completed[INTRO])
				{
					quests_completed[INTRO] = true;
					converse(_ecs.get<Conversation>(questEntities[INTRO]));
				}
				if (entity_other == flipper_in_tree)
				{
					quests_completed[POOR_SQUIRREL] = true;
				}
				Mix_PlayChannel(-1, bone_sound, 0);
				auto boneType = _ecs.get<Bone>(entity_other);
				lastPickedUp = boneType.type;
				_ecs.destroy(entity_other);
			}
		}
		else if (_ecs.has<Ball>(entity_other) && _ecs.has<Player>(entity))
		{
			auto& playerInteraction = _ecs.get<Interaction>(entity);
			if (playerInteraction.canInteract)
			{
				setColorTimer(uiBallPanel, true);
				_ecs.destroy(entity_other);
			}
		}
		else if (_ecs.has<Dirt>(entity_other) && _ecs.has<Player>(entity))
		{
			auto& playerInteraction = _ecs.get<Interaction>(entity);
			if (playerInteraction.canInteract)
			{
				// start a timer so any bone underneath doesn't get
				// immediately picked up at the same time
				const auto& dirtMotion = _ecs.get<Motion>(entity_other);
				auto boneView = _ecs.view<Bone>();
				entt::entity closestBone = entt::null;

				for (auto bone : boneView)
				{
					const auto& boneMotion = _ecs.get<Motion>(bone);
					if (isWithinDistance(dirtMotion, boneMotion, 100.f))
					{
						closestBone = bone;
						break;
					}
				}
				if (closestBone != entt::null)
				{
					auto& closestBoneInteraction =
						_ecs.get<Interaction>(closestBone);
					closestBoneInteraction.timer = 0.f;
				}

				spawnDirtParticlePuff(50, dirtMotion.position);
				_ecs.destroy(entity_other);
			}
		}
		else if (_ecs.has<Socks>(entity_other) && _ecs.has<Player>(entity))
		{
			auto& playerInteraction = _ecs.get<Interaction>(entity);
			if (playerInteraction.canInteract)
			{
				// mark the unicyclist quest completed when the player has
				// picked up socks
				quests_completed[UNICYCLIST] = true;
				socksEffect = true;
				_ecs.destroy(entity_other);
			}
		}
		else if (_ecs.has<Dirt>(entity_other) && _ecs.has<Player>(entity))
		{
			auto& playerInteraction = _ecs.get<Interaction>(entity);
			if (playerInteraction.canInteract)
			{
				// start a timer so any bone underneath doesn't get immediately
				// picked up at the same time
				const auto& dirtMotion = _ecs.get<Motion>(entity_other);
				auto boneView = _ecs.view<Bone>();
				entt::entity closestBone = entt::null;

				for (auto bone : boneView)
				{
					auto& boneMotion = _ecs.get<Motion>(bone);
					if (isWithinDistance(dirtMotion, boneMotion, 100.f))
					{
						closestBone = bone;
						break;
					}
				}
				if (closestBone != entt::null)
				{
					auto& closestBoneInteraction =
						_ecs.get<Interaction>(closestBone);
					closestBoneInteraction.timer = 0.f;
				}

				_ecs.destroy(entity_other);
			}
		}
		// Collisions involving enemy
		if (_ecs.has<Enemy>(entity))
		{
			if (_ecs.has<Ball>(entity_other))
			{
				auto& ballPhysics = _ecs.get<Physics>(entity_other);
				if (!ballPhysics
						 .enabled) // enemy can only pick up the ball if at rest
				{
					auto& motion = _ecs.get<Motion>(entity);
					motion.velocity = vec2(0, 0);
					_ecs.destroy(entity_other);
				}
			}
		}
		// Collisions with ball
		if (_ecs.has<Ball>(entity))
		{
			if (_ecs.has<Building>(entity_other))
			{
				if (bonetree == entity_other)
				{
					auto& flipper_physics = _ecs.get<Physics>(flipper_in_tree);
					flipper_physics.enabled = true;
				}
				auto& ballPhysics = _ecs.get<Physics>(entity);
				auto& ballMotion = _ecs.get<Motion>(entity);
				auto& buildingMotion = _ecs.get<Motion>(entity_other);

				if (fabs(collision.uncollideMotion.x) <
					fabs(collision.uncollideMotion.y))
				{
					// cases 1 & 2: ball hits left or right side of building
					if ((collision.uncollideMotion.x < 0 &&
						 ballMotion.velocity.x > 0) ||
						(collision.uncollideMotion.x > 0 &&
						 ballMotion.velocity.x < 0))
					{
						ballMotion.position.x += collision.uncollideMotion.x;
						if (ballPhysics.enabled)
							ballMotion.velocity = {ballPhysics.restitution.x *
													   -ballMotion.velocity.x,
												   ballPhysics.restitution.y *
													   ballMotion.velocity.y};
					}
				}
				else
				{
					// case 3: ball hits top of building
					if (collision.uncollideMotion.y < 0)
                    {
                        ballMotion.position.y += collision.uncollideMotion.y;
						ballPhysics.groundLevel = buildingMotion.position.y - buildingMotion.scale.y / 2.f;
                        if (ballPhysics.enabled)
                            ballMotion.velocity = {ballPhysics.restitution.x * ballMotion.velocity.x, ballPhysics.restitution.y * -ballMotion.velocity.y};
                    }

					// case 4: ball hits bottom of building
					else if (collision.uncollideMotion.y > 0)
					{
						ballMotion.position.y += collision.uncollideMotion.y;
						if (ballPhysics.enabled)
							ballMotion.velocity = {ballPhysics.restitution.x *
													   ballMotion.velocity.x,
												   ballPhysics.restitution.y *
													   -ballMotion.velocity.y};
					}
				}
			}
		}
	}

	// Destroy all collisions in the view, effectively clearing all collisions
	_ecs.destroy(collisionsView.begin(), collisionsView.end());
}

// Should the game be over ?
bool WorldSystem::is_over() const { return glfwWindowShouldClose(window) > 0; }

// On mouse button callback
// GLFW Documentation:
// https://www.glfw.org/docs/3.3/input_guide.html#input_mouse_button
void WorldSystem::on_mouse_button(int button, int action, int mod)
{
	// Mouse Click
	if (action == GLFW_PRESS || action == GLFW_RELEASE)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (action == GLFW_PRESS)
			{
				input_mouse_click = true;
				auto xpos = mouse_world_position.x;
				auto ypos = mouse_world_position.y;

				// check if it's within the litter image
				// update bool for whether the
				// player is picking up a litter or not
				auto motionView = _ecs.view<Motion>();

				for (auto& entity : motionView)
				{
					auto& motion = _ecs.get<Motion>(entity);
					auto motionx = motion.position.x;
					auto motiony = motion.position.y;
					auto motionscalex = motion.scale.x;
					auto motionscaley = motion.scale.y;
					if (_ecs.has<Litter>(entity))
					{
						if ((xpos > motionx - motionscalex / 2 &&
							 xpos < motionx + motionscalex / 2 &&
							 ypos > motiony - motionscaley / 2 &&
							 ypos < motiony + motionscaley / 2) &&
							isPlayerNearDumpster()) // within entity pos +-
													// size/2
						{
							isGrabbing = true;
							grabObject = entity;
							std::cout << "Grabbed at " << xpos << " " << ypos
									  << "\n";
						}
					}
				}
			}
			if (action == GLFW_RELEASE)
			{
				input_mouse_click = false;

				// update bool for whether the player is picking up a litter or
				// not
				isGrabbing = false;
			}
		}
	}
}

void WorldSystem::incrementVolume(int amount)
{
	fx_volume += amount;
	music_volume += amount;
	Mix_Volume(-1, fx_volume);
	Mix_VolumeMusic(music_volume);
	std::cout << "FX volume: " << fx_volume
			  << " , music volume: " << music_volume << std::endl;
}

// On key callback
// GLFW Documentation: https://www.glfw.org/docs/3.3/input_guide.html
void WorldSystem::on_key(int key, int, int action, int mod)
{
	if (controls)
	{
		if (!currentlyConversing && !currentlyDisplayingCongrats)
		{
			if (action == GLFW_PRESS || action == GLFW_RELEASE)
			{
				// allow for movement by arrow keys or WASD
				if (key == GLFW_KEY_UP || key == GLFW_KEY_W)
					input_up = action == GLFW_PRESS;
				else if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)
					input_down = action == GLFW_PRESS;
				else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A)
					input_left = action == GLFW_PRESS;
				else if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D)
					input_right = action == GLFW_PRESS;

				// play walking audio if any input captured, stop otherwise
				if (input_down || input_up || input_right || input_left)
				{
					if (!currentlyWalking)
					{
						currentlyWalking = true;
						Mix_PlayChannel(WALK, walk_sound, 0);
					}
				}
				else
				{
					currentlyWalking = false;
					Mix_HaltChannel(WALK);
				}
			}
			if (action == GLFW_PRESS && key == GLFW_KEY_B && playerHasBall > 0)
			{
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				throwBall({xpos, ypos});
			}
		}

		auto& playerInteraction = _ecs.get<Interaction>(main_player);
		auto& playerNight = _ecs.get<Night>(main_player);

		if (action == GLFW_PRESS && key == GLFW_KEY_F)
		{
			if (playerNight.flashlight_on == 1)
				playerNight.flashlight_on = 0;
			else
				playerNight.flashlight_on = 1;
		}

		if (action == GLFW_PRESS && key == GLFW_KEY_E)
		{
			playerInteraction.canInteract = true;
			// start conversation (TODO move to a separate function)
			auto conversationView = _ecs.view<Conversation>();
			for (auto c : conversationView)
			{
				auto& player_motion = _ecs.get<Motion>(main_player);
				const auto& motion = _ecs.get<Motion>(c);
				Conversation& conversation = _ecs.get<Conversation>(c);
				if (isWithinDistance(motion, player_motion,
									 CONVERSATION_DISTANCE) ||
					conversation.conversationInProgress())
				{
					converse(conversation);
				}
			}
			if (currentlyDisplayingCongrats)
			{
				if (_ecs.has<UIPanel>(congratsPanel))
				{
					_ecs.remove<UIPanel>(congratsPanel);
				}
				if (_ecs.has<UILabel>(congratsPanel))
				{
					_ecs.remove<UILabel>(congratsPanel);
				}
				if (_ecs.has<UILabel>(congratsPanelTooltip))
				{
					_ecs.remove<UILabel>(congratsPanelTooltip);
				}
				currentlyDisplayingCongrats = false;
			}
		}

		if (action == GLFW_RELEASE && key == GLFW_KEY_E)
			playerInteraction.canInteract = false;

		// Resetting game
		if (action == GLFW_RELEASE && key == GLFW_KEY_O)
		{
			int w, h;
			glfwGetWindowSize(window, &w, &h);

			restart(false);
		}
		// Resetting game
		if (action == GLFW_RELEASE && key == GLFW_KEY_T)
		{
			int w, h;
			glfwGetWindowSize(window, &w, &h);

			restart(true);
			std::cout << "Save loaded!";
		}

		// Resetting game
		if (action == GLFW_RELEASE && key == GLFW_KEY_U)
		{
			int w, h;
			glfwGetWindowSize(window, &w, &h);
			if (currentlyConversing)
			{
				std::cout << "Can't save during a conversation!";
			}
			else
			{
				write_save();
				std::cout << "Saved successfully!";
			}
		}

		// Debugging
		// changed this key input because of the movement overlap
		if (key == GLFW_KEY_L && action == GLFW_PRESS)
		{
			DebugSystem::in_debug_mode = !DebugSystem::in_debug_mode;
		}
		if (key == GLFW_KEY_P && action == GLFW_PRESS)
		{
			printECSState(_ecs);
		}
		if (key == GLFW_KEY_N && action == GLFW_PRESS)
		{
			lightEffectsOn = !lightEffectsOn;
		}

		// Control volume with - = (secondary functions _ +)
		if (key == GLFW_KEY_MINUS && (action == GLFW_PRESS))
		{
			incrementVolume(-VOLUME_INCREMENT);
		}
		if (key == GLFW_KEY_EQUAL && (action == GLFW_PRESS))
		{
			incrementVolume(VOLUME_INCREMENT);
		}

		// Start the rabbit running (for M2 demonstration purposes, remove
		// later)
		if (key == GLFW_KEY_G && action == GLFW_PRESS)
		{
			auto rabbit = _ecs.view<Rabbit>()[0];
			auto& rabbitMotion = _ecs.get<Motion>(rabbit);
			rabbitMotion.angle = 0.f;
		}

		// Control the current speed with `<` `>`
		if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) &&
			key == GLFW_KEY_COMMA)
		{
			current_speed -= 0.1f;
			std::cout << "Current speed = " << current_speed << std::endl;
		}
		if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) &&
			key == GLFW_KEY_PERIOD)
		{
			current_speed += 0.1f;
			std::cout << "Current speed = " << current_speed << std::endl;
		}
		current_speed = std::max(0.f, current_speed);
	}
}

void WorldSystem::on_mouse_move(vec2 mouse_pos)
{
	mouse_screen_position = mouse_pos;
}

void WorldSystem::loadUI(glm::ivec2 screenSize)
{
	gameFont = UI::loadFont("data/fonts/comic.ttf");
	UI::setFont(gameFont, 50);
	// Could load more fonts here if desired

	float loadingScreenWidth = 1200;
	float loadingScreenHeight = 800;
	uiLoadingScreenTexture.load_from_file("data/textures/StartScreen.png");
	uiLoadingScreenPanel = _ecs.create();
	_ecs.emplace<UIPanel>(
		uiLoadingScreenPanel,
		UIPanel{{-7, -7, 998},
				{loadingScreenWidth + 14, loadingScreenHeight + 14},
				{1, 1, 1, 1},
				40});
	_ecs.emplace<UIImage>(
		uiLoadingScreenPanel,
		UIImage{
			&uiLoadingScreenTexture,                             // texture
			{-7, -7, 999},                                       // position
			{loadingScreenWidth + 14, loadingScreenHeight + 14}, // size
			{1, 1, 1, 1},                                        // color
			0,                                                   // angle
		});
	_ecs.emplace<UILabel>(uiLoadingScreenPanel,
						  UILabel{
							  gameFont,        // font
							  50,              // font size
							  "START",         // text
							  {500, 10, 1000}, // position
							  {150, 100},      // size
							  {0, 0, 0, 1}     // color
						  });
	playButton = UI::createClickable({500, 10}, {125, 100});

	// Add bone counter panel
	float boneCounterPanelWidth = 300;
	uiBoneTexture.load_from_file("data/textures/skull.png");
	uiBonePanel = _ecs.create();
	_ecs.emplace<UIPanel>(uiBonePanel, UIPanel{{10, 10, 1},
											   {boneCounterPanelWidth, 80},
											   {1, 1, 0, uiPanelOpacity},
											   40});
	_ecs.emplace<UIImage>(uiBonePanel,
						  UIImage{
							  &uiBoneTexture, // texture
							  {40, 20, 2},    // position
							  {60, 60 / uiBoneTexture.aspectRatio()}, // size
							  uiPanelColor,                           // color
							  15.0f,                                  // angle
						  });
	_ecs.emplace<UILabel>(uiBonePanel,
						  UILabel{
							  gameFont,                          // font
							  50,                                // font size
							  "",                                // text
							  {100, 30, 2},                      // position
							  {boneCounterPanelWidth - 100, 50}, // size
							  {0, 0, 0, uiPanelOpacity}          // color
						  });
	button = UI::createClickable({10, 10}, {boneCounterPanelWidth, 80});

	// add ball counter panel
	uiBallTexture.load_from_file("data/textures/ball.png");
	uiBallPanel = _ecs.create();
	_ecs.emplace<UIPanel>(uiBallPanel, UIPanel{{screenSize.x - 310, 10, 1},
											   {300, 80},
											   {1, 1, 0, uiPanelOpacity},
											   40});
	_ecs.emplace<UIImage>(uiBallPanel,
						  UIImage{
							  &uiBallTexture,
							  {screenSize.x - 280, 20, 2},
							  {60, 60 / uiBallTexture.aspectRatio()},
							  uiPanelColor,
							  15.0f,
						  });
	_ecs.emplace<UILabel>(uiBallPanel, UILabel{gameFont,
											   50,
											   "",
											   {screenSize.x - 210, 30, 2},
											   {300, 50},
											   {0, 0, 0, uiPanelOpacity}});

	// create Tooltip/basic controls
	float toolTipHeight = screenSize.y - 50;
	uiToolTipTexture.load_from_file("data/textures/questionMark.png");
	uiToolTipPanel = _ecs.create();
	_ecs.emplace<UIPanel>(
		uiToolTipPanel,
		UIPanel{{10, toolTipHeight, 1}, {40, 40}, {1, 1, 1, 0}, 40});
	_ecs.emplace<UIImage>(uiToolTipPanel,
						  UIImage{
							  &uiToolTipTexture,
							  {13, toolTipHeight + 2, 2},
							  {35, 35 / uiToolTipTexture.aspectRatio()},
							  {0, 0, 0, 1},
							  0.0f,
						  });
	float toolTipOpacity = 0;
	// ToolTip Words
	// uiToolTipWordsTexture.load_from_file("data/textures/toolTipText.png");
	uiToolTipWordsPanel = _ecs.create();
	_ecs.emplace<UIPanel>(
		uiToolTipWordsPanel,
		UIPanel{{60, toolTipHeight - 65, 1}, {200, 110}, {1, 1, 1, 0}, 40});
	_ecs.emplace<UILabel>(uiToolTipWordsPanel,
						  UILabel{
							  gameFont, // font
							  13,       // font size
							  "W/Up = Move Up"
							  "\nA/Left = Move Left"
							  "\nS/Down = Move Down"
							  "\nD/Right = Move Right"
							  "\nB = Throw Ball"
							  "\nE = Interact",             // text
							  {90, toolTipHeight - 110, 2}, // position
							  {200, 150},                   // size
							  {0, 0, 0, 0}                  // color
						  });

	tooltip = UI::createClickable({10, toolTipHeight}, {400, 80});
}

void WorldSystem::throwBall(vec2 target)
{
	auto& playerMotion = _ecs.get<Motion>(main_player);

	// ball's initial position is just in front of the player
	vec2 position = {0.f, 0.f};
	if (playerMotion.scale.x < 0.f)
		position = {playerMotion.position.x + abs(playerMotion.scale.x) / 1.5f,
					playerMotion.position.y - abs(playerMotion.scale.y) / 1.7f};
	else
		position = {playerMotion.position.x - abs(playerMotion.scale.x) / 1.5f,
					playerMotion.position.y - abs(playerMotion.scale.y) / 1.7f};

	entt::entity ball = Ball::createBall(position);
	auto& ballMotion = _ecs.get<Motion>(ball);

	vec2 direction = normalize(mouse_world_position - playerMotion.position);

	// clamp so that can't throw backwards: constrain to first "quadrant"
	if (playerMotion.scale.x < 0.f)
		direction = normalize(clamp(direction, {0.f, -1.f}, {1.f, 0.f}));
	else
		direction = normalize(clamp(direction, {-1.f, -1.f}, {0.f, 0.f}));

	float speed = 500.f;
	ballMotion.velocity = speed * direction;

	auto& ballPhysics = _ecs.get<Physics>(ball);
	ballPhysics.groundLevel =
		playerMotion.position.y + playerMotion.scale.y / 2.f;
	ballPhysics.enabled = true;

	setColorTimer(uiBallPanel, false);

	playerHasBall -= 1;

	Conversation& ballTutorialConversation =
		_ecs.get<Conversation>(questEntities[BALL_TUTORIAL]);
	if (ballTutorialConversation.getState() == QUEST_INCOMPLETE)
	{
		quests_completed[BALL_TUTORIAL] = true;
		if (isWithinDistance(_ecs.get<Motion>(questEntities[BALL_TUTORIAL]),
							 _ecs.get<Motion>(main_player),
							 3 * CONVERSATION_DISTANCE))
		{
			// TODO you are allowed to wander a bit farther away from the water
			// bowl to throw the ball, and the conversation will automatically
			// continue. But if the water bowl is out of the screen, the player
			// can't see the text being displayed. change the text to be
			// displayed above one of the UI panels so it is always in the
			// screen (change the entity associated with this to a UI panel
			// entity?)
			converse(ballTutorialConversation);
		}
	}
}

bool WorldSystem::isPlayerNearDumpster()
{
	auto& playerMotion = _ecs.get<Motion>(main_player);
	auto dumpster = _ecs.view<Dumpster>()[0];
	auto& dumpsterMotion = _ecs.get<Motion>(dumpster);
	float minDistance = 300.f;
	vec2 distanceVector = dumpsterMotion.position - playerMotion.position;
	float distanceSq = dot(distanceVector, distanceVector);
	if (distanceSq < pow(minDistance, 2))
		return true;
	return false;
}

void WorldSystem::dropBone()
{
	entt::entity e = _ecs.view<Enemy>()[0];
	auto& enemyBoolean = _ecs.get<Enemy>(e).takenBone;
	auto& enemyMotion = _ecs.get<Motion>(e);
	if (enemyBoolean)
	{
		vec2 position = enemyMotion.position;
		entt::entity newBone =
			Bone::createBone(position, (BoneAsset)lastPickedUp);
		enemyBoolean = false;
	}
}

void WorldSystem::displayText(char* text) 
{
	if (!congratsPanelsInitialized)
	{
		congratsPanelsInitialized = true;
		congratsPanel = _ecs.create();
		congratsPanelTooltip = _ecs.create();
	}

	float fontSize = 20; // increase
	vec2 maxSpeechBubbleSize = {fontSize * 20, fontSize * 5};
	const auto& motion = _ecs.get<Motion>(main_player);
	glm::ivec3 pos = UI::worldPosToUIPos(vec2(motion.position.x, motion.position.y - motion.scale.y), window_size_gu);
	float x_pos = pos.x;
	float y_pos = pos.y;
	float opacity = 1.f;
	UI::setFont(gameFont, fontSize); // is this needed
	vec2 size = UI::getMaxLineWidthAndNumLines(text, maxSpeechBubbleSize,
											   gameFont, fontSize);
	vec2 panelSize = vec2(size.x + fontSize * 3, size.y + fontSize * 2);
	_ecs.emplace<UIPanel>(
		congratsPanel, UIPanel{{x_pos, y_pos, 1}, panelSize, {1, 1, 1, opacity}, 40});
	_ecs.emplace<UILabel>(
		congratsPanel,
		UILabel{
			gameFont,                                           // font
			fontSize,                                           // font size
			text,                                          // text
			{x_pos + fontSize, y_pos, 2},                       // position
			vec2(size.x + fontSize * 2, size.y + fontSize * 2), // size
			{0, 0, 0, opacity}                                  // color
		});
	char* tooltipText = "Press E to continue...";
	float tooltipFontSize = fontSize / 2;
	vec2 tooltipSize = UI::getMaxLineWidthAndNumLines(
		tooltipText, maxSpeechBubbleSize, gameFont, tooltipFontSize);
	if (tooltipSize.x > panelSize.x)
	{
		tooltipText = "Press E...";
		tooltipSize = UI::getMaxLineWidthAndNumLines(
			tooltipText, maxSpeechBubbleSize, gameFont, tooltipFontSize);
	}
	UILabel label = UILabel{
		gameFont,        // font
		tooltipFontSize, // font size
		tooltipText,     // text
		{x_pos + panelSize.x - tooltipSize.x - tooltipFontSize,
		 y_pos + tooltipSize.y, 2}, // position
		tooltipSize,                // size
		{0.6, 0.6, 0.6, opacity}    // color
	};
	_ecs.emplace<UILabel>(congratsPanelTooltip, label);

	currentlyDisplayingCongrats = true;
}

void WorldSystem::update(Event event)
{
	switch (event)
	{
	case BONE_COUNT_INCREASE:
		points += 1;
		std::cout << "points is " << points << ", total bone count is "
				  << total_bone_count << ", fraction is "
				  << float(points) / float(total_bone_count) << std::endl;
		if (float(points) / float(total_bone_count) == 1.0)
		{
			displayText(
				"Congratulations, Grover. Using your dog-given abilities, you "
				"have retrieved all of the blue whale bones!\nFrank will be "
				"going back to dog jail, so he won't be causing any nuisances "
				"around here for a while. Feel free to wander the campus for "
				"as long as you like.\nAll the residents of UBC thank you!");
			_ecs.destroy(_ecs.view<Enemy>()[0]);
		}
		if (float(points) / float(total_bone_count) >= 0.5)
		{
			if (!haveDisplayedHalfway && !currentlyDisplayingCongrats)
			{
				displayText("You've collected HALF of the bones! Damn, they "
							"should call you Santa Bone-o.");
				haveDisplayedHalfway = true;
			}
		}
		break;
	case BONE_COUNT_DECREASE:
		if (points > 0)
		{
			setColorTimer(uiBonePanel, false);
			entt::entity e = _ecs.view<Enemy>()[0];
			auto& enemy = _ecs.get<Enemy>(e).takenBone = true;
			points -= 1;
			Conversation& enemyConversation = _ecs.get<Conversation>(e);
			if (enemyConversation.getState() == INITIAL ||
				enemyConversation.getState() == QUEST_INCOMPLETE)
			{
				quests_completed[FRANK_ATTACK] = true;
				converse(enemyConversation);
			}
		}
		break;
	case BALL_PICKED_UP:
		dropBone();
		ballRespawn = true;
		break;
	case BALL_DROPPED:
		playerHasBall -= 1;
		break;
	case BALL_PICKED_UP_BY_PLAYER:
		playerHasBall += 1;
		break;
	case SOCKS_PICKED_UP:
		playerHasSocks += 1;
		break;
	default:
		std::cout << "Invalid event passed to WorldSystem::update" << std::endl;
	}
}

void WorldSystem::spawnDirtParticlePuff(uint32_t count, vec2 position)
{
	const float xRadius = 10;
	const float yRadius = 2;
	const float angleVariation = 45; // degrees
	const float minSize = 50;
	const float maxSize = 100;
	const float minSpeed = 300;
	const float maxSpeed = 500;
	for (uint32_t i = 0; i < count; i++)
	{
		// Random position
		float rx = (uniform_dist(rng) * 2 - 1) * xRadius;
		float ry = (uniform_dist(rng) * 2 - 1) * yRadius;
		auto pos = position + vec2{rx, ry};

		// Random velocity
		float rAngle = (uniform_dist(rng) * 2 - 1) * angleVariation;
		float angle = -90 + rAngle; // degrees
		float angleRads = (PI / 180.f) * angle;
		float speed = minSpeed + uniform_dist(rng) * (maxSpeed - minSpeed);
		auto vel = speed * vec2{cos(angleRads), sin(angleRads)};
		// Logger::info(angle, " " , vel.x, " " , vel.y);

		// Random size
		float size = minSize + uniform_dist(rng) * (maxSize - minSize);

		DirtParticle::create(pos, vel, vec2{size, size});
	}
}

void WorldSystem::respawnBall() 
{ 
	Ball::createBall({500, 600}); 
	ballRespawn = false;
	ballRespawnTimer = 15000;
}
