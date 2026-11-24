#define _CRT_SECURE_NO_WARNINGS

#include "tools.h"

#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

// 返回距离上一次调用经过的毫秒数
int getDelay() {
	static ULONGLONG last = GetTickCount64();
	ULONGLONG now = GetTickCount64();
	int delay = (int)(now - last);
	last = now;
	return delay;
}

// 将图片按 alpha 通道混合到当前绘图缓冲区
// clipWidth 用于限制绘制区域宽度，<=0 表示整个窗口
static void blendImage(int x, int y, int clipWidth, IMAGE* picture) {
	if (picture == NULL) {
		return;
	}

	DWORD* dst = GetImageBuffer();
	DWORD* src = GetImageBuffer(picture);
	if (dst == NULL || src == NULL) {
		return;
	}

	const int picWidth = picture->getwidth();
	const int picHeight = picture->getheight();
	const int screenWidth = getwidth();
	const int screenHeight = getheight();
	const int limitWidth = (clipWidth > 0 && clipWidth < screenWidth) ? clipWidth : screenWidth;

	if (x >= limitWidth || y >= screenHeight) {
		return;
	}

	// 超出窗口的部分直接裁掉，避免越界读写
	int startX = x < 0 ? -x : 0;
	int startY = y < 0 ? -y : 0;
	int endX = picWidth;
	int endY = picHeight;
	if (x + endX > limitWidth) {
		endX = limitWidth - x;
	}
	if (y + endY > screenHeight) {
		endY = screenHeight - y;
	}

	for (int iy = startY; iy < endY; iy++) {
		for (int ix = startX; ix < endX; ix++) {
			DWORD color = src[iy * picWidth + ix];
			int sa = (int)((color >> 24) & 0xFF);
			if (sa == 0) {
				continue;
			}

			DWORD* pixel = dst + (y + iy) * screenWidth + (x + ix);
			if (sa == 255) {
				*pixel = color | 0xFF000000;
				continue;
			}

			int sr = (color >> 16) & 0xFF;
			int sg = (color >> 8) & 0xFF;
			int sb = color & 0xFF;
			int dr = (*pixel >> 16) & 0xFF;
			int dg = (*pixel >> 8) & 0xFF;
			int db = *pixel & 0xFF;

			// Cp = alpha * Fp + (1 - alpha) * Bp
			int r = (sr * sa + dr * (255 - sa)) / 255;
			int g = (sg * sa + dg * (255 - sa)) / 255;
			int b = (sb * sa + db * (255 - sa)) / 255;
			*pixel = 0xFF000000 | (r << 16) | (g << 8) | b;
		}
	}
}

// 带透明度的贴图（不允许超出窗口左上角）
void putimagePNG(int x, int y, IMAGE* picture) {
	blendImage(x, y, 0, picture);
}

// 带透明度的贴图（支持负坐标，用于滚动背景）
void putimagePNG2(int x, int y, IMAGE* picture) {
	blendImage(x, y, 0, picture);
}

// 带透明度的贴图（限制在指定宽度内绘制）
void putimagePNG2(int x, int y, int winWidth, IMAGE* picture) {
	blendImage(x, y, winWidth, picture);
}

// 判断两个矩形是否相交
bool rectIntersect(int a1X, int a1Y, int a2X, int a2Y,
	int b1X, int b1Y, int b2X, int b2Y) {
	if (a1X > a2X) {
		int t = a1X; a1X = a2X; a2X = t;
	}
	if (a1Y > a2Y) {
		int t = a1Y; a1Y = a2Y; a2Y = t;
	}
	if (b1X > b2X) {
		int t = b1X; b1X = b2X; b2X = t;
	}
	if (b1Y > b2Y) {
		int t = b1Y; b1Y = b2Y; b2Y = t;
	}
	return !(a2X < b1X || a2Y < b1Y || b2X < a1X || b2Y < a1Y);
}

// 根据文件路径生成一个合法的 MCI 设备别名
static void makeSoundAlias(const char* name, char* alias, int capacity) {
	int j = 0;
	for (int i = 0; name[i] != '\0' && j < capacity - 1; i++) {
		unsigned char c = (unsigned char)name[i];
		alias[j++] = (isalnum(c) || c == '_') ? (char)c : '_';
	}
	alias[j] = '\0';
}

// 预加载音效，避免第一次播放时卡顿
void preLoadSound(const char* name) {
	if (name == NULL) {
		return;
	}
	char alias[128];
	makeSoundAlias(name, alias, sizeof(alias));

	char cmd[512];
	sprintf(cmd, "open \"%s\" alias %s", name, alias);
	mciSendStringA(cmd, NULL, 0, NULL);
}

// 从头播放音效
void playSound(const char* name) {
	if (name == NULL) {
		return;
	}
	char alias[128];
	makeSoundAlias(name, alias, sizeof(alias));

	char cmd[512];
	sprintf(cmd, "seek %s to start", alias);
	mciSendStringA(cmd, NULL, 0, NULL);
	sprintf(cmd, "play %s", alias);
	mciSendStringA(cmd, NULL, 0, NULL);
}

// 绘制血条：外框 + 空槽 + 按百分比填充
void drawBloodBar(int x, int y, int width, int height, int lineWidth,
	int boardColor, int emptyColor, int fillColor, float percent) {
	if (width <= 0 || height <= 0) {
		return;
	}
	if (percent < 0.0f) {
		percent = 0.0f;
	}
	if (percent > 1.0f) {
		percent = 1.0f;
	}
	if (lineWidth < 1) {
		lineWidth = 1;
	}

	setlinestyle(PS_SOLID, lineWidth);
	setlinecolor(boardColor);
	setfillcolor(emptyColor);
	fillrectangle(x, y, x + width, y + height);

	int left = x + lineWidth;
	int top = y + lineWidth;
	int right = x + width - lineWidth;
	int bottom = y + height - lineWidth;
	int innerWidth = right - left;
	if (innerWidth <= 0 || bottom <= top) {
		return;
	}

	int fillRight = left + (int)(innerWidth * percent + 0.5f);
	if (fillRight > left) {
		setfillcolor(fillColor);
		solidrectangle(left, top, fillRight, bottom);
	}
}
