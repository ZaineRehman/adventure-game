/*
 * CREATED BY ZAINE REHMAN
 * MUSIC BY RAVI ZUTCHI
 * 
 * started 10/10/2024
 * 
 * 
 */

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <string>
#include <sstream>

// https://solhsa.com/soloud/quickstart.html
//#define WITH_MINIAUDIO
#include "SoLoud/soloud.h"
#include "SoLoud/miniaudio.h"
#include "SoLoud/soloud_wav.h"
#include "SoLoud/soloud_wavstream.h"

#ifdef _WIN32
	#include <conio.h>
	#include <windows.h>
#endif

#define LOOP(x) for(int i = 0; i < x; ++i)
#define SLEEP(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))

// time between frames in ms (default 50ms)
constexpr int FRAME_DELAY = 50;

// show debug info?
#ifdef _WIN32
	constexpr bool DEBUG_INFO = false;
#else
	constexpr bool DEBUG_INFO = true;
#endif

// map size
int MAP_X = 0;
int MAP_Y = 0;

// cheats (for debugging ofc)
bool CHEATS_NODIE = false;

// mode to clear terminal (see clear function)
#ifdef _WIN32
	constexpr int CLEAR_MODE = 3;
#else
	constexpr int CLEAR_MODE = 2;
#endif

// key lookup table for every zone with a key
/*
 * B:  0b1000000000000000
 * C:  0b0100000000000000
 * D:  0b0010000000000000
 * E:  0b0001000000000000
 * F:  0b0000100000000000
 * G:  0b0000010000000000
 * ?#: 0b0000000011110000
 */
std::vector<std::pair<std::string,int>> KEY_LOOKUP {
	{"A3",0b1000000000000000}, // B key
	{"B2",0b0100000000000000}, // C key
	{"C5",0b0010000000000000}, // D key
	{"B5",0b0001000000000000}, // E key
	{"E4",0b0000100000000000}, // F key
	{"F1",0b0000010000000000}, // G key
	{"G5",0b0000000010000000}, // ?1 key
	{"A5",0b0000000001000000}, // ?2 key
	{"F2",0b0000000000100000}, // ?3 key
	{"E5",0b0000000000010000}  // ?4 key
};
// corresponding zones
const std::vector<std::string> KEY_LOOKUP_ZONES {"B","C","D","E","F","G","?1","?2","?3","?4"};

// stores if theres a coin in a zone or not
std::vector<std::pair<std::string,bool>> ZONE_COINS {};

// holds all adjacent rooms to the current room
std::vector<std::string> ADJACENT_ZONES {};

// store keypress
std::atomic<int> KEY(-1);
// for keys that store multiple things on the input buffer at once
std::atomic<int> KEY2(-1);
std::atomic<int> KEY3(-1);

bool QUIT = false;

// MAP.
std::vector<std::vector<char>> MAP (MAP_Y, std::vector<char> (MAP_X, ' '));

#ifdef _WIN32
void ShowConsoleCursor(bool showFlag) {
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO cursorInfo;

	GetConsoleCursorInfo(out, &cursorInfo);
	cursorInfo.bVisible = showFlag; // set the cursor visibility
	SetConsoleCursorInfo(out, &cursorInfo);
}
#endif

// very cool clear screen
inline void clear(uint8_t type = 3, uint16_t newLineCharNum = 72) {
	switch (type) {
		case 0:
			for (uint16_t i = 0; i <= newLineCharNum; ++i) std::cout << '\n';
			break;

		case 1:
			#ifdef _WIN32
			system("cls");
			#else
			system("clear");
			#endif
			break;

		case 2:
			std::cout<<"\e[1;1H\e[2J";
			break;
		
		#ifdef _WIN32
		case 3: {
			HANDLE hOut;
			COORD Position;
			hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			Position.X = 0;
			Position.Y = 0;
			SetConsoleCursorPosition(hOut, Position);
			break;
		}
		case 4: {
			// Get the Win32 handle representing standard output.
			// This generally only has to be done once, so we make it static.
			static const HANDLE hOut2 = GetStdHandle(STD_OUTPUT_HANDLE);

			CONSOLE_SCREEN_BUFFER_INFO csbi;
			COORD topLeft = { 0, 0 };

			// std::cout uses a buffer to batch writes to the underlying console.
			// We need to flush that to the console because we're circumventing
			// std::cout entirely; after we clear the console, we don't want
			// stale buffered text to randomly be written out.
			std::cout.flush();

			// Figure out the current width and height of the console window
			if (!GetConsoleScreenBufferInfo(hOut2, &csbi)) {
				// TODO: Handle failure!
				abort();
			}
			DWORD length = csbi.dwSize.X * csbi.dwSize.Y;
			
			DWORD written;

			// Flood-fill the console with spaces to clear it
			FillConsoleOutputCharacter(hOut2, TEXT(' '), length, topLeft, &written);

			// Reset the attributes of every character to the default.
			// This clears all background colour formatting, if any.
			FillConsoleOutputAttribute(hOut2, csbi.wAttributes, length, topLeft, &written);

			// Move the cursor back to the top left for the next sequence of writes
			SetConsoleCursorPosition(hOut2, topLeft);
			break;
		}
		#else
			case 3: break;
			case 4: break;
		#endif

		case 5:
			std::cout << "\033[2J\033[1;1H";
			break;
	}
}

// does not work
/*inline void cursor(bool show) {
	std::cout << show ? "\e[?25h" : "\e[?25l";
}*/

namespace Colors {
	const std::string
		reset = "\e[0m",
		
		bold          = "\e[1m",
		italic        = "\e[3m",
		underline     = "\e[4m",
		strikethrough = "\e[9m",
		mystery1      = "\e[31m",
		mystery2      = "\x1B[31m",
		
		black  = "\e[0;30m",
		red    = "\e[0;31m",
		green  = "\e[0;32m",
		yellow = "\e[0;33m",
		blue   = "\e[0;34m",
		purple = "\e[0;35m",
		cyan   = "\e[0;36m",
		white  = "\e[0;37m",
		
		black_bold  = "\e[1;30m",
        red_bold    = "\e[1;31m",
        green_bold  = "\e[1;32m",
        yellow_bold = "\e[1;33m",
        blue_bold   = "\e[1;34m",
        purple_bold = "\e[1;35m",
        cyan_bold   = "\e[1;36m",
        white_bold  = "\e[1;37m",
        
        black_underline  = "\e[4;30m",
        red_underline    = "\e[4;31m",
        green_underline  = "\e[4;32m",
        yellow_underline = "\e[4;33m",
        blue_underline   = "\e[4;34m",
        purple_underline = "\e[4;35m",
        cyan_underline   = "\e[4;36m",
        white_underline  = "\e[4;37m",
        
        black_background  = "\e[40m",
        red_background    = "\e[41m",
        green_background  = "\e[42m",
        yellow_background = "\e[43m",
        blue_background   = "\e[44m",
        purple_background = "\e[45m",
        cyan_background   = "\e[46m",
        white_background  = "\e[47m",
        
        black_highIntensity  = "\e[0;90m",
        red_highIntensity    = "\e[0;91m",
        green_highIntensity  = "\e[0;92m",
        yellow_highIntensity = "\e[0;93m",
        blue_highIntensity   = "\e[0;94m",
        purple_highIntensity = "\e[0;95m",
        cyan_highIntensity   = "\e[0;96m",
        white_highIntensity  = "\e[0;97m",
        
        black_boldHighIntensity  = "\e[1;90m",
        red_boldHighIntensity    = "\e[1;91m",
        green_boldHighIntensity  = "\e[1;92m",
        yellow_boldHighIntensity = "\e[1;93m",
        blue_boldHighIntensity   = "\e[1;94m",
        purple_boldHighIntensity = "\e[1;95m",
        cyan_boldHighIntensity   = "\e[1;96m",
        white_boldHighIntensity  = "\e[1;97m",
        
        black_backgroundHighIntensity  = "\e[0;100m",
        red_backgroundHighIntensity    = "\e[0;101m",
        green_backgroundHighIntensity  = "\e[0;102m",
        yellow_backgroundHighIntensity = "\e[0;103m",
        blue_backgroundHighIntensity   = "\e[0;104m",
        purple_backgroundHighIntensity = "\e[0;105m",
        cyan_backgroundHighIntensity   = "\e[0;106m",
        white_backgroundHighIntensity  = "\e[0;107m";
};

void inputHelper() {
	while (1) {
		// windows and linux have different key mappings isnt that lovely
		// and!!! windows uses both 224 and 0 for the first value in the buffer sometimes!
		#ifdef _WIN32
			#define INPUT_FUNC _getch
		#else
			#define INPUT_FUNC getchar
		#endif

		int key = INPUT_FUNC();
		// do i even need this check though
		if (key != EOF) KEY = key;
		
		// idc it works and it works fine
		// checks for keyboard inputs that put multiple things on the input buffer
		// (esc, F1, arrow keys, etc)
		// still have yet to crack the block of keys with insert, home, pgup
		#ifdef _WIN32
			if ((key == 224) || (key == 0)) KEY2 = INPUT_FUNC();
		#else
			if (key == 27) KEY2 = INPUT_FUNC();
			if ((KEY2 == 91) || (KEY2 == 79)) KEY3 = INPUT_FUNC();
		#endif
	}
	// should never get here so ill have fun with random code if i wanna
	// non letter key = 27
	__asm("nop");
	return;
}

void fillMap(char chr) {
	for (int i = 0; i < MAP_Y; ++i) {
		for (int x = 0; x < MAP_X; ++x) {
			MAP[i][x] = chr;
		}
	}
}

struct Ent {
	char type;
	char aggression;
	int16_t x;
	int16_t y;
	char health;
	char moveDelay;
};
// stores all active entities
std::vector<Ent> ENTS {};

struct Player {
	int16_t x;
	int16_t y;
	uint16_t inventory;
	char blit;
	int coins;

	// latest checkpoint zone
	std::string checkpoint;
	std::string zone; // accepted defeat
}; // theres literally only 1 of these i dont need to have it properly aligned

// DEBUGGING!
class Cheats {
public:
	Player* currentPlayer;
	
	Cheats(Player* player) {
		currentPlayer = player;
	}
	
	void noDie(bool option) {
		CHEATS_NODIE = option;
	}
	
	void giveKey(const std::string& zone) {
		switch (zone[0]) {
		case 'B':
			currentPlayer->inventory |= KEY_LOOKUP[0].second;
			KEY_LOOKUP[0].second = -1;
			break;
		case 'C':
			currentPlayer->inventory |= KEY_LOOKUP[1].second;
			KEY_LOOKUP[1].second = -1;
			break;
		case 'D':
			currentPlayer->inventory |= KEY_LOOKUP[2].second;
			KEY_LOOKUP[2].second = -1;
			break;
		case 'E':
			currentPlayer->inventory |= KEY_LOOKUP[3].second;
			KEY_LOOKUP[3].second = -1;
			break;
		case 'F':
			currentPlayer->inventory |= KEY_LOOKUP[4].second;
			KEY_LOOKUP[4].second = -1;
			break;
		case 'G':
			currentPlayer->inventory |= KEY_LOOKUP[5].second;
			KEY_LOOKUP[5].second = -1;
			break;
		case '?':
			switch (zone[1]) {
			case '1':
				currentPlayer->inventory |= KEY_LOOKUP[6].second;
				KEY_LOOKUP[6].second = -1;
				break;
			case '2':
				currentPlayer->inventory |= KEY_LOOKUP[7].second;
				KEY_LOOKUP[7].second = -1;
				break;
			case '3':
				currentPlayer->inventory |= KEY_LOOKUP[8].second;
				KEY_LOOKUP[8].second = -1;
				break;
			case '4':
				currentPlayer->inventory |= KEY_LOOKUP[9].second;
				KEY_LOOKUP[9].second = -1;
				break;
			}
			break;
		}
	}
};

/*
 * MAP KEY FILE
 * 
 * room number, 2 spaces, N, E, S, W with 1 space in between
 * no room = xx
 * 
 * example-
 * A0  A3 A1 A2 xx
 * 
 * CHECKPOINT ROOMS: A0, B1, C3, E1, F3, G1
 * 
 */
std::vector<std::string> adjacentZones(const std::string& zone) {
	std::vector<std::string> zones (4,"");
	// NORTH, EAST, SOUTH, WEST
	
	// i dont want to talk about it.
	//    malloc_consolidate(): unaligned fastbin chunk detected
	//    free(): invalid next size (fast)
	//    Segmentation fault
	// all of which "Aborted"
	// literally just from the std::ifstream line where it reads the file
	// ONLY FOR ROOM A2
	// ... adding on any i find: B2
	// AND LITERALLY ZERO HELPFUL INFO ONLINE
	if (zone == "A2") {
		zones = {"A0","?3","E2","xx"};
		return zones;
	}
	if (zone == "B2") {
		zones = {"B1","C3","C4","B4"};
		return zones;
	}
	
	if (DEBUG_INFO) std::cout << "adjacentZones: BEFORE LOADING FILE\n";
	std::ifstream file("maps/KEY.txt");
	if (DEBUG_INFO) std::cout << "adjacentZones: AFTER LOADING FILE\n";
	std::string line;
	
	if (file.is_open()) {
		std::cout << "adjacentZones: FILE OPEN\n";
		while (std::getline(file,line)) {
			if (line.substr(0,2) == zone) {
				// 4,5  7,8  10,11  13,14
				zones[0] = line.substr(4,2);
				zones[1] = line.substr(7,2);
				zones[2] = line.substr(10,2);
				zones[3] = line.substr(13,2);
				break;
			}
		}
		file.close();
	}
	else std::cout << "adjacentZones: where yo key file at!??\n";
	
	return zones;
}

/*
 * INVENTORY SLOTS STORED IN UI16
 * 
 *   1    2    3    4
 * 0000 0000 0000 0000
 * 
 * group 1: keys BCDE
 * group 2: keys FG
 * group 3: ? keys 1-4
 * group 4: potions?
 * 
 */
 
 /*
  * MAP FILES
  * 
  * line 1: map size (x,y)
  * 
  * next lines have stats for any ents in the area
  * 	 (type aggression x y health movedelay)
  * the line after the last ent must have "END"
  * 
  * every line beyond that is for the map (MUST FIT DIMENSIONS FROM LINE 1)
  * ' ' or . = empty
  *        X = wall
  * 
  * MINIMUM MAP SIZE IS 7x4 IDK PROBABLY
  * 
  */

// i wonder what this function does
void loadMap(const Player& player) {
	#ifdef _WIN32
		clear(1);
	#endif

	std::stringstream fileName;
	fileName << "maps/" << player.zone << ".txt";
	if (DEBUG_INFO) std::cout << "loadMap: opening file\n";
	std::ifstream mapFile(fileName.str());
	if (DEBUG_INFO) std::cout << "loadMap: file opened\n"; // segfault passed here
	
	std::string line;
	int mapX, mapY;
	int lineNum = 0;
	int mapLine = 0;
	std::vector<std::vector<char>> loadedMap {};
	
	ENTS = {};
	
	if (mapFile.is_open()) {
		bool ended = false;
		while (std::getline(mapFile,line)) {
			if (DEBUG_INFO) std::cout << "loadMap: READING LINE\n";
			lineNum++;
			//std::cout << "\tREADING LINE: " << line << '\n';
			// map dimensions
			if (lineNum == 1) {
				int commaLine = 0;
				// find the comma, everything before it is X
				for (int i = 0; i < (int)line.length(); ++i) {
					if (line[i] == ',') {
						commaLine = i;
						mapX = std::stoi(line.substr(0,i));
						break;
					}
				}
				// everything after comma is Y
				mapY = std::stoi(line.substr(commaLine+1,line.length()-commaLine));
				continue;
			}
			
			// done with loading ents, put together map vec
			if (line == "END") {
				ended = true;
				for (int a = 0; a < mapY; ++a) {
					std::vector<char> temp {}; 
					for (int b = 0; b < mapX; ++b)
						temp.push_back(' ');
					loadedMap.push_back(temp);
				}
				continue;
			}
			
			// ents
			// order: type aggression x y health movedelay
			if (!ended) {
				if (DEBUG_INFO) std::cout << "\tloadMap: LOADING ENT\n";
				char type, aggression, health, moveDelay;
				int16_t x, y;
				
				int commaLines[5] {-1};
				int commaNum = 0;
				
				for (int i = 0; i < (int)line.length(); ++i) {
					if (line[i] == ',') {
						commaLines[commaNum] = i;
						commaNum++;
					}
				}
				if (commaNum != 5) {
					// FUCK YOU THE FILE ISNT FORMATTED RIGHT
					std::cout << Colors::red_boldHighIntensity << "ENTS NOT FORMATTED RIGHT\n" << Colors::reset;
				}
				
				// assemble the ent
				// that lowkey sounds badass
				
				type       = std::stoi(line.substr(0,commaLines[0]));
				aggression = std::stoi(line.substr(commaLines[0]+1,commaLines[1]-commaLines[0]));
				x          = std::stoi(line.substr(commaLines[1]+1,commaLines[2]-commaLines[1]));
				y          = std::stoi(line.substr(commaLines[2]+1,commaLines[3]-commaLines[2]));
				health     = std::stoi(line.substr(commaLines[3]+1,commaLines[4]-commaLines[3]));
				moveDelay  = std::stoi(line.substr(commaLines[4]+1,line.length()-commaLines[4]));
								
				Ent newEnt {type,aggression,x,y,health,moveDelay};
				ENTS.push_back(newEnt);
				
				//printEntInfo(newEnt);
			}
			
			// time to actually load the map
			else {
				if (DEBUG_INFO) std::cout << "\tloadMap: ACTUALLY LOADING MAP\n";
				if (mapLine > mapY) break;
				for (int i = 0; i < mapX; ++i) {
					if (DEBUG_INFO) std::cout << "\t\tloadMap: lmX = " << loadedMap[0].size() << ", lmY = " << loadedMap.size() << ", checking x=" << i << ", y=" << mapLine << '\n'; 
					switch (line[i]) {
					case ' ':
					case '.': {
						loadedMap[mapLine][i] = ' ';
						break;
					}
					default:
						loadedMap[mapLine][i] = line[i];
						break;
					}
				}
				mapLine++;
			}
			
		}
		mapFile.close();
		
		MAP_X = mapX;
		MAP_Y = mapY;
		
		MAP = loadedMap;
	}
	// else ahg... fack
	else {
		std::cout << Colors::red_boldHighIntensity << "Failed to open file: " << fileName.str() << Colors::reset << '\n';
	}
	ADJACENT_ZONES = adjacentZones(player.zone);
}

// can player go in zone?
bool checkZone(const std::string& wantedZone, const Player& player) {
	bool canGo = false;
	
	// check permission
		 if  (wantedZone[0] == 'A') canGo = true;
	else if ((wantedZone[0] == 'B') && (player.inventory & 0b1000000000000000)) canGo = true;
	else if ((wantedZone[0] == 'C') && (player.inventory & 0b0100000000000000)) canGo = true;
	else if ((wantedZone[0] == 'D') && (player.inventory & 0b0010000000000000)) canGo = true;
	else if ((wantedZone[0] == 'E') && (player.inventory & 0b0001000000000000)) canGo = true;
	else if ((wantedZone[0] == 'F') && (player.inventory & 0b0000100000000000)) canGo = true;
	else if ((wantedZone[0] == 'G') && (player.inventory & 0b0000010000000000)) canGo = true;
	else if  (wantedZone[0] == '?') {
		switch (wantedZone[1]) {
			case '1':
				if (player.inventory & 0b0000000010000000) canGo = true;
				break;
			case '2':
				if (player.inventory & 0b0000000001000000) canGo = true;
				break;
			case '3':
				if (player.inventory & 0b0000000000100000) canGo = true;
				break;
			case '4':
				if (player.inventory & 0b0000000000010000) canGo = true;
				break;
		}
	}
	return canGo;
}

// returns the coordinates for respawning at checkpoints
std::pair<int,int> checkpointLookup(const std::string& zone) {
	switch(zone[0]) {
		case 'A': return {1,1};
		default: return {0,0}; 
	}
}

// returns the color associated with a zone
// main means highlight or text folor
std::string zoneColor(const std::string& zone, bool main = true) {
	std::string zoneColor = "";
	
	switch (zone[0]) {
		case 'A':
			zoneColor = main ? Colors::cyan_background : Colors::cyan;
			break;
		case 'B':
			zoneColor = main ? Colors::blue_background : Colors::blue;
			break;
		case 'C':
			zoneColor = main ? Colors::yellow_backgroundHighIntensity : Colors::yellow;
			break;
		case 'D':
			zoneColor = main ? Colors::red_background : Colors::red;
			break;
		case 'E':
			zoneColor = main ? Colors::purple_background : Colors::purple;
			break;
		case 'F':
			zoneColor = main ? Colors::green_background : Colors::green;
			break;
		case 'G':
			zoneColor = main ? Colors::purple_backgroundHighIntensity : Colors::purple_highIntensity;
			break;
		case '?':
			zoneColor = main ? Colors::black+Colors::white_background : "";
			break;
	}
	return zoneColor;
}

// pauses the game and shows the map
void showMap(const Player& player) {
	clear(1);
	// Open the map file
	std::vector<std::string> lines {};

	std::ifstream mapFile("MAP.txt", std::ios::binary);

	if (!mapFile.is_open()) {
		std::cerr << "Failed to open MAP.txt\n";
		return;
	}

	std::string line;
	while (std::getline(mapFile, line)) {
		lines.push_back(line);
	}
	mapFile.close();

	while (1) {
		clear(CLEAR_MODE);
		if (KEY == 113) break;

		std::stringstream atEnd {};
		for (const std::string i : lines) {
			std::string color = "";
			int index = 0;
			for (const char x : i) {
				if (x >= '0' && x <= '9') {
					std::cout << color << x << Colors::reset;
					color = "";
					index++;
					continue;
				}
				if ((x >= 'A' && x <= 'G') || (x == '?')) {
					char nextToCheck = i[index+1];
					if (x == player.zone[0] && i[index+1] == player.zone[1]) {
						color = zoneColor(std::string(1,x),true);
					}
					else {
						color = zoneColor(std::string(1,x),false);
					}
					std::cout << color;
				}
				std::cout << x;
				index++;
			}
			std::cout << '\n';
		}
		std::cout << "\n                CURRENT ZONE: " << zoneColor(player.zone) << player.zone << Colors::reset << '\n';
		std::cout << atEnd.str() << '\n';
		SLEEP(FRAME_DELAY);
	}
	clear(1);
}

int main() {
	srand(time(0));

	// soloud engine
	SoLoud::Soloud* SOLOUD_ENGINE = new SoLoud::Soloud;
	SOLOUD_ENGINE->init();

	SoLoud::Wav* bgMusic = new SoLoud::Wav;
	bgMusic->load("audio/music/zones_D.wav");
	bgMusic->setLooping(true);

	//int bgMusicHandle = SOLOUD_ENGINE->play(*bgMusic,0);
	//SOLOUD_ENGINE->fadeVolume(bgMusicHandle,1,10);

	#ifdef _WIN32
		// cuz the windows clear i have lowkey sucks balls
		//clear(1);
		ShowConsoleCursor(false);

		// complicated ahh stuff i probably dont need (changing terminal modes)
		// lowkey dont even work but whatever
		HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
		DWORD mode = 0;
		GetConsoleMode(hStdin, &mode);
		mode &= ~ENABLE_ECHO_INPUT;
		mode &= ~ENABLE_PROCESSED_OUTPUT;
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hStdin, mode);

		// sets the console mode to code page 437 (needed for displaying map)
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);
	#else
		system("setterm -cursor off");
		// set terminal to raw mode and turn off echo
		// most of these are the same as doing "raw" just without -opost because that messes shit up
		system("stty -ignbrk -brkint -ignpar -inlcr -icanon -ixoff -igncr -icrnl -parmrk -inpck -istrip -ixon -isig -iuclc -ixany -imaxbel -xcase min 1 time 0 -echo");
	#endif

	// yay
	std::thread(inputHelper).detach();

	fillMap(' ');
	//int16_t x, y, inventory, blit, coins, checkpoint, zone
	Player PLAYER {1,1,0,'@',0,"A0","A0"};
	Cheats CHEATS {&PLAYER};
	//CHEATS.noDie(true);
	
	//CHEATS.giveKey("B");
	//CHEATS.giveKey("C");
	
	loadMap(PLAYER);
	//std::cout << MAP_X << "," << MAP_Y << '\n';

	// input debugger
	while(0) if (KEY != -1) {
		std::cout << KEY << '\n'; 
		if (KEY2 != -1) {
			std::cout << '\t' << KEY2 << '\n';
			KEY2 = -1;
			if (KEY3 != -1) {
				std::cout << "\t\t" << KEY3 << '\n';
				KEY3 = -1;
			}
		}
		KEY = -1;
	}
	
	// guess what this is for
	std::stringstream debugText {};
	
	uint64_t framecounter = 0;

	while (!QUIT) {
		clear(CLEAR_MODE);
		
		// key press
		int wantX = 0;
		int wantY = 0;
		
		if (KEY != -1) {
			switch (KEY) {
			#ifdef _WIN32
				case 27: QUIT = true; break;

				case 0: case 224:
			#else
				case 27:
			#endif
				switch (KEY2) {
				#ifdef _WIN32
					case 72: wantY--; break;
					case 80: wantY++; break;
					case 77: wantX++; break;
					case 75: wantX--; break;
				#else
					switch (KEY3) {
						case 27:
							// escape
							QUIT = true;
							break;
							
						case 91:
							// up down right left
							switch (KEY3) {
								case 65: wantY--; break;
								case 66: wantY++; break;
								case 67: wantX++; break;
								case 68: wantX--; break;
							}
							break;
						case 79:
							switch (KEY3) {
							case 80:
								// F1
								break;
							case 81:
								// F2
								break;
							case 82:
								// F3
								break;
							case 83:
								// F4
								break;
							}
							break;
					}
					#endif
				}
				break;
			case 119:
				// W
				break;
			case 97:
				// A
				break;
			case 115:
				// S
				break;
			case 100:
				// D
				break;
			case 113:
				// Q, show map
				KEY = -1;
				showMap(PLAYER);
				break;
			}
			
			KEY = -1;
			KEY2 = -1;
			KEY3 = -1;
		}
		if (QUIT) break;
		
		// bounds checking
		int testX = PLAYER.x + wantX;
		int testY = PLAYER.y + wantY;
		
		if (testY >= MAP_Y) wantY = 0;
		if (testY < 0     ) wantY = 0;
		if (testX >= MAP_X) wantX = 0;
		if (testX < 0     ) wantX = 0;
		
		if ((wantY || wantX) && (MAP[testY][testX] != ' ')) {
			// still wanna go? lets see what yo dumbass run into
			if (MAP[testY][testX] == 'X') { 
				wantX = 0;
				wantY = 0;
			}
			else if ((MAP[testY][testX] == 'N') || (MAP[testY][testX] == 'E') || (MAP[testY][testX] == 'S') || (MAP[testY][testX] == 'W')) {
				// going into new zone
				if (DEBUG_INFO) std::cout << "GOING INTO NEW ZONE:\n";
				
				std::string wantedZone = "";
				// grab zone
					 if (MAP[testY][testX] == 'N') wantedZone = ADJACENT_ZONES[0];
				else if (MAP[testY][testX] == 'E') wantedZone = ADJACENT_ZONES[1];
				else if (MAP[testY][testX] == 'S') wantedZone = ADJACENT_ZONES[2];
				else if (MAP[testY][testX] == 'W') wantedZone = ADJACENT_ZONES[3];
				if (DEBUG_INFO) std::cout << "  - ZONE GRABBED\n";
				
				if (checkZone(wantedZone,PLAYER)) {
					PLAYER.zone = wantedZone;
					if (DEBUG_INFO) std::cout << "  - SPAWNING PLAYER\n";
					//std::cout << "     - MAP_X = " << MAP_X << '\n';
					//std::cout << "     - MAP_Y = " << MAP_Y << '\n';
					//std::cout << "     - testX = " << testX << '\n';
					//std::cout << "     - testY = " << testY << '\n';
					
					// spawn them in in front of the appropriate door
					// this is gonna be a bitch to do (maybe)
					int newX = 0;
					int newY = 0;
					if (MAP[testY][testX] == 'N') {
						loadMap(PLAYER);
						for (int q = 0; q < MAP_Y; ++q) {
							for (int w = 0; w < MAP_X; ++w) {
								if (MAP[q][w] == 'S') {
									newX = w;
									newY = q-1;
								}
							}
						}
					}
					else if (MAP[testY][testX] == 'E') {
						loadMap(PLAYER);
						for (int q = 0; q < MAP_Y; ++q) {
							for (int w = 0; w < MAP_X; ++w) {
								if (MAP[q][w] == 'W') {
									newX = w+1;
									newY = q;
								}
							}
						}
					}
					else if (MAP[testY][testX] == 'S') {
						loadMap(PLAYER);
						for (int q = 0; q < MAP_Y; ++q) {
							for (int w = 0; w < MAP_X; ++w) {
								if (MAP[q][w] == 'N') {
									newX = w;
									newY = q+1;
								}
							}
						}
					}
					else if (MAP[testY][testX] == 'W') {
						loadMap(PLAYER);
						for (int q = 0; q < MAP_Y; ++q) {
							for (int w = 1; w < MAP_X; ++w) {
								if (MAP[q][w] == 'E') {
									newX = w-1;
									newY = q;
								}
							}
						}
					}
					PLAYER.x = newX;
					PLAYER.y = newY;
					// player has now gone and map loaded
					if (DEBUG_INFO) std::cout << "  - ALL GOOD, CONTINUE LOOP\n";
					continue;
				}
				// back off bud you cant go through that door
				else {
					wantX = 0;
					wantY = 0;
				}
			}
			else if (MAP[testY][testX] == 'k') {
				// key!
				for (int i = 0; i < (int)KEY_LOOKUP.size(); ++i) {
					if (KEY_LOOKUP[i].first == PLAYER.zone) {
						PLAYER.inventory |= KEY_LOOKUP[i].second;
						// set the key value to -1 so it knows that key has been acquired
						KEY_LOOKUP[i].second = -1;
					}
				}
			}
			else if (MAP[testY][testX] == 'c') {
				// ...
			}
			else {
				// running into something i havent accounted for? you cant!
				wantX = 0;
				wantY = 0;
			}
		}
		PLAYER.x += wantX;
		PLAYER.y += wantY;
		
		// put ents and player on map
		// ... aaand update the ents because why not
		// possible optimization: dont check for self when checking possible directions
		bool killedPlayer = false;
		for (int i = 0; i < (int)ENTS.size(); ++i) {
			if (!(framecounter % ENTS[i].moveDelay)) {
				// it is time to move lil bro
				
				// check all directions and flip the proper bit to 1 if you can move
				// last 4 bits: up, down, left, right
				char canMove = 0b0000;
				
				// UP
				if ((ENTS[i].y > 0) && (MAP[ENTS[i].y-1][ENTS[i].x] == ' ')) {
					// no wall, now just check for other ents
					canMove |= 0b1000;
					// hesitant to be doing nested for loop but idgaf
					for (auto x : ENTS) {
						if ((x.x == ENTS[i].x) && (x.y == ENTS[i].y-1)) {
							// well shit cant move there
							canMove &= 0b0111;
							break;
						}
					}
				}
				// DOWN
				if ((ENTS[i].y < MAP_Y-1) && (MAP[ENTS[i].y+1][ENTS[i].x] == ' ')) {
					canMove |= 0b0100;
					for (auto x : ENTS) {
						if ((x.x == ENTS[i].x) && (x.y == ENTS[i].y+1)) {
							canMove &= 0b1011;
							break;
						}
					}
				}
				// LEFT
				if ((ENTS[i].x > 0) && (MAP[ENTS[i].y][ENTS[i].x-1] == ' ')) {
					canMove |= 0b0010;
					for (auto x : ENTS) {
						if ((x.x == ENTS[i].x-1) && (x.y == ENTS[i].y)) {
							canMove &= 0b1101;
							break;
						}
					}
				}
				// RIGHT
				if ((ENTS[i].x < MAP_X-1) && (MAP[ENTS[i].y][ENTS[i].x+1] == ' ')) {
					canMove |= 0b0001;
					for (auto x : ENTS) {
						if ((x.x == ENTS[i].x+1) && (x.y == ENTS[i].y)) {
							canMove &= 0b1110;
							break;
						}
					}
				}
				
				// ok time to pick a direction
				// i swear to god bruh there is 100% a better way to do this im just dumb
			
				char directions[4] = {'.','.','.','.'};
				int moves = 0;
				char decision = 'x';
				
				if (canMove & 0b1000) directions[0] = 'U';
				if (canMove & 0b0100) directions[1] = 'D';
				if (canMove & 0b0010) directions[2] = 'L';
				if (canMove & 0b0001) directions[3] = 'R';
				
				for (int k = 0; k < 4; ++k) if (directions[k] != '.') moves++;
				
				// bros stuck (laughing emoji)
				if (!moves) continue;
				
				/*
				 * AGGRESSION
				 * 
				 * chance to prefer a direction that would go towards the player
				 * 
				 * 0 = 0% chance
				 * 1 = 25% chance
				 * 2 = 50% chance
				 * 3 = 75% chance
				 * 4 = 100% chance (ONE AGGRESSIVE BOI)
				 */
				 
				char aggrDir[4];
				memcpy(aggrDir,directions,sizeof(aggrDir));
				
				if (PLAYER.y >= ENTS[i].y) aggrDir[0] = '.';
				if (PLAYER.y <= ENTS[i].y) aggrDir[1] = '.';
				if (PLAYER.x >= ENTS[i].x) aggrDir[2] = '.';
				if (PLAYER.x <= ENTS[i].x) aggrDir[3] = '.';
				
				// use aggressive movement list?
				int newMoves = 0;
				for (int m = 0; m < 4; ++m) {
					if (aggrDir[m] != '.') {
						newMoves++;
					}
				}
				if ((ENTS[i].aggression) && newMoves && ((rand() % 4)+1 <= ENTS[i].aggression)) {
					
					// intense debugging occured here.
					// because i mixed up X and Y.
					
					debugText << Colors::reset;
					
					memcpy(directions,aggrDir,sizeof(directions));
					moves = newMoves;
				}
				
				if (moves == 1) {
					for (int g = 0; g < 4; ++g) {
						if (directions[g] != '.')  {
							decision = directions[g];
							break;
						}
					}
				}
				else {
					int moveDir = rand() % 4;
					while (directions[moveDir] == '.') {
						moveDir++;
						if (moveDir == 4) moveDir = 0;
					}
					decision = directions[moveDir];
				}
				
				switch (decision) {
				case 'U':
					ENTS[i].y--;
					break;
				case 'D':
					ENTS[i].y++;
					break;
				case 'L':
					ENTS[i].x--;
					break;
				case 'R':
					ENTS[i].x++;
					break;
				}
			}
			
			char blit = 'x';
			switch (ENTS[i].type) {
			case 0:
				blit = '#';
				break;
			case 1:
				blit = '&';
				break;
			case 2:
				blit = '!';
				break;
			}
			MAP[ENTS[i].y][ENTS[i].x] = blit;
			
			if (ENTS[i].y == PLAYER.y && ENTS[i].x == PLAYER.x) {
				// death
				if (!CHEATS_NODIE) {
					PLAYER.zone = PLAYER.checkpoint;
					std::pair<int,int> coords = checkpointLookup(PLAYER.zone);
					PLAYER.x = coords.first;
					PLAYER.y = coords.second;
					killedPlayer = true;
					loadMap(PLAYER);
				}
			}
		}
		if (killedPlayer) continue; // just in case!
		MAP[PLAYER.y][PLAYER.x] = PLAYER.blit;
		
		std::cout << debugText.str();
		
		// blit the map
		LOOP(MAP_X+2) std::cout << 'X';
		std::cout << '\n';
		for (int i = 0; i < MAP_Y; ++i) {
			std::cout << 'X';
			for (int x = 0; x < MAP_X; ++x) {
				char blit = MAP[i][x];
				bool changed = true;
				bool noBlit = false;
				
				switch (blit) {
					case '#':
						std::cout << Colors::red_boldHighIntensity;
						break;
					case '&':
						std::cout << Colors::red_boldHighIntensity;
						break;
					case '!':
						std::cout << Colors::red_boldHighIntensity;
						break;
					case 'c':
						std::cout << Colors::yellow_bold << Colors::yellow_underline;
						break;
					case 'N': case 'E': case 'S': case 'W': {
						// green if can go, red if cant
						std::string wantedZone = "";
						// grab zone
							 if (blit == 'N') wantedZone = ADJACENT_ZONES[0];
						else if (blit == 'E') wantedZone = ADJACENT_ZONES[1];
						else if (blit == 'S') wantedZone = ADJACENT_ZONES[2];
						else if (blit == 'W') wantedZone = ADJACENT_ZONES[3];
						
						std::cout << (checkZone(wantedZone,PLAYER) ? Colors::green_background : Colors::red_background);
						break;
					}
					// KEY!
					case 'k':
						// key!
						for (int i = 0; i < (int)KEY_LOOKUP.size(); ++i) {
							if (KEY_LOOKUP[i].first == PLAYER.zone) {
								if (KEY_LOOKUP[i].second == -1) {
									noBlit = true;
									break;
								}
								// change background color according to which zone it belongs to
								std::cout << zoneColor(KEY_LOOKUP_ZONES[i]);
								break;
							}
						}
						break;
					default:
						changed = false;
						break;
				}
				// PLAYER.blit is not constant so cant have it in switch
				if (blit == PLAYER.blit) {
					std::cout << Colors::green_boldHighIntensity;
					changed = true;
				}
				
				if (!noBlit) std::cout << MAP[i][x];
				if (changed) std::cout << Colors::reset;
			}
			
			// DISPLAY ZONES!
			if ((i == 1) && (ADJACENT_ZONES[0] != "xx")) {
				std::cout << 'X';
				std::cout 
					<< (checkZone(ADJACENT_ZONES[0],PLAYER) ? Colors::green : Colors::red)
					<< "\tNORTH GATE: " << Colors::reset << zoneColor(ADJACENT_ZONES[0]) << ADJACENT_ZONES[0]
					<< Colors::reset << '\n';
			}
			else if ((i == 3) && (ADJACENT_ZONES[1] != "xx")) {
				std::cout << 'X';
				std::cout 
					<< (checkZone(ADJACENT_ZONES[1],PLAYER) ? Colors::green : Colors::red)
					<< "\tEAST GATE: " << Colors::reset << zoneColor(ADJACENT_ZONES[1]) << ADJACENT_ZONES[1]
					<< Colors::reset << '\n';
			}
			else if ((i == 5) && (ADJACENT_ZONES[2] != "xx")) {
				std::cout << 'X';
				std::cout 
					<< (checkZone(ADJACENT_ZONES[2],PLAYER) ? Colors::green : Colors::red)
					<< "\tSOUTH GATE: " << Colors::reset << zoneColor(ADJACENT_ZONES[2]) << ADJACENT_ZONES[2]
					<< Colors::reset << '\n';
			}
			else if ((i == 7) && (ADJACENT_ZONES[3] != "xx")) {
				std::cout << 'X';
				std::cout 
					<< (checkZone(ADJACENT_ZONES[3],PLAYER) ? Colors::green : Colors::red)
					<< "\tWEST GATE: " << Colors::reset << zoneColor(ADJACENT_ZONES[3]) << ADJACENT_ZONES[3]
					<< Colors::reset << '\n';
			}
			else std::cout << "X\n";
		}
		LOOP(MAP_X+2) std::cout << 'X';
		std::cout << '\n';
				
		for (auto i : ENTS) {
			MAP[i.y][i.x] = ' ';
		}
		
		// take away character char after done displaying
		MAP[PLAYER.y][PLAYER.x] = ' ';
		
		// show map zone
		std::string spaces = "";
		for (int i = 0; i < MAP_X/2-1; ++i) spaces += " ";
		std::cout 
			<< spaces << zoneColor(PLAYER.zone) << ' ' << PLAYER.zone << ' ' << Colors::reset
			<< "   "
			<< Colors::reset
			<< "\n\n";
			
		// show keys
		std::cout 
			<< "KEYS: "
			<< ((PLAYER.inventory & 0b1000000000000000) ? (Colors::green_background) : (Colors::red_background)) << 'B' << Colors::reset << ' '  
			<< ((PLAYER.inventory & 0b0100000000000000) ? (Colors::green_background) : (Colors::red_background)) << 'C' << Colors::reset << ' '
			<< ((PLAYER.inventory & 0b0010000000000000) ? (Colors::green_background) : (Colors::red_background)) << 'D' << Colors::reset << ' '
			<< ((PLAYER.inventory & 0b0001000000000000) ? (Colors::green_background) : (Colors::red_background)) << 'E' << Colors::reset << ' '
			<< ((PLAYER.inventory & 0b0000100000000000) ? (Colors::green_background) : (Colors::red_background)) << 'F' << Colors::reset << ' '
			<< ((PLAYER.inventory & 0b0000010000000000) ? (Colors::green_background) : (Colors::red_background)) << 'G' << Colors::reset << ' '
			<< ((PLAYER.inventory & 0b0000000011110000) ? (Colors::green_background) : (Colors::red_background)) << '?'
			<< Colors::reset << '\n';
			
		if (DEBUG_INFO || 1) std::cout
			<< PLAYER.x
			<< ","
			<< PLAYER.y
			<< "   "
			<< "FRAME: "
			<< Colors::blue_underline
			<< framecounter
			<< Colors::reset
			<< '\n';
		
		SLEEP(FRAME_DELAY);
		framecounter++;
	}

	// clean up time!

	// kill sound engine
	SOLOUD_ENGINE->deinit();
	delete SOLOUD_ENGINE;
	// ... and sources
	delete bgMusic;

	#ifdef _WIN32
		clear(1);
		ShowConsoleCursor(true);
	#else
		// set terminal back
		system("setterm -cursor on");
		system("stty cooked echo");
	#endif
	return 0;
}
// rin!
