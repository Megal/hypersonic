#include <iostream>
#include <cassert>
#include <vector>
#include <map>
#include <sstream>

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


	enum ItemType: int {
		ItemType_extraRange	= 1,
		ItemType_extraBomb	= 2,
	};
	union {
		int param1;
		int bombsRemains;
		int detonationCountdown;
		int itemType;
	};
	union {
		int param2;
		int explosionRange;
	};
};

struct EntityEx {
	enum EntityExType: uint {
		EntityExType_empty = 0,
		EntityExType_player,
		EntityExType_box,
		EntityExType_bomb,
		EntityExType_wall,
		EntityExType_item,
	};
	uint type;

	enum Traits: uint {
		Traits_impassable		= 1 << 0, // Player cannot walkthrough, unless already stays there
		Traits_destuctable		= 1 << 1, // Will disappear from field if explosion will hit it
		Traits_absorbsExplosion	= 1 << 2, // Stops further explosion spreading
		Traits_chainDetonation	= 1 << 3, // Will detonate immediately if explosion hit it
		Traits_containedInBox	= 1 << 4, // Explosion will not affect it if there is other entity, such a box, the in same position
	};
	uint flags;

	union {
		int param0;
		int owner;
	};

	enum ItemType: int {
		ItemType_extraRange	= 1,
		ItemType_extraBomb	= 2,
	};
	union {
		int param1;
		int itemType;
	};

	union {
		int param2;
	};
};


struct World {
	using container_t = vector<EntityEx>;
	container_t grid[HeightOfTheWorld][WidthOfTheWorld];

	void updateWithGrid(const char *bytegrid, size_t height, size_t width)
	{
		assert( height >= HeightOfTheWorld );
		assert( width >= WidthOfTheWorld );

		for( int i = 0; i < HeightOfTheWorld; ++i ) {
			for( int j = 0; j < WidthOfTheWorld; ++j ) {
				auto c = bytegrid[i*width + j];
				assert( Grid::isValid(c) );

				if( !Grid::isEmpty(c) ) {
					updateWithChar( {j, i}, c );
				}
			}
		}
	}

	void updateWithEntities(const vector<Entity>& entities) {
		for( const auto& entity: entities ) {
			updateWithEntity(entity);
		}
	}


	int count()
	{
		int count = 0;

		for( int i = 0; i < HeightOfTheWorld; ++i ) {
			for( int j = 0; j < WidthOfTheWorld; ++j ) {
				const auto& list = grid[i][j];
				count += list.size();
			}
		}

		return  count;
	}
private:
	void update(const int2d& pos, const EntityEx& entity)
	{
		auto list = grid[pos.y][pos.x];

		list.push_back(entity);
		assert(list.size() < 10);
	}


	void updateWithEntity(const Entity& entity)
	{
		switch( entity.entityType ) {
			case Entity::EntityType_player:
				updateWithPlayer(entity);
				break;

			case Entity::EntityType_bomb:
				updateWithBomb(entity);
				break;

			case Entity::EntityType_item:
				updateWithItem(entity.pos, entity.itemType);
				break;

			default:
				assert( false && "unexpected entityType" );
				break;
		}
	}


	void updateWithChar(const int2d& pos, const char c)
	{
		assert( Grid::isValid(c) );
		assert( !Grid::isEmpty(c) );

		if( Grid::isBox(c) ) {
			updateWithBox(pos);

			int itemValue = c - '0';
			if( itemValue ) {
				updateWithItem(pos, itemValue);
			}
		}
		else if( Grid::isWall(c) ) {
			updateWithWall(pos);
		}
		else {
			assert( false && "cannot interpret character" );
		}
	}


	void updateWithPlayer(const Entity &entityPlayer)
	{
		EntityEx player;

		prepareUpgradeFromEntity(player, entityPlayer);
		player.type = EntityEx::EntityExType_player;
		player.flags = 0;
		player.flags |= EntityEx::Traits_destuctable;

		update(entityPlayer.pos, player);
	}


	void updateWithBomb(const Entity &entityBomb)
	{
		EntityEx bomb;

		prepareUpgradeFromEntity(bomb, entityBomb);
		bomb.type = EntityEx::EntityExType_bomb;
		bomb.flags = 0;
		bomb.flags |= EntityEx::Traits_impassable;
		bomb.flags |= EntityEx::Traits_destuctable;
		bomb.flags |= EntityEx::Traits_chainDetonation;

		update(entityBomb.pos, bomb);
	}


	void prepareUpgradeFromEntity(EntityEx &target, const Entity &source)
	{
		target.param0 = source.owner;
		target.param1 = source.param1;
		target.param2 = source.param2;
	}


	void updateWithBox(const int2d& pos)
	{
		EntityEx box;

		box.type = EntityEx::EntityExType_box;
		box.flags = 0;
		box.flags |= EntityEx::Traits_impassable;
		box.flags |= EntityEx::Traits_absorbsExplosion;
		box.flags |= EntityEx::Traits_destuctable;

		update(pos, box);
	}


	void updateWithItem(const int2d& pos, int itemValue)
	{
		EntityEx item;

		item.type = EntityEx::EntityExType_item;
		item.flags = 0;
		item.flags |= EntityEx::Traits_destuctable;
		item.flags |= EntityEx::Traits_absorbsExplosion;
		item.flags |= EntityEx::Traits_containedInBox;
		item.itemType = itemValue;

		assert( item.itemType == EntityEx::ItemType_extraRange || item.itemType == EntityEx::ItemType_extraBomb );

		update(pos, item);
	}


	void updateWithWall(int2d pos)
	{
		EntityEx wall;

		wall.type = EntityEx::EntityExType_wall;
		wall.flags = 0;
		wall.flags |= EntityEx::Traits_impassable;
		wall.flags |= EntityEx::Traits_absorbsExplosion;

		update(pos, wall);
	}
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


vector<Entity> loadEntitiesAndLog(std::stringstream& log)
{
	int entityCount;
	cin >> entityCount;

	vector<Entity> loaded;
	for( int i = 0; i < entityCount; ++i ) {
		Entity entity;
		string line;
		while( line.length() == 0 ) {
			getline(cin, line);
		}
		log << line << " ";
		stringstream ss(line);

		ss >> entity.entityType >> entity.owner >> entity.pos >> entity.param1 >> entity.param2;
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

			int2d box = {x, y};
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
			int2d pos = {x, y};

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
		stringstream logstream;
		auto entities = loadEntitiesAndLog(logstream);
		string logstring; getline(logstream, logstring);
		auto player = findPlayer(entities);

		World theWorld;
		theWorld.updateWithGrid(&grid[0][0], HeightOfTheWorld, WidthOfTheWorld+1);
		theWorld.updateWithEntities(entities);
		auto count = theWorld.count();

		int2d gotoBox;
		if( countItemsOnGrid(entities) > 0 ) {
			gotoBox = nearestItem(player, entities);
		}
		else {
			gotoBox = nearestBox(player, entities);
		}


		if( canBombBoxes(player, entities) ) {
			printf("BOMB %d %d count=%d log=%s\n", gotoBox.x, gotoBox.y, count, logstring.c_str());
		}
		else {
			printf("MOVE %d %d count=%d log=%s\n", gotoBox.x, gotoBox.y, count, logstring.c_str());
		}
	}

	return 0;
}