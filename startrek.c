//
// startrek.c
//

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define START_DATE 2700
#define START_DAYS 26
#define NUM_KL 26
#define NUM_SB 3
#define NUM_STARS 262
#define QS_SIZE 8
#define PLAYER_ENERGY 3000
#define STR_SIZE 50


// Structs & Enums
typedef enum conditions_t {green, yellow, red, docked} Condition;
enum status_t {warp, srs, lrs, pha, tor, damageControl, she, libComp};

typedef struct Entity {
    int energy;
    int pos[4];
} Entity;
typedef struct Entity Starbase;
typedef struct Entity Klingon;
typedef struct Entity Star;

typedef struct Player {
    int energy;
    int shield;
    int photon;
    int pos[4]; // Player position Q1, Q2, S1, S2
    double damage[8];
    Condition condition;
} Player;

typedef struct Quadrant {
    int numKlingons;
    int numStarbases;
    int numStars;
    bool scanned;
    char sector[QS_SIZE][QS_SIZE];
} Quadrant;

typedef struct World {
    int date;
    int daysRem;
    int numKlingons;
    int numStarbases;
    char quadNames[9][STR_SIZE];
    char statNames[8][STR_SIZE];
    Klingon klingons[NUM_KL];
    Starbase starbases[NUM_SB];
    Star stars[NUM_STARS];
    Quadrant quadrant[QS_SIZE][QS_SIZE];
    Quadrant quadrantArch[QS_SIZE][QS_SIZE];
    Player player;
    bool gameOver;
} World;


// Small Functions
int randRange(int lo, int hi) {
    return lo + rand() % (hi - lo);
}

double drand() {
    return (rand() % 1000) / (1000.00);
}

double getDistance(Player *player, const int *to) {
    int x = ((player->pos[1] * 8) + player->pos[3]) - ((to[1]*8)+to[3]);
    int y = ((player->pos[0] * 8) + player->pos[2]) - ((to[0]*8)+to[2]);
    return sqrt(pow(x,2) + pow(y,2));
}


// Functions Header
void printTitle();
World generateWorld();
void updateCond(World *world);
Entity *getNearbyEntity(World *world, int n, char type);
void getCmd(World *world);
void cmdNAV(World *world);
void printStat(World *world, int n);
void cmdSRS(World *world);
void cmdLRS(World *world);
void cmdPHA(World *world);
void cmdTOR(World *world);
void cmdSHE(World *world);
void cmdDAM(World *world);
void cmdCOM(World *world);
void cmdXXX(World *world);
void sortEntities(Entity **entity, double *dist, int n);
void klingonShooting(World *world);
void checkGameOver(World *world);
void printInstructions();


// MARK - Game Main Entry Point //
int main() {
    srand(time(NULL));

    printTitle();
    World world = generateWorld();

    while (!world.gameOver){
        getCmd(&world);
        checkGameOver(&world);
    }

    return 0;
}
// END OF MAIN //


// Functions :-
void printTitle(){
    printf("\n*****************************************\n");
    printf("*                   *                   *\n");
    printf("*                 * * *                 *\n");
    printf("*     * *    SUPER STAR TREK   * *      *\n");
    printf("*               * *   * *               *\n");
    printf("*              * *     * *              *\n");
    printf("*****************************************\n");
    printf("\n                                    ,------*------,\n");
    printf("                    ,-------------   '---  ------'\n");
    printf("                     '-------- --'      / /\n");
    printf("                         ,---' '-------/ /--,\n");
    printf("                          '----------------'\n");
    printf("                    THE USS ENTERPRISE --- NCC-1701\n");

    printf("\n\nDO YOU WANT TO SEE THE GAME INSTRUCTIONS BEFORE STARTING (y/n)? ");
    int ans = tolower(getchar());
    while(fgetc(stdin)!='\n'); // Trim extra input
    if (ans == 'y') printInstructions();
}

World generateWorld(){

    // Initialize World & Player
    World world = {
            .date = START_DATE,
            .daysRem = START_DAYS,
            .numKlingons = 0,
            .numStarbases = 0,
            .gameOver = false,
            .player = {
                    .energy = PLAYER_ENERGY,
                    .shield = 0,
                    .photon = 10,
                    .pos = {4, 5, 3, 0}
            },
            .statNames = {"WARP ENGINES", "SHORT RANGE SENSORS", "LONG RANGE SENSORS", "PHASER CONTROL",
                          "PHOTON TUBES", "DAMAGE CONTROL", "SHIELD CONTROL", "LIBRARY-COMPUTER"}
    };
    int numK = 0, numSB = 0, numStars = 0;
    char empty = ' '; // EMPTY SPACE {Mainly for easier debugging}


    // Initialize world to be empty
    for (int q=0; q<QS_SIZE; q++) {
        for (int qq=0; qq<QS_SIZE; qq++){
            world.quadrant[q][qq].numKlingons = 0;
            world.quadrant[q][qq].numStars = 0;
            world.quadrant[q][qq].numStarbases = 0;
            world.quadrant[q][qq].scanned = false;
            world.quadrantArch[q][qq].scanned = false;
            for (int s=0; s<QS_SIZE; s++) {
                for (int ss=0; ss<QS_SIZE; ss++){
                    world.quadrant[q][qq].sector[s][ss] = empty;
                }
            }
        }
    }


    // Generate starbases in the world
    for (int i=0; i<NUM_SB; i++) {
        while (1) {
            // Make sure starbases don't spawn in the first quadrant
            int q1 = 4, q2 = 5;
            while (q1 == 4 || q2 == 5) {
                q1 = randRange(0, 8);
                q2 = randRange(0, 8);
            }
            int s1 = randRange(0, 8);
            int s2 = randRange(0, 8);
            // Make sure it's placed in an empty spot & the quadrant doesn't have any already
            if (world.quadrant[q1][q2].sector[s1][s2] == empty && world.quadrant[q1][q2].numStarbases <= 1){
                world.quadrant[q1][q2].sector[s1][s2] = 'B';
                world.starbases[numSB].pos[0] = q1;
                world.starbases[numSB].pos[1] = q2;
                world.starbases[numSB].pos[2] = s1;
                world.starbases[numSB].pos[3] = s2;
                world.starbases[numSB].energy = PLAYER_ENERGY;
                world.quadrant[q1][q2].numStarbases++;
                numSB++;
                break;
            }
        }

    }

    // Generate klingons in the world
    for (int i=0; i<NUM_KL; i++) {
        while (1) {
            int q1 = randRange(0, 8);
            int q2 = randRange(0, 8);
            int s1 = randRange(0, 8);
            int s2 = randRange(0, 8);

            // Only Spawn 1 Klingon in The First Quadrant
            if (i < 1) {
                 q1 = 4;
                 q2 = 5;
                while ((s1 == 3 && s2 == 0)) { // Location can't overlap with player's
                    s1 = randRange(0, 8);
                    s2 = randRange(0, 8);
                }
            } else {
                while (q1 == 4 || q2 == 5) {
                    q1 = randRange(0, 8);
                    q2 = randRange(0, 8);
                }
            }
            // Make sure that there isn't 3 klingons in the same quadrant already & there isn't any starbases
            if (world.quadrant[q1][q2].sector[s1][s2] == empty && world.quadrant[q1][q2].numKlingons <= 3 && !world.quadrant[q1][q2].numStarbases){
                world.quadrant[q1][q2].sector[s1][s2] = 'K';
                world.klingons[numK].pos[0] = q1;
                world.klingons[numK].pos[1] = q2;
                world.klingons[numK].pos[2] = s1;
                world.klingons[numK].pos[3] = s2;
                world.klingons[numK].energy = randRange(100, 301);
                world.quadrant[q1][q2].numKlingons++;
                numK++;
                break;
            }
        }

    }

    // Generate stars in the world
    for (int i=0; i<NUM_STARS; i++){
        while (1) {
            int q1 = randRange(0, 8);
            int q2 = randRange(0, 8);
            int s1 = randRange(0, 8);
            int s2 = randRange(0, 8);
            while ((s1 == 3 && s2 == 0) && (q1 == 4 && q2 == 5)) { // Location can't overlap with player's
                s1 = randRange(0, 8);
                s2 = randRange(0, 8);
            }
            if (world.quadrant[q1][q2].sector[s1][s2] == empty){
                world.quadrant[q1][q2].sector[s1][s2] = '*';
                world.stars[numStars].pos[0] = q1;
                world.stars[numStars].pos[1] = q2;
                world.stars[numStars].pos[2] = s1;
                world.stars[numStars].pos[3] = s2;
                world.stars[numStars].energy = 0; // Stars don't have energy
                world.quadrant[q1][q2].numStars++;
                numStars++;
                break;
            }
        }
    }

    world.numKlingons = numK;
    world.numStarbases = numSB;
    cmdSRS(&world);

    return world;
}


void updateCond(World *world) {
    world->player.condition = green;
    int q1 = world->player.pos[0], q2 = world->player.pos[1];
    if (world->quadrant[q1][q2].numStarbases > 0) world->player.condition = docked;
    else if (world->quadrant[q1][q2].numKlingons > 0) world->player.condition = red;
    else if (world->player.energy+world->player.shield < (PLAYER_ENERGY*0.1)) world->player.condition = yellow;
}

// Gets the n'th nearest chosen entity (klingons, starbases, or stars)
Entity *getNearbyEntity(World *world, int n, char type) {

    int q1 = world->player.pos[0], q2 = world->player.pos[1];
    int numEntity;
    int size;

    // Get how many (chosen) entities in the quadrant
    if (type == 's') {
        numEntity = world->quadrant[q1][q2].numStars;
        size = NUM_STARS;
    } else if (type == 'b') {
         numEntity = world->quadrant[q1][q2].numStarbases;
        size = NUM_SB;
    } else {
        numEntity = world->quadrant[q1][q2].numKlingons;
        size = NUM_KL;
    }

    if (numEntity <= 0) return NULL;

    Entity *ePtr[numEntity];
    double distance[numEntity];

    int i = 0;
    if (type == 's') { // If stars
        for (int j=0; j<size; j++) {
            if (world->stars[j].pos[0] == q1 && world->stars[j].pos[1] == q2) {
                ePtr[i] = &world->stars[j];
                distance[i] = getDistance(&(world->player), ePtr[i]->pos);
                i++;
            }
        }

    } else if (type == 'b') { // If starbases
        for (int j=0; j<size; j++) {
            if (world->starbases[j].pos[0] == q1 && world->starbases[j].pos[1] == q2) {
                ePtr[i] = &world->starbases[j];
                distance[i] = getDistance(&(world->player), ePtr[i]->pos);
                i++;
            }
        }

    } else { // If Klingons
        for (int j=0; j<size; j++) {
            if (world->klingons[j].pos[0] == q1 && world->klingons[j].pos[1] == q2) {
                ePtr[i] = &world->klingons[j];
                distance[i] = getDistance(&(world->player), ePtr[i]->pos);
                i++;
            }
        }

    }

    // Sort entities based on distance
    sortEntities(ePtr, distance, numEntity);

    return ePtr[n];
}



void getCmd(World *world) {
    printf("COMMAND: ");
    char input[STR_SIZE];
    fgets(input, STR_SIZE, stdin);
    for (int i=0; i<3; i++) input[i] = (char) toupper(input[i]); // Turn first 3 chars to upper

    if (!strncmp(input, "NAV", 3)) cmdNAV(world);
    else if (!strncmp(input, "SRS", 3)) cmdSRS(world);
    else if (!strncmp(input, "LRS", 3)) cmdLRS(world);
    else if (!strncmp(input, "PHA", 3)) cmdPHA(world);
    else if (!strncmp(input, "TOR", 3)) cmdTOR(world);
    else if (!strncmp(input, "SHE", 3)) cmdSHE(world);
    else if (!strncmp(input, "DAM", 3)) cmdDAM(world);
    else if (!strncmp(input, "COM", 3)) cmdCOM(world);
    else if (!strncmp(input, "XXX", 3)) cmdXXX(world);
    else if (!strncmp(input, "INS", 3)) printInstructions();
    else {
        printf("ENTER ONE OF THE FOLLOWING : \n"
               "  NAV  (TO SET COURSE)\n"
               "  SRS  (FOR SHORT RANGE SENSOR SCAN)\n"
               "  LRS  (FOR LONG RANGE SENSOR SCAN)\n"
               "  PHA  (TO FIRE PHASERS)\n"
               "  TOR  (TO FIRE PHOTON TORPEDOES)\n"
               "  SHE  (TO RAISE OR LOWER SHIELDS)\n"
               "  DAM  (FOR DAMAGE CONTROL REPORTS)\n"
               "  COM  (TO CALL ON LIBRARY-COMPUTER)\n"
               "  XXX  (TO RESIGN YOUR COMMAND)\n"
               "  INS  (TO PRINT GAME INSTRUCTIONS)\n\n");
    }

}


void cmdNAV(World *world){
    double courseInput;
    printf("COURSE (1-9) ");
    scanf("%lf", &courseInput);
    getchar();

    if ((courseInput < 1.0) || (courseInput > 9.0)) {
        printf("LT. SULU REPORTS, 'INCORRECT COURSE DATA, SIR!'\n");
    } else {

        double maxWarp = 8.0, warpInput;
        if (world->player.damage[warp] < 0) {
            maxWarp = 0.2;
        }
        printf("WARP FACTOR (0-%.1f) ", maxWarp);
        scanf("%lf", &warpInput);
        getchar();
        double N = floor(warpInput * 8 + 0.5);

        if ((warpInput < 0) || (warpInput > 8)) {
            printf("CHIEF ENGINEER SCOTT REPORTS, 'THE ENGINES WON'T TAKE WARP %.1lf!'\n", warpInput);

        } else if (warpInput > maxWarp) {
            printf("WARP ENGINES ARE DAMAGED.  MAXIMUM SPEED = WARP %.1lf\n", maxWarp);

        } else if (N > (world->player.energy+world->player.shield)) {
            printf("ENGINEERING REPORTS 'INSUFFICIENT ENERGY AVAILABLE\n"
                   "                     FOR MANEUVERING AT WARP %.1lf!'\n", warpInput);
            return;

        } else {
            int q1 = world->player.pos[0], q2 = world->player.pos[1];

            // Let the klingons nearby shoot
            if (world->quadrant[q1][q2].numKlingons > 0) {
                klingonShooting(world);
            }

            int courseMovement[10][3] = {
                    {0,0,0},    // NOTHING
                    {0,0,1},    // 1
                    {0,-1,1},   // 2
                    {0,-1,0},   // 3
                    {0,-1,-1},  // 4
                    {0,0,-1},   // 5
                    {0,1,-1},   // 6
                    {0,1,0},    // 7
                    {0,1,1},    // 8
                    {0,0,1},    // 9
            };

            double rowStep,  colStep; // torpedo course
            int s1 = world->player.pos[2], s2 = world->player.pos[3];
            double posRowFl, posColFl;
            int posRow, posCol; // used for torpedo track

            world->quadrant[q1][q2].sector[s1][s2] = ' ';// remove player from current pos

            double quadFl;
            double sectFl = modf(warpInput, &quadFl);
            int numQuad = floor(quadFl);
            int numSect = (int) floor(sectFl * 10.00) + 1;
            int countQuad = 0, countSect = 0;
            int count = ((numQuad * 8) + numSect) - 1;

            int courseInt = (int)floor(courseInput);
            rowStep = courseMovement[courseInt][1] + (courseMovement[courseInt + 1][1] - courseMovement[courseInt][1]) * (courseInput - courseInt);
            colStep = courseMovement[courseInt][2] + (courseMovement[courseInt + 1][2] - courseMovement[courseInt][2]) * (courseInput - courseInt);


            // SHIP MOVEMENT:
            bool stop = false;
            while (countSect < count && !stop) {
                ////////////////
                posRowFl = s1 + 1;
                posColFl = s2 + 1;

                // GET ALL STARS NEARBY
                int stars = world->quadrant[q1][q2].numStars;
                const Star *sNearby[stars];
                for (int k=0; k<stars; k++) {
                    sNearby[k] = getNearbyEntity(world, k, 's');
                }


                // For every possible move inside the quadrant
                while (countSect < count) {
                    countSect++;
                    posRowFl = (posRowFl + rowStep);
                    posColFl = (posColFl + colStep);
                    posRow = (int) floor(posRowFl + 0.5);
                    posCol = (int) floor(posColFl + 0.5);

                    // Ship went outside quadrant
                    if ((posRow < 1) || (posRow > 8) || (posCol < 1) || (posCol > 8)){
                        countQuad++;
                        if (posRow < 1) {
                            posRow = 8;
                            s1 = 7;
                            q1 -= 1;
                        } else if (posRow > 8) {
                            posRow = 1;
                            s1 = 0;
                            q1 += 1;
                        } else if (posCol < 1) {
                            posCol = 8;
                            s2 = 7;
                            q2 -= 1;
                        } else if (posCol > 8) {
                            posCol = 1;
                            s2 = 0;
                            q2 += 1;
                        }
                        break;
                    }

                    // Check for stars collision
                    for (int k=0; k<stars-1; k++) {
                        if (posRow == sNearby[k]->pos[2]+1 && posCol == sNearby[k]->pos[3]+1) {
                            // Torpedo hit star
                            printf("WARP ENGINES SHUT DOWN AT SECTOR %i,%i DUE TO BAD NAVIGATION.\n", posRow, posCol);
                            stop = true;
                            break;
                        }
                    }
                    if (stop) break;

                }
                ///////////////
            }


            Player *pl = &world->player;
            pl->pos[0] = q1;
            pl->pos[1] = q2;
            pl->pos[2] = posRow-1;
            pl->pos[3] = posCol-1;

            //Advance days & subtract energy
            world->daysRem -= 1;
            world->date += 1;
            world->player.energy -= floor(N);

            if (!world->gameOver) {
                if (randRange(0, 5) == 0) {
                    for (int i=0; i<8; i++){
                        printf("REPAIRING IN PROGRESS\n");
                        world->player.damage[i] = 0;
                    }
                }

                cmdSRS(world);
            }


            ////////
        }

    }

}


void printStat(World *world, int n){
    printf("       ");
    switch (n) {
        case 0:
            printf("STARDATE:            %i\n", world->date);
            break;
        case 1: {
            char cond[7] = "GREEN";
            if (world->player.condition == yellow) strcpy(cond, "*YELLOW*");
            else if (world->player.condition == red) strcpy(cond, "*RED*");
            else if (world->player.condition == docked) strcpy(cond, "DOCKED");
            printf("CONDITION:           %s\n", cond);
            break;
        }
        case 2:
            printf("QUADRANT:            %i,%i\n", world->player.pos[0]+1, world->player.pos[1]+1);
            break;
        case 3:
            printf("SECTOR:              %i,%i\n", world->player.pos[2]+1, world->player.pos[3]+1);
            break;
        case 4:
            printf("PHOTON TORPEDOES:    %i\n", world->player.photon);
            break;
        case 5:
            printf("TOTAL ENERGY:        %i\n", world->player.energy+world->player.shield);
            break;
        case 6:
            printf("SHIELDS:             %i\n", world->player.shield);
            break;
        case 7:
            printf("KLINGONS REMAINING:  %i [%i]\n", world->numKlingons, world->quadrant[world->player.pos[0]][world->player.pos[1]].numKlingons);
            break;
        default:
            break;
    }
}


void cmdSRS(World *world){
    // Check if SRS is operable
    if (world->player.damage[srs] < 0) {
        printf("*** SHORT RANGE SENSORS ARE OUT ***\n");
        return;
    }
    int q1 = world->player.pos[0], q2 = world->player.pos[1];
    int s1P = world->player.pos[2], s2P = world->player.pos[3];
    world->quadrant[q1][q2].sector[s1P][s2P] = 'E';

    // Check shield & nearby klingons
    printf("\n");
    if (world->quadrant[q1][q2].numKlingons > 0) printf("COMBAT AREA      CONDITION RED\n");
    if (world->player.shield <= 200) printf("   SHIELDS DANGEROUSLY LOW\n");
    updateCond(world);


//    printf("QUADRANT: %i,%i\n", world->player.pos[0]+1, world->player.pos[1]+1);
    printf("------------------------------------\n");
    for (int s1=0; s1<QS_SIZE; s1++) {

        // Buffer spaces at the start of each sector row
        // (and + or < instead of space if the column after contains the player or klingons)
        for (int i=0; i<3; i++) {
            if (world->quadrant[q1][q2].sector[s1][0] == 'K' && i == 2) printf("+");
            else if (world->quadrant[q1][q2].sector[s1][0] == 'E' && i == 2) printf("<");
            else if (world->quadrant[q1][q2].sector[s1][0] == 'B' && i == 2) printf(">");
            else printf(" ");
        }

        // Go through sector columns
        for (int s2=0; s2<QS_SIZE; s2++){

            // Print sector column
            printf("%c",world->quadrant[q1][q2].sector[s1][s2]);

            // Buffer spaces after each sector column
            // (and + or <> instead of space if the column contains the player or klingons)
            for (int i=0; i<3; i++) {
                if ((world->quadrant[q1][q2].sector[s1][s2] == 'K' && i == 0)
                    || (world->quadrant[q1][q2].sector[s1][s2+1] == 'K' && i == 2 && s2<QS_SIZE-1)) printf("+");
                else if ((world->quadrant[q1][q2].sector[s1][s2] == 'E' && i == 0)
                        || (world->quadrant[q1][q2].sector[s1][s2+1] == 'B' && i == 2 && s2<QS_SIZE-1)) printf(">");
                else if ((world->quadrant[q1][q2].sector[s1][s2+1] == 'E' && i == 2 && s2<QS_SIZE-1)
                        || (world->quadrant[q1][q2].sector[s1][s2] == 'B' && i == 0)) printf("<");
                else printf(" ");
            }

        }
        printStat(world, s1);

    }
    printf("------------------------------------\n");

    world->quadrant[q1][q2].scanned = true;
    world->quadrantArch[q1][q2] = world->quadrant[q1][q2];

}


void cmdLRS(World *world){
    if (world->player.damage[lrs] < 0) {
        printf("LONG RANGE SENSORS ARE INOPERABLE\n");
        return;
    }

    int q1 = world->player.pos[0], q2 = world->player.pos[1];
    printf("LONG RANGE SCAN FOR QUADRANT %d,%d\n", q1+1, q2+1);
    printf("    -------------------\n");

    // Rows
    for (int i=q1-1; i<=q1+1; i++) {
        printf(" :  ");
        // Columns
        for (int j=q2-1; j<=q2+1; j++) {
            // Print nearby quadrants
            printf("%d%d%d  :  ", world->quadrant[i][j].numKlingons, world->quadrant[i][j].numStarbases, world->quadrant[i][j].numStars);
            // Set nearby quadrants as scanned and save their info in the archive
            world->quadrant[i][j].scanned = true;
            world->quadrantArch[i][j] = world->quadrant[i][j];
        }
        printf("\n    -------------------\n");
    }


}


void cmdPHA(World *world){
    int q1 = world->player.pos[0], q2 = world->player.pos[1];

    if (world->player.damage[pha] < 0) {
        printf("PHASERS INOPERATIVE\n");

    } else if (world->quadrant[q1][q2].numKlingons <= 0) {
        printf("SCIENCE OFFICER SPOCK REPORTS  'SENSORS SHOW NO ENEMY SHIPS\n"
               "                              IN THIS QUADRANT'");

    } else {

        if (world->player.damage[libComp] < 0) {
            printf("COMPUTER FAILURE HAMPERS ACCURACY\n");
        }

        printf("PHASERS LOCKED ON TARGET;  ENERGY AVAILABLE = %d UNITS\nNUMBER OF UNITS TO FIRE? ",
               world->player.energy);

        int input = 0;
        bool inputAccepted = false;
        while (!inputAccepted) {
            if (!scanf("%i", &input)) {
                while (fgetc(stdin) != '\n');  // Trim extra input in buffer
                printf("?REENTER\n NUMBER OF UNITS TO FIRE? ");
                continue;
            }
            while(fgetc(stdin)!='\n');  // Trim extra input in buffer
            if (input > world->player.energy) {
                printf("ENERGY AVAILABLE = %i\n NUMBER OF UNITS TO FIRE? ", world->player.energy);

            }
            else if (input > 0) inputAccepted = true;
            else return;
        }
        // Handling Attacking
        world->player.energy -= input; // Remove used energy
        int dmgPerK = input / world->quadrant[q1][q2].numKlingons;

        // Generate array of type Klingon that points to all nearby klingons
        int klingons = world->quadrant[q1][q2].numKlingons;
        Klingon *kNearby[klingons];
        for (int k=0; k<klingons; k++) {
            kNearby[k] = getNearbyEntity(world, k, 'k');
        }

        for (int i=0; i<klingons; i++) {

            Klingon *target = kNearby[i];
            int dmgApplied = (int) ((dmgPerK / getDistance(&(world->player), target->pos)) * (drand()+2));

            if (dmgApplied > (0.15 * target->energy)) {
                printf("%i UNIT HIT ON KLINGON AT SECTOR %i,%i\n", dmgApplied, target->pos[2]+1, target->pos[3]+1);
                target->energy -= dmgApplied;

                if (target->energy <= 0) { // Klingon destroyed
                    printf("*** KLINGON DESTROYED ***\n");
                    world->numKlingons--;
                    world->quadrant[q1][q2].numKlingons--;
                    world->quadrant[q1][q2].sector[target->pos[2]][target->pos[3]] = ' ';
                    target->energy = -1;
                    target->pos[0] = -1; // So it doesn't count when performing other commands

                } else {
                    printf("    (SENSORS SHOW %i UNITS REMAINING)\n", target->energy);
                }

            } else {
                printf("SENSORS SHOW NO DAMAGE TO ENEMY AT %d,%d\n", target->pos[2]+1, target->pos[3]+1);

            }


        }
        klingonShooting(world);

    }
}


void klingonShooting(World *world) {
    int q1 = world->player.pos[0], q2 = world->player.pos[1];

    if (world->quadrant[q1][q2].numStarbases > 0) {
        printf("STARBASE SHIELDS PROTECT THE ENTERPRISE\n");
        return;
    }

    // For every klingon nearby
    for (int i=0; i<world->quadrant[q1][q2].numKlingons; i++) {
        Klingon *shooter = getNearbyEntity(world, i, 'k');
        int dmg = (int) ((shooter->energy / getDistance(&(world->player), shooter->pos)) * (drand()+2));
        shooter->energy /= (int) (drand()+3);
        world->player.shield -= dmg; // Deduct damage taken
        printf("%i UNIT HIT ON ENTERPRISE FROM SECTOR %i,%i\n", dmg, shooter->pos[2]+1, shooter->pos[3]+1);

        if (world->player.shield < 0) {
            printf("\n\nTHE ENTERPRISE HAS BEEN DESTROYED. THE FEDERATION WILL BE CONQUERED\n");
            world->gameOver = true;
            return;
        }
        printf("      <SHIELDS DOWN TO %i UNITS>\n", world->player.shield);

        //IF RND(1)>.6ORH/S<=.02
        if (drand() > 0.6 && (dmg/world->player.shield) <= 0.2) {
            int damageIndex = floor(drand() * 8);
            world->player.damage[damageIndex] -= -(dmg / world->player.shield - 0.5*drand());
            printf("DAMAGE CONTROL REPORTS %s DAMAGED BY THE HIT'\n\n", world->statNames[damageIndex]);

        }

    }

}


void cmdSHE(World *world){
    if (world->player.damage[she] < 0) {
        printf("SHIELD CONTROL INOPERABLE\n\n");
        return;
    }

    printf("ENERGY AVAILABLE = %i\n     NUMBER OF UNITS TO SHIELDS? ", world->player.energy+world->player.shield);
    int input = 0;
    scanf("%i", &input);
    while(fgetc(stdin)!='\n');  // Trim extra input in buffer

    if (input == world->player.shield || input < 1) {
        printf("\n<SHIELDS UNCHANGED>\n");

    } else if (input <= world->player.energy+world->player.shield) {
        world->player.energy = (world->player.energy+world->player.shield) - input;
        world->player.shield = input;
        printf("DEFLECTOR CONTROL ROOM REPORT : \n"
               "\'SHIELDS NOW AT %i UNITS PER YOUR COMMAND\'\n\n", world->player.shield);
    } else {
        printf("SHIELD CONTROL REPORTS : \n"
               " \'THIS IS NOT THE FEDERATION TREASURY.\'\n\n");
    }

}


void cmdDAM(World *world){
    if (world->player.damage[libComp] < 0) {
        printf("DAMAGE CONTROL REPORT NOT AVAILABLE\n");
        return;
    }
    printf("DEVICE                    STATE OF REPAIR\n");
    printf("WARP ENGINES:             %lf\n", world->player.damage[warp]);
    printf("SHORT RANGE SENSORS:      %lf\n", world->player.damage[srs]);
    printf("LONG RANGE SENSORS:       %lf\n", world->player.damage[lrs]);
    printf("PHASER CONTROL:           %lf\n", world->player.damage[pha]);
    printf("PHOTON TUBES:             %lf\n", world->player.damage[tor]);
    printf("DAMAGE CONTROL:           %lf\n", world->player.damage[damageControl]);
    printf("SHIELD CONTROL:           %lf\n", world->player.damage[she]);
    printf("LIBRARY-COMPUTER:         %lf\n", world->player.damage[libComp]);
    printf("\n");

}

void cmdTOR(World *world){
    int q1 = world->player.pos[0], q2 = world->player.pos[1];

    double courseInput; //user input for torpedo course
    double rowStep,  colStep; // torpedo course
    int s1 = world->player.pos[2], s2 = world->player.pos[3];
    double posRowFl, posColFl;
    int posRow, posCol; // used for torpedo track

    if (world->player.photon <= 0){
        printf("ALL PHOTON TORPEDOES EXPENDED\n");
        return;
    } else if (world->player.damage[tor] < 0) {
        printf("PHOTON TUBES ARE NOT OPERATIONAL\n");
        return;
    }

    int courseMovement[10][3] = {
            {0,0,0},    // NOTHING
            {0,0,1},    // 1
            {0,-1,1},   // 2
            {0,-1,0},   // 3
            {0,-1,-1},  // 4
            {0,0,-1},   // 5
            {0,1,-1},   // 6
            {0,1,0},    // 7
            {0,1,1},    // 8
            {0,0,1},    // 9
    };

    printf("PHOTON TORPEDO COURSE (1-9) ");
    scanf("%lf", &courseInput);
    getchar();
    if (floor(courseInput) == 9) {
        courseInput -= 8.00;
    }

    if ((courseInput < 1) || (courseInput > 9)) {
        printf("ENSIGN CHEKOV REPORTS,  'INCORRECT COURSE DATA, SIR!'\n");
        return;
    // Course accepted:
    } else if ((courseInput >= 1) && (courseInput <= 9)) {
        // Deduct energy & torpedos
        world->player.energy -= 2;
        world->player.photon -= 1;

        int courseInt = (int)floor(courseInput);
        rowStep = courseMovement[courseInt][1] + (courseMovement[courseInt + 1][1] - courseMovement[courseInt][1]) * (courseInput - courseInt);
        colStep = courseMovement[courseInt][2] + (courseMovement[courseInt + 1][2] - courseMovement[courseInt][2]) * (courseInput - courseInt);

        posRowFl = s1 + 1;
        posColFl = s2 + 1;

        printf("TORPEDO TRACK : \n");

        // GET ALL KLINGONS NEARBY
        int klingons = world->quadrant[q1][q2].numKlingons;
        Klingon *kNearby[klingons];
        for (int k=0; k<klingons; k++) {
            kNearby[k] = getNearbyEntity(world, k, 'k');
        }

        // GET ALL STARBASES NEARBY
        int starbases = world->quadrant[q1][q2].numStarbases;
        Starbase *bNearby[starbases];
        for (int k=0; k<starbases; k++) {
            bNearby[k] = getNearbyEntity(world, k, 'b');
        }

        // GET ALL STARS NEARBY
        int stars = world->quadrant[q1][q2].numStars;
        const Star *sNearby[stars];
        for (int k=0; k<stars; k++) {
            sNearby[k] = getNearbyEntity(world, k, 's');
        }

        bool stop = false;
        // For every possible move inside the quadrant
        for (int i=0; i<9; i++) {
            posRowFl = (posRowFl + rowStep);
            posColFl = (posColFl + colStep);
            posRow = (int) floor(posRowFl + 0.5);
            posCol = (int) floor(posColFl + 0.5);

            // Torpedo went outside quadrant
            if ((posRow < 1) || (posRow > 8) || (posCol < 1) || (posCol > 8)){
                printf("TORPEDO MISSED\n");
                break;
            }

            printf("               %i,%i\n", posRow, posCol);

            // Check for klingons collision
            for (int k=0; k<klingons; k++) {
                if (posRow == kNearby[k]->pos[2]+1 && posCol == kNearby[k]->pos[3]+1) {
                    // Torpedo hit klingon
                    printf("*** KLINGON DESTROYED ***\n");
                    world->numKlingons--;
                    world->quadrant[q1][q2].numKlingons--;
                    world->quadrant[q1][q2].sector[kNearby[k]->pos[2]][kNearby[k]->pos[3]] = ' ';
                    kNearby[k]->energy = -1;
                    kNearby[k]->pos[0] = -1; // So it doesn't count when performing other commands
                    stop = true;
                    break;
                }
            }
            if (stop) break;

            // Check for starbases collision
            for (int k=0; k<starbases; k++) {
                if (posRow == bNearby[k]->pos[2]+1 && posCol == bNearby[k]->pos[3]+1) {
                    // TODO: Torpedo hit starbase
                    printf("*** STARBASE DESTROYED ***\n");
                    world->numStarbases--;
                    world->quadrant[q1][q2].numStarbases--;
                    world->quadrant[q1][q2].sector[bNearby[k]->pos[2]][bNearby[k]->pos[3]] = ' ';
                    bNearby[k]->energy = -1;
                    bNearby[k]->pos[0] = -1; // So it doesn't count when performing other commands
                    stop = true;
                    if (world->numStarbases == 2) {
                        printf("STARFLEET COMMAND REVIEWING YOUR RECORD TO CONSIDER\nCOURT MARTIAL!\n");
                        break;
                    } else {
                        printf("THAT DOES IT, CAPTAIN!! YOU ARE HEREBY RELIEVED OF COMMAND\n");
                        printf("AND SENTENCED TO 99 STARDATES AT HARD LABOR ON CYGNUS 12!!\n\n");
                        world->gameOver = true;
                        return;
                    }
                }
            }
            if (stop) break;

            // Check for stars collision
            for (int k=0; k<stars-1; k++) {
                if (posRow == sNearby[k]->pos[2]+1 && posCol == sNearby[k]->pos[3]+1) {
                    // Torpedo hit star
                    printf("STAR AT %i,%i ABSORBED TORPEDO ENERGY.\n", posRow, posCol);
                    stop = true;
                    break;
                }
            }
            if (stop) break;


        }
        // Let the klingons nearby shoot
        if (world->quadrant[q1][q2].numKlingons > 0) {
            klingonShooting(world);
        }

    } else { //invalid
        printf("?REENTER\n?");
        // repeat prompt
    }

}


void cmdCOM(World *world){
    int q1 = world->player.pos[0], q2 = world->player.pos[1];

    while (true) {

        int i = 0;
        int numCOM = 6;
        printf("COMPUTER ACTIVE AND AWAITING COMMAND: ");
        scanf("%d", &numCOM);  // Get COM#
        while (fgetc(stdin) != '\n');

        if (numCOM == 0) {

            printf("\n");
            printf("        COMPUTER RECORD OF GALAXY FOR QUADRANT %d,%d\n", world->player.pos[0]+1, world->player.pos[1]+1);
            printf("       1     2     3     4     5     6     7     8\n"); // Column #s
            printf("     ----- ----- ----- ----- ----- ----- ----- -----\n");

            // Print each row
            for (int r=0; r<QS_SIZE; r++) {
                printf("%d     ", r+1); // Row #
                // Each column
                for (int c=0; c<QS_SIZE; c++) {
                    Quadrant *quad = &world->quadrantArch[r][c];
                    if (world->quadrantArch[r][c].scanned) printf("%d%d%d   ", quad->numKlingons, quad->numStarbases, quad->numStars);
                    else printf("***   ");
                }
                printf("\n     ----- ----- ----- ----- ----- ----- ----- -----\n");
            }
            break;



        } else if (numCOM == 1) {

            printf("\n  STATUS REPORT :\n");
            printf("KLINGONS LEFT : %d\n", world->numKlingons);
            printf("MISSION MUST BE COMPLETED IN %d STARDATES\n", world->daysRem);
            printf("THE FEDERATION IS MAINTAINING %d STARBASES IN THE GALAXY\n\n", world->numStarbases);
            printf("DEVICE                    STATE OF REPAIR\n");
            printf("WARP ENGINES              %lf\n", world->player.damage[warp]);
            printf("SHORT RANGE SENSORS       %lf\n", world->player.damage[srs]);
            printf("LONG RANGE SENSORS        %lf\n", world->player.damage[lrs]);
            printf("PHASER CONTROL            %lf\n", world->player.damage[pha]);
            printf("PHOTON TUBES              %lf\n", world->player.damage[tor]);
            printf("DAMAGE CONTROL            %lf\n", world->player.damage[damageControl]);
            printf("SHIELD CONTROL            %lf\n", world->player.damage[she]);
            printf("LIBRARY-COMPUTER          %lf\n", world->player.damage[libComp]);
            printf("\n");
            break;


        } else if (numCOM == 2) {

            if (world->quadrant[q1][q2].numKlingons <= 0) { // no klingon
                printf("SCIENCE OFFICER SPOCK REPORTS  \'SENSORS SHOW NO ENEMY SHIPS\n                                IN THIS QUADRANT\'\n");
            }
            else {
                printf("\nFROM ENTERPRISE TO KLINGON BATTLE CRUISER(S)\n");
                for (i = 0; i < world->quadrant[q1][q2].numKlingons; ++i){ //variable for # of klingon in the sector.
                    Klingon *target = getNearbyEntity(world, i, 'k');
                    double C1 =  world->player.pos[2]+1;
                    double A = world->player.pos[3]+1;
                    double W1 = target->pos[2]+1; // set W1 equal to nearest klingon
                    double X = target->pos[3]+1; // set W1 equal to nearest klingon
                    double DIR = 0;
                    double DIST = 0;
                    X = X-A;
                    A = C1-W1;
                    if (X < 0) {
                        if (A > 0) {
                            C1 = 3;
                            if (fabs(A) >= fabs(X)){
                                DIR = C1+(fabs(X)/fabs(A));
                            }
                            else {
                                DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                            }
                        }
                        else if (X != 0){
                            C1 = 5;
                            if (fabs(A)<=fabs(X)) {
                                DIR = C1+(fabs(A)/fabs(X));
                            }
                            else {
                                DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                            }
                        }
                    }
                    else if (A < 0) {
                        C1 = 7;
                        if (fabs(A) >= fabs(X)){
                            DIR = C1+(fabs(X)/fabs(A));
                        }
                        else {
                            DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                        }
                    }
                    else if (X > 0) {
                        C1 = 1;
                        if (fabs(A)<=fabs(X)) {
                            DIR = C1+(fabs(A)/fabs(X));
                        }
                        else {
                            DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                        }
                    }
                    else if (A == 0) {
                        C1 = 5;
                        if (fabs(A)<=fabs(X)) {
                            DIR = C1+(fabs(A)/fabs(X));
                        }
                        else {
                            DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                        }
                    }
                    DIST = sqrt( pow(X, 2)+pow(A, 2));
                    printf("DIRECTION = %lf\n", DIR);
                    printf("DISTANCE = %lf\n", DIST);
                }
            }
            break;


        }  else if (numCOM == 3) {

            if (world->quadrant[q1][q2].numStarbases == 0){ // no base
                printf("\nMR. SPOCK REPORTS,  \'SENSORS SHOW NO STARBASES IN THIS QUADRANT.\'\n");
            }
            else {
                printf("\nFROM ENTERPRISE TO KLINGON STARBASE");
                for (i = 0; i < world->quadrant[q1][q2].numStarbases; ++i){
                    Starbase *target = getNearbyEntity(world, i, 'b');
                    double C1 =  world->player.pos[2]+1;
                    double A = world->player.pos[3]+1;
                    double W1 = target->pos[2]+1; // set W1 equal to nearest starbase
                    double X = target->pos[3]+1; // set X equal to nearest starbase
                    double DIR = 0;
                    double DIST = 0;
                    X = X-A;
                    A = C1-W1;
                    if (X < 0) {
                        if (A > 0) {
                            C1 = 3;
                            if (fabs(A) >= fabs(X)){
                                DIR = C1+(fabs(X)/fabs(A));
                            }
                            else {
                                DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                            }
                        }
                        else if (X != 0){
                            C1 = 5;
                            if (fabs(A)<=fabs(X)) {
                                DIR = C1+(fabs(A)/fabs(X));
                            }
                            else {
                                DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                            }
                        }
                    }
                    else if (A < 0) {
                        C1 = 7;
                        if (fabs(A) >= fabs(X)){
                            DIR = C1+(fabs(X)/fabs(A));
                        }
                        else {
                            DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                        }
                    }
                    else if (X > 0) {
                        C1 = 1;
                        if (fabs(A)<=fabs(X)) {
                            DIR = C1+(fabs(A)/fabs(X));
                        }
                        else {
                            DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                        }
                    }
                    else if (A == 0) {
                        C1 = 5;
                        if (fabs(A)<=fabs(X)) {
                            DIR = C1+(fabs(A)/fabs(X));
                        }
                        else {
                            DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                        }
                    }
                    DIST = sqrt( pow(X, 2)+pow(A, 2));
                    printf("DIRECTION = %lf\n", DIR);
                    printf("DISTANCE = %lf\n", DIST);
                }
            }
            break;


        } else if (numCOM == 4) {

            double C1;
            double A;
            double W1;
            double X;
            printf("DIRECTION/DISTANCE CALCULATOR :\n");
            printf("YOU ARE AT QUADRANT %d,%d SECTOR %d,%d\n",world->player.pos[0]+1, world->player.pos[1]+1, world->player.pos[2]+1, world->player.pos[3]+1);
            printf("PLEASE ENTER\n");
            printf("  INITIAL COORDINATES (X,Y) ");
            scanf("%lf,%lf", &C1, &A);
            getchar();
            printf("  FINAL COORDINATES (X,Y) ");
            scanf("%lf,%lf", &W1, &X);
            getchar();
            double DIR = 0;
            double DIST = 0;
            X = X-A;
            A = C1-W1;
            if (X < 0) {
                if (A > 0) {
                    C1 = 3;
                    if (fabs(A) >= fabs(X)){
                        DIR = C1+(fabs(X)/fabs(A));
                    }
                    else {
                        DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                    }
                }
                else if (X != 0){
                    C1 = 5;
                    if (fabs(A)<=fabs(X)) {
                        DIR = C1+(fabs(A)/fabs(X));
                    }
                    else {
                        DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                    }
                }
            }
            else if (A < 0) {
                C1 = 7;
                if (fabs(A) >= fabs(X)){
                    DIR = C1+(fabs(X)/fabs(A));
                }
                else {
                    DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                }
            }
            else if (X > 0) {
                C1 = 1;
                if (fabs(A)<=fabs(X)) {
                    DIR = C1+(fabs(A)/fabs(X));
                }
                else {
                    DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                }
            }
            else if (A == 0) {
                C1 = 5;
                if (fabs(A)<=fabs(X)) {
                    DIR = C1+(fabs(A)/fabs(X));
                }
                else {
                    DIR = C1+(((fabs(X)-fabs(A))+fabs(X))/fabs(X));
                }
            }
            DIST = sqrt( pow(X, 2)+pow(A, 2));
            printf("DIRECTION = %lf\n", DIR);
            printf("DISTANCE = %lf\n", DIST);
            break;


        } else if (numCOM == 5){

            printf("\n\t\t       THE GALAXY\n");
            printf("      1     2     3     4     5     6     7     8\n");
            printf("    ----- ----- ----- ----- ----- ----- ----- -----\n");
            printf("1        ANTARES\t          SIRIUS\n");
            printf("    ----- ----- ----- ----- ----- ----- ----- -----\n");
            printf("2         RIGEL\t\t          DENEB\n");
            printf("    ----- ----- ----- ----- ----- ----- ----- -----\n");
            printf("3        PROCYON\t         CAPELLA\n");
            printf("    ----- ----- ----- ----- ----- ----- ----- -----\n");
            printf("4          VEGA\t\t        BETELGEUSE\n");
            printf("    ----- ----- ----- ----- ----- ----- ----- -----\n");
            printf("5        CANOPUS\t        ALDEBRAN\n");
            printf("    ----- ----- ----- ----- ----- ----- ----- -----\n");
            printf("6         ALTAIR\t         REGULUS\n");
            printf("    ----- ----- ----- ----- ----- ----- ----- -----\n");
            printf("7      SAGITTARIUS\t         ARCTURUS\n");
            printf("    ----- ----- ----- ----- ----- ----- ----- -----\n");
            printf("8         POLLUX\t          SPICA\n");
            printf("    ----- ----- ----- ----- ----- ----- ----- -----\n");


        } else if ((numCOM > 5) || (numCOM < 0)) {

            printf("FUNCTIONS AVAILABLE FROM LIBRARY-COMPUTER :\n");
            printf("   0 = CUMULATIVE GALACTIC RECORD\n");
            printf("   1 = STATUS REPORT\n");
            printf("   2 = PHOTON TORPEDO DATA\n");
            printf("   3 = STARBASE NAV DATA\n");
            printf("   4 = DIRECTION/DISTANCE CALCULATOR\n");
            printf("   5 = GALAXY 'REGION NAME' MAP\n\n");


        } else {
            printf("?REENTER \n");
        }

    }
}


void cmdXXX(World *world){
    world->gameOver = true;
}


void checkGameOver(World *world) {

    if (world->daysRem <= 0) {
        world->gameOver = true;
    }
    if (world->gameOver) {
        printf("IT IS STARDATE %i\n", world->date);
        printf("THERE WERE %i KLINGON BATTLE CRUISERS LEFT AT\n", world->numKlingons);
        printf("THE END OF YOUR MISSION.\n\n");

        printf("THE FEDERATION IS IN NEED OF A NEW STARSHIP COMMANDER\n"
               "FOR A SIMILAR MISSION -- IF THERE IS A VOLUNTEER,\n"
               "LET HIM STEP FORWARD AND ENTER 'AYE': ");

        char input[STR_SIZE];
        fgets(input, STR_SIZE, stdin);
        for (int i=0; i<3; i++) input[i] = (char) toupper(input[i]); // Turn first 3 chars to upper

        if (!strncmp(input, "AYE", 3)) {
            printTitle();
            *world = generateWorld();
            return;

        } else {
            printf("\n*****************************************\n");
            printf("*                   *                   *\n");
            printf("*                 * * *                 *\n");
            printf("*     * *   THANKS FOR PLAYING!   * *   *\n");
            printf("*               * *   * *               *\n");
            printf("*              * *     * *              *\n");
            printf("*****************************************\n\n");
            exit(0);
        }

    }

}


void sortEntities(Entity **entity, double *dist, int n) {
    double temp1;
    Entity *temp2;

    for (int i = 0 ; i<n-1; i++) {
        for (int j=0; j<n-i-1; j++) {
            if (dist[j] > dist[j+1]) {

                /* Swapping */
                temp1 = dist[j];
                dist[j]   = dist[j+1];
                dist[j+1] = temp1;

                temp2 = entity[j];
                entity[j] = entity[j + 1];
                entity[j + 1] = temp2;
            }
        }
    }
}

void printInstructions() {
    printf("            INSTRUCTIONS FOR 'SUPER STAR TREK'            \n");
    printf("===========================================================\n");
    printf("1.When you see COMMAND ? printed, enter one of the legal\n");
    printf("Commands(NAV, SRS, LRS, PHA, TOR, SHE, DAM, COM, or XXX).\n");
    printf("2.If you should type an illegal command, you'll get a short\n");
    printf("list of legal commands printed out.\n");
    printf("3.Some commands require you to enter data (For example, the\n");
    printf("'NAV' comes back with 'Course (1-9) ?'.) if you\n");
    printf("type in illegal data, like negative numbers, that command\n");
    printf("will be aborted\n");
    printf("\n");
    printf("The galaxy is divided into an 8x8 quadrant grid\n");
    printf("and each quadrant is further divided into an 8x8 sector grid\n");
    printf("\n");
    printf("You will be assigned a starting point somewhere in the\n");
    printf("galaxy to begin a tour of duty as commander of the Starship\n");
    printf("Enterprise. your mission: to seek and destroy the fleet of\n");
    printf("Klingon Warships which are menacing the United Federation of\n");
    printf("planets\n");
    printf("\n");
    printf("You have the following commands available to you as captain\n");
    printf("of the Starship Enterprise: \n");
    printf("\n");
    printf("NAV Command = Warp Engine Control --\n");
    printf("   Course is in a circular numerical      4  3  2\n");
    printf("   vector arrangement as shown.            * * *\n");
    printf("   Integers and Reals may be                ***\n");
    printf("   used. (thus course 1.5 is half-     5  ---*---  1\n");
    printf("   way between 1 and 2)                     ***\n");
    printf("                                           * * *\n");
    printf("   Values may approach 9.0, which         6  7  8\n");
    printf("   itself is equivalent to 1.0\n");
    printf("                                           Course\n");
    printf("   One warp factor is the size of\n");
    printf("   one quadrant. Therefore, to get\n");
    printf("   from quadrant 6,5 to 5,5, you would\n");
    printf("   use course 3, warp factor 1.\n");
    printf("\n");
    printf("SRS Command = Short Range Sensor Scan\n");
    printf("   Shows you a scan of your present quadrant.\n");
    printf("\n");
    printf("   Symbology on your sensor screen is as follows:\n");
    printf("      <*> = Your starship's position\n");
    printf("      +K+ = Klingon Battle Cruiser\n");
    printf("      >!< = Federation Starbase (Refuel/Repair/Re-arm here!)\n");
    printf("\n");
    printf("       * = Star\n");
    printf("\n");
    printf("    A condensed status report will also be presented.\n");
    printf("\n");
    printf("LRS Command = Long Range Sensor Scan\n");
    printf("   Shows conditions in space for one quadrant on each side\n");
    printf("   of the Enterprise (which is in the middle of the scan)\n");
    printf("   The scan is coded in the form ###, where the units digit\n");
    printf("   is the number of stars, the tens digit is the number of\n");
    printf("   starbases, and the hundreds digit is the number of Klingons\n");
    printf("\n");
    printf("   Example - 207 = 2 Klingons, No Starbases & 7 Stars\n");
    printf("\n");
    printf("PHA Command = Phaser Control\n");
    printf("   Allows you to destroy the Klingon Battle Cruisers by\n");
    printf("   zapping them with suitably large units of energy to\n");
    printf("   deplete their shield power. (Remember, Klingons have\n");
    printf("   Phasers too!)\n");
    printf("\n");
    printf("TOR Command = Photon Torpedo Control\n");
    printf("   Torpedo course is the same as used in warp engine control\n");
    printf("   If you hit the Klingon Vessel, he is destroyed and\n");
    printf("   cannot fire back at you. If you miss, you are subject to\n");
    printf("   his Phaser fire. In either case, you are also subject to\n");
    printf("   the Phaser fire of all other Klingons in the quadrant\n");
    printf("   The library computer ('COM' Command) has an option to\n");
    printf("   compute torpedo trajectory for you (Option 2)\n");
    printf("\n");
    printf("SHE Command = Shield Control\n");
    printf("   Defines the number of energy units to be assigned to the\n");
    printf("   shields. Energy is taken from total ship's energy. Note\n");
    printf("   that the status display total energy includes shield energy\n");
    printf("\n");
    printf("DAM Command = Damage Control Report\n");
    printf("   Gives the state of repair of all devices. Where a negative\n");
    printf("   'State of Repair' shows that the device is temporarily damaged\n");
    printf("\n");
    printf("COM Command = Library-Computer\n");
    printf("   The Library-Computer contains six options:\n");
    printf("   Option 0 = Cumulative Galactic Record\n");
    printf("      This option shows computer memory of the results of \n");
    printf("      previous short and long range sensor scans\n");
    printf("   Option 1 = Status Report\n");
    printf("      This option shows the number of Klingons, Stardates,\n");
    printf("      and Starbases remaining in the game.\n");
    printf("   Option 2 = Photon Torpedo Data\n");
    printf("      Which gives directions and distance from the Enterprise\n");
    printf("      to all Klingons in your quadrant.\n");
    printf("   Option 3 = Starbase NAV Data\n");
    printf("      This option gives direction and distance to any\n");
    printf("      Starbase within your quadrant\n");
    printf("   Option 4 = Direction/Distance Calculator\n");
    printf("      This option allows you to enter coordinates for\n");
    printf("      Direction/Distance calculations\n");
    printf("   Option 5 = Galactic / Region Name / Map\n");
    printf("      This option prints the names of the sixteen major\n");
    printf("      galactic regions referred to in the game\n");
    printf("===========================================================\n");
    printf("               (ENTER ANY KEY TO CONTINUE) ");
    char input[STR_SIZE];
    fgets(input, STR_SIZE, stdin);
}
