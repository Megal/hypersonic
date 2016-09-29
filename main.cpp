#include <iostream>
#include <cassert>
#include <vector>
#include <map>

using namespace std;

#pragma mark - Global scope variables

int W, H;
int myId;
const size_t HeightOfTheWorld	= 11;
const size_t WidthOfTheWorld	= 13;
char grid[HeightOfTheWorld][WidthOfTheWorld+1];
namespace Grid {
	bool isBox(const char c) { return c >= '0' && c <= '9'; }
	bool isEmpty(const char c) { return c == '.'; }
	bool isWall(const char c) { return c == 'X'; }
	bool isValid(const char c)
	{
		return isBox(c) || isEmpty(c) || isWall(c);
	}
};

#pragma mark - Read from file

bool readStdinFromFile(const char *filename)
{
	FILE *opened = freopen(filename, "r", stdin);

	return opened != NULL;
}

#pragma mark - Math helpers

using uint = unsigned int;

struct int2d{
	int x, y;
	int2d(const int& x, const int &y): x(x), y(y) {}
	int2d(): x(0), y(0) {}
	bool isOnGrid() {
		if( x < 0 ) {
			return false;
		}
		else if( x >= W ) {
			return false;
		}
		else if( y < 0 ) {
			return false;
		}
		else if( y >= H ) {
			return false;
		}
		else {
			return true;
		}
	};
};


std::istream& operator>>(std::istream& is, int2d& object) {
	is >> object.x >> object.y;

	return is;
}

#pragma mark - Read game input

void loadInit()
{
	cin >> W >> H >> myId;
	assert(W == WidthOfTheWorld);
	assert(H == HeightOfTheWorld);
}

void loadGrid()
{
	for( int y = 0; y < H; ++y ) {
		scanf("%s", &grid[y][0]);
	}

	for( int i = 0; i < H; ++i ) {
		for( int j = 0; j < W; ++j ) {
			char item = grid[i][j];
			assert( Grid::isValid(item) );
		}
	}
}

struct Entity {
	enum EntityType: int {
		EntityType_player = 0,
		EntityType_bomb = 1,
		EntityType_item = 2,
	};
	int entityType;
	int owner;
	int2d pos;
	union {
		int param1;
		int bombsRemains;
		int detonationCountdown;
	};
	union {
		int param2;
		int explosionRange;
	};
};

struct EntityEx {
	enum EntityExType: uint {
		EntityExType_unknown = 0, // should not be used
		EntityExType_player,
		EntityExType_bomb,
		EntityExType_wall,
		EntityExType_item,
	};
	uint type;

	enum Traits: uint {
		Traits_impassable		= 1 << 0,
		Traits_destuctable		= 1 << 1,
		Traits_absorbsExplosion	= 1 << 2,
		Traits_chainDetonation	= 1 << 3,
		Traits_dropsItem		= 1 << 4,
	};
	uint flags;

	union {
		int param0;
		int owner;
	};

	union {
		int param1;
	};

	union {
		int param2;
	};

	using params_key_t = std::string;
	using params_value_t = vector<int>;
	using params_container_t = std::map<params_key_t, params_value_t>;
	params_container_t otherParams;
};

struct World {
	EntityEx grid[HeightOfTheWorld][WidthOfTheWorld];

};

bool boxWillBeDestroyed(int2d box, const vector<Entity>& entities);


vector<Entity> loadEntities()
{
	int entityCount;
	cin >> entityCount;

	vector<Entity> loaded;
	for( int i = 0; i < entityCount; ++i ) {
		Entity entity;
		cin >> entity.entityType >> entity.owner >> entity.pos >> entity.param1 >> entity.param2;
		assert( entity.entityType == Entity::EntityType_player || entity.entityType == Entity::EntityType_bomb || entity.entityType == Entity::EntityType_item );

		loaded.push_back(entity);
	}

	return loaded;
}

Entity findPlayer(const vector<Entity>& entities)
{
	for( const auto &entity: entities ) {
		if( entity.entityType == Entity::EntityType_player )
		{
			if( entity.owner == myId ) {
				return entity;
			}
		}
	}

	assert(false && "should find player");
}


int2d nearestBox(const Entity& player, const vector<Entity>& entities)
{
	// TODO: also prefer boxes with items
	int2d bestBox;
	int bestLength = 999;

	for( int y = 0; y < H; ++y ) {
		for( int x = 0; x < W; ++x ) {
			auto c = grid[y][x];
			if( !Grid::isBox(c) ) {
				continue;
			}

			auto box = int2d(x, y);
			if( boxWillBeDestroyed(box, entities) ) {
				continue;
			}

			int length = abs(player.pos.x - box.x) + abs(player.pos.y - box.y);
			if( length < bestLength ) {
				bestBox = box;
				bestLength = length;
			}
		}
	}

	return bestBox;
}


bool canBombBoxes(const Entity& player, const vector<Entity>& entities)
{
	static int dx[4] = {-1, 0, 1, 0};
	static int dy[4] = {0, 1, 0, -1};

	for( int k = 0; k < 4; ++k ) {
		for( int r = 1; r < player.explosionRange; ++r ) {
			auto x = player.pos.x + r*dx[k];
			auto y = player.pos.y + r*dy[k];
			auto pos = int2d(x, y);

			if( !pos.isOnGrid() ) {
				break;
			}

			auto c = grid[y][x];
			if( Grid::isEmpty(c) ) {
				continue;
			}
			else if( !Grid::isBox(c) ) {
				break;
			}

			if( !boxWillBeDestroyed(pos, entities) ){
				return true;
			}
		}
	}

	return false;
}


bool boxWillBeDestroyed(int2d box, const vector<Entity>& entities)
{
	assert( box.isOnGrid() );
	auto c = grid[box.y][box.x];
	assert( Grid::isBox(c) );

	for( const auto& bomb : entities ) {
		if( bomb.entityType != Entity::EntityType_bomb ) {
			continue;
		}

		// TODO check if there any obstacles in between box and bomb
		if( bomb.pos.x == box.x ) {
			if( abs(bomb.pos.y - box.y) < bomb.explosionRange ) {
				return true;
			}
		}
		else if( bomb.pos.y == box.y ) {
			if( abs(bomb.pos.x - box.x) < bomb.explosionRange ) {
				return true;
			}
		}
	}

	return false;
}


int countItemsOnGrid(const vector<Entity>& entities)
{
	int count = 0;

	for( const auto& entity : entities ) {
		if( entity.entityType == Entity::EntityType_item ) {
			++count;
		}
	}

	return count;
}


int2d nearestItem(const Entity& player, const vector<Entity>& entities)
{
	assert( countItemsOnGrid(entities) > 0 );

	int2d bestItem;
	int bestLength = 999;

	for( const auto& item : entities ) {
		if( item.entityType != Entity::EntityType_item ) {
			continue;
		}
		int length = abs(player.pos.x - item.pos.x) + abs(player.pos.y - item.pos.y);
		if( length < bestLength ) {
			bestItem = item.pos;
			bestLength = length;
		}
	}

	return bestItem;
}


int main()
{
	readStdinFromFile("input.txt");
	loadInit();

	for( int turn = 0; turn < 200; ++turn ) {
		loadGrid();
		auto entities = loadEntities();
		auto player = findPlayer(entities);
		int2d gotoBox;
		if( countItemsOnGrid(entities) > 0 ) {
			gotoBox = nearestItem(player, entities);
		}
		else {
			gotoBox = nearestBox(player, entities);
		}


		if( canBombBoxes(player, entities) ) {
			printf("BOMB %d %d\n", gotoBox.x, gotoBox.y);
		}
		else {
			printf("MOVE %d %d\n", gotoBox.x, gotoBox.y);
		}
	}

	return 0;
}