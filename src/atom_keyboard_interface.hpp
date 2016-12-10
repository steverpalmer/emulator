// atom_keyboard_interface.hpp
// Copyright 2016 Steve Palmer

#ifndef ATOM_KEYBOARD_INTERFACE_HPP_
#define ATOM_KEYBOARD_INTERFACE_HPP_

#include <set>

#include "common.hpp"

namespace Atom
{

	class KeyboardInterface
		: protected NonCopyable
	{
	public:
		enum class Key
		{
			SPACE,
			LEFTBRACKET,
			BACKSLASH,
			RIGHTBRACKET,
			CIRCUMFLEX,
			LOCK,
			LEFT_RIGHT,
			UP_DOWN,
			SPARE1,
			SPARE2,
			SPARE3,
			SPARE4,
			SPARE5,
			RETURN,
			COPY,
			DELETE,
			_0,
			_1_EXCLAMATION,
			_2_DOUBLEQUOTE,
			_3_NUMBER,
			_4_DOLLAR,
			_5_PERCENT,
			_6_AMPERSAND,
			_7_SINGLEQUOTE,
			_8_LEFTPARENTHESIS,
			_9_RIGHTPARENTHESIS,
			COLON_ASTERISK,
			SEMICOLON_PLUS,
			COMMA_LESSTHAN,
			MINUS_EQUALS,
			PERIOD_GREATERTHAN,
			SLASH_QUESTION,
			AT,
			A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
			ESCAPE,
			CTRL = 0x80,
			SHIFT,
			REPT,
			BREAK = 0xC0
		};

	protected:
		KeyboardInterface() = default;
	public:
		virtual ~KeyboardInterface() = default;
		virtual void down(Key) = 0;
		virtual void up(Key) = 0;

	};

}

#endif
