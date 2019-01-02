/*
    Diablo chess engine, Copyright 2006, Marcus Prewarski
          
    This program is free software. It may be copied or modified under the terms
    of the GNU General Public License version 2 as published by the Free Software 
    Foundation.  This program is distributed without any warranties whatsoever.
    See the file COPYING included with the distribution for details of the GNU general 
    public license.
 */
#define VERSION		"0.5.1"
#define NAME		"Diablo"

#define START_POSITION  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 2"
#define INFINITY        32767
#define PLY         	4       // fractional PLYs
#define MAX_DEPTH	60
#define MAX_SEARCHDEPTH	50
#define DEF_HASH_SIZE	4       // default hash size
#define MAX_HASH_SIZE   256     // max hash size
#define DRAW_SCORE	0
#define MAX_HIST_VALUE  256
#define MAX_HIST_SHIFT  8

// game flags
#define UCI_FLAG	1

// search flags
#define FLAG_ANALYZE     1
#define FLAG_PONDERING   2

#define WHITE           0
#define BLACK           1

// castle flags
#define WK_CASTLE       1
#define WQ_CASTLE       2
#define BK_CASTLE       4
#define BQ_CASTLE       8

// piece types
#define WPAWN		1
#define WKNIGHT		2
#define WBISHOP		3
#define WROOK		4
#define WQUEEN		5
#define WKING		6
#define XTRAPIECE1	7
#define XTRAPIECE2	8
#define BPAWN		9
#define BKNIGHT		10	
#define BBISHOP		11
#define BROOK		12
#define BQUEEN		13
#define BKING		14
#define MAX_PIECE	14
#define EMPTY		16
#define OFFBOARD	32

#define PAWN		1
#define KNIGHT		2
#define BISHOP		3
#define ROOK		4
#define QUEEN		5
#define KING		6

// move flags
#define KNIGHT_PROM     0x00010000
#define BISHOP_PROM     0x00020000 
#define ROOK_PROM       0x00040000
#define QUEEN_PROM     	0x00080000
#define PROMOTION     	0x000f0000
#define CAPTURE		0x00100000 
#define ENPASSANT	0x00200000
#define PAWNMOVE	0x00400000
#define CASTLE		0x00800000 

// piece values
#define PAWN_VAL	  100
#define KNIGHT_VAL	  330
#define BISHOP_VAL	  330
#define ROOK_VAL	  530
#define QUEEN_VAL	 1050
#define KING_VAL	10000

// game phase
#define OPENING_MAT     ((2*QUEEN_VAL)+(4*ROOK_VAL)+(4*BISHOP_VAL)+(4*KNIGHT_VAL)+(16*PAWN_VAL))
#define MIDGAME_MAT	((2*QUEEN_VAL)+(3*ROOK_VAL)+(2*BISHOP_VAL)+(12*PAWN_VAL))
#define ENDGAME_MAT	((2*QUEEN_VAL)+ROOK_VAL+PAWN_VAL)

// attack table flags
#define PAWN_ATK        0x08
#define KNIGHT_ATK      0x10
#define BISHOP_ATK	0x10
#define ROOK_ATK	0x20
#define QUEEN_ATK	0x40
#define KING_ATK	0x80

// attack potential flags
#define BPAWN_THREAT	0x01
#define WPAWN_THREAT	0x02
#define KNIGHT_THREAT	0x04
#define BISHOP_THREAT	0x08
#define ROOK_THREAT	0x10
#define QUEEN_THREAT	0x20
#define KING_THREAT	0x40

// macros
#define COLOR(p)	((p) >> 3)
#define RANK(s)		((s) >> 4)
#define COLUMN(s)	((s) & 0xf)
#define FROM(m)		(((m) >> 8) & 0xff)
#define TO(m)		((m) & 0xff)
#define Square(s)	(board.square[s])
#define SWAP(p, a, d)   quicksee_tab[(p)-1][((((int)a) << 8) | ((int)d)) & 0xffff]

#define PL_FIRST(p)	(board.plist[0xb0 | p].n)
#define PL_NEXT(s)	(board.plist[s].n)
#define PL_PREV(s)	(board.plist[s].p)

#define PL_INSERT(p, s)	do { \
    		PL_NEXT(s) = PL_FIRST(p);	\
		PL_PREV(s) = 0xb0 | p; 		\
		PL_PREV(PL_NEXT(s)) = s;	\
		PL_FIRST(p) = s;		\
	} while(0);
				
#define PL_REMOVE(s) do { \
		PL_NEXT(PL_PREV(s)) = PL_NEXT(s); \
		PL_PREV(PL_NEXT(s)) = PL_PREV(s); \
		PL_NEXT(s) = 0; PL_PREV(s) = 0; \
	} while(0);

#define PL_MOVE(x,y) do { \
		board.plist[y]=board.plist[x]; 	\
		PL_PREV(PL_NEXT(y)) = PL_NEXT(PL_PREV(y)) = y; \
		PL_NEXT(x) = 0; PL_PREV(x) = 0; \
	} while(0);


#define ML_FIRST	know_stack[ply].ms
#define ML_LAST		know_stack[ply+1].ms		

#define KSQ(c)		PL_FIRST(KING | ((c) << 3))

#define SET_ATTACK_TAB() do { \
   		attack_tab[0] = &allattack_tab[ply][0][64]; \
        	attack_tab[1] = &allattack_tab[ply][1][64]; \
	} while(0);


enum Square {
	A1=0, B1, C1, D1, E1, F1, G1, H1, I1, J1, K1, L1, M1, N1, O1, P1,
	A2, B2, C2, D2, E2, F2, G2, H2, I2, J2, K2, L2, M2, N2, O2, P2,
	A3, B3, C3, D3, E3, F3, G3, H3, I3, J3, K3, L3, M3, N3, O3, P3,
	A4, B4, C4, D4, E4, F4, G4, H4, I4, J4, K4, L4, M4, N4, O4, P4,
	A5, B5, C5, D5, E5, F5, G5, H5, I5, J5, K5, L5, M5, N5, O5, P5,
	A6, B6, C6, D6, E6, F6, G6, H6, I6, J6, K6, L6, M6, N6, O6, P6,
	A7, B7, C7, D7, E7, F7, G7, H7, I7, J7, K7, L7, M7, N7, O7, P7,
	A8, B8, C8, D8, E8, F8, G8, H8, I8, J8, K8, L8, M8, N8, O8, P8};

enum File { A_FILE, B_FILE, C_FILE, D_FILE, E_FILE, F_FILE, G_FILE, H_FILE };

typedef unsigned long long      HashKey;

#ifndef uint8_t
typedef unsigned char   uint8_t;
#endif
typedef unsigned char           uint8;
typedef signed char             int8;
typedef unsigned short          uint16;
typedef short                   int16;

typedef struct SearchStats {
    int ext;
    int prom;
    int pawn7;
    int single;
    int mthreat;
    int kt;
    int check;
    int simple;
    int iid_cnt;
} SearchStats;

typedef struct Attack {
    unsigned short type;
    short vector;
} Attack;

typedef struct Move {
	int move;
	int score;
} Move;

typedef struct MoveHist {
    int     move;
    HashKey hash_key;
} MoveHist;

typedef struct PieceList {
	uint8_t n;
	uint8_t p;
} PieceList;

typedef struct Undo {
    int 	enpassant;
    unsigned int castle;
    int 	fifty;
    int		capture;
    HashKey	hash;
} Undo;

typedef struct Knowledge {
	int	material;
	int	check;
	int	extend;
	int	king_threat[2];
	int     mate_threat[2];
	int	pawns[2];
	int	knights[2];
	int	bishops[2];
	int	rooks[2];
	int	queens[2];
	int	pieces[2];
	int 	move;
	int     killer1;
	int     killer2;
	int     mate_killer;
	Move   *ms;
	Move   *ml_last;
	Undo	undo;
} Knowledge;

typedef struct HashEntry
{
    HashKey	hkey;
    short	depth;
    short	score;
    unsigned int move;
} HashEntry;

typedef struct Board {
    uint8_t		all_squares[256];
    PieceList		piece_list[256];
    uint8_t            *square;
    PieceList	       *plist;
    unsigned int	castle;
    int			side;
    int			xside;
    int             	enpassant;
    int             	fifty;
    int             	gameply;
    HashKey		hash;
} Board;

// function prototypes
extern int generate_moves(void);
extern int generate_captures(void);
extern int generate_evasions(void);
extern int init_board(void);
extern int init_diablo(void);
extern int perft (int);
extern int readline(char *, int, FILE *);
extern int uci_command(char *);
extern int make_move(int);
extern int unmake_move(int);
extern int make_null_move(void);
extern int unmake_null_move(void);
extern int gtime(void);
extern int evaluate(void);
extern int init_search(void);
extern int update_pv(int );
extern int show_pv(int );
extern int show_update();
extern int think(void);
extern int is_input(void);
extern int stop_think(void);
extern int checkup(void);
extern int alloc_time(void);
extern int show_board(void);
extern int show_lists(void);
extern int show_search_stats(void);
extern int benchmark(void);
extern int see(int);
extern int quicksee(int);
extern int quickswap(int, int, int);
extern int isa_check(int );
extern int init_quicksee(void);


// global variables
extern Board board;
extern char inputline[4096];
extern MoveHist move_hist[];
extern int bishop_move_vec[];
extern int rook_move_vec[];
extern int queen_move_vec[];
extern int king_move_vec[];
extern int knight_move_vec[];
extern Move move_stack[];
extern Knowledge know_stack[];
extern int ply;
extern int nodes;
extern int bestmove;
extern int stop_search;
extern int game_flags;
extern int search_flags;
extern int idepth;
extern char move_str[];
extern int piece_value[];
extern int max_search_depth;
extern int start_time;
extern unsigned int stop_time;
extern int time_left;
extern int otime_left;
extern int moves_to_go;
extern int root_moves;
extern unsigned int maxhist;
extern int hash_entries;
extern int best_root_value;
extern HashKey zobrist_side[2];
extern HashKey zobrist_castle[16];
extern HashKey zobrist_board[15][128];
extern int history[2][128][128];
extern unsigned char distance[128];
extern unsigned char allattack_tab[MAX_DEPTH][2][256];
extern unsigned char *attack_tab[2];
extern int *piece_vec_table[];
extern SearchStats search_stats;
extern int threattype[18];
extern int promoted[9];
extern char quicksee_tab[6][0x10000];

extern Attack attack_opportunity_[256];
extern Attack *attack_opportunity;



