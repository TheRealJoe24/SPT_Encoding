/*
    00B1 100 080 P PD = 
    A0B9 050 P MD = 
    89C0 HIT S HF = 
    AB41 1 B LB = 
    0001 60F D WT =

    4DEF 1 1 0 B LD = 
    54E3 100m S DD =
    8B7A 054 D SD =
    A65F 002kn S VD =
    E6D5 050 P BD =



    Possibilities for data sequence

    xxx xxx  P
    xxx      P
    <str>    S
    xxF      D
    xxC      D
    b b b    B

*/

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

enum {
    DD_PERCENTAGE=0,
    DD_STRING,
    DD_BOOL,
    DD_DEGREES
};

enum {
    DD_PD=0,
    DD_PB,
    DD_MD,
    DD_HF,
    DD_LB,
    DD_WT,
    DD_LD,
    DD_DD,
    DD_SD,
    DD_VD,
    DD_BD,
};

typedef struct {
    /* pointer to command bytes */
    uint8_t *buf;
    /* for decoder */
    uint16_t identifier;
    char *dataSequence;
    uint8_t dataType;
    uint8_t sensor;
} command_t;

typedef struct {
    /* pointer to command */
    command_t *cmd;
} decoder_t;

typedef struct {
    int props[2];
    int motors[2];
    int pH, temp, mast, pressure, sonar, battery, light, video_t;
    int RF;
    int limit[3];
    char *hit;
    int video_b;
    char *speed;
    char *depth;
} state_t;

void assembleCommand(command_t *, const char *);
void runInputs(char *);
void runCommand(state_t *state, command_t *command);

/**
 * prints an update regarding the previous command and its resultant state
 * @param  command - provides a pointer to the current command
 * @param  state - provides a pointer to the current state of the system
 */
void printStateChange(command_t *command, state_t *state) {
    int sensor = command->sensor;
    printf("UPDATE: ");
    switch (sensor) {
        case DD_PD:
            printf("Left prop set to: %i, right prop set to: %i\n", state->props[0], state->props[1]);
            break;
        case DD_MD:
            printf("Left motor set to: %i, right motor set to: %i\n", state->motors[0], state->motors[1]);
            break;
        case DD_HF:
            printf("Hit flag changed to: %s\n", state->hit);
            break;
        case DD_LB:
            printf("Light button changed to: %i\n", state->light);
            break;
        case DD_WT:
            printf("Water temperature: %i\n", state->temp);
            break;
        case DD_LD:
            printf("Limit switches: %i%i%i\n", state->limit[0], state->limit[1], state->limit[2]);
            break;
        case DD_DD:
            printf("Limit switches: %i%i%i\n", state->limit[0], state->limit[1], state->limit[2]);
            break;
        case DD_SD:
            printf("Depth data: %s\n", state->depth);
            break;
        case DD_VD:
            printf("Velocity data: %s\n", state->speed);
            break;
        case DD_BD:
            printf("Battery level: %i percent\n", state->battery);
            break;
        default:
            break;
    }
}

/**
 * initializes the state
 * @param  state - provides a pointer to the current state of the system
 */
void initState(state_t *state) {
    state->props[0] = 0;
    state->props[1] = 99;
    state->motors[0] = 69;
    state->motors[1] = 50;
    state->pH = 7;
    state->speed = "0kn";
    state->temp = 89;
    state->mast = 76;
    state->pressure = 1;
    state->sonar = 0;
    state->battery = 100;
    state->light = 0;
    state->video_t = 720;
    state->RF = 1;
    state->limit[0] = 0;
    state->limit[1] = 0;
    state->limit[2] = 0;
    state->hit = "Miss";
    state->depth = "30m";
}

/**
 * takes a character and returns an integer containing the system data type
 * @param  c - character containing the data type
 */
uint8_t charToDataType(char c) {
    uint8_t dt = 0;
    switch (c) {
        case 'P':
            dt = DD_PERCENTAGE;
            break;
        case 'S':
            dt = DD_STRING;
            break;
        case 'B':
            dt = DD_BOOL;
            break;
        case 'D':
            dt = DD_DEGREES;
            break;
    }
    return dt;
}

/**
 * take a two byte string and convert it into an integer token
 * @param  s - two byte string
 */
uint8_t strToToken(char s[2]) {
    uint8_t tk = 0;
    //printf("%s\n", s);
    switch (s[0]) {
        default:
            break;
        case 'P':
            if (s[1] == 'D') tk = DD_PD; /* propellor data */
            break;
        case 'M':
            if (s[1] == 'D') tk = DD_MD; /* motor data */
            break;
        case 'H':
            if (s[1] == 'F') tk = DD_HF; /* hit flag data */
            break;
        case 'L':
            if (s[1] == 'B') tk = DD_LB; /* Flight button */
            else if (s[1] == 'D') tk = DD_LD; /* Limit Switch data */
            break;
        case 'W':
            if (s[1] == 'T') tk = DD_WT; /* Water temperature data */
            break;
        case 'D':
            if (s[1] == 'D') tk = DD_DD; /* depth data */
            break;
        case 'S':
            if (s[1] == 'D') tk = DD_SD; /* Sonar Data */
            break;
        case 'V':
            if (s[1] == 'D') tk = DD_VD; /* Velocity Data */
            break;
        case 'B':
            if (s[1] == 'D') tk = DD_BD; /* Battery Data */
            break;
    }
    return tk;
}

void runInputs(char *input_buf) {
    size_t len = strlen(input_buf);
    char *buf = (char*)malloc(len); /* strtok cannot tokenize compile-time strings (read-only memory) */
    strcpy(buf, input_buf);         /* so we must copy the string into a dynamically allocated buffer */
    char *token = strtok(buf, "="); /* now we can tokenize the string, without a segmentation fault */
    command_t **commands = (command_t **)malloc(sizeof(command_t **));
    state_t *state = (state_t*)malloc(sizeof(state_t));
    initState(state);
    for (int i = 0; token != NULL; i++) {
        commands[i] = (command_t *)malloc(sizeof(command_t *));
        char *tk = (char*)malloc(strlen(token)+1); /* token + delim ('=') */
        strcpy(tk, token);
        tk[strlen(token)] = '=';
        assembleCommand(commands[i], tk); /* assemble each command individually */
        printf("\n-------------------------------------------\n\n");
        printf("\nReceived command: '%s'\n", tk);
        printf("Command ID: 0x%04X\n", commands[i]->identifier);
        printf("Data Type ID: 0x%01X\n", commands[i]->dataType);
        printf("Data Sensor ID: 0x%01X\n", commands[i]->sensor);
        printf("DataSequence: %s\n", commands[i]->dataSequence);
        runCommand(state, commands[i]);
        printStateChange(commands[i], state);
        token = strtok(NULL, "=");
    }
}

/**
 * assembles the given command into the command
 * @param  command - provides a pointer to the command for buf to be assembled to
 * @param  buf - provides a pointer to a string containing the command
 */
void assembleCommand(command_t *command, const char *buf) {
    size_t buf_len = strlen(buf); /* calculate the length of the input buffer. This will be used for future calculations */

    command->buf = (uint8_t*)malloc(buf_len*sizeof(uint8_t)); /* allocate the internal command buffer */

    char id_buf[5]; /* one extra byte for null termination */
    memcpy(id_buf, buf, 4); /* copy 4 bytes from the input buffer into the identification buffer */
    command->identifier = strtol(id_buf, NULL, 16); /* store the value of the id */

    char termination_buf[5]; /* buffer for the termination sequence */
    memcpy(termination_buf, buf+buf_len-4, 4); /* copy the last four bytes of the input buffer to the termination buffer */

    char data_type = termination_buf[0]; /* the data type is stored in the first index of the termination sequence, lets store it */
    command->dataType = charToDataType(data_type); /* store the data type converted to an enum for use later */

    char *sensor = (char*)malloc(2); /* 2 byte buffer containing the termination token EX. PD */
    memcpy(sensor, (termination_buf+1), 2); /* store 2 bytes from the termination buffer in the sensor buffer */
    command->sensor = strToToken(sensor); /* convert the sensor buffer into an enum */

    /* where in memory is the data sequence? */
    char *start = buf+4; /* 4 byte offset from the input buffer -- the identification is 4 bytes, so lets start after that */
    char *end = (buf+buf_len)-4; /* 4 byte offset from the end of the buffer -- same as above but in reverse */
    size_t size = end-start; /* the size would be the offset from the start to end addresses */
    command->dataSequence = (char*)malloc(size+1); /* allocate the data sequence buffer with the size in bytes */
    memcpy(command->dataSequence, start, size); /* now we copy the buffer, given the offsets we calculated, into the data sequence allocated above. */
}

/**
 * runs the given command and stores the resultant state
 * @param  state - provides a pointer to the current state of the system
 * @param  command - provides a pointer to the current command
 */
void runCommand(state_t *state, command_t *command) {
    uint8_t opcode = (command->identifier & 0xF000) >> 12; /* highest byte */
    uint16_t word  = (command->identifier & 0x0FFF); /* lowest 3 bytes */

    size_t ds_len = strlen(command->dataSequence);

    /* is the data sequence a single percentage value? */
    bool single_percentage = false;
    /* are degrees in temperature or rotation? */
    bool rotation_degrees = false;

    char **args; /* pointer to strings */
    switch (command->dataType) {
        case DD_PERCENTAGE:
            args = malloc(2*sizeof(char*)); /* 2 buffers */
            args[0] = (char*)malloc(4); /* 4 bytes */
            memcpy(args[0], command->dataSequence, 3);
            if (ds_len > 3) { /* check for single or double percentage value */
                args[1] = (char*)malloc(4);
                memcpy(args[1], command->dataSequence+3, 3);
            } else single_percentage = true;
            break;
        case DD_STRING:
            args = (char**)malloc(sizeof(char*));
            args[0] = (char*)malloc(32); /* arbitrary max size */
            strcpy(args[0], command->dataSequence);
            break;
        case DD_BOOL:
            args = (char**)malloc(sizeof(char*));
            for (int i = 0; i < ds_len; i++) {
                args[i] = (char*)malloc(2); /* 1 byte for single character + null termination */
                memcpy(args[i], command->dataSequence+i, 1);
            }
            break;
        case DD_DEGREES:
            args = (char**)malloc(sizeof(char*));
            args[0] = malloc(5); /* 3 for temp, 1 for sign, 1 for null termination */
            args[1] = malloc(2); /* 1 for c or f and 1 for null termination */
            memcpy(args[0], command->dataSequence, ds_len-1); /* exlclude the f or c from the temp */
            strcpy(args[1], (command->dataSequence+ds_len)-1); /* only include the f or c */
            if (args[1] != 'c' || args[1] != 'f') {
                rotation_degrees = true;
                free(args[1]); /* the arg is no longer necessary, free it */
            }
            break;
    }
    /* TODO: Error if input string is not to spec, to keep user informed */
    switch (opcode) {
        case 0x0:
            if (word == 0xB1 && !single_percentage) { /* 00B1 */
                int p0 = atoi(args[0]); /* left percentage */
                int p1 = atoi(args[1]); /* right percentage */
                /* decide whether motor data or prop data should be changed */
                if (command->sensor == DD_PD) {
                    state->props[0] = p0;
                    state->props[1] = p1;
                } else if (command->sensor == DD_MD) {
                    state->motors[0] = p0;
                    state->motors[1] = p1;
                }
            } else if (word == 0x01) { /* 0001 */
                int temp = atoi(args[0]);
                state->temp = (temp << 8) | (args[1][0]); /* store the metric in the lower 8 bytes */
            }
            break;
        case 0xA:
            if (word == 0xB8 && single_percentage) { /* A0B8 */
                int p = atoi(args[0]);
                if (command->sensor == DD_PD) {
                    state->props[0] = p;
                } else if (command->sensor == DD_MD) {
                    state->motors[0] = p;
                }
            } else if (word == 0xB9 && single_percentage) { /* A0B9 */
                int p = atoi(args[0]);
                if (command->sensor == DD_PD) {
                    state->props[1] = p;
                } else if (command->sensor == DD_MD) {
                    state->motors[1] = p;
                }
            } else if (word == 0x65F) { /* A65F */
                char *s = args[0];
                state->speed = s;
            } else if (word == 0xB41) { /* AB41 */
                bool b = (bool)atoi(args[0]);
                state->light = b;
            }
            break;
        case 0x8:
            if (word == 0x9C0) { /* 89C0 */
                char *s = args[0];
                state->hit = (strcmp(s, "HIT") ? "Miss" : "Hit"); /* convert to more readable string */
            } else if (word == 0xB7A && rotation_degrees) { /* 8B7A */
                int rot = atoi(args[1]);
                if (rot >= 0 && rot <= 180) {
                    state->sonar = rot;
                }
            }
            break;
        case 0x4:
            if (word == 0xDEF) { /* 4DEF */
                /* TODO: iterate through instead of hardcoding values */
                bool b0 = (bool)atoi(args[0]); /* three limit switches */
                bool b1 = (bool)atoi(args[1]);
                bool b2 = (bool)atoi(args[2]);
                #define FORMAT_SWITCH(b) (b ? "Pressed" : "Released")
                state->limit[0] = b0; /* im not even going to try and say this is good code */
                state->limit[1] = b1;
                state->limit[2] = b2;
            }
            break;
        case 0x5:
            if (word == 0x4E3) { /* 54E3 */
                char *str = args[0];
                state->depth = str;
            }
            break;
        case 0xE:
            if (word == 0x6D5 && single_percentage) { /* E6D5 */
                int p = atoi(args[0]);
                state->battery = p;
            }
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argv[1] != NULL) {
        FILE *fp = fopen(argv[1], "r");
        const char *buf = (char*)malloc(1024); /* maximum input length */
        fgets(buf, 1024, fp);
        runInputs(buf);
    } else {
        printf("Please run the code with a path to an input file\n");
        return -1;
    }
}
