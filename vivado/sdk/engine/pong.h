#pragma once

#include "game_engine.h"

class PongTheGame;
//Forward declarations
void initializeGame(void);
void updateGame(void);
// Shapes 
class Paddle2D : public Rectangle2D {};
class Ball2D : public Rectangle2D {};
// Game Objects
class Paddle : public GameObject {
public:
    bool* upSignal;
    bool* downSignal;
    bool win;
    int speed = 10;
    u32 original_color;

    Paddle(Vec2<int> size, Vec2<int> position, uint32_t color)
        : GameObject({size, position, color}) {
            win = false;
            original_color = shape.color;
        }

    void update() override {
        if (upSignal && *upSignal) {
            if (shape.position.y <= 0) {
                return;
            }
            shape.position.y -= 1 * speed;
        }
        if (downSignal && *downSignal) {
            if (shape.position.y + shape.size.y >= 1080) {
                return;
            }
            shape.position.y += 1 * speed;
        }
        if (win) {
            static u32 count = 0;
            shape.color = RainbowRGB();
            if (count++ >= 180) {
                shape.color = original_color;
                count = 0;
                win = false;
            }
        }
    }
    Vec2<int> get_position() {
        return shape.position;
    }
    Vec2<int> get_size() {
        return shape.size;
    }
};

class Ball : public GameObject {
public:
    Vec2<int> velocity;
    int speed = 1;
    Paddle* paddle_1;
    Paddle* paddle_2;
    Ball(Vec2<int> size, Vec2<int> position, uint32_t color, Vec2<int> velocity, Paddle* paddle_1, Paddle* paddle_2)
        : GameObject({size, position, color}), velocity(velocity), paddle_1(paddle_1), paddle_2(paddle_2) {}

        bool start = true;

    void update() override {
        static u32 count = 0;
        if (start && count++ <= 180) {
            return;
        } else {
            count = 0;
            start = false;
        }

        velocity = velocity * speed;

        shape.position.x += velocity.x;
        shape.position.y += velocity.y;

        if (shape.position.y <= 0 || shape.position.y + shape.size.y >= 1080) {
            velocity.y = -velocity.y;
        }

        Vec2<int> paddle_1_pos = paddle_1->get_position();
        Vec2<int> paddle_2_pos = paddle_2->get_position();
        Vec2<int> paddle_1_size = paddle_1->get_size();
        Vec2<int> paddle_2_size = paddle_2->get_size();
        if (shape.position.x <= paddle_1_pos.x + paddle_1_size.x) {
            if (shape.position.y >= paddle_1_pos.y && shape.position.y <= paddle_1_pos.y + paddle_1->shape.size.y) {
                velocity.x = -velocity.x;
            }
        }
        if (shape.position.x + shape.size.x >= paddle_2_pos.x) {
            if (shape.position.y >= paddle_2_pos.y && shape.position.y <= paddle_2_pos.y + paddle_2->shape.size.y) {
                velocity.x = -velocity.x;
            }
        }

        if (shape.position.x <= 0) {
            shape.position = {1920/2, 1080/2};
            start = true;
            paddle_2->win = true;
        }
        if (shape.position.x + shape.size.x >= 1920) {
            shape.position = {1920/2, 1080/2};
            start = true;
            paddle_1->win = true;
        }
    }
};

class PongTheGame : public GameContext{ };
