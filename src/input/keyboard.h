#ifndef KEYBOARD_H_
#define KEYBOARD_H_

namespace llt
{
	constexpr int MAX_TEXT_INPUT = 512;

	enum KeyboardKey
	{
		KEY_UNKNOWN					= 0,
		KEY_A						= 4,
		KEY_B						= 5,
		KEY_C						= 6,
		KEY_D						= 7,
		KEY_E						= 8,
		KEY_F						= 9,
		KEY_G						= 10,
		KEY_H						= 11,
		KEY_I						= 12,
		KEY_J						= 13,
		KEY_K						= 14,
		KEY_L						= 15,
		KEY_M						= 16,
		KEY_N						= 17,
		KEY_O						= 18,
		KEY_P						= 19,
		KEY_Q						= 20,
		KEY_R						= 21,
		KEY_S						= 22,
		KEY_T						= 23,
		KEY_U						= 24,
		KEY_V						= 25,
		KEY_W						= 26,
		KEY_X						= 27,
		KEY_Y						= 28,
		KEY_Z						= 29,
		KEY_D1						= 30,
		KEY_D2 						= 31,
		KEY_D3 						= 32,
		KEY_D4 						= 33,
		KEY_D5 						= 34,
		KEY_D6 						= 35,
		KEY_D7 						= 36,
		KEY_D8 						= 37,
		KEY_D9 						= 38,
		KEY_D0 						= 39,
		KEY_ENTER 					= 40,
		KEY_ESCAPE 					= 41,
		KEY_BACKSPACE 				= 42,
		KEY_TAB 					= 43,
		KEY_SPACE 					= 44,
		KEY_MINUS 					= 45,
		KEY_EQUALS 					= 46,
		KEY_LEFT_BRACKET 			= 47,
		KEY_RIGHT_BRACKET 			= 48,
		KEY_BACKSLASH 				= 49,
		KEY_NON_US_HASH 			= 50,
		KEY_SEMICOLON 				= 51,
		KEY_APOSTROPHE 				= 52,
		KEY_GRAVE 					= 53,
		KEY_COMMA 					= 54,
		KEY_PERIOD 					= 55,
		KEY_SLASH 					= 56,
		KEY_CAPSLOCK 				= 57,
		KEY_F1 						= 58,
		KEY_F2 						= 59,
		KEY_F3 						= 60,
		KEY_F4 						= 61,
		KEY_F5 						= 62,
		KEY_F6 						= 63,
		KEY_F7 						= 64,
		KEY_F8 						= 65,
		KEY_F9 						= 66,
		KEY_F10 					= 67,
		KEY_F11 					= 68,
		KEY_F12 					= 69,
		KEY_PRINT_SCREEN 			= 70,
		KEY_SCROLL_LOCK 			= 71,
		KEY_PAUSE 					= 72,
		KEY_INSERT 					= 73,
		KEY_HOME 					= 74,
		KEY_PAGE_UP 				= 75,
		KEY_DELETE 					= 76,
		KEY_END 					= 77,
		KEY_PAGE_DOWN 				= 78,
		KEY_RIGHT 					= 79,
		KEY_LEFT 					= 80,
		KEY_DOWN 					= 81,
		KEY_UP 						= 82,
		KEY_NUMLOCK_CLEAR 			= 83,
		KEY_KP_DIVIDE 				= 84,
		KEY_KP_MULTIPLY 			= 85,
		KEY_KP_MINUS 				= 86,
		KEY_KP_PLUS 				= 87,
		KEY_KP_ENTER 				= 88,
		KEY_KP_1 					= 89,
		KEY_KP_2 					= 90,
		KEY_KP_3 					= 91,
		KEY_KP_4 					= 92,
		KEY_KP_5 					= 93,
		KEY_KP_6 					= 94,
		KEY_KP_7 					= 95,
		KEY_KP_8 					= 96,
		KEY_KP_9 					= 97,
		KEY_KP_0 					= 98,
		KEY_KP_PERIOD 				= 99,
		KEY_NON_US_BACKSLASH 		= 100,
		KEY_APPLICATION 			= 101,
		KEY_POWER 					= 102,
		KEY_KP_EQUALS 				= 103,
		KEY_F13 					= 104,
		KEY_F14 					= 105,
		KEY_F15 					= 106,
		KEY_F16 					= 107,
		KEY_F17 					= 108,
		KEY_F18 					= 109,
		KEY_F19 					= 110,
		KEY_F20 					= 111,
		KEY_F21 					= 112,
		KEY_F22 					= 113,
		KEY_F23 					= 114,
		KEY_F24 					= 115,
		KEY_EXECUTE 				= 116,
		KEY_HELP 					= 117,
		KEY_MENU 					= 118,
		KEY_SELECT 					= 119,
		KEY_STOP 					= 120,
		KEY_AGAIN 					= 121,
		KEY_UNDO 					= 122,
		KEY_CUT 					= 123,
		KEY_COPY 					= 124,
		KEY_PASTE 					= 125,
		KEY_FIND 					= 126,
		KEY_MUTE 					= 127,
		KEY_VOLUME_UP 				= 128,
		KEY_VOLUME_DOWN 			= 129,
		KEY_KP_COMMA 				= 133,
		KEY_KP_EQUALS_AS_400 		= 134,
		KEY_INTERNATIONAL1 			= 135,
		KEY_INTERNATIONAL2 			= 136,
		KEY_INTERNATIONAL3 			= 137,
		KEY_INTERNATIONAL4 			= 138,
		KEY_INTERNATIONAL5 			= 139,
		KEY_INTERNATIONAL6 			= 140,
		KEY_INTERNATIONAL7 			= 141,
		KEY_INTERNATIONAL8 			= 142,
		KEY_INTERNATIONAL9 			= 143,
		KEY_LANGUAGE1 				= 144,
		KEY_LANGUAGE2 				= 145,
		KEY_LANGUAGE3 				= 146,
		KEY_LANGUAGE4 				= 147,
		KEY_LANGUAGE5 				= 148,
		KEY_LANGUAGE6 				= 149,
		KEY_LANGUAGE7 				= 150,
		KEY_LANGUAGE8 				= 151,
		KEY_LANGUAGE9 				= 152,
		KEY_ALT_ERASE 				= 153,
		KEY_SYS_REQ 				= 154,
		KEY_CANCEL 					= 155,
		KEY_CLEAR 					= 156,
		KEY_PRIOR 					= 157,
		KEY_RETURN2 				= 158,
		KEY_SEPERATOR 				= 159,
		KEY_OUT 					= 160,
		KEY_OPER 					= 161,
		KEY_CLEAR_AGAIN 			= 162,
		KEY_CRSEL 					= 163,
		KEY_EXSEL 					= 164,
		KEY_KP_00 					= 176,
		KEY_KP_000 					= 177,
		KEY_THOUSANDS_SEPERATOR 	= 178,
		KEY_DECIMAL_SEPERATOR 		= 179,
		KEY_CURRENCY_UNIT 			= 180,
		KEY_CURRENCY_SUBUNIT 		= 181,
		KEY_KP_LEFT_PARENT 			= 182,
		KEY_KP_RIGHT_PARENT 		= 183,
		KEY_KP_LEFT_BRACE 			= 184,
		KEY_KP_RIGHT_BRACE 			= 185,
		KEY_KP_TAB 					= 186,
		KEY_KP_BACKSPACE 			= 187,
		KEY_KP_A 					= 188,
		KEY_KP_B 					= 189,
		KEY_KP_C 					= 190,
		KEY_KP_D 					= 191,
		KEY_KP_E 					= 192,
		KEY_KP_F 					= 193,
		KEY_KP_XOR 					= 194,
		KEY_KP_POWER 				= 195,
		KEY_KP_PERCENT 				= 196,
		KEY_KP_LESS 				= 197,
		KEY_KP_GREATER 				= 198,
		KEY_KP_AMPERSAND 			= 199,
		KEY_KP_DOUBLE_AMPERSAND 	= 200,
		KEY_KP_VERTICAL_BAR 		= 201,
		KEY_KP_DOUBLE_VERTICAL_BAR 	= 202,
		KEY_KP_COLON 				= 203,
		KEY_KP_HASH 				= 204,
		KEY_KP_SPACE 				= 205,
		KEY_KP_AT 					= 206,
		KEY_KP_EXCLAM 				= 207,
		KEY_KP_MEM_STORE 			= 208,
		KEY_KP_MEM_RECALL 			= 209,
		KEY_KP_MEM_CLEAR 			= 210,
		KEY_KP_MEM_ADD 				= 211,
		KEY_KP_MEM_SUBTRACT 		= 212,
		KEY_KP_MEM_MULTIPLY 		= 213,
		KEY_KP_MEM_DIVIDE 			= 214,
		KEY_KP_PLUS_MINUS 			= 215,
		KEY_KP_CLEAR 				= 216,
		KEY_KP_CLEAR_ENTRY 			= 217,
		KEY_KP_BINARY 				= 218,
		KEY_KP_OCTAL 				= 219,
		KEY_KP_DECIMAL 				= 220,
		KEY_KP_HEXADECIMAL 			= 221,
		KEY_LEFT_CONTROL 			= 224,
		KEY_LEFT_SHIFT 				= 225,
		KEY_LEFT_ALT 				= 226,
		KEY_LEFT_SUPER 				= 227,
		KEY_RIGHT_CONTROL 			= 228,
		KEY_RIGHT_SHIFT 			= 229,
		KEY_RIGHT_ALT 				= 230,
		KEY_RIGHT_SUPER 			= 231,
		KEY_MAX_ENUM
	};
}

#endif // KEYBOARD_H_