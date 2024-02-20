// snake v4.2_N Monte Carlo Simulation
// (v4.2.3)
// STABLE RELEASE

// IMPORTANT BUG FIXES, IMPROVED CODE QUALITY AND UI#

#include <iostream>
#include <random>
#include <thread> // for time
#include <tuple> // for multiple return values
#include <ctime> // for time out
#include <iomanip> // for table output
#define SECOND 1
#define HALF_SECOND 0,5
#define RUNOUT_TIME 0,1
using namespace std;

const int boardsize = 11;
int score;
int highscore = 47;
int current_highscore = 0;
int food_map[boardsize][boardsize];
int player_map[boardsize][boardsize];
int deadend_map[boardsize+2][boardsize+2];

int above_avg_amount;
int below_avg_amount;
int above_avg_percent;
int below_avg_percent;
int at_avg_percent;

int measurment_interval; // time interval for measuring runtime
int output_interval; // idk
int output_counter; // idk
int output_index; // hard to explain, scales MC output
int delay; // time delay between games
int progress_interval; // every how many games should it show the current nr when doing MC SIM
int game_counter; // how many games have been played in time measurment interval
int total_game_counter; // how many games have been played overall
int seconds; // for counting how many times it has measured the programs runtime
int move_time; // time between moves

int moves_done;
int moves_considered;
int deadend_map_iterations;

vector<int> score_list;
double avg_score;
int outputmode;
const int scores_expected_end = 91;
const int scores_expected_start = 4;
int scores[scores_expected_end];

// player position
int i_player;
int j_player;

// suggested new player position
int i_player_sug;
int j_player_sug;

// food position
int i_food;
int j_food;

// bool values
bool alive;
bool scored;
bool won;
bool move_ok;
bool move_ideal;
bool consider_deadend;
bool run_games = true;
bool show_restarts;
bool play_with_deadendmap;


// sleep between moves
void sleep1() {
    this_thread::sleep_for(std::chrono::milliseconds(move_time));
}

// sleep between completing games
void sleep2() {
    this_thread::sleep_for(std::chrono::milliseconds(delay));
}

// sleep after timing out and before restarting
void sleep3() {
    this_thread::sleep_for(std::chrono::milliseconds(1000));
}

// create 2D Array for board
void create_board() {
    for (int i = 0; i < boardsize; i++) {
        for (int j = 0; j < boardsize; j++) {
            food_map[i][j] = 0;
        }
    }
}

// create 2D Array for player
void create_player() {
    for (int i = 0; i < boardsize; i++) {
        for (int j = 0; j < boardsize; j++) {
            player_map[i][j] = 0;
        }
    }
}

// updating player: reducing player value by one
void update_player() {
    for (int i = 0; i < boardsize; i++) {
        for (int j = 0; j < boardsize; j++) {
            if (player_map[i][j] != 0)
                player_map[i][j] --;
        }
    }
}

// food spawn: creating random integers and and checking if the coordinates are unoccupied
void spawn_new_food() {
    bool free_space = false;
    int random_number_1 = 0;
    int random_number_2 = 0;
    while (!free_space) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> distribution(0, boardsize-1);
        random_number_1 = distribution(gen);
        random_number_2 = distribution(gen);
        
        //check if free space
        if (food_map[random_number_1][random_number_2] == 0 && player_map[random_number_1][random_number_2] == 0) {
            free_space = true;
            food_map[random_number_1][random_number_2] = -1;
        }
    }
    i_food = random_number_1;
    j_food = random_number_2;
}


// create 2D Array for dead-end map:
// first marking all player positions and then iteratively going through the matrix and marking fields that border 3 other marked fields until no new fields are found
void create_deadend_map() {
    // enter all player positions
    for (int i = 0; i < boardsize+2; i++) {
        for (int j = 0; j < boardsize+2; j++) {
            if (i != 0 && j != 0) {
                if (player_map[i-1][j-1] != 0) {
                    deadend_map[i][j] = 1;
                } else {
                    deadend_map[i][j] = 0;
                }
            }
            // draw border
            if (i == 0 || i == boardsize+1) {
                deadend_map[i][j] = 3;
            }
            if (j == 0 || j == boardsize+1) {
                deadend_map[i][j] = 3;
            }
        }
    }
    
    // iteratively mark fields that border 3 other marked fiels
    bool new_spot_found = true;
    int counter = 0;
    deadend_map_iterations = 0;
    while (new_spot_found) {
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
        deadend_map_iterations ++;
    }
}


// checking if player has scored (player coords = food coords)
bool check_scored() {
    if (i_player == i_food && j_player == j_food) {
        score ++;
        return true;
    } else {
        return false;
    }
}

// checking if player has left the boundaries of the board
bool check_alive_boundaries() {
    if (i_player > boardsize-1 || j_player > boardsize-1 || i_player < 0 || j_player < 0)
        return false;
    else
        return true;
}

// checking if the player has crashed into itself
bool check_alive_snake() {
    if (player_map[i_player][j_player] != 0)
        return false;
    else
        return true;
}

/*
// checking if game has been won
bool check_alive_won() {
    if (score == (boardsize*boardsize))
        return false;
    else
        return true;
}
 */

// checking if game has been won
bool check_won() {
    if (score == (boardsize*boardsize))
        return true;
    else
        return false;
}

// checking all alive conditions
bool check_alive() {
    if (check_alive_boundaries() && check_alive_snake() && !check_won()) {
        return true;
    } else {
        return false;
    }
}

// checking if player would move into a dead end with move suggestion
bool check_deadend() {
    // check if dead end expected
    if (deadend_map[i_player_sug+1][j_player_sug+1] != 0) {
        return false;
    } else {
        return true;
    }
}

// checking if suggested move would kill snake
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

// checking suggested move (depending if dead-ends should be considered)
bool check_move() {
    if (check_move_ok()) {
        if (consider_deadend) {
            if (check_deadend()) {
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    } else {
        return false;
    }
}

// getting random move: left
void getmove6() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    j_player_sug --;
    moves_considered ++;
    
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        if (consider_deadend) {
            // -> going through getmove again, now without considering
            consider_deadend = false;
        } else {
            // -> no move found -> DEAD
            alive = false;
        }
    }
}

// trying random move: up
void getmove5() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    i_player_sug --;
    
    moves_considered ++;
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove6();
    }
}

// trying random move: right
void getmove4() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    j_player_sug ++;
    
    moves_considered ++;
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove5();
    }
}

// trying random move: down
void getmove3() {
    i_player_sug = i_player;
    j_player_sug = j_player;
    
    i_player_sug ++;
    moves_considered ++;

    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove4();
    }
}

// getting move from j and then i direction of food
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
    
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove3();
    }
}

// getting move from i and then j direction of food
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
    
    if (check_move()) {
        i_player = i_player_sug;
        j_player = j_player_sug;
    } else {
        getmove2();
    }
}

// looking for a new move
// first considering dead-ends
// then again without
void getmove() {
    if (play_with_deadendmap) {
        consider_deadend = true;
        getmove1();
        if (!consider_deadend) { // <- BUG FIX
            getmove1();
        }
    } else {
        consider_deadend = false;
        getmove1();
    }
    
    
    moves_done ++;
}

// output mode 4:
// monte carlo simulation with probability curve and progress updates
void outputmode_4() {
    if (output_counter == 1) {
        cout << "[MCS] calculating probabilities..." << endl;
        cout << "[MCS] this can take a few moments." << endl;
        sleep3();
        output_counter ++;
    } else if (output_counter == output_interval) {
        cout << "\n[MCS] CALCULATIONS COMPLETE" << endl;
        sleep3();
        cout << "--------------" << endl;
        cout << "games: " << output_interval << endl;
        cout << "avg: " << avg_score << endl;
        cout << "max: " << current_highscore << endl;
        cout << "above avg: " << above_avg_percent << "%" << endl;
        cout << "below avg: " << below_avg_percent << "%" << endl;
        cout << "--------------" << endl;
        for (int i = scores_expected_start; i < scores_expected_end; i++) {
            if (i < 10) {
                cout << i << " : " << scores[i] << " ";
            } else {
                cout << i << ": " << scores[i] << " ";
            }
            for (int j = 0; j < (scores[i]/(output_interval/output_index)); j++) {
                if (i == avg_score) {
                    cout << "E";
                } else {
                    cout << "#";
                }
            }
            cout << "\n";
        }
        cout << "-------------" << endl;
        run_games = false;
    } else if (output_counter % progress_interval == 0) {
        cout << output_counter << endl;
        output_counter ++;
    } else {
        output_counter ++;
    }
}

// output mdoe 5:
// monte carlo simulation with probability curve and constant output monitoring
void outputmode_5() {
    if (output_counter == 1) {
        cout << "[MCS] calculating probabilities..." << endl;
        cout << "[MCS] this can take a few moments." << endl;
        output_counter ++;
        sleep3();
    } else if (output_counter == output_interval) {
        cout << "\n[MCS] CALCULATIONS COMPLETE" << endl;
        sleep3();
        cout << "--------------" << endl;
        cout << "games: " << output_interval << endl;
        cout << "avg: " << avg_score << endl;
        cout << "max: " << current_highscore << endl;
        cout << "above avg: " << above_avg_percent << "%" << endl;
        cout << "below avg: " << below_avg_percent << "%" << endl;
        cout << "--------------" << endl;
        for (int i = scores_expected_start; i < scores_expected_end; i++) {
            if (i < 10) {
                cout << i << " : " << scores[i] << " ";
            } else {
                cout << i << ": " << scores[i] << " ";
            }
            for (int j = 0; j < (scores[i]/(output_interval/output_index)); j++) {
                if (i == avg_score) {
                    cout << "E";
                } else {
                    cout << "#";
                }
            }
            cout << "\n";
        }
        run_games = false;
    } else if (output_counter % progress_interval == 0) {
        cout << output_counter << endl;
        output_counter ++;
    } else { // BETTER OUTPUT FORMATING !!
        cout << score_list.size() << ": (" << left << setw(7) << avg_score << "|" << current_highscore << ") (" << below_avg_percent << "|" << at_avg_percent << "|" << above_avg_percent << "): ";
        for (int i = 1; i <= score; i++) {
            cout << "#";
        }
        cout << " " << score << endl;
        output_counter ++;
    }
}

void outputmode_6() {
    cout << left << "gps: " << setw(6) << game_counter << "avg: " << (total_game_counter/((measurment_interval/1000)*seconds)) << endl;
}

void outputmode_7() {
    for (int k = 1; k < (2*boardsize); k++) {
        cout << "-";
    }
    cout << "\n";
    
    cout << setw(20) << "score: " << score << endl;
    cout << setw(20) << "moves done: " << moves_done << endl;
    cout << setw(20) << "moves considered: " << moves_considered << endl;
    cout << setw(20) << "deadend iterations: " << deadend_map_iterations << endl;

    
    for (int k = 1; k < (2*boardsize); k++) {
        cout << "-";
    }
    cout << "\n";
    
    for (int i = 0; i < boardsize; i++) {
        for (int j = 0; j < boardsize; j++) {
            if (player_map[i][j] != 0) {
                cout << "0 ";
            } else {
                if (i == i_food && j == j_food) {
                    cout << "X ";
                } else if (deadend_map[i+1][j+1] != 0) {
                    cout << "+ ";
                } else {
                    cout << "- ";
                }
            }
        }
        cout << "\n";
    }
}


// different output modes to choose from in the beginning
void output() {
    switch (outputmode) {
        case 1:
            cout << "\n\n\n\n\n";
            cout << score_list.size() << ": " << score << endl;
            cout << "-------" << endl;
            
            cout << "avg: " << avg_score << endl;
            cout << "max: " << current_highscore << endl;
            break;
            
        case 2:
            cout << score_list.size() << ": ("  << left << setw(7) << avg_score << "|" << current_highscore << ") (" << below_avg_percent << "|" << at_avg_percent << "|" << above_avg_percent << "): ";
            for (int i = 1; i <= score; i++) {
                cout << "#";
            }
            cout << " " << score << endl;
            break;
            
        case 3:
            cout << score << endl;
            break;
            
        case 4:
            outputmode_4();
            break;
        case 5:
            outputmode_5();
            break;
    }
}

// resets neccesary before starting a new game
void resets() {
    alive = true;
    create_player();
    create_board();
    scored = true;
    score = 1;
    i_player = (boardsize-1)/2;
    j_player = (boardsize-1)/2;
    avg_score = 0;
    moves_done = 0;
    moves_considered = 0;
}

// updating statistics and storing result values
void ingame_updates() {
    score_list.push_back(score);
    game_counter ++;
    total_game_counter ++;
    int tot_score = 0;
    int tot_score_new = 0;
    for (int i = 0; i < score_list.size(); i++) {
        tot_score_new = tot_score + score_list[i];
        tot_score = tot_score_new;
        avg_score = (static_cast<double>(tot_score))/(static_cast<double>(i+1));
    }
    if (score > current_highscore)
        current_highscore = score;
    if (score > avg_score) {
        above_avg_amount ++;
    } else if (score < avg_score) {
        below_avg_amount ++;
    }
    below_avg_percent = (100*below_avg_amount/score_list.size());
    above_avg_percent = (100*above_avg_amount/score_list.size());
    at_avg_percent = (100*score_list[avg_score]/score_list.size());
    
    scores[score] ++;
}

// game loop
void game() {
    clock_t start_measure = clock();
    clock_t now_measure;
    while (run_games) {
        resets();
        // restart clock
        clock_t start_timeout = clock();
        clock_t now_timeout;

        while (alive && check_alive()) { // also consider timeout-kill
            if (check_scored())
                spawn_new_food(); //spawn new food
            else
                update_player();
            player_map[i_player][j_player] = score; //draw new player position
            if (play_with_deadendmap)
                create_deadend_map();
            getmove();
            
            now_timeout = clock();
            if (((now_timeout-start_timeout)/CLOCKS_PER_SEC) >= SECOND) { // check runtime / timeout
                alive = false;
                if (show_restarts)
                    cout << "[MCS] system timed out - restarting..." << endl;
                sleep3();
                start_measure = clock();
                break;
            }
            if (outputmode == 6) {
                now_measure = clock();
                if (((now_measure-start_measure)/CLOCKS_PER_SEC) >= ((SECOND*measurment_interval)/1000)) {
                    outputmode_6();
                    alive = false;
                    game_counter = 0;
                    seconds ++;
                    start_measure = clock();
                    break;
                }
            }
            if (outputmode == 7) {
                outputmode_7();
                sleep1();
            }
        }
        ingame_updates();
        output();
        sleep2();
    }
}

// output after starting program
void start_output() {
    cout << "WELCOME TO SNAKE MONTE CARLO SIMULATION BY TUM AEROSNAKE" << endl;
    cout << "========================================================" << endl;
    cout << "v4.2.3: important bug fixes, improved UI" << endl;
    cout << "========================================" << endl;
    cout << "mode 1: nr: score\n        avg: avg\n        max: max" << endl;
    cout << "mode 2: nr: (avg|max): 000000000 score" << endl;
    cout << "mode 3: score" << endl;
    cout << "mode 4: monte carlo simulation" << endl;
    cout << "mode 5: mode 2 & mode 4" << endl;
    cout << "mode 6: time measurement" << endl;
    cout << "mode 7: classical snake game" << endl;
    cout << "--------------------------------" << endl;
}

// get input values for different output modes
void get_inputs() {
    switch (outputmode) {
        case 1:
            show_restarts = false;
            break;
        case 2:
            show_restarts = true;
            break;
        case 3:
            show_restarts = false;
            break;
        case 4: {
            cout << "[MCQ] nr. of games (>index): ";
            cin >> output_interval;
            cout << "[MCQ] output index (4000/5000): ";
            cin >> output_index;
            cout << "[MCQ] progress update interval: ";
            cin >> progress_interval;
            cout << "[MCQ] show timeout restart sequence (y/n): ";
            string input;
            cin >> input;
            if (input == "y")
                show_restarts = true;
            else
                show_restarts = false;
            } // brackets are to seperate output_interval inputs
            break;
        case 5:
            show_restarts = true;
            cout << "[MCQ] nr. of games (>index): ";
            cin >> output_interval;
            cout << "[MCQ] output index (4000/5000): ";
            cin >> output_index;
            break;
        case 6:
            cout << "[MCQ] enter time interval to measure (milliseconds): ";
            cin >> measurment_interval;
            total_game_counter = 0;
            seconds = 1;
            show_restarts = true;
            break;
        case 7:
            cout << "[MCQ] move time (milliseconds): ";
            cin >> move_time;
            cout << "[MCQ] reboot time (milliseconds): ";
            cin >> delay;
        default:
            cout << "[MCE] ERROR (101): UNABLE TO IDENTIFIE OUTPUTMODE" << endl;
    }
}

// main function: choosing output mode, getting input values, running games
int main() {
    start_output();
    bool correct_input;
    do {
        cout << "[MCQ] choose output mode: ";
        cin >> outputmode;
        correct_input = true;
        if (outputmode >= 1 && outputmode <= 5) {
            get_inputs();
            cout << "[MCQ] enter time delay (milliseconds): ";
            cin >> delay;
            cout << "[MCQ] play with deadend maps? (y/n): ";
            string input;
            cin >> input;
            if (input == "n") {
                play_with_deadendmap = false;
            } else {
                play_with_deadendmap = true;
            }
            game();
        } else if (outputmode == 6 || outputmode == 7){
            get_inputs();
            game();
        } else {
            cout << "[MCE] ERROR (102): UNABLE TO IDENTIFIE OUTPUT MODE" << endl;
            correct_input = false;
        }
    } while (!correct_input);
    return 0;
}
