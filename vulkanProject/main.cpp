#include <unistd.h>
#include "vulkan.cpp"

using namespace std;

Camera* mainCam;
Light* mainLight;

GameObject* charactor;

Transpose transpose;

GameObject* skybox;
GameObject* bottom;

GameObject* lightPos;

UI* button;

// camera
float camSpeed = 5000.0f;
float camQuart = 100.0f;

float yOfCam = 0.0f;

glm::vec3 frontOfCam(0.0f);
glm::vec3 rightOfCam(0.0f);

std::string getAbsolutePath();

class standardRoutine : public routine {
public:
    void Awake() override{
        routine::Awake();

        std::string pwd = getAbsolutePath();

        mainCam = createCamera(glm::vec3(0.0f, 2.0f, 20.0f));
        mainLight = createLight(glm::vec3(0.0f, 1.5f, 1.0f));

        // Duplication 함수 생성으로 instance(캐릭터 값, 위치, 로테이션)하면 drawFrame에서 여러번 돌릴 수 있도록 만들어보자
        charactor = createObject(   "body", 
                                    "models/charactor/body.obj", 
                                    "textures/charactor/body.png", 
                                    glm::vec3(5.0f, 5.0f, 0.0f), 
                                    glm::vec3(0, 0, 0), 
                                    glm::vec3(1.0f), 
                                    std::string("spv/GameObject/soft.spv")
                                );

        charactor->appendModel(     "bow",
                                    "models/charactor/bow.obj", 
                                    "textures/charactor/bow.png", 
                                    "spv/GameObject/glass.spv"
                            );
        charactor->appendModel(     "clothes",
                                    "models/charactor/clothes.obj", 
                                    "textures/charactor/clothes.png", 
                                    "spv/GameObject/leather.spv"
                            );
        charactor->appendModel(     "hair",
                                    "models/charactor/hair.obj", 
                                    "textures/charactor/hair.png", 
                                    "spv/GameObject/clothes.spv"
                            );

        charactor->setCollider(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.5f, 1.0f, 0.5f));
        charactor->drawCollider();

        // mainCam->target = charactor;

        lightPos = createObject(    "lightPos",
                                    "models/Light.obj", 
                                    "textures/Light.png", 
                                    mainLight->getPosition(), 
                                    glm::vec3(0.0f), 
                                    glm::vec3(0.1f), 
                                    "spv/GameObject/glass.spv");

        skybox = createObject(      "skybox", 
                                    "models/skybox/Cube.obj", 
                                    "textures/skybox/Cube.png",  
                                    glm::vec3(0.0f), 
                                    glm::vec3(0.0f), 
                                    glm::vec3(100.0f), 
                                    "spv/GameObject/base.spv");
                                    
        skybox->models[0]->_initParam.cullMode = VK_CULL_MODE_NONE;

        // Collision Test
        bottom = createObject(  "Bottom", 
                                "models/bottom.obj", 
                                "textures/bottom.jpg", 
                                glm::vec3(0.0f, 0.0f, 0.0f));

        bottom->setCollider(glm::vec3(0.0f, 0.1f, 0.0f), glm::vec3(10.0f, 0.5f, 10.0f));
        bottom->drawCollider();

        // UI
        button = createUI("Button", true, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 50.0f, 50.0f));
    }

    void Start() override {
        routine::Start();

        charactor->initObject();

        lightPos->initObject();
        skybox->initObject();

        bottom->initObject();
        button->initObject();

        transpose.torque = glm::vec3(0.0f, 0.0f, 5.0f);
    }

    void Update() override {
        routine::Update(); 
        
        // Light Dot Object
        lightPos->setPosition(mainLight->getPosition());     

        // interactive with bottom
        if (charactor->onColliderEnter(bottom)) {
            transpose.velo = glm::vec3(0.0f, 0.0f, 0.0f);
            transpose.accel = glm::vec3(0.0f);
        }
        else if (charactor->onCollider(bottom)) {
            transpose.interaction(charactor->Position, bottom->Position, charactor->Rotation, bottom->Rotation, _TIME_PER_UPDATE);

            bottom->Rotate(transpose.torqBySec(_TIME_PER_UPDATE));
        }
        else {
            // Gravity
            transpose.accel = glm::vec3(0, -2.0f, 0);
        }

        charactor->Move(transpose.velBySec(_TIME_PER_UPDATE));

        // 관측자 이동
        yOfCam = glm::radians(mainCam->getRotate().y);

        frontOfCam  = glm::vec3(sin(-yOfCam), 0.0f, cos(-yOfCam)) * camSpeed * _TIME_PER_UPDATE;
        rightOfCam  = glm::vec3(cos(yOfCam), 0.0f, sin(yOfCam)) * camSpeed * _TIME_PER_UPDATE;

        if (!input::getKey(GLFW_KEY_LEFT_ALT)) {
            if (input::getKey(GLFW_KEY_W))
                mainCam->moveZ(-frontOfCam);
            else if (input::getKey(GLFW_KEY_S))
                mainCam->moveZ(frontOfCam);

            if (input::getKey(GLFW_KEY_A))
                mainCam->moveX(-rightOfCam);
            else if (input::getKey(GLFW_KEY_D))
                mainCam->moveX(rightOfCam);

            if (input::getKey(GLFW_KEY_Q))
                mainCam->moveY(camSpeed * _TIME_PER_UPDATE);
            else if (input::getKey(GLFW_KEY_E))
                mainCam->moveY(-camSpeed * _TIME_PER_UPDATE);
        }
        else {
            if (input::getKey(GLFW_KEY_W))
                mainCam->pitch(-camQuart * _TIME_PER_UPDATE);
            else if (input::getKey(GLFW_KEY_S))
                mainCam->pitch(camQuart * _TIME_PER_UPDATE);

            if (input::getKey(GLFW_KEY_A))
                mainCam->yaw(-camQuart * _TIME_PER_UPDATE);
            else if (input::getKey(GLFW_KEY_D))
                mainCam->yaw(camQuart * _TIME_PER_UPDATE);
        }

        // 빛 오브젝트 이동
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

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // rt.PhysicalUpdate();
            rt.Update();
            
            if (input::getMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
                getUIIdx();

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