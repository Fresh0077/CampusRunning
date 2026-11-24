#define _CRT_SECURE_NO_WARNINGS
#define WIN_SCORE 10
#define WIDTH 1012
#define HEIGHT 396
#define OBSTACLE_COUNT 10
#include<stdio.h>
#include<graphics.h>//引入图形库
#include<conio.h>//按键输入
#include"tools.h"//添加本地头文件
#include<vector>//引入c++库用于使用容器变长数组
using namespace std;//声明命名空间
//游戏初始化
IMAGE imgBgs[3];//全局变量，存放三重背景
int bgX[3];//背景图片的x坐标
int bgSpeed[3] = { 1,2,4 };//背景移动的速度
IMAGE imgHeros[12];//玩家奔跑动作的图片帧
int heroX;//玩家的x坐标
int heroY;
int heroIndex;//玩家奔跑的图片帧序号
bool heroJump;//玩家是否处于跳跃状态
int jumpHeightMax;//设置跳跃最大高度
int heroJumoOff;//跳跃偏移量
int update;//表示是否需要马上刷新
int heroBlood;//玩家血量
int score;//分数
typedef enum {
	TORTOISE,//乌龟
	LION,//狮子
	HOOK1,//钩子1
	HOOK2,//钩子2
	HOOK3,//钩子3
	HOOK4,//钩子4
	OBSTACLE_TYPE_COUNT//障碍物类型总数
}obstacle_type;
vector<vector<IMAGE>>obstacleImgs;//障碍物图片集合
typedef struct obstacle {
	int type;//障碍物的类型
	int imgIndex;//当前显示的图片的序号
	int x, y;//障碍物的坐标
	int speed;//速度
	int power;//杀伤力
	bool exist;//是否存在
	bool hited;//表示是否已经发生碰撞
	bool passed;//表示是否已经被通过
}obstacle_t;
obstacle_t obstacles[OBSTACLE_COUNT];//储存障碍物的数组
int lastObsIndex;
IMAGE imgHeroDown[2];//玩家下蹲照片
bool heroDown;//表示玩家是否处于下蹲状态
IMAGE imgSZ[10];
void init() {//加载资源，初始化全局变量
	//创建游戏窗口
	initgraph(WIDTH, HEIGHT);//页面大小
	char name[64];
	//加载游戏背景资源
	for (int i = 0; i < 3; i++) {
		sprintf(name, "res/bg%03d.png", i + 1);//宽3位不够前面补充0
		loadimage(&imgBgs[i], name);
		bgX[i] = 0;
		heroJump = false;
	}
	//加载玩家Hero奔跑的图片帧素材
	for (int i = 0; i < 12; i++) {
		sprintf(name, "res/hero%d.png", i + 1);
		loadimage(&imgHeros[i], name);
	}
	//设置玩家的初始位置
	heroX = WIDTH * 0.5 - imgHeros[0].getwidth() * 0.5;
	heroY = 345 - imgHeros[0].getheight();
	heroIndex = 0;
	heroJump = false;
	jumpHeightMax = 345 - imgHeros[0].getheight() - 120;
	heroJumoOff = -4;
	update = true;
	//用结构体加载小乌龟
	IMAGE imgTort;
	loadimage(&imgTort, "res/t1.png");
	vector<IMAGE>imgTortArray;
	imgTortArray.push_back(imgTort);
	obstacleImgs.push_back(imgTortArray);
	//加载狮子
	IMAGE imgLion;
	vector<IMAGE> imgLionArray;
	for (int i = 0; i < 6; i++) {
		sprintf(name, "res/p%d.png", i + 1);
		loadimage(&imgLion, name);
		imgLionArray.push_back(imgLion);
	}
	obstacleImgs.push_back(imgLionArray);
	//初始化障碍物池
	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		obstacles[i].exist = false;
	}
	//加载下蹲素材
	loadimage(&imgHeroDown[0], "res/d1.png");
	loadimage(&imgHeroDown[1], "res/d2.png");
	heroDown = false;
	IMAGE imgH;
	for (int i = 0; i < 4; i++) {
		vector<IMAGE>imgHookArray;
		sprintf(name, "res/h%d.png", i + 1);
		loadimage(&imgH, name, 60, 260, true);
		imgHookArray.push_back(imgH);
		obstacleImgs.push_back(imgHookArray);
	}
	heroBlood = 100;//初始化血量为满血
	//预加载音效
	preLoadSound("res/hit.mp3");
	mciSendString("play res/bg.mp3 repeat", 0, 0, 0);
	lastObsIndex = -1;
	score = 0;
	//加载数字图片
	for (int i = 0; i < 10; i++) {
		sprintf(name, "res/sz/%d.png", i);
		loadimage(&imgSZ[i], name);
	}
}
void createObstacle() {
	int i;
	for (i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist == false) {
			break;
		}
	}
	if (i >= OBSTACLE_COUNT) {
		return;
	}
	obstacles[i].exist = true;
	obstacles[i].hited = false;
	obstacles[i].imgIndex = 0;
	obstacles[i].type = (obstacle_type)(rand() % 3);
	if (lastObsIndex >= 0 &&
		obstacles[lastObsIndex].type >= HOOK1 &&
		obstacles[lastObsIndex].type <= HOOK4 &&
		obstacles[i].type == LION &&
		obstacles[lastObsIndex].x > (WIDTH - 500)) {
		obstacles[i].type = TORTOISE;
	}
	lastObsIndex = i;
	if (obstacles[i].type == HOOK1) {
		obstacles[i].type += rand() % 4; //0-3
	}
	obstacles[i].x = WIDTH;
	obstacles[i].y = 345 + 5 - obstacleImgs[obstacles[i].type][0].getheight();
	if (obstacles[i].type == TORTOISE) {
		obstacles[i].speed = 0;
		obstacles[i].power = 5;
	}
	else if (obstacles[i].type == LION) {
		obstacles[i].speed = 4;
		obstacles[i].power = 20;
	}
	else if (obstacles[i].type >= HOOK1 && obstacles[i].type <= HOOK4) {
		obstacles[i].speed = 0;
		obstacles[i].power = 20;
		obstacles[i].y = 0;
	}
	obstacles[i].passed = false;
}
void checkHit() {
	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist && obstacles[i].hited == false) {
			int alx, aly, a2x, a2y;
			int off = 30;
			if (!heroDown) {//非下蹲状态
				alx = heroX + off;
				aly = heroY + off;
				a2x = heroX + imgHeros[heroIndex].getwidth() - off;
				a2y = heroY + imgHeros[heroIndex].getheight();
			}
			else {
				alx = heroX + off;
				aly = 345 - imgHeroDown[heroIndex].getheight();
				a2x = heroX + imgHeroDown[heroIndex].getwidth() - off;
				a2y = 345;
			}
			IMAGE img = obstacleImgs[obstacles[i].type][obstacles[i].imgIndex];
			int blx = obstacles[i].x + off;
			int bly = obstacles[i].y + off;
			int b2x = obstacles[i].x + img.getwidth() - off;
			int b2y = obstacles[i].y + img.getheight() - 10;
			if (rectIntersect(alx, aly, a2x, a2y, blx, bly, b2x, b2y)) {
				heroBlood -= obstacles[i].power;
				printf("血量剩余 %d\n", heroBlood);
				playSound("res/hit.mp3");
				obstacles[i].hited = true;
			}
		}
	}
}
void fly() {
	for (int i = 0; i < 3; i++) {
		bgX[i] -= bgSpeed[i];
		if (bgX[i] < -WIDTH) {
			bgX[i] = 0;
		}
	}
	//实现跳跃
	if (heroJump) {
		if (heroY < jumpHeightMax) {//防止飞天
			heroJumoOff = 4;
		}
		heroY += heroJumoOff;//上升
		if (heroY > 345 - imgHeros[0].getheight()) {//防止遁地
			heroJump = false;
			heroJumoOff = -4;
		}
	}
	else if (heroDown) {
		static int count = 0;
		int delays[2] = { 8,30 };
		count++;
		if (count >= delays[heroIndex]) {
			count = 0;
			heroIndex++;
			if (heroIndex >= 2) {
				heroIndex = 0;
				heroDown = false;
			}
		}
	}
	else {//奔跑动画帧更新
		heroIndex = (heroIndex + 1) % 12;
	}
	//创建障碍物
	static int frameCount = 0;//静态变量永久有效
	static int enemyFre = 50;
	frameCount++;
	if (frameCount > enemyFre) {
		frameCount = 0;
		enemyFre = 50 + rand() % 50;//50-99随机数
		createObstacle();
	}
	//更新所有障碍物的坐标
	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist) {
			obstacles[i].x -= obstacles[i].speed + bgSpeed[2];
			if (obstacles[i].x < -obstacleImgs[obstacles[i].type][0].getwidth() * 2) {
				obstacles[i].exist = false;
			}
			int len = obstacleImgs[obstacles[i].type].size();
			obstacles[i].imgIndex = (obstacles[i].imgIndex + 1) % len;
		}
	}
	//玩家和障碍物的碰撞检测
	checkHit();
}
//渲染“游戏背景”
void updateBg() {
	putimagePNG2(bgX[0], 0, &imgBgs[0]);
	putimagePNG2(bgX[1], 119, &imgBgs[1]);
	putimagePNG2(bgX[2], 330, &imgBgs[2]);
}
void jump() {
	heroJump = true;
	update = true;
}
void down() {
	update = true;
	heroDown = true;
	heroIndex = 0;
}
//处理用户按键输入
void keyEvent() {
	
	if (GetAsyncKeyState(VK_UP))
	{
		jump();
	}
	if (GetAsyncKeyState(VK_DOWN))
	{
		down();
	}
}
void updateEnemy() {
	//渲染障碍物
	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist) {
			putimagePNG2(obstacles[i].x, obstacles[i].y, WIDTH, &obstacleImgs[obstacles[i].type][obstacles[i].imgIndex]);

		}
	}
}
void updateHero() {
	//渲染玩家
	if (!heroDown) {
		putimagePNG2(heroX, heroY, &imgHeros[heroIndex]);
	}
	else {
		int y = 345 - imgHeroDown[heroIndex].getheight();
		putimagePNG2(heroX, y, &imgHeroDown[heroIndex]);
	}
}
void updateBloodBar() {
	drawBloodBar(10, 10, 200, 10, 2, BLUE, DARKGRAY, RED, heroBlood / 100.0);
}
void checkOver() {
	if (heroBlood <= 0) {
		loadimage(0, "res/over.png");
		FlushBatchDraw();//刷新缓存
		mciSendString("stop res/bg.mp3", 0, 0, 0);
		mciSendString("play res/die.mp3", 0, 0, 0);
		system("pause");
		//暂停之后，复活或者开始下一局
		heroBlood = 100;
		score = 0;
		mciSendString("play res/bg.mp3 repeat", 0, 0, 0);
	}
}
void checkScore() {
	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist &&
			obstacles[i].passed == false &&
			obstacles[i].hited == false &&
			obstacles[i].x + obstacleImgs[obstacles[i].type][0].getwidth() < heroX) {
			score++;
			obstacles[i].passed = true;
			printf("score:%d\n", score);
		}
	}
}
void updateScore() {
	char str[8];
	sprintf(str, "%d", score);
	int x = 20;
	int y = 25;
	for (int i = 0; str[i]; i++) {
		int sz = str[i] - '0';
		putimagePNG(x, y, &imgSZ[sz]);
		x += imgSZ[sz].getwidth() + 5;
	}
}
void checkWin() {
	if (score >= WIN_SCORE) {
		loadimage(0, "res/win.png");
		FlushBatchDraw();
		mciSendString("play res/win.mp3", 0, 0, 0);
		mciSendString("stop res/bg.mp3", 0, 0, 0);
		system("pause");
		heroBlood = 100;
		score = 0;
		mciSendString("play res/bg.mp3 repeat", 0, 0, 0);
	}
}
int main(void) {
	init();
	loadimage(0, "res/begin.png");//初始画面
	system("pause");
	int timer = 0;
	while (1) {
		keyEvent();
		timer += getDelay();
		if (timer > 30) {
			timer = 0;
			update = true;
		}
		if (update) {//刷新页面
			update = false;
			BeginBatchDraw();//去除闪烁
			updateBg();
			updateHero();
			updateEnemy();
			updateBloodBar();
			updateScore();
			checkWin();
			EndBatchDraw();//去除闪烁
			checkOver();
			checkScore();
			fly();
		}
	}
	system("pause");
	return 0;
}