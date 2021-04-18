#include <unistd.h>
#include "vulkan.cpp"

using namespace std;

Camera* mainCam;
Light* mainLight;

GameObject* charactor[4];

GameObject* lightPos;

UI* button;

std::string getAbsolutePath();

class standardRoutine : public routine {
public:
    float inv = -1.0f;

    void Awake() override{
        routine::Awake();

        std::string pwd = getAbsolutePath();

        mainCam = createCamera(glm::vec3(2.0f, 1.0f, 5.0f));
        mainLight = createLight(glm::vec3(0.0f, 1.5f, 1.0f));

        glm::vec3 charRot = glm::vec3(0, 180, 0);

        // Duplication 함수 생성으로 instance(캐릭터 값, 위치, 로테이션)하면 drawFrame에서 여러번 돌릴 수 있도록 만들어보자
        charactor[0] = createObject("robot", "models/charactor/body.obj", "textures/charactor/body.png", glm::vec3(0.0f, 0.0f, 0.0f), charRot);
        charactor[1] = createObject("robot", "models/charactor/bow.obj", "textures/charactor/bow.png", glm::vec3(0.0f, 0.0f, 0.0f), charRot);
        charactor[2] = createObject("robot", "models/charactor/clothes.obj", "textures/charactor/clothes.png", glm::vec3(0.0f, 0.0f, 0.0f), charRot);
        charactor[3] = createObject("robot", "models/charactor/hair.obj", "textures/charactor/hair.png", glm::vec3(0.0f, 0.0f, 0.0f), charRot);

        charactor[0]->_initParam.fragPath = "spv/GameObject/soft.spv";
        charactor[1]->_initParam.fragPath = "spv/GameObject/glass.spv";
        charactor[2]->_initParam.fragPath = "spv/GameObject/leather.spv";


        lightPos = createObject("lightPos", "models/Light.obj", "textures/Light.png", mainLight->getPosition(), glm::vec3(0.0f), glm::vec3(0.1f));
        lightPos->_initParam.fragPath = "spv/GameObject/glass.spv";

        button = createUI("Button", true, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 100.0f, 100.0f));
    }

    void Start() override{
        routine::Start();

        for (int j = 0; j < 4 ; j++)
            charactor[j]->initObject();

        lightPos->initObject();

        button->initObject();
    }

    void Update() override{
        routine::Update(); 
        
        lightPos->setPosition(mainLight->getPosition());

        if (input::getKey(GLFW_KEY_UP)) {
            mainLight->move(glm::vec3(0.0f, 0.0f, -1.0f));
        }
        else if (input::getKey(GLFW_KEY_DOWN))
            mainLight->move(glm::vec3(0.0f, 0.0f, 1.0f));

        if (input::getKey(GLFW_KEY_LEFT))
            mainLight->move(glm::vec3(-1.0f, 0.0f, 0.0f));
        else if (input::getKey(GLFW_KEY_RIGHT))
            mainLight->move(glm::vec3(1.0f, 0.0f, 0.0f));

        if (input::getKey(GLFW_KEY_PAGE_UP))
            mainLight->move(glm::vec3(0.0f, -1.0f, 0.0f));
        else if (input::getKey(GLFW_KEY_PAGE_DOWN))
            mainLight->move(glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void PhysicalUpdate() override{
        routine::PhysicalUpdate();
    }

    void End() override{
        routine::End();
    } 
};

char** _UIMAP;

void batchUI();
uint32_t getUIIdx();

int main() {
    float camSpeed = 5.0f;
    float camQuart = 0.2f;

    standardRoutine rt;

    _UIMAP = new char*[HEIGHT];
    for (int i = 0; i< HEIGHT; i++) {
        _UIMAP[i] = new char[WIDTH];
        for (int j = 0; j < WIDTH; j++)
            _UIMAP[i][j] = 0;
    }
    
    rt.Awake();
    // UI 배치
    batchUI();

    try {
        initWindow();
        initVulkan();
        
        rt.Start();

        float y = 0.0f;
        glm::vec3 front(0.0f);
        glm::vec3 right(0.0f);

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            y = glm::radians(mainCam->getRotate().y);

            front = glm::vec3(sin(-y), 0.0f, cos(-y)) * camSpeed;
            right = glm::vec3(cos(y), 0.0f, sin(y)) * camSpeed;

            // rt.PhysicalUpdate();
            rt.Update();
            
            if (input::getMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
                getUIIdx();

            if (!input::getKey(GLFW_KEY_LEFT_ALT)) {
                if (input::getKey(GLFW_KEY_W))
                    mainCam->moveZ(-front);
                else if (input::getKey(GLFW_KEY_S))
                    mainCam->moveZ(front);

                if (input::getKey(GLFW_KEY_A))
                    mainCam->moveX(-right);
                else if (input::getKey(GLFW_KEY_D))
                    mainCam->moveX(right);

                if (input::getKey(GLFW_KEY_Q))
                    mainCam->moveY(camSpeed);
                else if (input::getKey(GLFW_KEY_E))
                    mainCam->moveY(-camSpeed);
            }
            else {
                if (input::getKey(GLFW_KEY_W))
                    mainCam->pitch(-camQuart);
                else if (input::getKey(GLFW_KEY_S))
                    mainCam->pitch(camQuart);

                if (input::getKey(GLFW_KEY_A))
                    mainCam->yaw(-camQuart);
                else if (input::getKey(GLFW_KEY_D))
                    mainCam->yaw(camQuart);
            }

            drawFrame();
        }
        vkDeviceWaitIdle(device);
        
        rt.End();
        
        for (GameObject* go : gameObjectList) {
            go->destroy();
            delete(go);
        }

        for (UI* ui : UIList) {
            ui->destroy();
            delete(ui);
        }

        for (Camera* cam : cameraObejctList) {
            delete(cam);
        }

        for (Light* light : lightObjectList) {
            delete(light);
        }

        cleanup();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    for (int i = 0; i < HEIGHT; i++) {
        delete(_UIMAP[i]);
    }
    delete[] (_UIMAP);

    return EXIT_SUCCESS;
}

void batchUI() {
    for (UI* ui : UIList) {
        if (!ui->clickable)
            continue;
        uint32_t x0(ui->getExtent()[0]),
                 y0(ui->getExtent()[1]),
                 x1(ui->getExtent()[2]), 
                 y1(ui->getExtent()[3]);

        uint32_t idx = ui->getIndex();

        for (int y = y0; y < y1; y++)
            for (int x = x0; x < x1; x++)
                _UIMAP[y][x] = idx;
    }
}

uint32_t getUIIdx() {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if ((int)xpos < WIDTH && (int)ypos < HEIGHT)
        std::cout << (int)_UIMAP[(int)ypos][(int)xpos] << std::endl;

    return 0;
}

std::string getAbsolutePath() {
    std::string buffer;
    buffer.resize(1000);

    if (getcwd(buffer.data(), 1000) != NULL)
        return buffer;
    return std::string();
}