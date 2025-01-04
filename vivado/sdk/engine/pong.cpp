#include "pong.h"
#include "cl.h"

bool upSignal1 = false;
bool downSignal1 = false;
bool upSignal2 = false;
bool downSignal2 = false;

static PongTheGame* PingPong = nullptr;

void initializeGame() {

    // Set up game objects

    PingPong = new PongTheGame();
    auto paddle1 = new Paddle({50, 300}, {100, (1080/2)-(300/2)}, 0xFFFF0000);
    auto paddle2 = new Paddle({50, 300}, {1820, (1080/2)-(300/2)}, 0xFF0000FF);
    auto ball = new Ball({50, 50}, {1920/2, 1080/2}, 0xFFFFFFFF, {10, 10}, paddle1, paddle2);

    auto scene = new GameScene();
    scene->gameObjects.push_back(paddle1);
    scene->gameObjects.push_back(paddle2);
    scene->gameObjects.push_back(ball);

    PingPong->setScene(scene);

    // Set up input signals
    paddle1->upSignal = &upSignal1;
    paddle1->downSignal = &downSignal1;
    paddle2->upSignal = &upSignal2;
    paddle2->downSignal = &downSignal2;

}

void updateGame() {

    PingPong->update();

    PingPong->render();

}
