#pragma once

#include "../Core/Vector.h"
#include <string>
#include <vector>
#include <windows.h>

using namespace Veena;

struct LineEditState
{
	std::wstring mString;
	std::u32string mLineShaped;
	int mCursourPos = 0;
	int mSelectionStart;
	int mSelectionEnd;

	void KeyPress(int key)
	{
		bool bIsShiftDown;

		switch (key)
		{
		case VK_RIGHT:
			MoveCursor(+1);
			break;
		case VK_LEFT:
			MoveCursor(-1);
			break;
		case VK_END:
		{
			mCursourPos = mString.size() - 1;
			break;
		};
		case VK_HOME:
		{
			mCursourPos = 0;
			break;
		};
		default:
			break;
		}

	}
	void MoveCursor(int value)
	{
		if (mCursourPos != -1)
		{
			mCursourPos = VClamp(mCursourPos + value, 0, mString.size());
		}
	}

};

struct SingleLineText
{
	std::string mUTF8String;
	std::u32string mShapedUnicodeString;

	SingleLineText(const char* utf8Str);
	void Shape();
};