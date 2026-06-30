#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// Holds the indices of the GPU "queue families" we need:
// one able to do graphics rendering, one able to present images to the screen
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; // index of the queue that can do graphics rendering
    std::optional<uint32_t> presentFamily;  // index of the queue that can present images to the screen

    // Checks that both indices above have actually been found
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

// Holds information about what the GPU supports for displaying images
// (color formats, presentation modes, etc.)
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;       // limits and capabilities of the display surface
    std::vector<VkSurfaceFormatKHR> formats;     // supported color formats
    std::vector<VkPresentModeKHR> presentModes;  // possible ways to present images (vsync, etc.)
};

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }
};

const std::vector<Vertex> vertices = {
    {{0.0f,-0.5f},  {1.0f,1.0f,1.0f}},
    {{0.5f,0.5f}, {0.0f,1.0f,0.0f}},
    {{-0.5f, 0.5f}, {0.0f,0.0f,1.0f}}
};

class BasicApp {
public:

    // Runs the application: opens the window, initializes Vulkan,
    // starts the render loop, then cleans everything up at the end
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window; // the application window created by GLFW
    VkInstance instance; // main entry point into the Vulkan API
    VkDebugUtilsMessengerEXT debugMessenger; // object that receives Vulkan debug messages
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // the chosen physical GPU
    VkDevice device; // "virtual GPU" used to communicate with the physical device
    VkQueue graphicsQueue; // queue used to submit rendering commands
    VkSurfaceKHR surface; // surface representing the window Vulkan draws onto
    VkQueue presentQueue; // queue used to present images to the screen
    VkSwapchainKHR swapChain; // set of images used in turn for displaying
    std::vector<VkImage> swapChainImages; // images belonging to the swap chain
    VkFormat swapChainImageFormat; // color format of the swap chain images
    VkExtent2D swapChainExtent; // size (width/height) of the swap chain images
    std::vector<VkImageView> swapChainImageViews; // "views" used to access the swap chain images
    VkRenderPass renderPass; // describes the rendering steps and targets (color, depth, etc.)
    VkPipelineLayout pipelineLayout; // describes the resources (uniforms, etc.) used by the graphics pipeline
    VkPipeline graphicsPipeline; // the full graphics pipeline (shaders, render configuration)
    std::vector<VkFramebuffer> swapChainFramebuffers; // render targets associated with each swap chain image
    VkCommandPool commandPool; // pool from which command buffers are allocated
    std::vector<VkCommandBuffer> commandBuffers; // lists of commands sent to the GPU to draw
    std::vector<VkSemaphore> imageAvailableSemaphores; // signal that an image is ready to be drawn into
    std::vector<VkSemaphore> renderFinishedSemaphores; // signal that rendering of an image has finished
    std::vector<VkFence> inFlightFences; // let the CPU wait until a frame's work is finished
    uint32_t currentFrame = 0; // index of the frame currently being processed (for multiple frames in flight)
    bool framebufferResized = false; // true if the window was resized and the swap chain needs recreating
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    // Creates the application window using GLFW
    void initWindow();

    // Initializes Vulkan by calling, in order, all the creation functions
    // needed (instance, device, swap chain, pipeline...)
    void initVulkan();

    // Main application loop: as long as the window isn't closed,
    // process events and draw a new frame
    void mainLoop();

    // Properly releases all the Vulkan and GLFW resources created by the application
    void cleanup();

    // instance

    // Creates the Vulkan instance, the main entry point of the API,
    // specifying the required extensions and validation layers
    void createInstance();

    // Checks that the requested validation layers (useful for debugging)
    // are actually available on the machine
    bool checkValidationLayerSupport(); // validation layer help in debug

    // Builds the list of extensions Vulkan needs,
    // adding the ones required by GLFW and debugging if needed
    std::vector<const char*> getRequiredExtensions();

    // debug

    // Callback function called by Vulkan to print validation-related
    // error, warning, or informational messages
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    // Fills in a configuration struct defining which debug messages
    // should be captured and which function should handle them
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    // Creates the debug "messenger" that will receive Vulkan
    // validation messages while the application runs
    void setupDebugMessenger();

    // physical device

    // Goes through the available GPUs and picks
    // the one suitable for running the application
    void pickPhysicalDevice();

    // Checks whether a given GPU has all the features
    // the application needs (queues, extensions, swap chain...)
    bool isDeviceSuitable(VkPhysicalDevice device);

    // queue family

    // Looks through a GPU's queue families to find
    // the ones capable of rendering and presenting
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    // logical device

    // Creates the "logical device", the interface the application
    // uses to talk to the chosen physical GPU
    void createLogicalDevice();

    // surface

    // Creates the Vulkan surface tied to the window, i.e.
    // the area the application will be able to draw onto
    void createSurface();

    // swapchain

    // Checks that the GPU supports the required extensions,
    // in particular the swap chain extension
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    // Retrieves the GPU's display capabilities
    // for the window's surface (formats, modes, resolutions)
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    // surface format

    // Picks the best available color format
    // among those supported by the surface
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    // Picks the mode used to present images to the screen
    // (for example with or without vertical sync)
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    // swap extent

    // Determines the resolution of the swap chain images
    // based on the current size of the window
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    // create swapchain

    // Creates the swap chain, the set of images used in turn
    // to display the rendering result on screen
    void createSwapChain();

    // image view

    // Creates a "view" for each swap chain image,
    // letting Vulkan know how to access those images
    void createImageViews();

    // graphics pipeline

    // Creates the full graphics pipeline: shaders, rasterizer
    // configuration, color blending, etc., used to draw the triangle
    void createGraphicsPipeline();

    // readfile

    // Reads the binary content of a file (e.g. a compiled shader)
    // and returns it as a byte array
    static std::vector<char> readFile(const std::string& filename);

    // shader module

    // Turns the binary code of a compiled shader into a Vulkan
    // object usable by the graphics pipeline
    VkShaderModule createShaderModule(const std::vector<char>& code);

    // render pass

    // Creates the "render pass", which describes the rendering
    // steps and the target images used (color, depth...)
    void createRenderPass();

    // frame buffer

    // Creates a framebuffer for each swap chain image,
    // i.e. the actual target Vulkan will draw onto
    void createFramebuffers();

    // command pool

    // Creates the command pool, a kind of reservoir
    // from which command buffers will be allocated
    void createCommandPool();

    // command buffer

    // Allocates the command buffers that will hold
    // the drawing instructions sent to the GPU
    void createCommandBuffers();

    // Records into a command buffer the sequence of instructions
    // needed to draw a frame (start render pass, draw, end)
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    // drawframe

    // Draws and presents a new frame: acquires an image
    // from the swap chain, submits the render commands, then presents it
    void drawFrame();


    // Creates the semaphores and fences used to synchronize
    // the CPU and GPU while rendering the different frames
    void createSyncObjects();

    // Recreates the swap chain and everything depending on it,
    // for example when the window has been resized
    void recreateSwapChain();

    // Destroys the swap chain and the objects depending on it,
    // ahead of recreating it or shutting down the application
    void cleanupSwapChain();

    // Callback function called by GLFW when the window
    // is resized, to signal that the swap chain needs recreating
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void createVertexBuffer();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};