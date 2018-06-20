//
//  World.cpp
//  GameProject
//
//  Created by Orin Elmquist on 6/12/18.
//  Copyright © 2018 Orin Elmquist. All rights reserved.
//

#include "World.hpp"

const int DEFAULT_MAP_SIZE = 100;

enum {
    NORTH = 1,
    EAST  = 2,
    SOUTH = 3,
    WEST  = 4
};

enum {
    FLOOR = 0,
    WALL  = 1,
    DOOR  = 2
};


////////////////
//Disjoint Set//
////////////////

class DJS {
public:
    class Node {
    public:
        Node *parent;
        int val;
        int rank;
        
        Node(int _val) : val(_val), rank(0), parent(this) { };
    };
    
    DJS(int size);
    Node* find(Node* i);
    void merge(int i, int j);
    bool connected();
    
    std::vector<DJS::Node*> v;
};

DJS::DJS(int size) {
    for (int i = 0; i < size; i++)
        v.push_back(new Node(i));
}

DJS::Node* DJS::find(Node* n) {
    int i = n->val;
    if (v[i] != v[i]->parent)
        v[i]->parent = find(v[i]->parent);
    
    return v[i]->parent;
}

void DJS::merge(int i, int j) {
    Node* a = find(v[i]);
    Node* b = find(v[j]);
    
    if (a->rank > b->rank) {
        b->parent = a;
    } else {
        a->parent = b;
        if (a->rank == b->rank)
            b->rank++;
    }
}

bool DJS::connected() {
    int c = v[0]->parent->val;
    
    for (size_t i = 1; i < v.size(); i++)
        if (find(v[i])->val != c)
            return false;
    
    return true;
}


/////////////////
// World Class //
/////////////////

/*
 Cave Build Functions
 
 buildCave:
     percentWall - (Default 45) Bigger number for less floor space
 */
void World::buildCave() {
    int percentWall = 46;
    
    //Randomly Generate Walls and Floors
    
    for (int y = 1; y < size - 1; y++)
        for (int x = 1; x < size - 1; x++)
            if (rand() % 100 > percentWall)
                map[y * size + x] = FLOOR;
    
//    std::cout << "Random Map:" << std::endl;
//    std::cout << (*this) << std::endl << std::endl << std::endl;
    
    //Clean it up
    
    std::vector<int> temp;
    int n;
    temp = map;
    
    for (int i = 0; i < 5; i++) {
        for (int y = 1; y < size - 1; y++)
            for (int x = 1; x < size - 1; x++) {
                n = 0;
                for (int yy = -1; yy <= 1; yy++)
                    for (int xx = -1; xx <= 1; xx++)
                        if (map[(y + yy) * size + x + xx] == WALL)
                            n++;
                if (n >= 5)
                    temp[y * size + x] = WALL;
                else
                    temp[y * size + x] = FLOOR;
        }
        map = temp;
        
//        std::cout << "Iteration #" << i + 1 << std::endl;
//        std::cout << (*this) << std::endl << std::endl;
    }
    
    //Remove inaccessable caverns
    
    std::vector<bool> connected, visited;
    int c = 0, rx, ry;
    
    for (int i = 0; i < size * size; i++) {
        connected.push_back(false);
        visited.push_back(false);
    }
    
    while ((100 * c) / (size * size) <= 25) {
        c = 0;
    
        do {
            rx = rand() % (size - 1) + 1;
            ry = rand() % (size - 1) + 1;
        } while (map[ry * size + rx] != FLOOR);
    
        flood(visited, connected, rx, ry, c);
//        std::cout << "Random cavern covers " << (100 * c) / (size * size) << " percent of the cave." << std::endl << std::endl;
    }
    
    for (int i = 0; i < size * size; i++)
        if (!connected[i])
            map[i] = WALL;

//    std::cout << "Flood:" << std::endl;
//    std::cout << (*this) << std::endl << std::endl;
}


/*
 Dungeon build functions
 
 buildDungeon:
     numOfRooms - Adjusts the maximum number of rooms the generator will try to fit in.
     maxAttempts - Maximum number of tries to place a room before the generator gives up.
     roomDistanceThreshold - Minimum area between rooms
 */

void World::buildDungeon() {
    int numOfRooms = 10;
    int maxAttempts = 2000, attempts = 0;
    int roomDistanceThreshold = 3;
    rooms.clear();
    clear();
    
    //Add rooms until either a time-out or number of desired rooms is reached.
    for (int i = 0; i < numOfRooms; i++) {
        Room temp = Room();
        attempts = 0;
        while (!temp.valid(rooms, roomDistanceThreshold) || attempts++ >= maxAttempts)
            temp = Room();
        if (attempts >= maxAttempts)
            break;
        temp.set();
        rooms.push_back(temp);
    }
    
    for (Room r : rooms)
        placeRoom(r);
    std::cout << "Before centralization: " << std::endl;
    std::cout << (*this) << std::endl;

    //Sort rooms by distance to center and compact the rooms
    int moves;
    
    do {
        moves = 0;
        std::sort(rooms.begin(), rooms.end(), Room::compareXY);
        for (size_t i = 0; i < rooms.size(); i++)
            while (rooms[i].moveXY(rooms, roomDistanceThreshold))
                moves++;
    } while (moves > 0);
    
    clear();
    for (Room r : rooms)
        placeRoom(r);
    
    //Minimally connect the rooms
    std::vector<Hall> possHalls;
    DJS connSet(rooms.size());
    Hall curr;
    bool strandedRoom = false;
    
    do {
        //Update possible hallways, if none, break and note the stranded room
        possHalls = updateHalls();
        
        if (possHalls.size() == 0) {
            strandedRoom = true;
            break;
        }
        
        //Randomly select a room, if it is a new connection add to the list and update the DJS
        curr = possHalls[rand() % possHalls.size()];
        
        if (connSet.find(connSet.v[curr.rooms().first])->val != connSet.find(connSet.v[curr.rooms().second])->val) {
            halls.push_back(curr);
            connSet.merge(curr.rooms().first, curr.rooms().second);
            placeHall(curr);
        }
    } while (!connSet.connected());
    
    //Deal with the standed room
}

std::vector<Hall> World::setPossHalls() {
    int x, y, n, endRoom;
    std::vector<std::vector<int>> temp;
    std::set<int> edges;
    std::vector<Hall> possibles;
    
    for (Room r : rooms)
        for (std::vector<int> v : r.edges())
            for (int i : v)
                edges.insert(i);
    
    for (Room r : rooms) {
        temp = r.edges();
        
        for (int i : temp[0]) {
            n = 0;
            x = i % size;
            y = i / size;
            while (y - n - 2 > 0) {
                n++;
                if (map[(y - n - 1) * size + x] == FLOOR) {
                    endRoom = getRoomByEdge((y - n) * size + x);
                    if (edges.find((y - n) * size + x) != edges.end() && endRoom >= -1 && map[(y - n - 2) * size + x] == FLOOR)
                        possibles.push_back(Hall(r.num(), y * size + x, endRoom, (y - n) * size + x, NORTH));
                    break;
                }
            }
        }
        
        for (int i : temp[1]) {
            n = 0;
            x = i % size;
            y = i / size;
            while (x + n + 2 < size) {
                n++;
                if (map[y * size + x + n + 1] == FLOOR) {
                    endRoom = getRoomByEdge(y * size + x + n);
                    if (edges.find(y * size + x + n) != edges.end() && endRoom >= -1 && map[y * size + x + n + 2] == FLOOR)
                        possibles.push_back(Hall(r.num(), y * size + x, endRoom, y * size + x + n, EAST));
                    break;
                }
            }
        }
        
        for (int i : temp[2]) {
            n = 0;
            x = i % size;
            y = i / size;
            while (y + n + 2 < size) {
                n++;
                if (map[(y + n + 1) * size + x] == FLOOR) {
                    endRoom = getRoomByEdge((y + n) * size + x);
                    if (edges.find((y + n) * size + x) != edges.end() && endRoom >= -1 && map[(y + n + 2) * size + x] == FLOOR)
                        possibles.push_back(Hall(r.num(), y * size + x, endRoom, (y + n) * size + x, SOUTH));
                    break;
                }
            }
        }
        
        for (int i : temp[3]) {
            n = 0;
            x = i % size;
            y = i / size;
            while (x - n - 2 > 0) {
                n++;
                if (map[y * size + x - n - 1] == FLOOR) {
                    endRoom = getRoomByEdge(y * size + x - n);
                    if (edges.find(y * size + x - n) != edges.end() && endRoom >= 0 && map[y * size + x - n - 2] == FLOOR)
                        possibles.push_back(Hall(r.num(), y * size + x, endRoom, y * size + x - n, WEST));
                    break;
                }
            }
        }
    }
    
    return possibles;
}

std::vector<Hall> World::updateHalls() {
    std::vector<Hall> temp;
    std::vector<Hall> possibles = setPossHalls();
    bool matches;
    
    for (Hall p : possibles) {
        matches = false;
        
        for (Hall h : halls)
            if (h.sameConnection(p))
                matches = true;
        
        if (!matches)
            temp.push_back(p);
    }
    
    return temp;
}

int World::getRoomByEdge(int coord) {
    
    for (Room r : rooms)
        for (std::vector<int> v : r.edges())
            for (int i : v)
                if (coord == i)
                    return r.num();
    
    return -1;
}

void World::placeRoom(Room r) {
    for (int y = r.coords().second + 1; y < r.coords().second + r.dim().second; y++)
        for (int x = r.coords().first + 1; x < r.coords().first + r.dim().first; x++)
            map[y * size + x] = FLOOR;
}

void World::placeHall(Hall h) {
    int x1 = h.coords().first % size;
    int x2 = h.coords().second % size;
    int y1 = h.coords().first / size;
    int y2 = h.coords().second / size;
    switch (h.dir()) {
        case NORTH:
            for (int y = y1; y <= y2; y--) {
                map[y * size + x1 - 1] = WALL;
                map[y * size + x1] = FLOOR;
                map[y * size + x1 + 1] = WALL;
            }
            break;
            
        case EAST:
            for (int x = x1; x <= x2; x++) {
                map[(y1 - 1) * size + x] = WALL;
                map[y1 * size + x] = FLOOR;
                map[(y1 + 1) * size + x] = WALL;
            }
            break;
            
        case SOUTH:
            for (int y = y1; y <= y2; y++) {
                map[y * size + x1 - 1] = WALL;
                map[y * size + x1] = FLOOR;
                map[y * size + x1 + 1] = WALL;
            }
            break;
            
        case WEST:
            for (int x = x1; x <= x2; x++) {
                map[(y1 - 1) * size + x] = WALL;
                map[y1 * size + x] = FLOOR;
                map[(y1 + 1) * size + x] = WALL;
            }
            break;
    }
}


/*
 Constructor, misc helper functions, and overrides
 */

World::World() {
    size = DEFAULT_MAP_SIZE;
    clear();
}

void World::clear() {
    map.clear();
    for (int i = 0; i < size * size; i++)
        map.push_back(WALL);
}

void World::set(int x, int y, int val) {
    map[y * size + x] = val;
}

void World::swap(int x1, int y1, int x2, int y2) {
    int temp = map[y2 * size + x2];
    map[y2 * size + x2] = map[y1 * size + x1];
    map[y1 * size + x1] = temp;
}

std::ostream &operator<<(std::ostream &out, const World &w) {
    for (int y = 0; y < w.size; y++) {
        for (int x = 0; x < w.size; x++) {
            switch (w.map[y * w.size + x]) {
                case 0:
                    out << ' ';
                    break;
                case 1:
                    out << '#';
                    break;
                case 2:
                    out << 'D';
                    break;
            }
        }
        out << '\n';
    }
    return out;
}

// TODO: fix the recursive calls as they can/will cause a stack overflow
void World::flood(std::vector<bool> &visited, std::vector<bool> &connected, int x, int y, int &c) {
    visited[y * size + x] = true;
    if (map[y * size + x] == FLOOR) {
        connected[y * size + x] = true;
        c++;
        if (!visited[(y - 1) * size + x] && !connected[(y - 1) * size + x])
            flood(visited, connected, x, y - 1, c);
        if (!visited[y * size + x + 1] && !connected[y * size + x + 1])
            flood(visited, connected, x + 1, y, c);
        if (!visited[(y + 1) * size + x] && !connected[(y + 1) * size + x])
            flood(visited, connected, x, y + 1, c);
        if (!visited[y * size + x - 1] && !connected[y * size + x - 1])
            flood(visited, connected, x - 1, y, c);
    }
}


////////////////
// Room Class //
////////////////

/*
 Constructor - builds a random room
 */

int Room::count = 0;
int Room::size = DEFAULT_MAP_SIZE;

Room::Room() {
    int maxV = (2 * (size / 10)) / 4;
    int maxS = (2 * (size / 10)) - maxV;
    w = rand() % maxV + maxS;
    h = rand() % maxV + maxS;
    x = rand() % (size - w);
    y = rand() % (size - h);
    id = -1;
}

void Room::set() {
    if (id < 0)
        id = count++;
}


/*
 Validation, used to check for overlaps and valid moves. Can add an offset.
 */

bool Room::valid(Room other, int offset) {
    return (other.x > x + w + offset || other.x + other.w < x - offset) || (other.y > y + h + offset || other.y + other.h < y - offset);
}

bool Room::valid(std::vector<Room> &others, int offset) {
    for (Room r : others)
        if (!equals(r) && !valid(r, offset))
            return false;
    return true;
}

bool Room::validX(Room other, int offset) {
    if (other.y > y + h || other.y + other.h < y)
        return true;
    return (other.x > x + w + offset || other.x + other.w < x - offset);
}

bool Room::validX(std::vector<Room> &others, int offset) {
    for (Room r : others)
        if (!equals(r) && !validX(r, offset))
            return false;
    return true;
}

bool Room::validY(Room other, int offset) {
    if (other.x > x + w || other.x + other.w < x)
        return true;
    return (other.y > y + h + offset || other.y + other.h < y - offset);
}

bool Room::validY(std::vector<Room> &others, int offset) {
    for (Room r : others)
        if (!equals(r) && !validY(r, offset))
            return false;
    return true;
}

bool Room::equals(Room other) {
    return (x == other.x) && (y == other.y) && (w == other.w) && (h == other.h);
}


/*
 Returns edges in NESW order vector to find room connections
 */

std::vector<std::vector<int>> Room::edges() {
    std::vector<std::vector<int>> edges;
    std::vector<int> no, ea, so, we;
    
    for (int xx = x + 2; xx < x + w - 1; xx++)
        no.push_back(y * size + xx);
    
    for (int yy = y + 2; yy < y + h - 1; yy++)
        ea.push_back(yy * size + x + w);
    
    for (int xx = x + 2; xx < x + w - 1; xx++)
        so.push_back((y + h) * size + xx);
    
    for (int yy = y + 2; yy < y + h - 1; yy++)
        we.push_back(yy * size + x);
    
    edges.push_back(no);
    edges.push_back(ea);
    edges.push_back(so);
    edges.push_back(we);
    
    return edges;
}


/*
 Move functions
 */

bool Room::moveXY(std::vector<Room> &others, int offset) {
    bool mx = moveX(others, offset), my = moveY(others, offset);
    return mx || my;
}

bool Room::moveX(std::vector<Room> &others, int offset) {
    if (validX(others, offset + 1)) {
        if ((2 * x + w) / 2 > size / 2)
            x--;
        else if ((2 * x + w) / 2 < size / 2)
            x++;
        else
            return false;
        return true;
    }
    return false;
}

bool Room::moveY(std::vector<Room> &others, int offset) {
    if (validY(others, offset + 1)) {
        if ((2 * y + h) / 2 > size / 2)
            y--;
        else if ((2 * y + h) / 2 < size / 2)
            y++;
        else
            return false;
        return true;
    }
    return false;
}


/*
 Compare for sorting
 */

bool Room::compareXY(Room i, Room j) {
    int ix = (2 * i.x + i.w) / 2;
    int iy = (2 * i.y + i.h) / 2;
    int jx = (2 * j.x + j.w) / 2;
    int jy = (2 * j.y + j.h) / 2;
    int iDist = ((ix - (i.size / 2)) * (ix - (i.size / 2)) + (iy - (i.size / 2)) * (iy - (i.size / 2)));
    int jDist = ((jx - (j.size / 2)) * (jx - (j.size / 2)) + (jy - (j.size / 2)) * (jy - (j.size / 2)));
    return iDist < jDist;
}


/*
 Value retrevial
 */

int Room::num() {
    return id;
}

std::pair<int, int> Room::coords() {
    return std::pair<int, int>(x, y);
}

std::pair<int, int> Room::dim() {
    return std::pair<int, int>(w, h);
}


////////////////
// Hall Class //
////////////////

/*
 Constructor:
 s - start room
 sxy - y * size + x, (x,y) coords of starting point
 e - end room
 exy - y * size + x, (x,y) coords of ending point
 will adjust so sxy < exy to limit duplicate halls
 */

Hall::Hall(int s, int sxy, int e, int exy, int d) : start(s), startxy(sxy), end(e), endxy(exy), direction(d) {
    if (sxy > exy) {
        startxy = exy;
        endxy = sxy;
        start = e;
        end = s;
        if (d == NORTH)
            direction = SOUTH;
        else if (d == WEST)
            direction = EAST;
    }
};

Hall::Hall() : start(-1), startxy(-1), end(-1), endxy(-1), direction(-1) { }

bool Hall::equals(Hall other) {
    return (startxy == other.startxy) && (endxy == other.endxy);
}

bool Hall::sameConnection(Hall other) {
    return (start == other.start) && (end == other.end);
}

std::pair<int, int> Hall::rooms() {
    return std::pair<int, int>(start, end);
}

std::pair<int, int> Hall::coords() {
    return std::pair<int, int>(startxy, endxy);
}

int Hall::dir() {
    return direction;
}

