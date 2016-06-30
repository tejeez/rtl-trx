
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#define BAUDOT_START_BITS ((uint8_t) 0x00)
#define BAUDOT_STOP_BITS  ((uint8_t) 0xC0)

#define BAUDOT_FIGURES_SHIFT ((uint8_t) 0x1B)
#define BAUDOT_LETTERS_SHIFT ((uint8_t) 0x1F)

#ifndef BAUDOT_PREAMBLE_LENGTH
#define BAUDOT_PREAMBLE_LENGTH 2
#endif

#ifndef BAUDOT_PREAMBLE_SYMBOL
#define BAUDOT_PREAMBLE_SYMBOL BAUDOT_LETTERS_SHIFT // LTRS
#endif


#define BAUDOT_NONE       0
#define BAUDOT_LTRS       1
#define BAUDOT_FIGS       2
// BAUDOT_BOTH must be BAUDOT_LTRS | BAUDOT_FIGS
#define BAUDOT_BOTH       (BAUDOT_LTRS | BAUDOT_FIGS)
#define BAUDOT_LTRS_AFTER 4
#define BAUDOT_FIGS_AFTER 8


typedef struct {
	uint8_t bits;
	uint8_t shift;
} baudot_symbol_t_;

static const
baudot_symbol_t_ baudot_ccitt_from_ascii[UINT8_MAX] = {
	['\0'] = {0x00, BAUDOT_BOTH},

	['e']  = {0x01, BAUDOT_LTRS},
	['E']  = {0x01, BAUDOT_LTRS},
	['3']  = {0x01, BAUDOT_FIGS},

	['\n'] = {0x02, BAUDOT_BOTH},

	['a']  = {0x03, BAUDOT_LTRS},
	['A']  = {0x03, BAUDOT_LTRS},
	['-']  = {0x03, BAUDOT_FIGS},

	[' ']  = {0x04, BAUDOT_BOTH | BAUDOT_LTRS_AFTER},

	['s']  = {0x05, BAUDOT_LTRS},
	['S']  = {0x05, BAUDOT_LTRS},
	['\''] = {0x05, BAUDOT_FIGS},

	['i']  = {0x06, BAUDOT_LTRS},
	['I']  = {0x06, BAUDOT_LTRS},
	['8']  = {0x06, BAUDOT_FIGS},

	['u']  = {0x07, BAUDOT_LTRS},
	['U']  = {0x07, BAUDOT_LTRS},
	['7']  = {0x07, BAUDOT_FIGS},

	['\r'] = {0x08, BAUDOT_BOTH},

	['d']  = {0x09, BAUDOT_LTRS},
	['D']  = {0x09, BAUDOT_LTRS},
	// WRU / ENQ / "WHO ARE YOU"
	[5]    = {0x09, BAUDOT_FIGS},

	['r']  = {0x0A, BAUDOT_LTRS},
	['R']  = {0x0A, BAUDOT_LTRS},
	['4']  = {0x0A, BAUDOT_FIGS},

	['j']  = {0x0B, BAUDOT_LTRS},
	['J']  = {0x0B, BAUDOT_LTRS},
	['\a'] = {0x0B, BAUDOT_FIGS},

	['n']  = {0x0C, BAUDOT_LTRS},
	['N']  = {0x0C, BAUDOT_LTRS},
	[',']  = {0x0C, BAUDOT_FIGS},

	['f']  = {0x0D, BAUDOT_LTRS},
	['F']  = {0x0D, BAUDOT_LTRS},
	['!']  = {0x0D, BAUDOT_FIGS},

	['c']  = {0x0E, BAUDOT_LTRS},
	['C']  = {0x0E, BAUDOT_LTRS},
	[':']  = {0x0E, BAUDOT_FIGS},

	['k']  = {0x0F, BAUDOT_LTRS},
	['K']  = {0x0F, BAUDOT_LTRS},
	['(']  = {0x0F, BAUDOT_FIGS},

	['t']  = {0x10, BAUDOT_LTRS},
	['T']  = {0x10, BAUDOT_LTRS},
	['5']  = {0x10, BAUDOT_FIGS},

	['z']  = {0x11, BAUDOT_LTRS},
	['Z']  = {0x11, BAUDOT_LTRS},
	['+']  = {0x11, BAUDOT_FIGS},

	['l']  = {0x12, BAUDOT_LTRS},
	['L']  = {0x12, BAUDOT_LTRS},
	[')']  = {0x12, BAUDOT_FIGS},

	['w']  = {0x13, BAUDOT_LTRS},
	['W']  = {0x13, BAUDOT_LTRS},
	['2']  = {0x13, BAUDOT_FIGS},

	['h']  = {0x14, BAUDOT_LTRS},
	['H']  = {0x14, BAUDOT_LTRS},
	['#']  = {0x14, BAUDOT_FIGS},

	['y']  = {0x15, BAUDOT_LTRS},
	['Y']  = {0x15, BAUDOT_LTRS},
	['6']  = {0x15, BAUDOT_FIGS},

	['p']  = {0x16, BAUDOT_LTRS},
	['P']  = {0x16, BAUDOT_LTRS},
	['0']  = {0x16, BAUDOT_FIGS},

	['q']  = {0x17, BAUDOT_LTRS},
	['Q']  = {0x17, BAUDOT_LTRS},
	['1']  = {0x17, BAUDOT_FIGS},

	['o']  = {0x18, BAUDOT_LTRS},
	['O']  = {0x18, BAUDOT_LTRS},
	['9']  = {0x18, BAUDOT_FIGS},

	['b']  = {0x19, BAUDOT_LTRS},
	['B']  = {0x19, BAUDOT_LTRS},
	['?']  = {0x19, BAUDOT_FIGS},

	['g']  = {0x1A, BAUDOT_LTRS},
	['G']  = {0x1A, BAUDOT_LTRS},
	['&']  = {0x1A, BAUDOT_FIGS},

	// 0x1B is FIGURES SHIFT

	['m']  = {0x1C, BAUDOT_LTRS},
	['M']  = {0x1C, BAUDOT_LTRS},
	['.']  = {0x1C, BAUDOT_FIGS},

	['x']  = {0x1D, BAUDOT_LTRS},
	['X']  = {0x1D, BAUDOT_LTRS},
	['/']  = {0x1D, BAUDOT_FIGS},

	['v']  = {0x1E, BAUDOT_LTRS},
	['V']  = {0x1E, BAUDOT_LTRS},
	['=']  = {0x1E, BAUDOT_FIGS},

	// 0x1F is LETTERS SHIFT
};


static inline 
size_t bytes_required_for_word(uintmax_t word) {
	if (word < 0x100ULL) return 1;
	if (word < 0x10000ULL) return 2;
	if (word < 0x1000000ULL) return 3;
	if (word < 0x100000000ULL) return 4;
	if (word < 0x10000000000ULL) return 5;
	if (word < 0x1000000000000ULL) return 6;
	if (word < 0x100000000000000ULL) return 7;
	return 8;
}


static inline
uint16_t baudot_get_next(uint8_t ascii_letter, uint8_t *current_shift) {
	baudot_symbol_t_ symbol = baudot_ccitt_from_ascii[ascii_letter];

	uint16_t finished_symbol = 0;

	// Not found
	if (symbol.shift == BAUDOT_NONE) {
		return 0;
	}

	// Case/shift mismatch? Change case and insert shift symbol
	if ((symbol.shift & *current_shift) == 0) {
		if (symbol.shift == BAUDOT_LTRS) {
			finished_symbol |= BAUDOT_STOP_BITS | (BAUDOT_LETTERS_SHIFT << 1) | BAUDOT_START_BITS;
		} else {
			finished_symbol |= BAUDOT_STOP_BITS | (BAUDOT_FIGURES_SHIFT << 1) | BAUDOT_START_BITS;
		}
		*current_shift = symbol.shift;
		finished_symbol <<= 8;
	}

	// Actual symbol
	finished_symbol |= BAUDOT_STOP_BITS | (symbol.bits << 1) | BAUDOT_START_BITS;

	// Special case -- mostly used to handle space returning to letters
	if (symbol.shift & BAUDOT_LTRS_AFTER) {
		*current_shift = BAUDOT_LTRS;
	} else if (symbol.shift & BAUDOT_FIGS_AFTER) {
		*current_shift = BAUDOT_FIGS;
	}

	return finished_symbol;
}


ssize_t baudot_from_ascii(uint8_t *buffer, size_t buffer_size, char const *string) {
	if (!buffer)
		return -1;
	if (!string)
		return -1;

	size_t bytes_read = 0;
	size_t bytes_written = 0;
	uint8_t current_shift = BAUDOT_LTRS;

	if (BAUDOT_PREAMBLE_LENGTH > buffer_size) {
		return bytes_written;
	}
	memset(buffer,
		   BAUDOT_STOP_BITS | (BAUDOT_PREAMBLE_SYMBOL << 1) | BAUDOT_STOP_BITS,
		   BAUDOT_PREAMBLE_LENGTH);
	bytes_written += BAUDOT_PREAMBLE_LENGTH;

	while (string[bytes_read] != '\0') {
		uint16_t next_bits = baudot_get_next(string[bytes_read], &current_shift);
		ssize_t bytes_required = bytes_required_for_word(next_bits);
		if (bytes_written + bytes_required > buffer_size) {
			// Buffer full, writing ends here
			break;
		}
		// Possible shift character
		if (bytes_required > 1) {
			buffer[bytes_written] = (next_bits & 0xFF00) >> 8;
			bytes_written++;
		}
		// Actual character
		buffer[bytes_written] = next_bits & 0xFF;
		bytes_written++;
		bytes_read++;
	}

	return bytes_written;
}
