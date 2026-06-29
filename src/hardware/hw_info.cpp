#include "hw_info.hpp"

#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

// macOS system headers
#include <sys/sysctl.h>
#include <sys/types.h>
#include <mach/mach.h>

// IOKit for GPU core count via AGXAccelerator
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

namespace macbenchforge {

// ── HardwareDetector ──────────────────────────────────────────────────────────

HardwareInfo HardwareDetector::detect() {
    HardwareInfo hw;

    hw.chip_name        = chip_name();
    hw.perf_cores       = perf_cores();
    hw.efficiency_cores = efficiency_cores();
    hw.total_cores      = total_cores();
    hw.gpu_cores        = gpu_cores();
    hw.memory_bytes     = memory_bytes();
    hw.memory_label     = memory_label(hw.memory_bytes);
    hw.macos_version    = macos_version();
    hw.cpu_arch         = "arm64";

    uint64_t vram_limit_mb = 0;
    size_t vram_size = sizeof(vram_limit_mb);
    if (sysctlbyname("iogpu.wired_limit_mb", &vram_limit_mb, &vram_size, nullptr, 0) == 0 && vram_limit_mb > 0) {
        hw.vram_limit_bytes = vram_limit_mb * 1024ULL * 1024ULL;
    } else {
        hw.vram_limit_bytes = static_cast<uint64_t>(hw.memory_bytes * 0.75); // fallback
    }

    return hw;
}

// ── sysctl helpers ────────────────────────────────────────────────────────────

namespace {

// Read a string value from sysctl by name.
// Returns empty string on failure.
std::string sysctl_string(const char* name) {
    size_t size = 0;
    if (sysctlbyname(name, nullptr, &size, nullptr, 0) != 0 || size == 0)
        return "";
    std::string result(size, '\0');
    if (sysctlbyname(name, result.data(), &size, nullptr, 0) != 0)
        return "";
    // Trim null terminator
    while (!result.empty() && result.back() == '\0')
        result.pop_back();
    return result;
}

// Read an int value from sysctl by name.
// Returns 0 on failure.
int sysctl_int(const char* name) {
    int value = 0;
    size_t size = sizeof(value);
    sysctlbyname(name, &value, &size, nullptr, 0);
    return value;
}

// Read a uint64_t value from sysctl by name.
// Returns 0 on failure.
uint64_t sysctl_uint64(const char* name) {
    uint64_t value = 0;
    size_t size = sizeof(value);
    sysctlbyname(name, &value, &size, nullptr, 0);
    return value;
}

} // anonymous namespace

// ── Individual Detectors ──────────────────────────────────────────────────────

std::string HardwareDetector::chip_name() {
    // On Apple Silicon, machdep.cpu.brand_string returns "Apple M1 Pro" etc.
    std::string name = sysctl_string("machdep.cpu.brand_string");
    if (!name.empty()) return name;

    // Fallback: read hw.model (returns board identifier e.g. "MacBookPro18,3")
    // Not ideal but better than nothing
    std::string model = sysctl_string("hw.model");
    if (!model.empty()) return model;

    return "Apple Silicon";
}

int HardwareDetector::perf_cores() {
    // hw.perflevel0 = performance cores on Apple Silicon
    int cores = sysctl_int("hw.perflevel0.physicalcpu");
    if (cores > 0) return cores;

    // Fallback for older sysctl key
    return sysctl_int("hw.physicalcpu");
}

int HardwareDetector::efficiency_cores() {
    // hw.perflevel1 = efficiency cores on Apple Silicon
    // Returns 0 on chips with no efficiency cores (e.g. M1/M2 base)
    return sysctl_int("hw.perflevel1.physicalcpu");
}

int HardwareDetector::total_cores() {
    int total = sysctl_int("hw.logicalcpu");
    if (total > 0) return total;
    return perf_cores() + efficiency_cores();
}

int HardwareDetector::gpu_cores() {
    // Query IOKit AGXAccelerator for Metal GPU core count
    // This is the only reliable way to get GPU cores on Apple Silicon
    io_service_t service = IOServiceGetMatchingService(
        kIOMainPortDefault,
        IOServiceMatching("AGXAccelerator")
    );

    if (!service) return 0;

    // Try "gpu-core-count" property first
    CFTypeRef prop = IORegistryEntryCreateCFProperty(
        service,
        CFSTR("gpu-core-count"),
        kCFAllocatorDefault,
        0
    );

    int cores = 0;

    if (prop) {
        if (CFGetTypeID(prop) == CFNumberGetTypeID()) {
            CFNumberGetValue(
                (CFNumberRef)prop,
                kCFNumberIntType,
                &cores
            );
        }
        CFRelease(prop);
    }

    // Fallback: try "GPUCoreCount" (seen on some macOS versions)
    if (cores == 0) {
        CFTypeRef prop2 = IORegistryEntryCreateCFProperty(
            service,
            CFSTR("GPUCoreCount"),
            kCFAllocatorDefault,
            0
        );
        if (prop2) {
            if (CFGetTypeID(prop2) == CFNumberGetTypeID()) {
                CFNumberGetValue(
                    (CFNumberRef)prop2,
                    kCFNumberIntType,
                    &cores
                );
            }
            CFRelease(prop2);
        }
    }

    IOObjectRelease(service);
    return cores;
}

uint64_t HardwareDetector::memory_bytes() {
    return sysctl_uint64("hw.memsize");
}

std::string HardwareDetector::memory_label(uint64_t bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0);

    // Apple reports unified memory in clean GB increments
    double gb = (double)bytes / (1024.0 * 1024.0 * 1024.0);

    // Round to nearest integer for clean display (8 GB, 16 GB, 32 GB, 64 GB)
    int gb_rounded = (int)std::round(gb);
    oss << gb_rounded << " GB";

    return oss.str();
}

std::string HardwareDetector::macos_version() {
    // kern.osproductversion returns e.g. "14.5"
    std::string version = sysctl_string("kern.osproductversion");
    if (!version.empty()) return version;
    return "Unknown";
}

// ── MemoryPoller ──────────────────────────────────────────────────────────────

MemoryPoller::MemoryPoller(int poll_interval_ms)
    : poll_interval_ms_(poll_interval_ms)
    , running_(false)
    , peak_ram_mb_(0.0)
{}

MemoryPoller::~MemoryPoller() {
    if (running_) stop();
}

void MemoryPoller::start() {
    running_ = true;
    thread_  = std::thread(&MemoryPoller::poll_loop, this);
}

void MemoryPoller::stop() {
    running_ = false;
    if (thread_.joinable())
        thread_.join();
}

void MemoryPoller::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    peak_ram_mb_ = 0.0;
}

double MemoryPoller::peak_ram_mb() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return peak_ram_mb_;
}

void MemoryPoller::poll_loop() {
    while (running_) {
        uint64_t bytes = current_resident_bytes();
        double   mb    = (double)bytes / (1024.0 * 1024.0);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (mb > peak_ram_mb_)
                peak_ram_mb_ = mb;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(poll_interval_ms_)
        );
    }
}

uint64_t MemoryPoller::current_resident_bytes() {
    // Use mach_task_basic_info -- most accurate for resident memory
    // on macOS. Returns the physical footprint of the process.
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;

    kern_return_t kr = task_info(
        mach_task_self(),
        MACH_TASK_BASIC_INFO,
        (task_info_t)&info,
        &count
    );

    if (kr != KERN_SUCCESS) return 0;
    return (uint64_t)info.resident_size;
}

} // namespace macbenchforge