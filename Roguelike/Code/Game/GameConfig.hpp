
struct GraphicsOptions {
    float WindowWidth = 1600.0f;
    float WindowHeight = 900.0f;
    float WindowAspectRatio = WindowWidth / WindowHeight;
    float Fov = 70.0f;
    float MaxShakeAngle = 0.0f;
    float MaxShakeOffsetHorizontal = 0.0f;
    float MaxShakeOffsetVertical = 0.0f;
    float MaxMouseSensitivityX = 0.1f;
    float MaxShakeSensitivityY = 0.1f;
    bool InvertMouseY = false;
    bool InvertMouseX = false;
    bool vsync = true;
};

static GraphicsOptions defaultGraphicsOptions{};
static GraphicsOptions currentGraphicsOptions{};
