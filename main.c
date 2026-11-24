#include "msp430fr6989.h"
#include <stdbool.h>
#include "Grlib/grlib/grlib.h"
#include "LcdDriver/lcd_driver.h"
#include <stdio.h>


#define redLED BIT0
#define greenLED BIT7
#define S1 BIT1
#define S2 BIT2
#define Horizontal BIT2
#define Vertical BIT7
#define Select BIT5 // P3.5
#define Buzzer BIT7 // P2.7


// Win conditions
const uint8_t wincon[8][3] = {
   {0, 1, 2},
   {3, 4, 5},
   {6, 7, 8},
   {0, 3, 6},
   {1, 4, 7},
   {2, 5, 8},
   {0, 4, 8},
   {2, 4, 6},
};


typedef enum {
   result_none = 0,
   x_wins,
   o_wins,
   draw
} game_result;


typedef enum {
   mode_none = 0,
   mode_pve,
   mode_pvp,
} mode;


typedef enum {
   shape_none = 0,
   shape_x,
   shape_o
} shape;


typedef enum {
   pos1 = 0,
   pos2,
   pos3,
   pos4,
   pos5,
   pos6,
   pos7,
   pos8,
   pos9
} cursor_pos;


typedef enum {
   none = 0,
   left,
   right,
   up,
   down
} direction;


typedef struct {
   uint16_t x;
   uint16_t y;
   bool active;
   shape type;
} square;


typedef struct {
   square squares[9];
   shape current_turn;
   cursor_pos saved_cursor;
   mode gamemode;
   bool game_over;
   game_result result;
   bool banner;
} board;


// Function Defs
void Initialize_Clock_System(void);
void Initialize_ADC(void);
void Initialize_Clock_System(void);
void Initialize_Timer(void);
void cursor_update(board *Board1);
void blink_cursor(board *Board1, Graphics_Context *g_sContext, uint16_t width, uint16_t height);
void gridlines(uint16_t vline1, uint16_t vline2, uint16_t hline1, uint16_t hline2, uint16_t width, uint16_t height, Graphics_Context *g_sContext);
void blink_cursor_off(board *Board, Graphics_Context *g_sContext, uint16_t width, uint16_t height);
void play_move(board *Board, Graphics_Context *g_sContext, uint16_t width, uint16_t height);
void Initialize_PWM(void);
void buzz(void);
void drawX(board *Board, uint8_t index, Graphics_Context *g_sContext, uint16_t width, uint16_t height);
void drawO(board *Board, uint8_t index, Graphics_Context *g_sContext, uint16_t width);
game_result checkwin(board *Board, Graphics_Context *g_sContext);
void reset_board(board *Board, Graphics_Context *g_sContext, uint16_t vline1, uint16_t vline2, uint16_t hline1, uint16_t hline2, uint16_t width, uint16_t height);
void redraw_board(board *Board, Graphics_Context *g_sContext, uint16_t vline1, uint16_t vline2, uint16_t hline1, uint16_t hline2, uint16_t width, uint16_t height);
void draw_menu(board *Board, mode menu, Graphics_Context *g_sContext);
direction read_joystick(void);
void ai_move(board *Board, Graphics_Context *g_sContext, uint16_t width, uint16_t height);
void draw_win_banner(game_result result, Graphics_Context *g_sContext);
void return_to_menu(board *Board, mode *menu_cursor, Graphics_Context *g_sContext);


// Global Variables
cursor_pos cursor = pos1;
volatile bool blink = false;
direction dir = none;


void main(void) {
   // Configure WDT & GPIO
   WDTCTL = WDTPW | WDTHOLD;
   PM5CTL0 &= ~LOCKLPM5;


   // Configure LEDs
   P1DIR |= redLED;
   P9DIR |= greenLED;
   P1OUT &= ~redLED;
   P9OUT &= ~greenLED;

   // Configure buttons
   P1DIR &= ~(S1|S2);
   P1REN |= (S1|S2);
   P1OUT |= (S1|S2);
   P1IFG &= ~(S1|S2);
   P3DIR &= ~Select;
   P3REN |= Select;
   P3OUT |= Select;


   // Set the LCD backlight to highest level
   P2DIR |= BIT6;
   P2OUT |= BIT6;


   // Configure clock system
   Initialize_Clock_System();
   Initialize_ADC();
   Initialize_Timer();
   Initialize_PWM();


   ////////////////////////////////////////////////////////////////////////////////////////////
   // Graphics functions
   Graphics_Context g_sContext;        // Declare a graphic library context
   Crystalfontz128x128_Init();         // Initialize the display


   // Set the screen orientation
   Crystalfontz128x128_SetOrientation(0);


   // Initialize the context
   Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);


   // Set background and foreground colors
   Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
   Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);


   // Set the default font for strings
   GrContextFontSet(&g_sContext, &g_sFontFixed6x8);


   //Clear the screen
   Graphics_clearDisplay(&g_sContext);


   // Enable Interrupts
   __enable_interrupt();
   ////////////////////////////////////////////////////////////////////////////////////////////


   // Tic - Tac - Toe Grid
   uint16_t width = Graphics_getDisplayWidth(&g_sContext);   // 128
   uint16_t height = Graphics_getDisplayHeight(&g_sContext); // 128


   uint16_t vline1 = width / 3;          // x = 42
   uint16_t vline2 = (2 * width) / 3;    // x = 85
   uint16_t hline1 = height / 3;         // y = 42
   uint16_t hline2 = (2 * height) / 3;   // y = 85


   // Board setup
   board Board1 = {
       .squares = {
           { width / 6, height / 6, false, shape_none }, // [0] Top-Left
           { width / 2, height / 6, false, shape_none }, // [1] Top-Center
           { (5 * width) / 6, height / 6, false, shape_none }, // [2] Top-Right
           { width / 6, height / 2, false, shape_none }, // [3] Mid-Left
           { width / 2, height / 2, false, shape_none }, // [4] Center
           { (5 * width) / 6, height / 2, false, shape_none }, // [5] Mid-Right
           { width / 6, (5 * height) / 6, false, shape_none }, // [6] Bot-Left
           { width / 2, (5 * height) / 6, false, shape_none }, // [7] Bot-Center
           { (5 * width) / 6, (5 * height) / 6, false, shape_none }  // [8] Bot-Right
       },
       .current_turn = shape_x,
       .game_over = false,
       .saved_cursor = pos1,
       .gamemode = mode_none,
       .result = result_none,
       .banner = false
   };


   // Two boards
   board Board2 = Board1;
   board *activeBoard = &Board1;


   // Red for Board1, Green for Board2
   P1OUT |= redLED;
   P9OUT &= ~greenLED;


   // Track when joystick is centered
   bool center = true;


   // Variable to hold when to clear and draw the cursor
   bool clear = false;


   // count variable for debounce
   uint16_t count = 0;


   // Variable to hold button press
   bool but1_released = true;
   bool but2_released = true;
   bool but2_hold_handled = false;
   uint16_t but2_held = 0;
   uint16_t held_ticks = 0;


   // Start with cursor on PVE
   mode menu_cursor = mode_pve;
   draw_menu(activeBoard, menu_cursor, &g_sContext);


while (1) {
       direction input_dir = read_joystick();


       // switch between boards logic. short press -> swap boards | long press -> menu
       if ((P1IN & S2) == 0) {
           if (but2_released) {
               but2_released = false;
               but2_hold_handled = false;
               but2_held = TA0R;
           } else if (!but2_hold_handled) {
               // Measure elapsed time using TA0R, supporting rollover
               if ( TA0R >= but2_held ) {
                   held_ticks = TA0R - but2_held;
               } else { held_ticks = 32768 - but2_held + TA0R; }


               // Wait 0.8 seconds before going back to menu
               if (held_ticks > 26214) {
                   but2_hold_handled = true;
                   return_to_menu(activeBoard, &menu_cursor, &g_sContext);
                   buzz();
                   clear = true;
               }
           }
       } else {
           // If it was just a button push, work like normal
           if (!but2_released && !but2_hold_handled) {
               activeBoard->saved_cursor = cursor;


               if (activeBoard == &Board1) {
                   activeBoard = &Board2;
                   P1OUT &= ~redLED;
                   P9OUT |= greenLED;
               } else {
                   activeBoard = &Board1;
                   P1OUT |= redLED;
                   P9OUT &= ~greenLED;
               }


               cursor = activeBoard->saved_cursor;
               if (activeBoard->gamemode == mode_none) {
                   menu_cursor = mode_pve;
                   draw_menu(activeBoard, menu_cursor, &g_sContext);
               } else {
                   redraw_board(activeBoard, &g_sContext, vline1, vline2, hline1, hline2, width, height);
               }
               clear = true;
               __delay_cycles(250000);
           }
           but2_released = true;
       }
       // mode selection logic
       if (activeBoard->gamemode == mode_none) {
           if ((uint16_t)(TA0R - count) > 7150) {
               count = TA0R;
               if (input_dir == up || input_dir == down) {
                   if (center) {
                       center = false;
                       menu_cursor = (menu_cursor == mode_pve) ? mode_pvp : mode_pve;
                       draw_menu(activeBoard, menu_cursor, &g_sContext);
                   }
               } else if (input_dir == none) {
                   center = true;
               }
           }


           // Confirm gamemode selection and start game
           if ((P1IN & S1) == 0) {
               if (but1_released) {
                   but1_released = false;
                   activeBoard->gamemode = menu_cursor;
                   cursor = pos1;
                   Graphics_clearDisplay(&g_sContext);
                   gridlines(vline1, vline2, hline1, hline2, width, height, &g_sContext);
                   blink_cursor(activeBoard, &g_sContext, width, height);
                   clear = true;
                   __delay_cycles(320000);
               }
           } else {
               but1_released = true;
           }
       }
       // logic for tic tac toe game
       else {
           if ((P1IN & S1) == 0) {
               if (but1_released) {
                   but1_released = false;
                   if (activeBoard->game_over) {
                       if (!activeBoard->banner) {
                           activeBoard->banner = true;
                           redraw_board(activeBoard, &g_sContext, vline1, vline2, hline1, hline2, width, height);
                       } else {
                           reset_board(activeBoard, &g_sContext, vline1, vline2, hline1, hline2, width, height);
                       }
                   } else {
                       play_move(activeBoard, &g_sContext, width, height);
                       if (activeBoard->gamemode == mode_pve && !activeBoard->game_over && activeBoard->current_turn == shape_o) {
                           __delay_cycles(4000000);
                           ai_move(activeBoard, &g_sContext, width, height);
                       }
                   }
                   clear = true;
                   __delay_cycles(320000);
               }
           } else {
               but1_released = true;
           }


           // if game is not over, handle joystick inputs and cursor blinking
           if (!activeBoard->game_over && (uint16_t)(TA0R - count) > 7150) {
               count = TA0R;
               dir = input_dir;


               if (dir == none) {
                   center = true;
               } else if (center == true) {
                   center = false;
                   blink_cursor_off(activeBoard, &g_sContext, width, height);
                   cursor_update(activeBoard);
                   blink_cursor(activeBoard, &g_sContext, width, height);
                   clear = true;
               }


               if (blink) {
                   if (!clear) {
                       blink_cursor(activeBoard, &g_sContext, width, height);
                   } else {
                       blink_cursor_off(activeBoard, &g_sContext, width, height);
                   }
                   clear = !clear;
               }
           }
       }
   }
}






void Initialize_Clock_System() {
 // DCO frequency = 16 MHz
 // MCLK = fDCO/1 = 16 MHz
 // SMCLK = fDCO/1 = 16 MHz


 // Activate memory wait state
 FRCTL0 = FRCTLPW | NWAITS_1;    // Wait state=1
 CSCTL0 = CSKEY;
 // Set DCOFSEL to 4 (3-bit field)
 CSCTL1 &= ~DCOFSEL_7;
 CSCTL1 |= DCOFSEL_4;
 // Set DCORSEL to 1 (1-bit field)
 CSCTL1 |= DCORSEL;
 // Change the dividers to 0 (div by 1)
 CSCTL3 &= ~(DIVS2|DIVS1|DIVS0);    // DIVS=0 (3-bit)
 CSCTL3 &= ~(DIVM2|DIVM1|DIVM0);    // DIVM=0 (3-bit)
 CSCTL0_H = 0;


 return;
}


void Initialize_ADC(void) {
   // Configure P9.2 (A10) and P8.7 (A4) for analog tertiary function
   P9SEL1 |= Horizontal;
   P9SEL0 |= Horizontal;
   P8SEL1 |= Vertical;
   P8SEL0 |= Vertical;


   // Turn on ADC12 and disable conversions during configuration
   ADC12CTL0 |= ADC12ON;
   ADC12CTL0 &= ~ADC12ENC;


   // Sample-and-hold time & Multiple Sample and Conversion (sequence mode)
   ADC12CTL0 |= ADC12SHT0_2 | ADC12MSC;


   // Software trigger (ADC12SC), sampling timer, MODOSC, sequence-of-channels
   ADC12CTL1 |= (ADC12SHS_0 | ADC12SHP | ADC12SSEL_0 | ADC12CONSEQ_1);
   ADC12CTL1 &= ~(ADC12DIV_0);


   // 12-bit resolution (0–4095), unsigned binary format
   ADC12CTL2 |= (ADC12RES_2);
   ADC12CTL2 &= ~(ADC12DF);


   // Start sequence at MEM0
   ADC12CTL3 |= ADC12CSTARTADD_0;


   // MEM0 -> A10 (Horizontal)
   ADC12MCTL0 &= ~(ADC12VRSEL_0);
   ADC12MCTL0 |= (ADC12INCH_10);


   // MEM1 -> A4 (Vertical) + End of Sequence (EOS)
   ADC12MCTL1 &= ~(ADC12VRSEL_0);
   ADC12MCTL1 |= (ADC12INCH_4 | ADC12EOS);


   // Enable conversions
   ADC12CTL0 |= ADC12ENC;
}


void Initialize_Timer(void) {
   TA0CCR0 = 32767;
   TA0CCTL0 |= CCIE;
   TA0CTL = TASSEL_1 | MC_1 | TACLR;
}


// Intuitive cursor controls, including % 9 failsafe so we don't go past the board.
void cursor_update(board *board){
int steps = 0;
   switch (dir) {
       case left:
           do {
               cursor = (cursor_pos)((cursor + 8) % 9); // Step -1 with wrap (pos1 -> pos9)
               steps++;
           } while (board->squares[cursor].active == true && steps < 9);
           break;


       case right:
           do {
               cursor = (cursor_pos)((cursor + 1) % 9); // Step +1 with wrap (pos9 -> pos1)
               steps++;
           } while (board->squares[cursor].active == true && steps < 9);
           break;


       case up:
           do {
               cursor = (cursor_pos)((cursor + 6) % 9); // Step -3 in column with wrap (pos1->pos7, pos2->pos8, pos3->pos9)
               steps++;
           } while (board->squares[cursor].active == true && steps < 9);
           break;


       case down:
           do {
               cursor = (cursor_pos)((cursor + 3) % 9); // Step +3 in column with wrap (pos7->pos1, pos8->pos2, pos9->pos3)
               steps++;
           } while (board->squares[cursor].active == true && steps < 9);
           break;


       default:
           break;
   }
}


#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void) {
   blink = true;
}


void blink_cursor(board *Board, Graphics_Context *g_sContext, uint16_t width, uint16_t height) {
   blink = false;
   tRectangle rect = {
       .xMin = (Board->squares[cursor].x - (width / 6)) + 5,
       .yMin = (Board->squares[cursor].y - (width / 6)) + 5,
       .xMax = (Board->squares[cursor].x + (width / 6)) - 5,
       .yMax = (Board->squares[cursor].y + (width / 6)) - 5,
   };
   Graphics_drawRectangle(g_sContext, &rect);
   GrFlush(g_sContext);
}


void blink_cursor_off(board *Board, Graphics_Context *g_sContext, uint16_t width, uint16_t height) {
   blink = false;
   tRectangle rect = {
       .xMin = (Board->squares[cursor].x - (width / 6)) + 5,
       .yMin = (Board->squares[cursor].y - (width / 6)) + 5,
       .xMax = (Board->squares[cursor].x + (width / 6)) - 5,
       .yMax = (Board->squares[cursor].y + (width / 6)) - 5,
   };
   Graphics_setForegroundColor(g_sContext, GRAPHICS_COLOR_BLACK);
   Graphics_drawRectangle(g_sContext, &rect);
   Graphics_setForegroundColor(g_sContext, GRAPHICS_COLOR_WHITE);
   GrFlush(g_sContext);
}




void gridlines(uint16_t vline1, uint16_t vline2, uint16_t hline1, uint16_t hline2, uint16_t width, uint16_t height, Graphics_Context *g_sContext) {
   // Vertical lines
   GrLineDraw(g_sContext, vline1, 0, vline1, height - 1);
   GrLineDraw(g_sContext, vline2, 0, vline2, height - 1);


   // Horizontal lines
   GrLineDraw(g_sContext, 0, hline1, width - 1, hline1);
   GrLineDraw(g_sContext, 0, hline2, width - 1, hline2);
   GrFlush(g_sContext);
}


// move logic
void play_move(board *Board, Graphics_Context *g_sContext, uint16_t width, uint16_t height) {
   // If square is active (somehow) buzz to indicate you can't play here
   if (Board->squares[cursor].active == true) { buzz(); return; }


   // Turn off cursor
   blink_cursor_off(Board, g_sContext, width, height);


   // Shape logic
   if (Board->current_turn == shape_x) {
       drawX(Board, cursor, g_sContext, width, height);
       Board->squares[cursor].type = shape_x;
       Board->current_turn = shape_o;
   } else {
       drawO(Board, cursor, g_sContext, width);
       Board->squares[cursor].type = shape_o;
       Board->current_turn = shape_x;
   }
   // Mark square as active
   Board->squares[cursor].active = true;


   // wincheck
   Board->result = checkwin(Board, g_sContext);
   if (Board->result != result_none) {
       Board->banner = false;
       draw_win_banner(Board->result, g_sContext);
       buzz();
       return;
   }


   // move cursor to next playable square
   if (Board->squares[cursor].active == true) {
       dir = right;
       cursor_update(Board);
   }
   blink_cursor(Board, g_sContext, width, height);
}


// Setup PWM, but leave off
void Initialize_PWM(void) {
   // Pin Setup
   P2DIR |= BIT7;
   P2SEL0 |= BIT7;
   P2SEL1 &= ~BIT7;


   // 2 khz, 50% duty cycle = loud noise on passive buzzer
   TB0CCR0 = 7812;
   TB0CCTL6 = OUTMOD_7;
   TB0CCR6 = 3906;
   TB0CTL = TASSEL_2 | MC_0 | TBCLR;
}


// Toggle PWM generator
void buzz(void) {
   TB0CTL |= MC_1;
   __delay_cycles(1600000);
   TB0CTL &= ~MC_3;
}


void drawX(board *Board, uint8_t index, Graphics_Context *g_sContext, uint16_t width, uint16_t height) {
   uint16_t xMin = Board->squares[index].x - (width / 6) + 5;
   uint16_t yMin = Board->squares[index].y - (height / 6) + 5;
   uint16_t xMax = Board->squares[index].x + (width / 6) - 5;
   uint16_t yMax = Board->squares[index].y + (height / 6) - 5;


   Graphics_drawLine(g_sContext, xMin, yMin, xMax, yMax); // topleft -> bottomright
   Graphics_drawLine(g_sContext, xMax, yMin, xMin, yMax); // topright -> bottomleft
   GrFlush(g_sContext);
}


void drawO(board *Board, uint8_t index, Graphics_Context *g_sContext, uint16_t width) {
   uint16_t radius = (width / 6) - 5;
   Graphics_drawCircle(g_sContext, Board->squares[index].x, Board->squares[index].y, radius);
   GrFlush(g_sContext);
}


game_result checkwin(board *Board, Graphics_Context *g_sContext) {
   // For loop checks all possible wins
   uint8_t i;
   for (i = 0; i < 8; i++) {
       uint8_t a = wincon[i][0];
       uint8_t b = wincon[i][1];
       uint8_t c = wincon[i][2];


       // Type check
       shape s = Board->squares[a].type;
       if (s != shape_none && s == Board->squares[b].type && s == Board->squares[c].type) {
           // Draw line across the winning 3 squares
           Graphics_drawLine(g_sContext, Board->squares[a].x, Board->squares[a].y, Board->squares[c].x, Board->squares[c].y);
           GrFlush(g_sContext);


           // Game over
           Board->game_over = true;
           if ( s == shape_x ) { return  x_wins; }
           else { return o_wins; }
       }
   }
   // Check for draw
   uint8_t active_count = 0;
   for (i = 0; i < 9; i++) {
       if (Board->squares[i].active) {
           active_count++;
       }
   }
   if (active_count == 9) {
       Board->game_over = true;
       return draw;
   }


   return result_none;
}


void reset_board(board *Board, Graphics_Context *g_sContext, uint16_t vline1, uint16_t vline2, uint16_t hline1, uint16_t hline2, uint16_t width, uint16_t height) {
   // Reset board
   uint8_t i;
   for (i = 0; i < 9; i++) {
       Board->squares[i].active = false;
       Board->squares[i].type = shape_none;
   }
   // Reset Board parameters
   Board->current_turn = shape_x;
   Board->game_over = false;
   Board->result = result_none;
   Board->banner = false;
   cursor = pos1;


   // Clear display and draw gridlines
   Graphics_clearDisplay(g_sContext);
   gridlines(vline1, vline2, hline1, hline2, width, height, g_sContext);
   blink_cursor(Board, g_sContext, width, height);
}


void redraw_board(board *Board, Graphics_Context *g_sContext, uint16_t vline1, uint16_t vline2, uint16_t hline1, uint16_t hline2, uint16_t width, uint16_t height) {
   // Clear board
   Graphics_clearDisplay(g_sContext);
   gridlines(vline1, vline2, hline1, hline2, width, height, g_sContext);


   // Redraw shapes
   uint8_t i;
   for (i = 0; i < 9; i++) {
       if (Board->squares[i].type == shape_x) {
           drawX(Board, i, g_sContext, width, height);
       } else if (Board->squares[i].type == shape_o) {
           drawO(Board, i, g_sContext, width);
       }
   }


   // If the game is over, redraw strikethrough lines and banner
   if (Board->game_over) {
       checkwin(Board, g_sContext);
       if (!Board->banner) {
           draw_win_banner(Board->result, g_sContext);
       }
   } else {
       blink_cursor(Board, g_sContext, width, height);
   }
   GrFlush(g_sContext);


}


// Check for win, check for block, check for middle, check corner, check open square
void ai_move(board *Board, Graphics_Context *g_sContext, uint16_t width, uint16_t height) {
   // If the game is already over, don't play
   if (Board->game_over) return;


   int8_t chosen = -1;
   uint8_t i;


   // check for win, then check for blocking x. 
   shape targets[2] = { shape_o, shape_x };
   uint8_t t;
   for (t = 0; t < 2 && chosen == -1; t++) {
       for (i = 0; i < 8; i++) {
           uint8_t a = wincon[i][0], b = wincon[i][1], c = wincon[i][2];
           uint8_t match = (Board->squares[a].type == targets[t]) +
                           (Board->squares[b].type == targets[t]) +
                           (Board->squares[c].type == targets[t]);
           uint8_t empty = (!Board->squares[a].active) +
                           (!Board->squares[b].active) +
                           (!Board->squares[c].active);
           if (match == 2 && empty == 1) {
               if (!Board->squares[a].active) chosen = a;
               else if (!Board->squares[b].active) chosen = b;
               else chosen = c;
               break;
           }    
       } 
   }
   // hardcoded priority values, checks center, then corners, then other squares.
   const uint8_t priority[9] = { 4, 0, 2, 6, 8, 1, 3, 5, 7 };
   for (i = 0; i < 9 && chosen == -1; i++) {
       if (!Board->squares[priority[i]].active) {
           chosen = priority[i];
       }
   }


   // pass chosen
   if (chosen != -1) {
       blink_cursor_off(Board, g_sContext, width, height);
       cursor = (cursor_pos)chosen;
       play_move(Board, g_sContext, width, height);
   }
}


void draw_menu(board *Board, mode menu, Graphics_Context *g_sContext) {
   Graphics_clearDisplay(g_sContext);
   Graphics_drawStringCentered(g_sContext, "PVE", AUTO_STRING_LENGTH, 64, 68, OPAQUE_TEXT);
   Graphics_drawStringCentered(g_sContext, "PVP", AUTO_STRING_LENGTH, 64, 92, OPAQUE_TEXT);


   tRectangle box;
   if (menu == mode_pve) {
       box.xMin = 14;
       box.yMin = 60;
       box.xMax = 114;
       box.yMax = 76;
   } else {
       box.xMin = 14;
       box.yMin = 84;
       box.xMax = 114;
       box.yMax = 100;
   }
   Graphics_drawRectangle(g_sContext, &box);
   GrFlush(g_sContext);
}


// read joystick and return a direction (from lab 8 submission)
direction read_joystick(void){
   ADC12CTL0 |= ADC12SC;
   while ((ADC12CTL1 & ADC12BUSY) == ADC12BUSY) {}


   unsigned int horiz = ADC12MEM0;
   unsigned int vert  = ADC12MEM1;


   if (horiz < 1000) { return left; }
   else if (horiz > 3000) { return right; }
   else if (vert > 3000)  { return up; }
   else if (vert < 1000)  { return down; }
   else { return none; }
}

// draw a banner saying who won
void draw_win_banner(game_result result, Graphics_Context *g_sContext){
   tRectangle banner = {
       .xMin = 18,
       .yMin = 46,
       .xMax = 110,
       .yMax = 82
   };
   Graphics_setForegroundColor(g_sContext, GRAPHICS_COLOR_BLACK);
   Graphics_fillRectangle(g_sContext, &banner);
   Graphics_setForegroundColor(g_sContext, GRAPHICS_COLOR_WHITE);
   Graphics_drawRectangle(g_sContext, &banner);



    // Write text into the banner
   if (result == x_wins) {
       Graphics_drawStringCentered(g_sContext, "X WINS!", AUTO_STRING_LENGTH, 64, 58, OPAQUE_TEXT);
   } else if (result == o_wins) {
       Graphics_drawStringCentered(g_sContext, "O WINS!", AUTO_STRING_LENGTH, 64, 58, OPAQUE_TEXT);
   } else if (result == draw) {
       Graphics_drawStringCentered(g_sContext, "DRAW!", AUTO_STRING_LENGTH, 64, 58, OPAQUE_TEXT);
   }
   Graphics_drawStringCentered(g_sContext, "Press S1", AUTO_STRING_LENGTH, 64, 71, OPAQUE_TEXT);
   GrFlush(g_sContext);
}


void return_to_menu(board *Board, mode *menu_cursor, Graphics_Context *g_sContext) {
   // mark all squares as inactive
   uint8_t i;
   for (i = 0; i < 9; i++) {
       Board->squares[i].active = false;
       Board->squares[i].type = shape_none;
   }


   // Reset Board values
   Board->current_turn = shape_x;
   Board->saved_cursor = pos1;
   Board->gamemode = mode_none;
   Board->game_over = false;
   Board->result = result_none;
   Board->banner = false;


   // initiate the menu again
   cursor = pos1;
   *menu_cursor = mode_pve;
   draw_menu(Board, *menu_cursor, g_sContext);
}
