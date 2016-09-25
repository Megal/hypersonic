#include <iostream>
#include <cassert>
#include <vector>

using namespace std;

#pragma mark - Global scope variables

int W, H;
int myId;
char grid[11][14];
namespace GridItem {
	enum : char {
		empty = '.',
		box = '0',
	};
};

#pragma mark - Read from file

bool readStdinFromFile(const char *filename)
{
	FILE *opened = freopen(filename, "r", stdin);

	return opened != NULL;
}

#pragma mark - Math helpers

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
	assert(W == 13);
	assert(H == 11);
}

void loadGrid()
{
	for( int y = 0; y < H; ++y ) {
		scanf("%s", &grid[y][0]);
	}

	for( int i = 0; i < H; ++i ) {
		for( int j = 0; j < W; ++j ) {
			char item = grid[i][j];
			assert( item == GridItem::box || item == GridItem::empty );
		}
	}
}

struct Entity {
	enum EntityType: int {
		EntityType_player = 0,
		EntityType_bomb = 1,
	};
	int entityType;
	int owner;
	int2d pos;
	int param1, param2;
};

vector<Entity> loadEntities()
{
	int entityCount;
	cin >> entityCount;

	vector<Entity> loaded;
	for( int i = 0; i < entityCount; ++i ) {
		Entity entity;
		cin >> entity.entityType >> entity.owner >> entity.pos >> entity.param1 >> entity.param2;
		assert( entity.entityType == Entity::EntityType_player || entity.entityType == Entity::EntityType_bomb );

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

int2d nearestBox(const Entity& player)
{
	int2d bestBox;
	int bestLength = 999;

	for( int y = 0; y < H; ++y ) {
		for( int x = 0; x < W; ++x ) {
			if( grid[y][x] != GridItem::box ) {
				continue;
			}

			int length = abs(player.pos.x - x) + abs(player.pos.y - y);
			if( length < bestLength ) {
				bestBox = int2d(x, y);
				bestLength = length;
			}
		}
	}

	return bestBox;
}

bool canBombBoxes(const Entity& player)
{
	static int dx[4] = {-1, 0, 1, 0};
	static int dy[4] = {0, 1, 0, -1};

	for( int r = 1; r <= 2; ++r ) {
		for( int k = 0; k < 4; ++k ) {
			auto x = player.pos.x + r*dx[k];
			auto y = player.pos.y + r*dy[k];
			auto pos = int2d(x, y);

			if( !pos.isOnGrid() ) {
				continue;
			}
			if( grid[y][x] == GridItem::box )
			{
				return true;
			}
		}
	}

	return false;
};

int main()
{
	readStdinFromFile("input.txt");
	loadInit();

	for( int turn = 0; turn < 200; ++turn ) {
		loadGrid();
		auto entities = loadEntities();
		auto player = findPlayer(entities);
		auto gotoBox = nearestBox(player);

		if( canBombBoxes(player) ) {
			printf("BOMB %d %d\n", gotoBox.x, gotoBox.y);
		}
		else {
			printf("MOVE %d %d\n", gotoBox.x, gotoBox.y);
		}
	}

	return 0;
}