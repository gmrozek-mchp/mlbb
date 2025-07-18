#ifndef BALANCE_H
#define	BALANCE_H


typedef enum {
    BALANCE_MODE_OFF = 0,
    BALANCE_MODE_PID,
    BALANCE_MODE_NN,
    BALANCE_MODE_HUMAN,

    BALANCE_MODE_NUM_MODES,
    BALANCE_MODE_INVALID = BALANCE_MODE_NUM_MODES
} balance_mode_t;


void BALANCE_Initialize( void );

balance_mode_t BALANCE_MODE_Get( void );
void BALANCE_MODE_Set( balance_mode_t mode );
void BALANCE_MODE_Next( void );
void BALANCE_MODE_Previous( void );


#endif	/* BALANCE_H */

