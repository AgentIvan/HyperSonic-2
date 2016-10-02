/*
 * CodinGame PROGRAMMING CONTEST
 * HYPERSONIC since 2016/09/25 01:00
 * (author: nyashiki)
 */

#pragma GCC optimize "O3,omit-frame-pointer,inline"


#include <iostream>
#include <string>
#include <cassert>
#include <sstream>


using namespace std;


//#define DEBUG
#define SPACE ' '

#ifdef _MSC_VER
#include <Windows.h>
double get_ms() { return (double)GetTickCount64() / 1000; }
#else
#include <sys/time.h>
double get_ms() { struct timeval t; gettimeofday(&t, NULL); return (double)t.tv_sec * 1000 + (double)t.tv_usec / 1000; }
#endif

const int MAX_TURN = 14;
const int INF = 9999999;

/* U, R, D, L */
const int dy[4] = { -1, 0, 1, 0 };
const int dx[4] = { 0, 1, 0, -1 };

enum Square {
  CELL = 0, EMPTY_BOX, RANGE_BOX, BOMB_BOX, BOX_BROKEN, WALL, Square_NB = 6
};

enum EntityType {
  PLAYER = 0, BOMB, ITEM, EntityType_NB = 3
};

enum ActionType {
  ACTION_MOVE = 0, ACTION_BOMB, ActionType_NB = 2
};

enum ItemType {
  DISAPPEARED = 0, EXTRA_RANGE, EXTRA_BOMB, ItemType_NB = 3
};



const int width = 13;
const int height = 11;
int myId = 0;
int playersCount;
int prevY = -1;
int prevX = -1;
int turn = 1;
double start = 0;

class Point {
public:
  int x, y;
  Point() : x(0), y(0) { }
  Point(int x, int y) : x(x), y(y) { }
};

class Player : public Point {
public:
  int bomsCount;
  int range;
  bool enable;
};

class Bomb : public Point {
public:
  int owner;
  int rounds;
  int range;
  bool ghost; /* 相手が置くと仮定した, 本当に置いてあるかどうかわからない爆弾 */
};

class Item : public Point {
public:
  ItemType itemType;
};

class Action : public Point {
public:
  ActionType actionType;
  string comment;
};

class State {
public:

  State() : value(0) { }

  int value;
  Square Field[height][width];
  Player me;
  Bomb bombs[80];
  Item items[80];
  int bombsCount;
  int itemsCount;
  int prevY;
  int prevX;
  int turn;
  Action actionLog[MAX_TURN + 1];
};

/*
 * STLのpriority_queueから、この自作のPriorityQueueに変えることにより
 * 約20倍の高速化.
 * 要素数が固定であることに注意.
 */
class PriorityQueue {

private:

  const int _MAX = 1024;
  State* _c;
  int _size;

public:

  PriorityQueue() {
    _c = new State[_MAX + 1];
  }

  ~PriorityQueue() {
    delete[] _c;
  }

  void init() {
    _size = 0;
  }

  bool empty() {
    return _size == 0;
  }

  State top() {
    return _c[0];
  }

  void push(State x) {
    int i = _size++;
    if (_size >= _MAX) {
      _size = _MAX - 1;
    }
    while (i > 0) {
      int p = (i - 1) / 2;
      if (_c[p].value >= x.value) {
        break;
      }
      _c[i] = _c[p];
      i = p;
    }
    _c[i] = x;
  }

  void pop() {
    State x = _c[--_size];
    int i = 0;
    while (i * 2 + 1 < _size) {
      int a = i * 2 + 1, b = i * 2 + 2;
      if (b < _size && _c[b].value > _c[a].value) {
        a = b;
      }
      if (_c[a].value <= x.value) {
        break;
      }
      _c[i] = _c[a];
      i = a;
    }
    _c[i] = x;
  }
};

namespace Game {

Square Field[height][width];
PriorityQueue que[MAX_TURN];
Player players[4];
Bomb bombs[80];
Item items[80];
int bombsCount = 0;
int itemsCount = 0;
int boxesCount = 0;


/* 標準入力から, 盤面を設定する */
void SetField() {
  boxesCount = 0;
  for (int y = 0; y < height; y++) {
    string str;
    cin >> str;
    if (y == 0) {
      start = get_ms();
    }
#ifdef DEBUG
    cerr << str << endl;
#endif
    for (int x = 0; x < width; x++) {
      if (str[x] == '.') {
        Field[y][x] = CELL;
      } else if (str[x] == '0') {
        boxesCount++;
        Field[y][x] = EMPTY_BOX;
      } else if (str[x] == '1') {
        boxesCount++;
        Field[y][x] = RANGE_BOX;
      } else if (str[x] == '2') {
        boxesCount++;
        Field[y][x] = BOMB_BOX;
      } else if (str[x] == 'X') {
        Field[y][x] = WALL;
      }
    }
  }
}

/* 標準入力からplayers, bombsを設定する */
void SetEntities() {
  bombsCount = 0;
  itemsCount = 0;
  playersCount = 0;
  for (int i = 0; i < 4; i++) {
    players[i].enable = false;
  }
  int n;
  cin >> n;
#ifdef DEBUG
  cerr << n << endl;
#endif
  for (int i = 0; i < n; i++) {
    int entityType;
    int owner;
    int x;
    int y;
    int param1;
    int param2;
    cin >> entityType >> owner >> x >> y >> param1 >> param2;
#ifdef DEBUG
    cerr << entityType << SPACE << owner << SPACE << x << SPACE << y << SPACE << param1 << SPACE << param2 << endl;
#endif
    switch (entityType) {
      case PLAYER: {
        players[owner].x = x;
        players[owner].y = y;
        players[owner].bomsCount = param1;
        players[owner].range = param2;
        players[owner].enable = true;
        playersCount++;
        break;
      }
      case BOMB: {
        bombs[bombsCount].x = x;
        bombs[bombsCount].y = y;
        bombs[bombsCount].owner = owner;
        bombs[bombsCount].rounds = param1;
        bombs[bombsCount].range = param2;
        bombs[bombsCount].ghost = false;
        bombsCount++;
        break;
      }
      case ITEM: {
        items[itemsCount].x = x;
        items[itemsCount].y = y;
        items[itemsCount].itemType = (ItemType)param1;
        itemsCount++;
        break;
      }
    }
  }
}

Action think() {
  Action action;

  for (int i = 0; i <= MAX_TURN; i++) {
    que[i].init();
  }

  State firstState;

  /* 初期状態の準備 */
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      firstState.Field[y][x] = Field[y][x];
    }
  }

  firstState.bombsCount = bombsCount;
  firstState.itemsCount = itemsCount;
  for (int i = 0; i < bombsCount; i++) {
    firstState.bombs[i] = bombs[i];
  }
  for (int i = 0; i < itemsCount; i++) {
    firstState.items[i] = items[i];
  }

  firstState.me = players[myId];
  firstState.prevY = prevY;
  firstState.prevX = prevX;
  firstState.turn = turn;
  que[0].push(firstState);

  double end;
  double elapsed;
  bool timeOut = false;
  int loopCount = 0;

  State bestState;
  bestState.value = -INF;
#ifdef DEBUG
  for (int l = 0; l < 200; l++) {
#else
  while (true) {
#endif
    loopCount++;
    for (int t = 0; t < MAX_TURN; t++) {
      for (int w = 0; w < 1; w++) {
        if (que[t].empty()) {
          break;
        }
        State state = que[t].top(); que[t].pop();
        /* まずは, 1ターン前に壊れた箱を取り除く */
        if (t > 0) {
          for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
              if (state.Field[y][x] == BOX_BROKEN) {
                state.Field[y][x] = CELL;
              }
            }
          }
        }
        /* 相手は爆弾を置けるなら, 置くと仮定する */
        if (t == 1) {
          for (int i = 0; i < 4; i++) {
            if (i == myId) {
              continue;
            }
            if (!players[i].enable) {
              continue;
            }
            if (players[i].bomsCount > 0) {
              state.bombs[state.bombsCount].owner = i;
              state.bombs[state.bombsCount].range = players[i].range;
              state.bombs[state.bombsCount].rounds = 8;
              state.bombs[state.bombsCount].y = players[i].y;
              state.bombs[state.bombsCount].x = players[i].x;
              state.bombs[state.bombsCount].ghost = true;
              state.bombsCount++;
            }
          }
        }

        bool isExploded = false;
        /* アイテム, 爆弾周りの処理 */
        int isBomb[height][width] = { 0 };
        int isItem[height][width] = { 0 };
        for (int i = 0; i < state.bombsCount; i++) {
          /* 爆弾が重なっていたら, 範囲の広いほうを優先する. */
          if (isBomb[state.bombs[i].y][state.bombs[i].x]) {
            if (state.bombs[isBomb[state.bombs[i].y][state.bombs[i].x] - 1].range > state.bombs[i].range) {
              state.bombs[i].rounds = -1;
              continue;
            }
          }
          state.bombs[i].rounds--;
          if (state.bombs[i].rounds == 0) {
            isExploded = true;
          }
          isBomb[state.bombs[i].y][state.bombs[i].x] = i + 1;
        }
        for (int i = 0; i < state.itemsCount; i++) {
          if (state.items[i].itemType != DISAPPEARED) {
            isItem[state.items[i].y][state.items[i].x] = i + 1;
          }
        }

        if (isItem[state.me.y][state.me.x]) {
          int i = isItem[state.me.y][state.me.x] - 1;
          if (state.items[i].itemType == EXTRA_BOMB) {
            if (state.me.bomsCount < 3) {
              state.value += 2000 - t * t;
            } else if (state.me.bomsCount < 8) {
              state.value += 400 - t;

            }
            state.me.bomsCount++;
          } else if (state.items[i].itemType == EXTRA_RANGE) {
            if (state.me.range < 13) {
              state.value += 200 - t;
            }
            state.me.range++;
          }
          isItem[state.me.y][state.me.x] = 0;
          state.items[i].itemType = DISAPPEARED;
        }


        bool isDead = false;

        /* 爆弾を爆発させる */
        while (isExploded && !isDead) {
          isExploded = false;
          for (int i = 0; i < state.bombsCount && !isDead; i++) {
            if (state.bombs[i].rounds == 0) {
              state.bombs[i].rounds = -1;
              isBomb[state.bombs[i].y][state.bombs[i].x] = 0;
              isExploded = true;
              for (int d = 0; d < 4 && !isDead; d++) {
                for (int j = 0; j < state.bombs[i].range; j++) {
                  int ny = state.bombs[i].y + dy[d] * j;
                  int nx = state.bombs[i].x + dx[d] * j;
                  if (ny < 0 || ny >= height) {
                    break;
                  }
                  if (nx < 0 || nx >= width) {
                    break;
                  }
                  if (ny == state.me.y &&
                    nx == state.me.x) {
                    if (state.bombs[i].ghost) {
                      state.value -= 35000; /* 相手が爆弾を置いていたら, 巻き込まれる */
                    } else {
                      isDead = true;
                    }
                    break;
                  }
                  if (state.bombs[i].owner == myId) {
                    state.me.bomsCount++;
                  }
                  bool breakSomething = false;
                  switch (state.Field[ny][nx]) {
                    case EMPTY_BOX: {
                      if (state.bombs[i].owner == myId) {
                        state.value += 2000 - t;
                      } else {
                        state.value -= 500;
                      }
                      state.Field[ny][nx] = BOX_BROKEN;
                      breakSomething = true;
                      break;
                    }
                    case RANGE_BOX: {
                      if (state.bombs[i].owner == myId) {
                        state.value += 2150 - t;
                      } else {
                        state.value -= 500;
                      }
                      state.Field[ny][nx] = BOX_BROKEN;
                      state.items[state.itemsCount].itemType = EXTRA_RANGE;
                      state.items[state.itemsCount].y = ny;
                      state.items[state.itemsCount].x = nx;
                      isItem[ny][nx] = state.itemsCount + 1;
                      state.itemsCount++;
                      breakSomething = true;
                      break;
                    }
                    case BOMB_BOX: {
                      if (state.bombs[i].owner == myId) {
                        state.value += 2260 - t;
                      } else {
                        state.value -= 500;
                      }
                      state.Field[ny][nx] = BOX_BROKEN;
                      state.items[state.itemsCount].itemType = EXTRA_BOMB;
                      state.items[state.itemsCount].y = ny;
                      state.items[state.itemsCount].x = nx;
                      isItem[ny][nx] = state.itemsCount + 1;
                      state.itemsCount++;
                      breakSomething = true;
                      break;
                    }
                    case WALL: {
                      /* 壊したわけではないが, 便宜上. */
                      breakSomething = true;
                      break;
                    }
                  }
                  /* 爆弾があった場合は, 即座に爆発させる*/
                  if (isBomb[ny][nx]) {
                    if (j > 0) {
                      state.value++;
                      state.bombs[isBomb[ny][nx] - 1].rounds = 0;
                    }
                    breakSomething = true;
                  }
                  /* アイテムがあった場合は, 消滅させる */
                  if (isItem[ny][nx]) {
                    state.items[isItem[ny][nx] - 1].itemType = DISAPPEARED;
                    state.value -= 100;
                    breakSomething = true;
                  }
                  if (breakSomething) {
                    break;
                  }
                }
              }
            }
          }
        }
        /* 爆風に巻き込まれた場合は, 即座に終了 */
        if (isDead) {
          break;
        }

        /* 爆弾が届く範囲にある箱の数を求める */
        int boxesAroundMe = 0;

        if (boxesCount > 0) {
          for (int i = 0; i < 4; i++) {
            for (int j = 1; j < state.me.range; j++) {
              int ny = state.me.y + dy[i] * j;
              int nx = state.me.x + dx[i] * j;
              if (ny < 0 || ny >= height) {
                continue;
              }
              if (nx < 0 || nx >= width) {
                continue;
              }
              if (state.Field[ny][nx] == EMPTY_BOX ||
                state.Field[ny][nx] == RANGE_BOX ||
                state.Field[ny][nx] == BOMB_BOX) {
                boxesAroundMe++;
              }
              if (isBomb[ny][nx] || isItem[ny][nx] || state.Field[ny][nx] != CELL)
              {
                break;
              }
            }
          }
        } else {
          for (int i = 0; i < 4; i++) {
            if (i != myId && players[i].enable) {
              state.value += (abs(state.me.y - players[i].y) + abs(state.me.x - players[i].x)) << 10;
            }
          }
        }

        int currentNextStateValue = -INF;
        if (t < MAX_TURN - 1 && !que[t + 1].empty()) {
          currentNextStateValue = que[t + 1].top().value;
        }

        State nextState = state;
        nextState.prevY = state.me.y;
        nextState.prevX = state.me.x;
        nextState.turn = state.turn + 1;
        /* 次の行動を全て列挙し, t + 1ターン目のqueueに入れる */
        for (int setBomb = 1; setBomb >= 0; setBomb--) {
          Action nextAction;
          if (setBomb) {
            /*
             * 爆弾を置く必要がないと考えられる場合(ヒューリスティック)は,
             * 爆弾を置くことを考えない.
             */
            if (isBomb[state.me.y][state.me.x]) {
              continue;
            }
            if (state.me.bomsCount == 0) {
              continue;
            } else if (boxesCount == 0) {
              continue;
            }

            nextState.value += -50 + 700 * boxesAroundMe;

            nextState.me.bomsCount--;
            nextAction.actionType = ACTION_BOMB;
            nextState.bombs[nextState.bombsCount].rounds = 8;
            nextState.bombs[nextState.bombsCount].y = state.me.y;
            nextState.bombs[nextState.bombsCount].x = state.me.x;
            nextState.bombs[nextState.bombsCount].range = state.me.range;
            nextState.bombs[nextState.bombsCount].owner = myId;
            nextState.bombs[nextState.bombsCount].ghost = false;
            nextState.bombsCount++;
          } else {
            nextAction.actionType = ACTION_MOVE;
          }

          /* 何もしない, または近傍4方向に移動することだけを考えれば良い */
          for (int i = 3; i >= -1; i--) {
            int ny = state.me.y;
            int nx = state.me.x;

            if (i == -1) {
              nextState.value -= 200;
            } else {
              ny += dy[i];
              nx += dx[i];
              /* 戻るのは, 動かないことと同じ */
              if (ny == state.prevY && nx == state.prevX) {
                continue;
              }
            }

            if (ny < 0 || ny >= height) {
              continue;
            }
            if (nx < 0 || nx >= width) {
              continue;
            }
            if (state.Field[ny][nx] != CELL) {
              continue;
            }
            if (i != -1 && isBomb[ny][nx] > 0) {
              continue;
            }

            int moveScore = 0;
            /* 基本的に, 端よりは中心にいるほうが良い */
            if ((state.me.y < (height >> 1) && state.me.y < ny) ||
              (state.me.y >(height >> 1) && state.me.y > ny) ||
              (state.me.x < (width >> 1) && state.me.x < nx) ||
              (state.me.x >(width >> 1) && state.me.x > nx)) {
              moveScore += 2;
            }
            nextState.value += moveScore;

            nextState.me.y = ny;
            nextState.me.x = nx;
            nextAction.y = ny;
            nextAction.x = nx;
            nextState.actionLog[t] = nextAction;

            if (t == MAX_TURN - 1) {
              if (nextState.value > bestState.value) {
                bestState = nextState;
              }
            } else {
              que[t + 1].push(nextState);
            }

            /* UNDOを忘れずに */
            if (i == -1) {
              nextState.value += 200;
            }
            nextState.value -= moveScore;
          }
          /* setBombでのUNDO */
          if (setBomb) {
            nextState.value -= -50 + 700 * boxesAroundMe;
            nextState.me.bomsCount++;
            nextState.bombsCount--;
          }
        }
      }
#ifndef DEBUG
      end = get_ms();
      elapsed = end - start;
      if (elapsed > 96) {
        timeOut = true;
        break;
      }
      if (boxesCount == 0 && elapsed > 93) {
        timeOut = true;
        break;
      }
#endif
    }

#ifndef DEBUG
    if (timeOut) {
      break;
    }
#endif       
  }
  Action bestAction = bestState.actionLog[0];
#ifdef DEBUG
  elapsed = end - start;
  ostringstream strs;
  strs << elapsed << "ms (" << loopCount << ")" << playersCount;
  bestAction.comment = strs.str();
#endif
  return bestAction;
}

/* 次の行動を標準出力に出力 */
void output(const Action& action) {
  if (action.actionType == ACTION_MOVE) {
    cout << "MOVE" << SPACE << action.x << SPACE << action.y << SPACE << action.comment << endl;
  } else if (action.actionType == ACTION_BOMB) {
    cout << "BOMB" << SPACE << action.x << SPACE << action.y << SPACE << action.comment << endl;
  }
}


/* メインループ */
void start() {
  while (true) {
    SetField();
    SetEntities();

    Action nextAction = think();
    output(nextAction);

    prevY = players[myId].y;
    prevX = players[myId].x;
    turn++;
  }
}

}

int main() {
  cin.tie(0);
  ios::sync_with_stdio(false);
  int w, h;
  cin >> w >> h >> myId;    /* width, heightは固定なので, 読み捨てる */

  Game::start();

  return 0;

}

