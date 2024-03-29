// v3.4.1 stable
// first steps towards merging of v3.3 with v4.2.1
// (remove global variables and use return functions where possible)

#include <iostream>
#include <random>
#include <thread> // for time
using namespace std;

const int boardsize = 11;

int score;
int highscore = 0;
int food_map[boardsize][boardsize];
int player_map[boardsize][boardsize];
int deadend_map[boardsize+2][boardsize+2];

int moves_done;
int nextmove;
long long recalculations;
long long moves_considered;
int current_moves_considered;

int deadend_iterations;
double mps;
double cpm;

//starting position
int i_player;
int j_player;
//now defined in game loop!

int i_player_sug;
int j_player_sug;

//food position
int i_food;
int j_food;

bool alive;
bool scored;
bool won;
bool move_ok;
bool move_ideal;
bool forever = true;
bool consider_deadend;

//sleep time values
int move_time;
int reboot_time;

void sleep() {
    this_thread::sleep_for(std::chrono::milliseconds(move_time));
}

void sleep2() {
    this_thread::sleep_for(std::chrono::milliseconds(reboot_time));
}

void create_board() {
    for (int i = 0; i < boardsize; i++) {
        for (int j = 0; j < boardsize; j++) {
            food_map[i][j] = 0;
        }
    }
}

void create_player() {
    for (int i = 0; i < boardsize; i++) {
        for (int j = 0; j < boardsize; j++) {
            player_map[i][j] = 0;
        }
    }
}

//reduce snake values by 1
void update_player() {
    for (int i = 0; i < boardsize; i++) {
        for (int j = 0; j < boardsize; j++) {
            if (player_map[i][j] != 0)
                player_map[i][j] --;
        }
    }
}

void spawn_new_food() {
    bool free_space = false;
    int randPosX = 0;
    int randPosY = 0;
    while (!free_space) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> distribution(0, boardsize-1);
        randPosX = distribution(gen);
        randPosY = distribution(gen);
        
        //check if free space
        bool cond1 = food_map[randPosX][randPosY] == 0;
        bool cond2 = player_map[randPosX][randPosY] == 0;
        if (cond1 && cond2) {
            free_space = true;
            food_map[randPosX][randPosY] = -1;
        }
    }
    // What is going on HERE????
    // DONT USE GLOBALS //
    i_food = randPosX;
    j_food = randPosY;
}

/**
 * Draws the board for the game, displaying the state of each cell and the location of the food.
 */
void draw_board() {
    for (int i = 0; i < boardsize; i++) {
        for (int j = 0; j < boardsize; j++) {
            if (player_map[i][j] != 0) {
                cout << "0 ";
            } else {
                if (i == i_food && j == j_food) {
                    cout << "X ";
                } else {
                    cout << "- ";
                }
            }
        }
        cout << "\n";
    }
}

void create_deadend_map() {
    // enter all player positions
    for (int i = 0; i < boardsize+2; i++) {
        for (int j = 0; j < boardsize+2; j++) {
            if (i != 0 && j != 0) {
                if (player_map[i-1][j-1] != 0)
                    deadend_map[i][j] = 1;
                else
                    deadend_map[i][j] = 0;
            }
            // draw border
            if (i == 0 || i == boardsize+1)
                deadend_map[i][j] = 3;
            if (j == 0 || j == boardsize+1)
                deadend_map[i][j] = 3;
        }
    }
    // iteratively mark fields that border 3 other marked fiels
    bool new_spot_found = true;
    int counter = 0;
    deadend_iterations = 0;
    while (new_spot_found) {
        deadend_iterations ++;
        new_spot_found = false;
        for (int i = 1; i < boardsize+1; i++) {
            for (int j = 1; j < boardsize+1; j++) {
                counter = 0;
                if (deadend_map[i][j] == 0) {
                    if (deadend_map[i+1][j] != 0) {
                        if (!(i+1 == i_player+1 && j == j_player+1))
                            counter ++;
                    }
                    if (deadend_map[i-1][j] != 0) {
                        if (!(i-1 == i_player+1 && j == j_player+1))
                            counter ++;
                    }
                    if (deadend_map[i][j+1] != 0) {
                        if (!(i == i_player+1 && j+1 == j_player+1))
                            counter ++;
                    }
                    if (deadend_map[i][j-1] != 0) {
                        if (!(i == i_player+1 && j-1 == j_player+1))
                            counter ++;
                    }
                }
                // if three surrounding:
                if (counter >= 3) {
                    deadend_map[i][j] = 2;
                    new_spot_found = true;
                }
            }
        }
    }
}

void draw_deadend_map() {
    cout << "dead end map matrix:" << endl;
    for (int i = 0; i < boardsize+2; i++) {
        for (int j = 0; j < boardsize+2; j++) {
            if (deadend_map[i][j] == 1) {
                cout << "0 ";
            } else if (deadend_map[i][j] == 2) {
                cout << "O ";
            } else if (deadend_map[i][j] == 3) {
                cout << "B ";
            } else {
                cout << "- ";
            }
        }
        cout << "\n";
    }
}

bool check_scored() {
    if (i_player == i_food && j_player == j_food) {
        score ++;
        return true;
    } else {
        return false;
    }
}

bool check_alive_boundaries() {
    if (i_player > boardsize-1 || j_player > boardsize-1 || i_player < 0 || j_player < 0)
        return false;
    else
        return true;
}

bool check_alive_snake() {
    if (player_map[i_player][j_player] != 0)
        return false;
    else
        return true;
}

bool check_alive_won() {
    if (score == (boardsize*boardsize))
        return false;
    else
        return true;
}

bool check_won() {
    if (score == (boardsize*boardsize))
        return true;
    else
        return false;
}

bool check_alive() {
    if (check_alive_boundaries() && check_alive_snake() && check_alive_won()) {
        return true;
    } else {
        return false;
    }
}

bool check_deadend() {
    // check if dead end expected
    if (deadend_map[i_player_sug+1][j_player_sug+1] != 0) {
        return false;
    } else {
        return true;
    }
}

bool check_move_ok() {
    // check if crash with player
    if (player_map[i_player_sug][j_player_sug] != 0) {
        return false;
    // check if crash with walls
    } else if (i_player_sug < 0) {
        return false;
    } else if (j_player_sug < 0) {
        return false;
    } else if (i_player_sug > boardsize-1) {
        return false;
    } else if (j_player_sug > boardsize-1) {
        return false;
    } else {
        return true;
    }
}

bool check_move() {
    if (check_move_ok()) {
        if (consider_deadend) {
            if (check_deadend()) {
                return true; // move ok, no deadend
            } else {
                return false; // move not ok
            }
        } else {
            return true; // move ok
        }
    } else {
        return false; // move not ok
    }
}

void getmove6() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    j_player_sug --;
    
    moves_considered ++;
    current_moves_considered ++;
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        if (consider_deadend) {
            consider_deadend = false;
        } else {
            alive = false;
        }
    }
}

void getmove5() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    i_player_sug --;
    
    moves_considered ++;
    current_moves_considered ++;
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove6();
    }
}

void getmove4() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    j_player_sug ++;
    
    moves_considered ++;
    current_moves_considered ++;
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove5();
    }
}

void getmove3() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    i_player_sug ++;
    
    moves_considered ++;
    current_moves_considered ++;
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove4();
    }
}

void getmove2() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    if (j_player < j_food) {
        j_player_sug ++;
    } else if (j_player > j_food) {
        j_player_sug --;
    } else if (i_player < i_food) {
        i_player_sug ++;
    } else if (i_player > i_food) {
        i_player_sug --;
    }
    moves_considered ++;
    current_moves_considered ++;
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove3();
    }
}

void getmove1() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    if (i_player < i_food) {
        i_player_sug ++;
    } else if (i_player > i_food) {
        i_player_sug --;
    } else if (j_player < j_food) {
        j_player_sug ++;
    } else if (j_player > j_food) {
        j_player_sug --;
    }
    moves_considered ++;
    current_moves_considered ++;
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove2();
    }
}

void getmove() {
    current_moves_considered = 0;
    consider_deadend = true;
    getmove1();
    if (!consider_deadend) {
        getmove1();
    }
}

void game_output() {
    cout << "---------------------------"<< endl;
    cout << "SCORE: " << score << endl;
    cout << "MOVES: " << moves_done << endl;
    cout << "moves considered tot: " << moves_considered << endl;
    cout << "moves considered now: " << current_moves_considered << endl;
    cout << "recalculations total: " << recalculations << endl;
    cout << "dead_end iterations: " << deadend_iterations << endl;
    // draw boards
    draw_board();
    draw_deadend_map();
}

void end_output() {
    cout << "---------------------------"<< endl;
    if (check_won())
        cout << "WINNER, WINNER, CHICKEN DINNER" << endl;
    else
        cout << "GAME OVER!" << endl;
    cout << "---------------------------"<< endl;
    if (score > highscore) {
        highscore = score;
        cout << "NEW CURRENT HIGHSCORE: " << highscore << endl;
    } else {
        cout << "SCORE: " << score << endl;
        cout << "current highscore: " << highscore << endl;
    }
    cout << "movements: " << moves_done << endl;
    cout << "moves considered: " << moves_considered << endl;
    cout << "recalculations total: " << recalculations << endl;
    cout << "---------------------------"<< endl;
    cout << "STATISTICS" << endl;
    mps = (static_cast<double>(moves_done))/(static_cast<double>(score));
    cout << "moves per score: " << mps << endl;
    cpm = (static_cast<double>(moves_considered))/(static_cast<double>(moves_done));
    cout << "calculations p move: " << cpm << endl;
}

void resets() {
    alive = true;
    create_player();
    create_board();
    scored = true;
    // Center snake on the board
    i_player = (boardsize-1)/2;
    j_player = (boardsize-1)/2;
    score = 1;
    moves_done = 0;
    recalculations = 0;
    moves_considered = 0;
    current_moves_considered = 0;
}

void game() {
    while (forever) {
        resets();
        //game loop:
        while (alive && check_alive() && !check_won()) {
            if (check_scored())
                spawn_new_food();
            else
                update_player();
            player_map[i_player][j_player] = score; //draw new player position
            current_moves_considered = 0;
            deadend_iterations = 0;
            create_deadend_map();
            //--------
            getmove(); // <== getmove
            game_output();
            //--------
            moves_done ++; //statistics
            sleep();
        }
        end_output();
        sleep2();
    }
}


int main() {
    cout << "[SNQ] move time (milliseconds): ";
    cin >> move_time;
    cout << "[SNQ] reboot time (milliseconds): ";
    cin >> reboot_time;
    game();
}
