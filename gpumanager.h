#pragma once

#include <QObject>
#include <QString>
#include <QSize>
#include <memory>
#include <vector>

#ifdef WITH_CUDA
#include <cuda_runtime.h>
#endif

class GPUManager : public QObject {
    Q_OBJECT

public:
    enum class AccelerationType {
        None,
        CUDA,
        OpenCL,
        DirectCompute,
        Metal
    };

    struct GPUDevice {
        QString name;
        size_t totalMemory;
        size_t availableMemory;
        AccelerationType type;
        int deviceId;
        bool isAvailable;
    };

    struct ProcessingCapabilities {
        bool supports4K;
        bool supports8K;
        bool supportsHDR;
        bool supportsRAW;
        size_t maxTextureSize;
        int maxThreadsPerBlock;
        QSize maxResolution;
    };

    static GPUManager& instance();

    // Device management
    bool initialize();
    QList<GPUDevice> getAvailableDevices() const;
    bool setActiveDevice(int deviceId);
    GPUDevice getCurrentDevice() const;
    ProcessingCapabilities getDeviceCapabilities() const;

    // Memory management
    bool allocateMemory(size_t size, void** ptr);
    void freeMemory(void* ptr);
    size_t getAvailableMemory() const;
    bool copyToDevice(void* dst, const void* src, size_t size);
    bool copyFromDevice(void* dst, const void* src, size_t size);

    // Processing functions
    bool processFrame(unsigned char* inputFrame, unsigned char* outputFrame,
                     int width, int height, int channels);
    bool applyEffect(const QString& effectName, unsigned char* frame,
                    int width, int height, int channels);
    bool scaleFrame(unsigned char* inputFrame, unsigned char* outputFrame,
                   int srcWidth, int srcHeight,
                   int dstWidth, int dstHeight,
                   int channels);

    // Performance monitoring
    float getGPUUsage() const;
    float getMemoryUsage() const;
    QString getPerformanceInfo() const;

signals:
    void deviceChanged(const GPUDevice& device);
    void memoryWarning(float availableMemory);
    void errorOccurred(const QString& error);
    void performanceStatus(const QString& status);

private:
    explicit GPUManager(QObject* parent = nullptr);
    ~GPUManager();

    // Internal helper functions
    bool initializeCUDA();
    bool initializeOpenCL();
    bool checkDeviceCompatibility();
    void monitorPerformance();
    void cleanupResources();

    // Device management
    bool selectBestDevice();
    void updateDeviceList();
    bool validateDevice(const GPUDevice& device) const;

    // Error handling
    QString getLastErrorMessage() const;
    void logError(const QString& error);

    // Member variables
    QList<GPUDevice> devices;
    GPUDevice currentDevice;
    ProcessingCapabilities capabilities;
    AccelerationType activeAcceleration;
    bool initialized;
    QString lastError;

    // Performance monitoring
    struct PerformanceMetrics {
        float gpuUsage;
        float memoryUsage;
        qint64 lastProcessingTime;
        int framesProcessed;
    } metrics;

    // Constants
    static const size_t MIN_REQUIRED_MEMORY;
    static const int MAX_TEXTURE_SIZE;
    static const int DEFAULT_BLOCK_SIZE;
};
