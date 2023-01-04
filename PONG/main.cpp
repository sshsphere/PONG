#include "../raylibcpp/include/raylib-cpp.hpp"
#include "../glm/glm/glm.hpp"
#include <iostream> 
#include <string>

constexpr int g_Width = 800;
constexpr int g_Height = 450;
constexpr float render_scale = 0.01f;
struct Arrow {
	raylib::Vector2 startPos{};
	raylib::Vector2 endPos{};
	float length{};
	float angle{};
	raylib::Color color{};
	void Draw() {
		float arrowangle = atan2(endPos.y - startPos.y, endPos.x - startPos.x) * RAD2DEG;
		float oang = 180 - angle;
		raylib::Vector2 lineend{ endPos + raylib::Vector2{cos(DEG2RAD * (oang + arrowangle)), sin(DEG2RAD * (oang + arrowangle))}*length };
		raylib::Vector2 lineend2{ endPos + raylib::Vector2{cos(DEG2RAD * (-oang + arrowangle)), sin(DEG2RAD * (-oang + arrowangle))}*length };
		startPos.DrawLine(endPos, 0.4f, color);
		endPos.DrawLine(lineend, 0.4f, color);
		endPos.DrawLine(lineend2, 0.4f, color);
		//Draw circles at ends of lines to make it prettier
		endPos.DrawCircle(0.2f, color);
		lineend.DrawCircle(0.2f, color);
		lineend2.DrawCircle(0.2f, color);
	}
};
struct Paddle {
	raylib::Vector2 position{};
	double speed{};
	double acceleration{};
	float width{};
	float height{};
	raylib::Rectangle GetRect() const
	{
		return { position.x - width / 2.0f, position.y - height / 2.0f, width, height };
	}
	void Draw() {
		GetRect().Draw(WHITE);
	}
};
struct Ball {
	raylib::Vector2 position{};
	raylib::Vector2 velocity{};
	float radius{};
	void Draw() {
		Arrow{ position,position + velocity / 15.0f,1.5f,45,WHITE }.Draw();
		position.DrawCircle(radius, WHITE);
	}
};
struct ScoreCounter {
	raylib::Vector2 position{};
	int score{};
	void Draw() {

		raylib::Text text{ GetFontDefault(),std::to_string(score),10.0f,1.0f };
		text.Draw({ position.x - text.Measure() / 2.0f ,position.y });
	}
};
struct Divider {
	raylib::Vector2 startPos;
	raylib::Vector2 endPos;
	int sphereCount{};
	float radius{};
	raylib::Color color{};
	void Draw() {
		raylib::Vector2 drawPos{ startPos };
		drawPos.DrawCircle(radius, color);
		endPos.DrawCircle(radius, color);
		if (!sphereCount)return;
		float stepX = (endPos.x - startPos.x) / static_cast<float>(sphereCount + 1);
		float stepY = (endPos.y - startPos.y) / static_cast<float>(sphereCount + 1);
		for (int i = 0; i < sphereCount; i++) {
			drawPos.x += stepX;
			drawPos.y += stepY;
			drawPos.DrawCircle(radius, color);
		}
	}

};
void processControlInput(raylib::Window& window) {
	static raylib::Vector2 previousSize = { g_Width,g_Height };
	if (IsKeyPressed(KEY_F11)) {
		if (window.IsFullscreen()) {
			window.SetFullscreen(false);
			window.SetSize(previousSize);
		}
		else {
			previousSize = window.GetSize();
			window.SetSize(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
			window.SetFullscreen(true);
		}
	}
}
void updateCameraPosition(const raylib::Window& window, raylib::Camera2D& cam) {
	if (!window.IsFullscreen()) {
		cam.SetOffset({ 0.5f * window.GetWidth(),0.5f * window.GetHeight() });
		cam.SetZoom(window.GetHeight() * render_scale);
	}
	else {
		cam.SetOffset({ 0.5f * GetMonitorWidth(GetCurrentMonitor()),0.5f * GetMonitorHeight(GetCurrentMonitor()) });
		cam.SetZoom(GetMonitorHeight(GetCurrentMonitor()) * render_scale);
	}
}
bool checkCollision(const Ball& ball, const Paddle& paddle) {

	return raylib::Rectangle{ ball.position.x - ball.radius,ball.position.y - ball.radius,ball.radius * 2,ball.radius * 2 }.CheckCollision(paddle.GetRect());
}
void simulatePlayer(Paddle& paddle, double deltatime, raylib::Vector2 arenasize, bool isPlayerOne) {
	constexpr double friction = 20.0f;
	KeyboardKey upkey{};
	KeyboardKey downkey{};
	if (isPlayerOne) {
		upkey = KEY_W;
		downkey = KEY_S;
	}
	else {
		upkey = KEY_UP;
		downkey = KEY_DOWN;
	}
	paddle.acceleration = 0;
	if (IsKeyDown(upkey))paddle.acceleration -= 3000;
	if (IsKeyDown(downkey))paddle.acceleration += 3000;
	paddle.acceleration -= paddle.speed * friction;
	paddle.position.y += deltatime * (paddle.speed + paddle.acceleration * deltatime * 0.5f); //s=ut+1/2at^2
	paddle.speed += paddle.acceleration * deltatime;//v=u+at
	if (paddle.GetRect().y + paddle.GetRect().height > arenasize.y)
	{
		paddle.position.y = arenasize.y - paddle.height / 2.0f;
		paddle.speed = 0;
	}
	else if (paddle.GetRect().y < -arenasize.y)
	{
		paddle.position.y = -arenasize.y + paddle.height / 2.0f;
		paddle.speed = 0;
	}
}
void simulateBall(Ball& ball, const Paddle& leftpaddle, const Paddle& rightpaddle, double deltatime, raylib::Vector2 arenasize, raylib::Sound& hitSound, raylib::Sound& scoreSound, ScoreCounter& scoreP1, ScoreCounter& scoreP2) {
	ball.position += ball.velocity * deltatime;
	const Paddle* targetPaddle{};
	if (checkCollision(ball, leftpaddle)) {
		targetPaddle = &leftpaddle;
	}
	else if (checkCollision(ball, rightpaddle)) {
		targetPaddle = &rightpaddle;
	}
	if (targetPaddle) {
		int side = (ball.velocity.x > 0 ? -1 : 1);
		ball.position.x = targetPaddle->position.x + side * (targetPaddle->width / 2.0f + ball.radius);
		ball.velocity.x *= -1;
		hitSound.PlayMulti();
		ball.velocity.y = (ball.position.y - targetPaddle->position.y) * 4 + targetPaddle->speed * 0.5f;
	}
	if (ball.position.y + ball.radius > arenasize.y)
	{
		ball.position.y = arenasize.y - ball.radius;
		ball.velocity.y *= -1;
		hitSound.PlayMulti();
	}
	if (ball.position.y - ball.radius < -arenasize.y)
	{
		ball.position.y = -arenasize.y + ball.radius;
		ball.velocity.y *= -1;
		hitSound.PlayMulti();
	}

	if (ball.position.x + ball.radius > arenasize.x)
	{
		ball.velocity.x *= -1;
		ball.velocity.y = 0;
		ball.position = { 0,0 };
		scoreSound.PlayMulti();
		scoreP1.score++;
	}
	else if (ball.position.x - ball.radius < -arenasize.x)
	{
		ball.velocity.x *= -1;
		ball.velocity.y = 0;
		ball.position = { 0,0 };
		scoreSound.PlayMulti();
		scoreP2.score++;
	}
}
void simulateGame(Paddle& leftpaddle, Paddle& rightpaddle, Ball& ball, double deltatime, raylib::Sound& hitSound, raylib::Sound& scoreSound, ScoreCounter& scoreP1, ScoreCounter& scoreP2) {
	raylib::Vector2 arenasize{ 85,50 };
	simulatePlayer(leftpaddle, deltatime, arenasize, 1);
	simulatePlayer(rightpaddle, deltatime, arenasize, 0);
	simulateBall(ball, leftpaddle, rightpaddle, deltatime, arenasize, hitSound, scoreSound, scoreP1, scoreP2);
}
//todo game context struct
int main() {
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	raylib::AudioDevice audioDevice{};
	static raylib::Window window(g_Width, g_Height, "PONG");
	HideCursor();

	raylib::Camera2D cam{};
	cam.SetTarget({ 0,0 });
	cam.SetOffset({ g_Width,g_Height });
	cam.SetRotation(0.0f);
	cam.SetZoom(1.0f);

	Ball ball{ {0,0},{100,0},1.0f };
	Paddle leftpaddle{ {-80,0}, 0, 0,5,24 };
	Paddle rightpaddle{ {80,0}, 0, 0,5,24 };
	ScoreCounter scoreP1{ {-10,-45} };
	ScoreCounter scoreP2{ {10,-45} };
	raylib::Sound scoreSound{ "b.wav" };
	raylib::Sound hitSound{ "c.wav" };
	scoreSound.SetVolume(0.4f);

	double deltatime = 1.0 / 60.0;
	double beginTime = GetTime();

	while (!window.ShouldClose())
	{
		//CONTROL INPUT
		processControlInput(window);
		//SIMULATE
		updateCameraPosition(window, cam);
		simulateGame(leftpaddle, rightpaddle, ball, deltatime, hitSound, scoreSound, scoreP1, scoreP2);
		//DRAW
		BeginDrawing();

		window.ClearBackground(SKYBLUE);
		//Draw in relative coordinates
		cam.BeginMode();

		ball.Draw();
		leftpaddle.Draw();
		rightpaddle.Draw();
		scoreP1.Draw();
		scoreP2.Draw();
		Divider{ {0, -45},{0,45},18,0.4f,WHITE }.Draw();
		//Divider{ {0, -45},{0,45},18,0.46f,{255,255,255,120} }.Draw();
		//DrawLine(0, -50, 0, 50, RAYWHITE);

		cam.EndMode();
		//Draw in screen coordinates
		EndDrawing();

		//Calculate delta time
		double endTime = GetTime();
		deltatime = endTime - beginTime;
		beginTime = endTime;
	}
}
