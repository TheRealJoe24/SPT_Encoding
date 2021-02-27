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

uint8_t charToDataType(char c) {
    uint8_t dt = 0;
    printf("%c",c);
    switch (c) {
        case 'P':
            printf("hey");
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

uint8_t strToToken(char s[2]) {
    uint8_t tk = 0;
    switch (s[0]) {
        default:
            break;
        case 'P':
            if (s[1] == 'D') tk = DD_PD; /* propellor data */
            break;
        case 'M':
            if (s[1] == 'M') tk = DD_MD; /* motor data */
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
            if (s[1] == 'B') tk = DD_BD; /* Battery Data */
            break;
    }
    return tk;
}

command_t *assembleCommand(const char *buf) {
    printf("Received command: '%s'\n", buf);

    size_t buf_len = strlen(buf); /* calculate the length of the input buffer. This will be used for future calculations */
    /* allocate the command buffer. Should be 32 bits as it this does not include the size of the buffers. 
       Those should be allocated later */
    command_t *command = (command_t*)malloc(sizeof(command_t));

    command->buf = (uint8_t*)malloc(buf_len*sizeof(uint8_t)); /* allocate the internal command buffer */

    char id_buf[5]; /* one extra byte for null termination */
    memcpy(id_buf, buf, 4); /* copy 4 bytes from the input buffer into the identification buffer */
    command->identifier = strtol(id_buf, NULL, 16); /* store the value of the id */

    char termination_buf[5]; /* buffer for the termination sequence */
    memcpy(termination_buf, buf+buf_len-4, 4); /* copy the last four bytes of the input buffer to the termination buffer */

    char data_type = termination_buf[0]; /* the data type is stored in the first index of the termination sequence, lets store it */
    command->dataType = charToDataType(data_type); /* store the data type converted to an enum for use later */

    char sensor[2]; /* 2 byte buffer containing the termination token EX. PD */
    memcpy(sensor, (termination_buf+1), 2); /* store 2 bytes from the termination buffer in the sensor buffer */
    command->sensor = strToToken(sensor); /* convert the sensor buffer into an enum */

    /* where in memory is the data sequence? */
    char *start = buf+4; /* 4 byte offset from the input buffer -- the identification is 4 bytes, so lets start after that */
    char *end = (buf+buf_len)-4; /* 4 byte offset from the end of the buffer -- same as above but in reverse */
    size_t size = end-start; /* the size would be the offset from the start to end addresses */
    command->dataSequence = (char*)malloc(size+1); /* allocate the data sequence buffer with the size in bytes */
    memcpy(command->dataSequence, start, size); /* now we copy the buffer, given the offsets we calculated, into the data sequence allocated above. */

    return command; /* Now, return the generated command */
}

void runCommand(command_t *command) {
    uint8_t opcode = (command->identifier & 0xF000) >> 12;
    uint16_t word  = (command->identifier & 0x0FFF);

    size_t ds_len = strlen(command->dataSequence);

    char **args; /* pointer to strings */
    switch (command->dataType) {
        case DD_PERCENTAGE:
            args = malloc(2*sizeof(char*)); /* 2 buffers */
            args[0] = (char*)malloc(4); /* 4 bytes */
            args[1] = (char*)malloc(4);
            memcpy(args[0], command->dataSequence, 3);
            memcpy(args[1], command->dataSequence+3, 3);
            break;
        case DD_STRING:
            args = (char**)malloc(sizeof(char*));
            args[0] = (char*)malloc(32); /* arbitrary max size */
            strcpy(args, command->dataSequence);
            break;
        case DD_BOOL:
            args = (char**)malloc(sizeof(char*));
            args[0] = malloc(2); /* 1 byte for single character + null termination */
            strcpy(args[0], command->dataSequence);
            break;
        case DD_DEGREES:
            args = (char**)malloc(sizeof(char*));
            args[0] = malloc(5); /* 3 for temp, 1 for sign, 1 for null termination */
            args[1] = malloc(2); /* 1 for c or f and 1 for null termination */
            memcpy(args[0], command->dataSequence, ds_len-1); /* exlclude the f or c from the temp */
            strcpy(args[1], (command->dataSequence+ds_len)-1); /* only include the f or c */
            break;
    }

    switch (opcode) {
        case 0x0:
            if (word == 0xB1) { /* 00B1 */
                int p0 = atoi(args[0]); /* left percentage */
                int p1 = atoi(args[1]); /* right percentage */
                printf("\nSetting left motor to %i percent and right motor to %i percent!\n", p0, p1);
            } else if (word == 0x01) { /* 0001 */
                int temp = atoi(args[0]);
                printf("\nWater temperature: %i%c\n", temp, args[1][0]);
            }
            break;
        case 0xA:
            if (word == 0xB8) { /* A0B8 */

            } else if (word == 0xB9) { /* A0B9 */
                
            } else if (word == 0x65F) { /* A65F */
                
            } else if (word == 0xB41) { /* AB41 */
                
            }
            break;
        case 0x8:
            if (word == 0x9C0) { /* 89C0 */

            } else if (word == 0xB7A) { /* 8B7A */
                
            }
            break;
        case 0x4:
            if (word == 0xDEF) { /* 4DEF */

            }
            break;
        case 0x5:
            if (word == 0x4E3) { /* 54E3 */

            }
            break;
        case 0xE:
            if (word == 0x6D5) { /* E6D5 */

            }
            break;
    }
}

int main(int argc, char *argv[]) {
    command_t *command = assembleCommand("000186FDWT=");
    printf("Command ID: 0x%04X\n", command->identifier);
    printf("Data Type ID: 0x%01X\n", command->dataType);
    printf("Data Sensor ID: 0x%01X\n", command->sensor);
    printf("DataSequence: %s\n", command->dataSequence);

    runCommand(command);
}
