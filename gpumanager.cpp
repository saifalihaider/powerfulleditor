#include "gpumanager.h"
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <algorithm>

#ifdef WITH_CUDA
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#endif

const size_t GPUManager::MIN_REQUIRED_MEMORY = 2ULL * 1024 * 1024 * 1024; // 2GB
const int GPUManager::MAX_TEXTURE_SIZE = 16384;
const int GPUManager::DEFAULT_BLOCK_SIZE = 256;

GPUManager& GPUManager::instance() {
    static GPUManager instance;
    return instance;
}

GPUManager::GPUManager(QObject* parent)
    : QObject(parent)
    , initialized(false)
    , activeAcceleration(AccelerationType::None)
{
    // Initialize performance metrics
    metrics = {0.0f, 0.0f, 0, 0};

    // Start performance monitoring
    QTimer* performanceTimer = new QTimer(this);
    connect(performanceTimer, &QTimer::timeout,
            this, &GPUManager::monitorPerformance);
    performanceTimer->start(1000); // Monitor every second
}

GPUManager::~GPUManager() {
    cleanupResources();
}

bool GPUManager::initialize() {
    if (initialized) {
        return true;
    }

    // Try CUDA first
    if (initializeCUDA()) {
        activeAcceleration = AccelerationType::CUDA;
        initialized = true;
        return true;
    }

    // Fall back to OpenCL
    if (initializeOpenCL()) {
        activeAcceleration = AccelerationType::OpenCL;
        initialized = true;
        return true;
    }

    // No GPU acceleration available
    activeAcceleration = AccelerationType::None;
    lastError = "No GPU acceleration available";
    return false;
}

bool GPUManager::initializeCUDA() {
#ifdef WITH_CUDA
    int deviceCount = 0;
    cudaError_t error = cudaGetDeviceCount(&deviceCount);
    if (error != cudaSuccess || deviceCount == 0) {
        return false;
    }

    devices.clear();
    for (int i = 0; i < deviceCount; ++i) {
        cudaDeviceProp props;
        error = cudaGetDeviceProperties(&props, i);
        if (error != cudaSuccess) {
            continue;
        }

        GPUDevice device;
        device.name = QString::fromLatin1(props.name);
        device.totalMemory = props.totalGlobalMem;
        device.type = AccelerationType::CUDA;
        device.deviceId = i;
        device.isAvailable = true;

        // Get available memory
        size_t free, total;
        cudaMemGetInfo(&free, &total);
        device.availableMemory = free;

        devices.append(device);
    }

    return selectBestDevice();
#else
    return false;
#endif
}

bool GPUManager::initializeOpenCL() {
    // OpenCL initialization code here
    // Similar to CUDA initialization but using OpenCL API
    return false;
}

bool GPUManager::selectBestDevice() {
    if (devices.isEmpty()) {
        return false;
    }

    // Sort devices by available memory
    std::sort(devices.begin(), devices.end(),
              [](const GPUDevice& a, const GPUDevice& b) {
                  return a.availableMemory > b.availableMemory;
              });

    // Select the device with the most available memory
    for (const GPUDevice& device : devices) {
        if (device.availableMemory >= MIN_REQUIRED_MEMORY) {
            return setActiveDevice(device.deviceId);
        }
    }

    return false;
}

bool GPUManager::setActiveDevice(int deviceId) {
#ifdef WITH_CUDA
    if (activeAcceleration == AccelerationType::CUDA) {
        cudaError_t error = cudaSetDevice(deviceId);
        if (error != cudaSuccess) {
            lastError = QString("Failed to set CUDA device: %1")
                           .arg(cudaGetErrorString(error));
            return false;
        }
    }
#endif

    // Find and set the current device
    for (const GPUDevice& device : devices) {
        if (device.deviceId == deviceId) {
            currentDevice = device;
            emit deviceChanged(currentDevice);
            return true;
        }
    }

    return false;
}

bool GPUManager::allocateMemory(size_t size, void** ptr) {
#ifdef WITH_CUDA
    if (activeAcceleration == AccelerationType::CUDA) {
        cudaError_t error = cudaMalloc(ptr, size);
        if (error != cudaSuccess) {
            lastError = QString("Failed to allocate GPU memory: %1")
                           .arg(cudaGetErrorString(error));
            return false;
        }
        return true;
    }
#endif

    return false;
}

void GPUManager::freeMemory(void* ptr) {
#ifdef WITH_CUDA
    if (activeAcceleration == AccelerationType::CUDA) {
        cudaFree(ptr);
    }
#endif
}

bool GPUManager::copyToDevice(void* dst, const void* src, size_t size) {
#ifdef WITH_CUDA
    if (activeAcceleration == AccelerationType::CUDA) {
        cudaError_t error = cudaMemcpy(dst, src, size, cudaMemcpyHostToDevice);
        if (error != cudaSuccess) {
            lastError = QString("Failed to copy to GPU: %1")
                           .arg(cudaGetErrorString(error));
            return false;
        }
        return true;
    }
#endif

    return false;
}

bool GPUManager::copyFromDevice(void* dst, const void* src, size_t size) {
#ifdef WITH_CUDA
    if (activeAcceleration == AccelerationType::CUDA) {
        cudaError_t error = cudaMemcpy(dst, src, size, cudaMemcpyDeviceToHost);
        if (error != cudaSuccess) {
            lastError = QString("Failed to copy from GPU: %1")
                           .arg(cudaGetErrorString(error));
            return false;
        }
        return true;
    }
#endif

    return false;
}

bool GPUManager::processFrame(unsigned char* inputFrame, unsigned char* outputFrame,
                            int width, int height, int channels) {
    if (!initialized) {
        return false;
    }

    // Calculate frame size
    size_t frameSize = width * height * channels;

    // Allocate GPU memory
    unsigned char *d_input, *d_output;
    if (!allocateMemory(frameSize, (void**)&d_input) ||
        !allocateMemory(frameSize, (void**)&d_output)) {
        return false;
    }

    // Copy input frame to GPU
    if (!copyToDevice(d_input, inputFrame, frameSize)) {
        freeMemory(d_input);
        freeMemory(d_output);
        return false;
    }

    // Process frame on GPU (implementation depends on effect type)
    bool success = true;
#ifdef WITH_CUDA
    if (activeAcceleration == AccelerationType::CUDA) {
        // Launch CUDA kernel here
        // dim3 blockSize(16, 16);
        // dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
        //               (height + blockSize.y - 1) / blockSize.y);
        // processFrameKernel<<<gridSize, blockSize>>>(d_input, d_output,
        //                                             width, height, channels);
        cudaDeviceSynchronize();
    }
#endif

    // Copy result back to CPU
    if (success) {
        success = copyFromDevice(outputFrame, d_output, frameSize);
    }

    // Clean up
    freeMemory(d_input);
    freeMemory(d_output);

    return success;
}

bool GPUManager::applyEffect(const QString& effectName, unsigned char* frame,
                           int width, int height, int channels) {
    if (!initialized) {
        return false;
    }

    // Implementation depends on effect type
    // This is a placeholder for actual effect implementation
    return true;
}

bool GPUManager::scaleFrame(unsigned char* inputFrame, unsigned char* outputFrame,
                          int srcWidth, int srcHeight,
                          int dstWidth, int dstHeight,
                          int channels) {
    if (!initialized) {
        return false;
    }

    // Calculate frame sizes
    size_t srcSize = srcWidth * srcHeight * channels;
    size_t dstSize = dstWidth * dstHeight * channels;

    // Allocate GPU memory
    unsigned char *d_input, *d_output;
    if (!allocateMemory(srcSize, (void**)&d_input) ||
        !allocateMemory(dstSize, (void**)&d_output)) {
        return false;
    }

    // Copy input frame to GPU
    if (!copyToDevice(d_input, inputFrame, srcSize)) {
        freeMemory(d_input);
        freeMemory(d_output);
        return false;
    }

    // Scale frame on GPU
    bool success = true;
#ifdef WITH_CUDA
    if (activeAcceleration == AccelerationType::CUDA) {
        // Launch CUDA scaling kernel here
        // dim3 blockSize(16, 16);
        // dim3 gridSize((dstWidth + blockSize.x - 1) / blockSize.x,
        //               (dstHeight + blockSize.y - 1) / blockSize.y);
        // scaleFrameKernel<<<gridSize, blockSize>>>(d_input, d_output,
        //                                           srcWidth, srcHeight,
        //                                           dstWidth, dstHeight,
        //                                           channels);
        cudaDeviceSynchronize();
    }
#endif

    // Copy result back to CPU
    if (success) {
        success = copyFromDevice(outputFrame, d_output, dstSize);
    }

    // Clean up
    freeMemory(d_input);
    freeMemory(d_output);

    return success;
}

float GPUManager::getGPUUsage() const {
    return metrics.gpuUsage;
}

float GPUManager::getMemoryUsage() const {
    return metrics.memoryUsage;
}

QString GPUManager::getPerformanceInfo() const {
    return QString("GPU Usage: %1%, Memory Usage: %2%, "
                  "Frames Processed: %3, Last Processing Time: %4ms")
        .arg(metrics.gpuUsage, 0, 'f', 1)
        .arg(metrics.memoryUsage, 0, 'f', 1)
        .arg(metrics.framesProcessed)
        .arg(metrics.lastProcessingTime);
}

void GPUManager::monitorPerformance() {
#ifdef WITH_CUDA
    if (activeAcceleration == AccelerationType::CUDA) {
        // Get GPU utilization
        // Note: This requires NVML library
        // nvmlDevice_t device;
        // nvmlDeviceGetHandleByIndex(currentDevice.deviceId, &device);
        // nvmlUtilization_t utilization;
        // nvmlDeviceGetUtilizationRates(device, &utilization);
        // metrics.gpuUsage = utilization.gpu;

        // Get memory utilization
        size_t free, total;
        cudaMemGetInfo(&free, &total);
        metrics.memoryUsage = 100.0f * (1.0f - float(free) / float(total));

        emit performanceStatus(getPerformanceInfo());
    }
#endif
}

void GPUManager::cleanupResources() {
    if (!initialized) {
        return;
    }

#ifdef WITH_CUDA
    if (activeAcceleration == AccelerationType::CUDA) {
        cudaDeviceReset();
    }
#endif

    initialized = false;
}
