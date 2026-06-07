#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <functional>

using namespace sf;
using namespace std;

// Forward declarations
class Player;
class Enemy;
class NPC;
class Bullet;
class TaskArea;
class HealthSystem;
class GameState;
class AnimatedSprite;
class StaticSprite;

// Constants (keep your existing constants)
const float NPC_SPEED = 100.f;
const float NPC_DIRECTION_CHANGE_INTERVAL = 2.f;
const float CHARACTER_HEIGHT = 60.f;
const float MOVE_SPEED = 200.f;
const float CAMERA_LERP_FACTOR = 0.08f;
const double FIXED_TIME_STEP = 1.0 / 60.0;
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int MAP_WIDTH = 4096;
const int MAP_HEIGHT = 2048;
const float CAMERA_ZOOM = 0.5f;
const float ENEMY_SPEED = 80.f;
const float ENEMY_DETECTION_RANGE = 300.f;
const float ENEMY_ATTACK_RANGE = 50.f;
const float ENEMY_ATTACK_COOLDOWN = 1.5f;
const int MAX_HEALTH_BLOCKS = 5;
const float TASK_HOLD_TIME = 3.0f;
const float WEAPON_RANGE = 80.f;
const float WEAPON_ATTACK_COOLDOWN = 0.8f;
const int ENEMY_MAX_HEALTH = 3;
const float BULLET_SPEED = 600.f;
const float BULLET_FIRE_RATE = 0.25f; 

// Forward declaration
Image collisionMap;
bool isColliding(Vector2f position, const Image& map);

// Game state
bool hasWeapon = false;
float weaponAttackTimer = 0.0f;


bool isColliding(Vector2f position, const Image& map) {
    if (map.getSize().x == 0 || map.getSize().y == 0) {

        return true;
    }

    float scaleX = static_cast<float>(map.getSize().x) / MAP_WIDTH;
    float scaleY = static_cast<float>(map.getSize().y) / MAP_HEIGHT;

    unsigned int x = static_cast<unsigned int>(position.x * scaleX);
    unsigned int y = static_cast<unsigned int>(position.y * scaleY);

    if (x >= map.getSize().x || y >= map.getSize().y)
        return true;

    Color pixel = map.getPixel(x, y);
    return (pixel.r == 0 && pixel.g == 0 && pixel.b == 0);
}




// Fast distance calculation for optimization
float fastDistance(Vector2f a, Vector2f b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
}


// Task Area class
class TaskArea {
public:
    TaskArea(Vector2f position, float radius, const string& taskMessage, function<void()> onComplete = nullptr)
        : position(position), radius(radius), taskMessage(taskMessage),
        taskCompleted(false), isPlayerInArea(false), holdProgress(0.0f),
        onCompleteCallback(onComplete) {
    }

    void update(float dt, Vector2f playerPos) {
        float distance = sqrt(pow(playerPos.x - position.x, 2) + pow(playerPos.y - position.y, 2));
        isPlayerInArea = distance <= radius;

        if (isPlayerInArea && !taskCompleted) {
            if (Keyboard::isKeyPressed(Keyboard::X)) {
                holdProgress += dt;
                if (holdProgress >= TASK_HOLD_TIME) {
                    taskCompleted = true;


                    if (onCompleteCallback) onCompleteCallback();

                }

            }
            else {
                holdProgress = 0.0f;
            }
        }
    }

    void draw(RenderWindow& window) {
        if (taskCompleted) return;

        CircleShape areaCircle(radius);
        areaCircle.setPosition(position.x - radius, position.y - radius);
        areaCircle.setFillColor(Color(255, 255, 0, 50));
        areaCircle.setOutlineColor(Color::Yellow);
        areaCircle.setOutlineThickness(3.0f);
        window.draw(areaCircle);

        CircleShape marker(15.0f);
        marker.setPosition(position.x - 15.0f, position.y - 30.0f);
        marker.setFillColor(Color::Red);
        window.draw(marker);
    }

    void drawUI(RenderWindow& window, const Font& font) const {
        if (isPlayerInArea && !taskCompleted) {
            View originalView = window.getView();
            View uiView(FloatRect(0, 0, window.getSize().x, window.getSize().y));
            window.setView(uiView);

            Text message(taskMessage, font, 24);
            message.setFillColor(Color::White);
            message.setPosition(window.getSize().x / 2 - message.getGlobalBounds().width / 2,
                window.getSize().y - 150);
            window.draw(message);

            float barWidth = 200.0f;
            float barHeight = 20.0f;
            float progress = holdProgress / TASK_HOLD_TIME;

            RectangleShape progressBg(Vector2f(barWidth, barHeight));
            progressBg.setPosition(window.getSize().x / 2 - barWidth / 2, window.getSize().y - 100);
            progressBg.setFillColor(Color(50, 50, 50));
            progressBg.setOutlineColor(Color::White);
            progressBg.setOutlineThickness(2.0f);
            window.draw(progressBg);

            RectangleShape progressBar(Vector2f(barWidth * progress, barHeight));
            progressBar.setPosition(window.getSize().x / 2 - barWidth / 2, window.getSize().y - 100);
            progressBar.setFillColor(Color::Green);
            window.draw(progressBar);

            Text holdText("Hold X to perform task", font, 16);
            holdText.setFillColor(Color::White);
            holdText.setPosition(window.getSize().x / 2 - holdText.getGlobalBounds().width / 2,
                window.getSize().y - 70);
            window.draw(holdText);

            window.setView(originalView);
        }
    }

    void drawTaskList(RenderWindow& window, const Font& font, int& lineIndex)  const {
        const float x = 20.f;
        const float baseY = 100.f;
        const float lineSpacing = 30.f;

        // Draw background only once on first task
        if (lineIndex == 0) {
            const int totalLines = 8; // You have 6 tasks
            float bgHeight = totalLines * lineSpacing + 20.f;
            float bgWidth = 300.f;

            RectangleShape bg(Vector2f(bgWidth, bgHeight));
            bg.setPosition(x - 10.f, baseY);
            bg.setFillColor(Color(200, 220, 255, 190)); // translucent slightly bluish white
            bg.setOutlineColor(Color::White);
            bg.setOutlineThickness(2.f);
            window.draw(bg);
        }

        // Draw task text with status
        Text taskText;
        taskText.setFont(font);
        taskText.setCharacterSize(22);
        taskText.setPosition(x, baseY + lineIndex * lineSpacing);

        if (taskCompleted) {
            taskText.setString("Done:" + taskMessage);
            taskText.setFillColor(Color::Green);
        }
        else {
            taskText.setString("- " + taskMessage);
            taskText.setFillColor(Color::Red);
        }

        window.draw(taskText);
        lineIndex++;
    }


    bool isCompleted() const { return taskCompleted; }

private:
    Vector2f position;
    float radius;
    string taskMessage;
    bool taskCompleted;
    bool isPlayerInArea;
    float holdProgress;
    function<void()> onCompleteCallback;
};

// Optimized AnimatedSprite class
class AnimatedSprite : public Drawable, public Transformable {
public:
    AnimatedSprite(const Texture& texture, int frameWidth, int frameHeight, int frameCount, float frameTime)
        : m_texture(texture), m_frameWidth(frameWidth), m_frameHeight(frameHeight),
        m_frameCount(frameCount), m_frameTime(frameTime), m_currentFrame(0), m_elapsedTime(0.f) {
        m_sprite.setTexture(m_texture);
        updateFrame();

        // Scale sprite to desired character height
        float scaleFactor = CHARACTER_HEIGHT / static_cast<float>(m_frameHeight);
        m_sprite.setScale(scaleFactor, scaleFactor);
        m_sprite.setOrigin(m_frameWidth / 2.f, m_frameHeight / 2.f);
    }

    FloatRect getGlobalBounds() const {
        return getTransform().transformRect(m_sprite.getGlobalBounds());
    }

    void update(float dt) {
        m_elapsedTime += dt;

        // Smoothly update frame based on elapsed time
        while (m_elapsedTime >= m_frameTime) {
            m_elapsedTime -= m_frameTime;
            m_currentFrame = (m_currentFrame + 1) % m_frameCount;
            updateFrame();
        }
    }

    void reset() {
        m_currentFrame = 0;
        m_elapsedTime = 0.f;
        updateFrame();
    }

    void setPosition(Vector2f pos) {
        m_sprite.setPosition(pos);
    }

    Vector2f getPosition() const {
        return m_sprite.getPosition();
    }

    void setScale(float scaleX, float scaleY) {
        m_sprite.setScale(scaleX, scaleY);
    }
    const Texture& getTexture() const {
        return m_texture;
    }


private:
    void updateFrame() {
        int left = m_currentFrame * m_frameWidth;
        m_sprite.setTextureRect(IntRect(left, 0, m_frameWidth, m_frameHeight));
    }

    virtual void draw(RenderTarget& target, RenderStates states) const override {
        states.transform *= getTransform();
        target.draw(m_sprite, states);
    }








private:
    const Texture& m_texture;
    Sprite m_sprite;
    int m_frameWidth;
    int m_frameHeight;
    int m_frameCount;
    float m_frameTime;
    int m_currentFrame;
    float m_elapsedTime;
};

// StaticSprite class for non-animated sprites (like idle)
class StaticSprite : public Drawable, public Transformable {
public:
    StaticSprite(const Texture& texture) : m_texture(texture) {
        m_sprite.setTexture(m_texture);

        // Scale sprite to desired character height
        float scaleFactor = CHARACTER_HEIGHT / static_cast<float>(m_texture.getSize().y);
        m_sprite.setScale(scaleFactor, scaleFactor);
        m_sprite.setOrigin(m_texture.getSize().x / 2.f, m_texture.getSize().y / 2.f);
    }

    FloatRect getGlobalBounds() const {
        return getTransform().transformRect(m_sprite.getGlobalBounds());
    }

    void setPosition(Vector2f pos) {
        m_sprite.setPosition(pos);
    }

    Vector2f getPosition() const {
        return m_sprite.getPosition();
    }

    void setScale(float scaleX, float scaleY) {
        m_sprite.setScale(scaleX, scaleY);
    }

private:
    virtual void draw(RenderTarget& target, RenderStates states) const override {
        states.transform *= getTransform();
        target.draw(m_sprite, states);
    }

private:
    const Texture& m_texture;
    Sprite m_sprite;
};

// Optimized NPC class
class NPC {
public:
    NPC(const Texture& texture, Vector2f startPos, bool collideOnBlackOnly, float npcSize = CHARACTER_HEIGHT)
        : animatedSprite(texture, texture.getSize().x / 7, texture.getSize().y, 7, 0.1f),
        position(startPos),
        timer(0.f),
        restrictToWhite(collideOnBlackOnly),
        npcHeight(npcSize),
        facingLeft(false)
    {
        direction = getRandomDirection();
        animatedSprite.setPosition(position);

        // Set initial scale properly and store it
        float frameHeight = static_cast<float>(texture.getSize().y);
        baseScale = npcHeight / frameHeight;
        animatedSprite.setScale(baseScale, baseScale);
    }

    void update(float dt, const Image& collisionMap) {
        timer += dt;
        if (timer >= NPC_DIRECTION_CHANGE_INTERVAL) {
            timer = 0.f;
            direction = getRandomDirection();
        }

        Vector2f proposedPosition = position + direction * NPC_SPEED * dt;

        if (isValidMove(proposedPosition, collisionMap)) {
            position = proposedPosition;
            animatedSprite.setPosition(position);
            animatedSprite.update(dt);

            // Only update sprite flipping when horizontal direction changes
            if (direction.x < 0 && !facingLeft) {
                facingLeft = true;
                animatedSprite.setScale(-baseScale, baseScale);
            }
            else if (direction.x > 0 && facingLeft) {
                facingLeft = false;
                animatedSprite.setScale(baseScale, baseScale);
            }
        }
    }

    void draw(RenderWindow& window) const {
        window.draw(animatedSprite);
    }

    Vector2f getPosition() const { return position; }

private:
    Vector2f getRandomDirection() {
        Vector2f dirs[] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
        int i = rand() % 4;
        return dirs[i];
    }

    bool isValidMove(Vector2f pos, const Image& collisionMap) {
        if (pos.x < 0 || pos.y < 0 || pos.x >= MAP_WIDTH || pos.y >= MAP_HEIGHT) {
            return false;
        }
        return !isColliding(pos, collisionMap);
    }

    AnimatedSprite animatedSprite;
    Vector2f position;
    Vector2f direction;
    float timer;
    bool restrictToWhite;
    float npcHeight;
    float baseScale;
    bool facingLeft;
};

class Bullet {
public:
    CircleShape shape;
    Vector2f velocity;
    float lifetime;

    Bullet(Vector2f pos, Vector2f dir) {
        shape.setRadius(5.f);
        shape.setFillColor(Color::Yellow);
        shape.setOrigin(5.f, 5.f); // center the origin
        shape.setPosition(pos);
        velocity = dir * 300.f; // default speed
        lifetime = 3.0f;
    }

    // 🔧 Add this constructor for enemies to use custom bullet speed
    Bullet(Vector2f pos, Vector2f dir, float speed) {
        shape.setRadius(5.f);
        shape.setFillColor(Color::Yellow);
        shape.setOrigin(5.f, 5.f);
        shape.setPosition(pos);
        velocity = dir * speed;
        lifetime = 3.0f;
    }

    void update(float dt, const Image& collisionMap) {
        shape.move(velocity * dt);
        lifetime -= dt;

        // Collision with black walls
        Vector2f pos = shape.getPosition();
        float scaleX = static_cast<float>(collisionMap.getSize().x) / MAP_WIDTH;
        float scaleY = static_cast<float>(collisionMap.getSize().y) / MAP_HEIGHT;

        unsigned int x = static_cast<unsigned int>(pos.x * scaleX);
        unsigned int y = static_cast<unsigned int>(pos.y * scaleY);

        if (x >= collisionMap.getSize().x || y >= collisionMap.getSize().y) {
            lifetime = 0.f;
            return;
        }

        Color pixel = collisionMap.getPixel(x, y);
        if (pixel.r == 0 && pixel.g == 0 && pixel.b == 0) {
            lifetime = 0.f; // Bullet disappears on hitting black
        }
    }

    bool isAlive() const { return lifetime > 0.f; }
    Vector2f getPosition() const { return shape.getPosition(); }
    FloatRect getBounds() const { return shape.getGlobalBounds(); }

    void draw(RenderWindow& window) const {
        window.draw(shape);
    }
};



// Optimized Enemy class with health
class Enemy {
public:
    Enemy(const Texture& texture, Vector2f startPos)
        : animatedSprite(texture, texture.getSize().x / 7.3, texture.getSize().y, 7.3, 0.12f),
        position(startPos), alive(true), attackTimer(0.f), facingLeft(false),
        health(ENEMY_MAX_HEALTH), maxHealth(ENEMY_MAX_HEALTH) {

        animatedSprite.setPosition(position);
        float frameHeight = static_cast<float>(texture.getSize().y);
        baseScale = (CHARACTER_HEIGHT / frameHeight) + 0.12f;
        animatedSprite.setScale(baseScale, baseScale);
    }
    const vector<Bullet>& getBullets() const { return bullets; }

    bool update(float dt, Vector2f playerPos, const Image& collisionMap) {
        if (!alive) return false;

        attackTimer += dt;
        shootCooldown -= dt;

        Vector2f toPlayer = playerPos - position;
        float distanceToPlayer = sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);

        bool playerAttacked = false;

        if (distanceToPlayer < ENEMY_DETECTION_RANGE && distanceToPlayer > 0) {
            Vector2f direction = toPlayer / distanceToPlayer;

            // Move toward player
            Vector2f newPos = position + direction * ENEMY_SPEED * dt;
            if (!isColliding(newPos, collisionMap)) {
                position = newPos;
                animatedSprite.setPosition(position);

                if (direction.x < 0 && !facingLeft) {
                    facingLeft = true;
                    animatedSprite.setScale(-baseScale, baseScale);
                }
                else if (direction.x > 0 && facingLeft) {
                    facingLeft = false;
                    animatedSprite.setScale(baseScale, baseScale);
                }
            }

            // Melee attack
            if (distanceToPlayer < ENEMY_ATTACK_RANGE && attackTimer >= ENEMY_ATTACK_COOLDOWN) {
                attackTimer = 0.f;
                playerAttacked = true;
            }

            // Shoot bullet
            if (shootCooldown <= 0.f) {
                Vector2f bulletDir = toPlayer / distanceToPlayer;
                bullets.emplace_back(position, bulletDir, BULLET_SPEED);
                //shootSound.play();
                shootCooldown = ENEMY_SHOOT_COOLDOWN;
            }
        }

        // Update bullets
        for (auto& b : bullets) {
            b.update(dt, collisionMap);
        }

        bullets.erase(remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.isAlive(); }),
            bullets.end());


        animatedSprite.update(dt);
        return playerAttacked;
    }


    void takeDamage() {
        if (!alive) return;
        if (--health <= 0) {
            alive = false;
        }
    }

    void draw(RenderWindow& window) {
        if (!alive) return;

        window.draw(animatedSprite);
        drawHealthBar(window);

        for (const auto& bullet : bullets) {
            bullet.draw(window);
        }
    }


    void drawHealthBar(RenderWindow& window) {
        if (!alive) return;

        const float barWidth = 50.0f;
        const float barHeight = 6.0f;
        const float offsetY = -35.0f;

        Vector2f barPos = position + Vector2f(-barWidth / 2, offsetY);

        // Background bar
        RectangleShape bgBar(Vector2f(barWidth, barHeight));
        bgBar.setPosition(barPos);
        bgBar.setFillColor(Color(60, 20, 20));
        bgBar.setOutlineColor(Color(100, 40, 40));
        bgBar.setOutlineThickness(1.0f);
        window.draw(bgBar);

        // Health bar
        float healthRatio = static_cast<float>(health) / static_cast<float>(maxHealth);
        RectangleShape healthBar(Vector2f(barWidth * healthRatio, barHeight));
        healthBar.setPosition(barPos);
        healthBar.setFillColor(Color(220, 50, 50));
        window.draw(healthBar);
    }

    // For bullet collision detection
    FloatRect getBounds() const {
        return animatedSprite.getGlobalBounds();
    }

    bool isAlive() const { return alive; }
    Vector2f getPosition() const { return position; }
    int getHealth() const { return health; }

private:
    vector<Bullet> bullets;
    float shootCooldown = 0.f;
    const float ENEMY_SHOOT_COOLDOWN = 2.0f;
    const float BULLET_SPEED = 300.f;

    AnimatedSprite animatedSprite;
    Vector2f position;
    bool alive;
    float attackTimer;
    float baseScale;
    bool facingLeft;
    int health;
    int maxHealth;
};

// Health system class
class HealthSystem {
public:
    HealthSystem() : currentHealthBlocks(MAX_HEALTH_BLOCKS), invulnerabilityTimer(0.f) {}

    void takeDamage() {
        if (invulnerabilityTimer <= 0.f && currentHealthBlocks > 0) {
            currentHealthBlocks--;
            invulnerabilityTimer = 1.0f; // 1 second invulnerability
        }
    }

    void update(float dt) {
        if (invulnerabilityTimer > 0.f) {
            invulnerabilityTimer -= dt;
        }
    }

    void drawHealthBlocks(RenderWindow& window, const Font& font, float startX = 20.f, string playername = "P1") const {

        View originalView = window.getView();
        View uiView(FloatRect(0, 0, window.getSize().x, window.getSize().y));
        window.setView(uiView);

        const float blockSize = 40.f;
        const float spacing = 5.f;

        // Draw health blocks
        for (int i = 0; i < MAX_HEALTH_BLOCKS; i++) {
            RectangleShape block(Vector2f(blockSize, blockSize));
            block.setPosition(startX + i * (blockSize + spacing), 20);

            if (i < currentHealthBlocks) {
                // Active health block
                block.setFillColor(Color(220, 50, 50));
                block.setOutlineColor(Color(255, 100, 100));
            }
            else {
                // Empty health block
                block.setFillColor(Color(60, 20, 20));
                block.setOutlineColor(Color(100, 40, 40));
            }
            block.setOutlineThickness(2.f);
            window.draw(block);
        }

        // Draw health text
        Text healthText1("Health " + playername, font, 20);
        healthText1.setFillColor(Color::White);
        healthText1.setPosition(startX, 20 + blockSize + 10);
        window.draw(healthText1);


        window.setView(originalView);
    }

    bool isAlive() const { return currentHealthBlocks > 0; }
    bool isInvulnerable() const { return invulnerabilityTimer > 0.f; }
    int getHealthBlocks() const { return currentHealthBlocks; }

private:
    int currentHealthBlocks;
    float invulnerabilityTimer;
};

// Game Win/Lose system
class GameState {
public:
    enum State { PLAYING, WON, LOST };
    GameState() : currentState(PLAYING) {}

    void checkWinCondition(const vector<Enemy>& enemies, const HealthSystem& health1, const HealthSystem& health2, const vector<TaskArea>& tasks) {
        if (currentState != PLAYING) return;

        if (!health1.isAlive() || !health2.isAlive()) {
            currentState = LOST;
            return;
        }

        // Check if all enemies are dead
        bool allEnemiesDead = true;
        for (const auto& enemy : enemies) {
            if (enemy.isAlive()) {
                allEnemiesDead = false;
                break;
            }
        }

        // Check if all tasks are completed
        bool allTasksCompleted = true;
        for (const auto& task : tasks) {
            if (!task.isCompleted()) {
                allTasksCompleted = false;
                break;
            }
        }

        // Player wins only if both conditions are met
        if (allEnemiesDead && allTasksCompleted) {
            currentState = WON;
        }
    }

    void drawGameState(RenderWindow& window, const Font& font) const {
        if (currentState == PLAYING) return;
        View originalView = window.getView();
        View uiView(FloatRect(0, 0, window.getSize().x, window.getSize().y));
        window.setView(uiView);
        // Draw overlay
        RectangleShape overlay(Vector2f(window.getSize().x, window.getSize().y));
        overlay.setFillColor(Color(0, 0, 0, 150));
        window.draw(overlay);
        // Draw message
        string message = (currentState == WON) ? "YOU WIN!" : "GAME OVER";
        Text gameText(message, font, 48);
        gameText.setFillColor((currentState == WON) ? Color::Green : Color::Red);
        gameText.setPosition(window.getSize().x / 2 - gameText.getGlobalBounds().width / 2,
            window.getSize().y / 2 - gameText.getGlobalBounds().height / 2);
        window.draw(gameText);
        Text restartText("Press R to restart or ESC to quit", font, 24);
        restartText.setFillColor(Color::White);
        restartText.setPosition(window.getSize().x / 2 - restartText.getGlobalBounds().width / 2,
            window.getSize().y / 2 + 60);
        window.draw(restartText);
        window.setView(originalView);
    }

    State getState() const { return currentState; }
    void reset() { currentState = PLAYING; }

private:
    State currentState;
};


// NEW PLAYER CLASS
class Player {
public:
    Player(const Texture& idleTexture, const Texture& runTexture, Vector2f startPos)
        : position(startPos),
        idleSprite(idleTexture),
        runSprite(runTexture, runTexture.getSize().x / 7, runTexture.getSize().y, 7, 0.1f),
        currentAnimation(AnimationState::IDLE),
        hasWeapon(false),
        weaponAttackTimer(0.0f),
        bulletCooldown(0.0f),
        facingLeft(false),
        isMoving(false) {

        // Set initial sprite positions
        idleSprite.setPosition(position);
        runSprite.setPosition(position);

        // Set proper scaling
        float baseScale = CHARACTER_HEIGHT / static_cast<float>(runTexture.getSize().y);
        runSprite.setScale(baseScale, baseScale);
        idleSprite.setScale(0.2f, 0.2f);
    }

    void update(float dt, const Image& collisionMap, vector<Bullet>& bullets, Sound& shootSound, Sound& walkSound, bool& isWalkSoundPlaying) {
        // Update timers
        if (weaponAttackTimer > 0.0f) {
            weaponAttackTimer -= dt;
        }
        bulletCooldown -= dt;

        // Handle movement input
        Vector2f movement(0.f, 0.f);
        isMoving = false;

        if (Keyboard::isKeyPressed(Keyboard::Left)) {
            movement.x -= 1.f;
            isMoving = true;
            facingLeft = true;
        }
        if (Keyboard::isKeyPressed(Keyboard::Right)) {
            movement.x += 1.f;
            isMoving = true;
            facingLeft = false;
        }
        if (Keyboard::isKeyPressed(Keyboard::Up)) {
            movement.y -= 1.f;
            isMoving = true;
        }
        if (Keyboard::isKeyPressed(Keyboard::Down)) {
            movement.y += 1.f;
            isMoving = true;
        }

        // Handle walking sound
        if (isMoving && !isWalkSoundPlaying) {
            walkSound.play();
            isWalkSoundPlaying = true;
        }
        else if (!isMoving && isWalkSoundPlaying) {
            walkSound.stop();
            isWalkSoundPlaying = false;
        }

        // Normalize movement
        if (isMoving) {
            float length = sqrt(movement.x * movement.x + movement.y * movement.y);
            if (length > 0) {
                movement /= length;
            }
        }
        movement *= MOVE_SPEED * dt;

        // Handle shooting
        if (hasWeapon && bulletCooldown <= 0.0f) {
            Vector2f bulletDirection(0.f, 0.f);

            if (Keyboard::isKeyPressed(Keyboard::A)) bulletDirection.x -= 1.f;
            if (Keyboard::isKeyPressed(Keyboard::D)) bulletDirection.x += 1.f;
            if (Keyboard::isKeyPressed(Keyboard::W)) bulletDirection.y -= 1.f;
            if (Keyboard::isKeyPressed(Keyboard::S)) bulletDirection.y += 1.f;

            if (bulletDirection.x != 0.f || bulletDirection.y != 0.f) {
                float length = sqrt(bulletDirection.x * bulletDirection.x + bulletDirection.y * bulletDirection.y);
                bulletDirection /= length;

                bullets.emplace_back(position, bulletDirection);
                shootSound.play();
                bulletCooldown = BULLET_FIRE_RATE;
            }
        }

        // Update position with collision detection
        Vector2f newPosition = position + movement;
        float halfSize = CHARACTER_HEIGHT / 2.f;

        newPosition.x = max(halfSize, min(newPosition.x, static_cast<float>(MAP_WIDTH) - halfSize));
        newPosition.y = max(halfSize, min(newPosition.y, static_cast<float>(MAP_HEIGHT) - halfSize));

        if (!::isColliding(newPosition, collisionMap)) {
            position = newPosition;
        }

        // Update animation
        if (isMoving) {
            if (currentAnimation != AnimationState::RUNNING) {
                currentAnimation = AnimationState::RUNNING;
                runSprite.reset();
            }
            runSprite.update(dt);

            float baseScale = CHARACTER_HEIGHT / static_cast<float>(runSprite.getTexture().getSize().y);
            if (facingLeft) {
                runSprite.setScale(-baseScale, baseScale);
            }
            else {
                runSprite.setScale(baseScale, baseScale);
            }
        }
        else {
            currentAnimation = AnimationState::IDLE;
        }

        // Update sprite positions
        idleSprite.setPosition(position);
        runSprite.setPosition(position);
    }

    void draw(RenderWindow& window) {
        if (currentAnimation == AnimationState::RUNNING) {
            window.draw(runSprite);

        }
        else {
            window.draw(idleSprite);
        }
    }

    // Getters and setters
    Vector2f getPosition() const { return position; }
    void setPosition(Vector2f newPos) { position = newPos; }
    bool getHasWeapon() const { return hasWeapon; }
    void setHasWeapon(bool weapon) { hasWeapon = weapon; }
    FloatRect getBounds() const {
        return FloatRect(position.x - 16, position.y - 16, 32, 32);
    }

    void takeDamage(HealthSystem& health1, HealthSystem& health2) {
        health1.takeDamage();
        health2.takeDamage();
    }

private:
    enum class AnimationState { IDLE, RUNNING };

    Vector2f position;
    StaticSprite idleSprite;
    AnimatedSprite runSprite;
    AnimationState currentAnimation;
    bool hasWeapon;
    float weaponAttackTimer;
    float bulletCooldown;
    bool facingLeft;
    bool isMoving;


};

// NEW UI CLASS
class UI {
public:
    UI() : fontLoaded(false) {}

    bool loadFont(const string& fontPath) {
        fontLoaded = font.loadFromFile(fontPath);
        return fontLoaded;
    }

    void drawTitleScreen(RenderWindow& window, const Sprite& titleSprite, const RectangleShape& playButton) {
        View uiView(FloatRect(0, 0, window.getSize().x, window.getSize().y));
        window.setView(uiView);

        window.draw(titleSprite);
        window.draw(playButton);

        if (fontLoaded) {
            Text playButtonText("PLAY", font, 32);
            playButtonText.setFillColor(Color::White);
            playButtonText.setPosition(
                playButton.getPosition().x + playButton.getSize().x / 2 - playButtonText.getGlobalBounds().width / 2,
                playButton.getPosition().y + playButton.getSize().y / 2 - playButtonText.getGlobalBounds().height / 2
            );
            window.draw(playButtonText);
        }
    }

    void drawGameUI(RenderWindow& window, const HealthSystem& health1, const HealthSystem& health2,
        const vector<TaskArea>& tasks, const GameState& gameState) {
        if (!fontLoaded) return;

        // Draw health bars
        health1.drawHealthBlocks(window, font, 20.f, "Player 1");

        // Draw task list
        View originalView = window.getView();
        View uiView(FloatRect(0, 0, window.getSize().x, window.getSize().y));
        window.setView(uiView);

        int lineIndex = 0;
        for (const auto& task : tasks) {
            task.drawTaskList(window, font, lineIndex);
        }

        window.setView(originalView);

        // Draw game state overlay
        gameState.drawGameState(window, font);
    }

    void drawTaskUI(RenderWindow& window, const vector<TaskArea>& tasks) {
        if (!fontLoaded) return;

        for (const auto& task : tasks) {
            task.drawUI(window, font);
        }
    }

    const Font& getFont() const { return font; }
    bool isFontLoaded() const { return fontLoaded; }

private:
    Font font;
    bool fontLoaded;
};

// NEW GAME CLASS
class Game {
public:
    enum class GameMode { TITLE_SCREEN, PLAYING, GAME_OVER };

    Game() : window(VideoMode(1920, 1080), "Dimension"),
        currentGameMode(GameMode::TITLE_SCREEN),
        player(nullptr),
        healthSystem1(),
        healthSystem2(),
        gameState(),
        ui(),
        isWalkSoundPlaying(false),
        accumulator(0.0) {

        window.setFramerateLimit(60);
        srand(static_cast<unsigned>(time(0)));

        // Initialize camera
        camera = View(FloatRect(0.f, 0.f, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT)));
        camera.zoom(CAMERA_ZOOM);
    }

    ~Game() {
        delete player;
    }

    bool initialize() {
        // Load textures
        if (!loadTextures()) return false;
        if (!loadSounds()) return false;
        if (!ui.loadFont("montserrat.ttf")) {
            cerr << "Warning: Failed to load font" << endl;
        }

        // Load collision map
        if (!collisionMap.loadFromFile("mapwt.png")) {
            cout << "Failed to load collision map" << endl;
            return false;
        }

        setupTitleScreen();
        return true;
    }

    void run() {
        auto currentTime = chrono::high_resolution_clock::now();

        while (window.isOpen()) {
            auto newTime = chrono::high_resolution_clock::now();
            double frameTime = chrono::duration_cast<chrono::duration<double>>(newTime - currentTime).count();
            currentTime = newTime;
            frameTime = min(frameTime, 0.05);
            accumulator += frameTime;

            handleEvents();

            if (currentGameMode == GameMode::PLAYING) {
                update();
            }

            render();
        }
    }

private:
    // Member variables
    RenderWindow window;
    GameMode currentGameMode;
    Player* player;
    vector<Enemy> enemies;
    vector<NPC> npcs;
    vector<Bullet> bullets;
    vector<TaskArea> tasks;
    HealthSystem healthSystem1, healthSystem2;
    GameState gameState;
    UI ui;
    View camera;
    Image collisionMap;

    // Textures
    Texture backgroundTexture, titleTexture;
    Texture bluTexture, grTexture, enemyTexture;
    Texture idleTexture, runTexture;

    // Sprites
    Sprite background, titleSprite;
    RectangleShape playButton;

    // Audio
    SoundBuffer walkSoundBuffer, shootSoundBuffer;
    Sound walkSound, shootSound;
    bool isWalkSoundPlaying;

    SoundBuffer taskCompleteSoundBuffer, ostSoundBuffer;
    Sound taskCompleteSound, ostSound;

    // Timing
    double accumulator;

    bool loadTextures() {
        return backgroundTexture.loadFromFile("map2.png") &&
            titleTexture.loadFromFile("title2.png") &&
            bluTexture.loadFromFile("brg.png") &&
            grTexture.loadFromFile("grmove.png") &&
            enemyTexture.loadFromFile("e.png") &&
            idleTexture.loadFromFile("gidle.png") &&
            runTexture.loadFromFile("gmove.png");
    }

    bool loadSounds() {
        bool success = true;

        if (!walkSoundBuffer.loadFromFile("walksound.wav")) {
            cerr << "Failed to load walk sound" << endl;
            success = false;
        }
        else {
            walkSound.setBuffer(walkSoundBuffer);
            walkSound.setLoop(true);
            walkSound.setVolume(7000);
        }

        if (!shootSoundBuffer.loadFromFile("shootsound.wav")) {
            cerr << "Failed to load shoot sound" << endl;
            success = false;
        }
        else {
            shootSound.setBuffer(shootSoundBuffer);
            shootSound.setVolume(70);
        }
        // Load OST (background music)
        if (!ostSoundBuffer.loadFromFile("ost.wav")) {
            cerr << "Failed to load OST sound" << endl;
            success = false;
        }
        else {
            ostSound.setBuffer(ostSoundBuffer);
            ostSound.setLoop(true);
            ostSound.setVolume(5000);
        }

        // Load task complete sound
        if (!taskCompleteSoundBuffer.loadFromFile("taskend.wav")) {
            cerr << "Failed to load task complete sound" << endl;
            success = false;
        }
        else {
            taskCompleteSound.setBuffer(taskCompleteSoundBuffer);
            taskCompleteSound.setVolume(80);
        }

        return success;
    }

    void setupTitleScreen() {
        titleSprite.setTexture(titleTexture);
        ostSound.play();

        // Scale title screen
        float titleScaleX = static_cast<float>(window.getSize().x) / titleTexture.getSize().x;
        float titleScaleY = static_cast<float>(window.getSize().y) / titleTexture.getSize().y;
        titleSprite.setScale(titleScaleX, titleScaleY);

        // Setup play button
        playButton.setSize(Vector2f(200, 60));
        playButton.setPosition(window.getSize().x / 2 - 100, window.getSize().y / 2 + 100);
        playButton.setFillColor(Color(70, 130, 180));
        playButton.setOutlineColor(Color::White);
        playButton.setOutlineThickness(3);
    }

    void startGame() {
        currentGameMode = GameMode::PLAYING;
        ostSound.stop();

        // Initialize player
        Vector2f startPos(2500.f, 500.f);
        player = new Player(idleTexture, runTexture, startPos);
        camera.setCenter(startPos);

        // Setup background
        float bgScaleX = static_cast<float>(MAP_WIDTH) / backgroundTexture.getSize().x;
        float bgScaleY = static_cast<float>(MAP_HEIGHT) / backgroundTexture.getSize().y;
        background.setTexture(backgroundTexture);
        background.setScale(bgScaleX, bgScaleY);

        // Initialize NPCs
        initializeNPCs();

        // Initialize enemies
        initializeEnemies();

        // Initialize tasks
        initializeTasks();
    }

    void initializeNPCs() {
        npcs.clear();

        // Original 2 NPCs
        Vector2f grStart = findValidPosition();
        Vector2f bluStart = findValidPosition();
        npcs.emplace_back(grTexture, grStart, true, 80.f);
        npcs.emplace_back(bluTexture, bluStart, false, 60.f);


        int additionalNPCs = 6;

        for (int i = 0; i < additionalNPCs; i++) {
            Vector2f npcPos = findValidPosition();

            if (i % 2 == 0) {
                // Green NPC (grTexture)
                npcs.emplace_back(grTexture, npcPos, true, 80.f);
            }
            else {
                // Blue NPC (bluTexture) 
                npcs.emplace_back(bluTexture, npcPos, false, 60.f);
            }
        }
    }

    void initializeEnemies() {
        enemies.clear();
        for (int i = 0; i < 3; i++) {
            Vector2f enemyPos = findValidPosition();
            enemies.emplace_back(enemyTexture, enemyPos);
        }
    }

    void initializeTasks() {
        tasks.clear();

        // Weapon task with callback
        tasks.emplace_back(Vector2f(3290.f, 600.f), 100.f, "Collect the Weapon.", [this]() {
            if (player) player->setHasWeapon(true);
            taskCompleteSound.play();
            });

        // Other tasks
        tasks.emplace_back(Vector2f(1760.f, 1000.f), 100.f, "Get Scanned");
        tasks.emplace_back(Vector2f(500.f, 900.f), 100.f, "Disable the Generator");
        tasks.emplace_back(Vector2f(1520.f, 1320.f), 100.f, "Fix the Lights");
        tasks.emplace_back(Vector2f(2460.f, 1900.f), 100.f, "Take out Trash");
        tasks.emplace_back(Vector2f(3850.f, 950.f), 100.f, "Navigate the Ship");
        tasks.emplace_back(Vector2f(2700.f, 1200.f), 100.f, "Swipe Card");
        tasks.emplace_back(Vector2f(800.f, 1520.f), 100.f, "Oil the Engine");
    }

    Vector2f findValidPosition() {
        Vector2f pos;
        int attempts = 0;
        do {
            pos = Vector2f(rand() % (MAP_WIDTH - 400) + 200, rand() % (MAP_HEIGHT - 400) + 200);
            attempts++;
        } while (isColliding(pos, collisionMap) && attempts < 100);
        return pos;
    }

    bool isColliding(Vector2f position, const sf::Image& map) {
        if (map.getSize().x == 0 || map.getSize().y == 0) {
            std::cerr << "Collision map not loaded!" << std::endl;
            return true;
        }

        float scaleX = static_cast<float>(map.getSize().x) / MAP_WIDTH;
        float scaleY = static_cast<float>(map.getSize().y) / MAP_HEIGHT;

        unsigned int x = static_cast<unsigned int>(position.x * scaleX);
        unsigned int y = static_cast<unsigned int>(position.y * scaleY);

        if (x >= map.getSize().x || y >= map.getSize().y)
            return true;

        Color pixel = map.getPixel(x, y);
        return (pixel.r == 0 && pixel.g == 0 && pixel.b == 0);
    }


    void handleEvents() {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
            else if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Escape) {
                    window.close();
                }
                else if (event.key.code == Keyboard::R && gameState.getState() != GameState::PLAYING) {
                    resetGame();
                }
            }
            else if (event.type == Event::MouseButtonPressed && currentGameMode == GameMode::TITLE_SCREEN) {
                if (event.mouseButton.button == Mouse::Left) {
                    Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                    if (playButton.getGlobalBounds().contains(mousePos)) {
                        startGame();
                    }
                }
            }
        }
    }

    void update() {
        while (accumulator >= FIXED_TIME_STEP && gameState.getState() == GameState::PLAYING) {
            float dt = static_cast<float>(FIXED_TIME_STEP);

            // Update player
            if (player) {
                player->update(dt, collisionMap, bullets, shootSound, walkSound, isWalkSoundPlaying);
            }

            // Update NPCs
            for (auto& npc : npcs) {
                npc.update(dt, collisionMap);
            }

            // Update enemies 
            for (auto& enemy : enemies) {
                if (player && enemy.update(dt, player->getPosition(), collisionMap)) {
                    player->takeDamage(healthSystem1, healthSystem2);
                }
            }

            // Update bullets
            for (auto& bullet : bullets) {
                bullet.update(dt, collisionMap);
            }

            // Check bullet-enemy collisions
            for (auto& bullet : bullets) {
                for (auto& enemy : enemies) {
                    if (enemy.isAlive() && enemy.getBounds().contains(bullet.getPosition())) {
                        enemy.takeDamage();
                        bullet.lifetime = 0.f;
                    }
                }
            }

            // Check enemy bullet-player collisions
            if (player) {
                for (auto& enemy : enemies) {
                    if (!enemy.isAlive()) continue;
                    for (const Bullet& b : enemy.getBullets()) {
                        if (player->getBounds().intersects(b.getBounds())) {
                            player->takeDamage(healthSystem1, healthSystem2);
                            break;
                        }
                    }
                }
            }

            // Remove dead bullets
            bullets.erase(remove_if(bullets.begin(), bullets.end(),
                [](const Bullet& b) { return !b.isAlive(); }),
                bullets.end());

            // Update tasks
            if (player) {
                for (auto& task : tasks) {
                    task.update(dt, player->getPosition());
                }
            }

            // Update health systems
            healthSystem1.update(dt);
            healthSystem2.update(dt);

            // Update camera
            if (player) {
                updateCamera();
            }

            // Check win/lose conditions
            gameState.checkWinCondition(enemies, healthSystem1, healthSystem2, tasks);

            accumulator -= FIXED_TIME_STEP;
        }
    }

    void updateCamera() {
        Vector2f cameraCenter = camera.getCenter();
        Vector2f targetCenter = player->getPosition();
        Vector2f newCenter = cameraCenter + (targetCenter - cameraCenter) * CAMERA_LERP_FACTOR;

        View tempCamera = camera;
        tempCamera.setCenter(newCenter);
        Vector2f clampedCenter = clampCamera(tempCamera, MAP_WIDTH, MAP_HEIGHT);
        camera.setCenter(clampedCenter);
    }

    Vector2f clampCamera(const View& cam, int mapWidth, int mapHeight) {
        Vector2f viewSize = cam.getSize();
        Vector2f viewHalfSize = viewSize / 2.f;
        Vector2f cameraPos = cam.getCenter();

        float minX = viewHalfSize.x;
        float minY = viewHalfSize.y;
        float maxX = mapWidth - viewHalfSize.x;
        float maxY = mapHeight - viewHalfSize.y;

        if (mapWidth > viewSize.x)
            cameraPos.x = max(minX, min(cameraPos.x, maxX));
        if (mapHeight > viewSize.y)
            cameraPos.y = max(minY, min(cameraPos.y, maxY));

        return cameraPos;
    }

    void render() {
        window.clear(Color::Black);

        if (currentGameMode == GameMode::TITLE_SCREEN) {
            ui.drawTitleScreen(window, titleSprite, playButton);
        }
        else if (currentGameMode == GameMode::PLAYING) {
            window.setView(camera);

            // Draw world
            window.draw(background);

            // Draw tasks
            for (auto& task : tasks) {
                task.draw(window);
            }

            // Draw NPCs
            for (auto& npc : npcs) {
                npc.draw(window);
            }

            // Draw enemies
            for (auto& enemy : enemies) {
                enemy.draw(window);
            }

            // Draw bullets
            for (auto& bullet : bullets) {
                bullet.draw(window);
            }

            // Draw player
            if (player) {
                player->draw(window);
            }

            // Draw UI
            ui.drawTaskUI(window, tasks);
            ui.drawGameUI(window, healthSystem1, healthSystem2, tasks, gameState);
        }

        window.display();
    }

    void resetGame() {
        gameState.reset();
        healthSystem1 = HealthSystem();
        healthSystem2 = HealthSystem();

        if (player) {
            player->setPosition(Vector2f(2500.f, 500.f));
            player->setHasWeapon(false);
        }

        // Reset enemies
        initializeEnemies();

        // Reset tasks
        initializeTasks();

        bullets.clear();
    }
};

// MAIN FUNCTION
int main() {
    Game game;

    if (!game.initialize()) {
        cerr << "Failed to initialize game" << endl;
        return EXIT_FAILURE;
    }

    game.run();
    return 0;
}
