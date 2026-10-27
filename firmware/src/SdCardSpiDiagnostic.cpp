// EnvNode: see repository LICENSE.
#include "SdCardSpiDiagnostic.h"
#include "SdCardDiagnostic.h"
#include "SdStorageArbiter.h"
#include "FirmwareBuildInfo.h"
#include "SdSystemLogService.h"
#include "SdCardMountPath.h"
#include "SdFatGeometry.h"
#include "SdRotationTest.h"
// Arduino binary literals collide with termios baud-rate names included by VFS.
// Isolate the SDK header, then restore the Arduino definitions for this source.
#pragma push_macro("B110")
#pragma push_macro("B1000000")
#undef B110
#undef B1000000
#include "esp_vfs.h"
#pragma pop_macro("B1000000")
#pragma pop_macro("B110")
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <new>
#include <atomic>
#include "sd_diskio.h"
#include "ff.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
namespace EnvNode {
namespace {
// Shared SPI wiring in the supported EnvNode core/mainboards.
static_assert(sizeof(SdCardMountPath) <= ESP_VFS_PATH_MAX - 1,
    "FAT VFS mount path must survive the legacy strlcpy truncation during unregister");
char activeSdLogPath[96]={};
bool rotateSdLog=false; // Protected by SdStorageLease.
constexpr uint8_t ClockPin = 18;
constexpr uint8_t MisoPin = 19;
constexpr uint8_t MosiPin = 23;
constexpr uint32_t DiagnosticClockHz = 1000000;
bool usesDiagnosticPin(const HardwareResourceAssignment& assignment,
    const BoardModuleSlotCapability& slot) {
    if (assignment.kind != HardwareResourceKind::GPIO) return false;
    const uint8_t pin = assignment.gpio.number;
    return pin == ClockPin || pin == MisoPin || pin == MosiPin
        || pin == slot.spiChipSelect.number || pin == slot.auxGpio1.number;
}
class PosixSdTestFile : public ISdTestFile {
public:
    ~PosixSdTestFile() override { if (fd_ >= 0) ::close(fd_); }
    SdTestFileCreate createExclusive(const char* path) override {
        char full[80]; fullPath(path, full, sizeof(full));
        fd_ = ::open(full, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd_ >= 0) return SdTestFileCreate::Created;
        return errno == EEXIST ? SdTestFileCreate::Exists : SdTestFileCreate::Failed;
    }
    int write(const uint8_t* data, size_t size) override { return ::write(fd_, data, size); }
    bool sync() override { return ::fsync(fd_) == 0; }
    bool close() override {
        const int fd = fd_; fd_ = -1;
        return fd >= 0 && ::close(fd) == 0;
    }
    bool openRead(const char* path) override {
        char full[80]; fullPath(path, full, sizeof(full));
        fd_ = ::open(full, O_RDONLY);
        return fd_ >= 0;
    }
    int read(uint8_t* data, size_t size) override { return ::read(fd_, data, size); }
    bool remove(const char* path) override {
        char full[80]; fullPath(path, full, sizeof(full));
        return ::unlink(full) == 0;
    }
private:
    static void fullPath(const char* path, char* output, size_t capacity) {
        snprintf(output, capacity, "%s%s", SdCardMountPath, path);
    }
    int fd_ = -1;
};
class SpiDiagnosticDevice : public ISdCardDiagnosticDevice {
public:
    explicit SpiDiagnosticDevice(const BoardModuleSlotCapability& slot,const char* operation="sd-access")
        : slot_(slot), spi_(VSPI),operation_(operation) {}
    bool cardDetected() override {
        // External 10k pull-up to 3V3 as on the SD module; internal pull-up is a fallback.
        pinMode(slot_.auxGpio1.number, INPUT_PULLUP);
        delay(5);
        return digitalRead(slot_.auxGpio1.number) == HIGH;
    }
    bool mount() override {
        digitalWrite(slot_.spiChipSelect.number, HIGH);
        pinMode(slot_.spiChipSelect.number, OUTPUT);
        spi_.begin(ClockPin, MisoPin, MosiPin, slot_.spiChipSelect.number);
        // Last argument MUST remain false: never format a card during diagnostics.
        const uint32_t started=millis(),heap=ESP.getFreeHeap(),largest=ESP.getMaxAllocHeap();
        const bool mounted=SD.begin(slot_.spiChipSelect.number, spi_, DiagnosticClockHz, SdCardMountPath, 2, false);
        if(!mounted) log_w("SD mount failed: operation=%s elapsed_ms=%lu heap_before=%lu largest_before=%lu heap_after=%lu largest_after=%lu fatfs_bytes=%u",
            operation_,static_cast<unsigned long>(millis()-started),static_cast<unsigned long>(heap),
            static_cast<unsigned long>(largest),static_cast<unsigned long>(ESP.getFreeHeap()),
            static_cast<unsigned long>(ESP.getMaxAllocHeap()),static_cast<unsigned>(sizeof(FATFS)));
        return mounted;
    }
    SdCardDiagnosticInfo information() override {
        SdCardDiagnosticInfo info;
        switch (SD.cardType()) {
            case CARD_MMC: info.type = "MMC"; break;
            case CARD_SD: info.type = "SDSC"; break;
            case CARD_SDHC: info.type = "SDHC/SDXC"; break;
            default: break;
        }
        info.cardBytes = SD.cardSize();
        // Never call totalBytes()/usedBytes(): f_getfree scans the whole FAT after
        // remount, potentially blocking the application for tens of seconds.
        info.spaceUsageKnown=false;
        uint8_t sector[512];SdFatGeometry geometry;
        bool parsed=false;
        if(SD.readRAW(sector,0)) {
            parsed=parseSdFatBoot(sector,0,SD.numSectors(),geometry);
            if(!parsed) {
                uint32_t starts[4]={};
                if(sector[510]==0x55 && sector[511]==0xAA)
                    for(unsigned part=0;part<4;++part) for(unsigned byte=0;byte<4;++byte)
                        starts[part]|=static_cast<uint32_t>(sector[454+16*part+byte])<<(8*byte);
                for(uint32_t start:starts) if(start && start<SD.numSectors() && SD.readRAW(sector,start)
                    && parseSdFatBoot(sector,start,SD.numSectors(),geometry)) {parsed=true;break;}
            }
        }
        if(parsed) {
            info.filesystemBytes=geometry.dataBytes;
            if(geometry.infoSector!=UINT32_MAX && SD.readRAW(sector,geometry.infoSector)
                && parseSdFatFreeHint(sector,geometry,info.usedBytes)) {
                info.spaceUsageKnown=true;info.spaceUsageEstimated=true;
            }
        }
        File root = SD.open("/", FILE_READ);
        info.rootReadable = root && root.isDirectory();
        root.close();
        return info;
    }
    bool writeReadTest(ILogger& logger) override {
        PosixSdTestFile file;
        return runSdWriteReadTest(file, logger);
    }
    void release() override {
        SD.end();
        spi_.end();
        digitalWrite(slot_.spiChipSelect.number, HIGH);
    }
private:
    const BoardModuleSlotCapability& slot_;
    SPIClass spi_;
    const char* operation_;
};
}
void runSdCardSpiDiagnostic(ModuleSlot slotId, const BoardProfile& board,
    const Configuration& configuration, const AutomaticActuatorDefinition* automaticActuators,
    size_t automaticCount, ILogger& logger) {
    SdStorageLease lease;
    if(!lease || !sdRuntimeStable() || !configuration.sdStorage.enabled || slotId!=configuration.sdStorage.slot) {
        logger.warn("SD diagnostic: storage disabled or busy; skipped");return;
    }
    const BoardModuleSlotCapability* slot = boardModuleSlot(board, slotId);
    if (!slot || !slot->fiveVoltSupplyAvailable) {
        logger.warn("SD diagnostic: requested slot unavailable; skipped");
        return;
    }
    for (const auto& sensor : configuration.sensorSlots) {
        if (sensor.enabled && usesDiagnosticPin(sensor.hardware, *slot)) {
            logger.warn("SD diagnostic: GPIO conflict with configured sensor; skipped");
            return;
        }
    }
    for (const auto& actuator : configuration.actuatorSlots) {
        if (actuator.enabled && usesDiagnosticPin(actuator.hardware, *slot)) {
            logger.warn("SD diagnostic: GPIO conflict with configured actuator; skipped");
            return;
        }
    }
    for (size_t index = 0; index < automaticCount; ++index) {
        if (usesDiagnosticPin(automaticActuators[index].hardware, *slot)) {
            logger.warn("SD diagnostic: GPIO conflict with module actuator; skipped");
            return;
        }
    }
    logger.infof("SD diagnostic: board=%s slot=%s SCK=%u MISO=%u MOSI=%u CS=%u CD=%u clock_hz=%lu",
        board.displayName, moduleSlotName(slotId), ClockPin, MisoPin, MosiPin,
        slot->spiChipSelect.number, slot->auxGpio1.number,
        static_cast<unsigned long>(DiagnosticClockHz));
    SpiDiagnosticDevice device(*slot);
    runSdCardDiagnostic(device, logger);
}

namespace {
const BoardModuleSlotCapability* sdFileSlot(ModuleSlot slotId, const BoardProfile& board,
    const Configuration& configuration, const ActuatorRuntime& actuators, SdDirectoryStatus& status) {
    if(!configuration.sdStorage.enabled) {status=SdDirectoryStatus::Disabled;return nullptr;}
    if(!sdRuntimeStable()) {status=SdDirectoryStatus::Busy;return nullptr;}
    if(slotId!=configuration.sdStorage.slot) {status=SdDirectoryStatus::Conflict;return nullptr;}
    const auto* slot = boardModuleSlot(board, slotId);
    if (!slot || !slot->fiveVoltSupplyAvailable) { status=SdDirectoryStatus::Conflict; return nullptr; }
    for (const auto& sensor : configuration.sensorSlots) {
        if (sensor.enabled && usesDiagnosticPin(sensor.hardware, *slot)) { status=SdDirectoryStatus::Conflict; return nullptr; }
    }
    for (const auto& actuator : configuration.actuatorSlots) {
        if (actuator.enabled && usesDiagnosticPin(actuator.hardware, *slot)) { status=SdDirectoryStatus::Conflict; return nullptr; }
    }
    for (size_t i=0; i<actuators.runtimeCount(); ++i) {
        ActuatorRuntimeInfo info;
        if (actuators.runtimeInfo(i, info) && usesDiagnosticPin(info.hardware, *slot)) { status=SdDirectoryStatus::Conflict; return nullptr; }
    }
    return slot;
}
template<class ReadSector>
uint64_t fingerprint(uint64_t capacity,uint32_t sectors,ReadSector readSector) {
    uint8_t sector[512];
    if (!readSector(sector,0)) return 0;
    uint64_t hash=14695981039346656037ULL;
    const auto add=[&]() { for (uint8_t byte:sector) { hash^=byte; hash*=1099511628211ULL; } };
    // Include partition table/superfloppy boot sector and first partition boot sector.
    uint32_t first=0;
    for (unsigned i=0;i<4;++i) first|=static_cast<uint32_t>(sector[454+i])<<(8*i);
    add();
    const bool boot=(sector[0]==0xEB || sector[0]==0xE9) && sector[11]==0 && sector[12]==2;
    if (!boot && sector[510]==0x55 && sector[511]==0xAA && first>0 && first<sectors) {
        if (!readSector(sector,first)) return 0;
        add();
    }
    hash^=capacity; hash*=1099511628211ULL;
    return hash ? hash : 1;
}
uint64_t mediaFingerprint() {
    return fingerprint(SD.cardSize(),SD.numSectors(),[](uint8_t* data,uint32_t sector) {return SD.readRAW(data,sector);});
}
class StoredSdFile : public ISdStoredFile {
public:
    explicit StoredSdFile(const BoardModuleSlotCapability& slot) : device_(slot), cd_(slot.auxGpio1.number) {}
    ~StoredSdFile() override { if(fd_>=0) ::close(fd_); if(mounted_) device_.release(); }
    bool open(const String& path, SdDirectoryStatus& status) {
        if(!lease_) {status=SdDirectoryStatus::Busy;return false;}
        if(path==activeSdLogPath) rotateSdLog=true;
        if(!device_.cardDetected()) {status=SdDirectoryStatus::NoCard; return false;}
        mounted_=true;
        if(!device_.mount()) {status=SdDirectoryStatus::MountFailed; return false;}
        snprintf(path_,sizeof(path_),"%s%s",SdCardMountPath,path.c_str());
        fd_=::open(path_,O_RDONLY);
        if(fd_<0) {status=errno==ENOENT ? SdDirectoryStatus::NotFound : SdDirectoryStatus::ReadFailed; return false;}
        struct stat descriptor,pathInfo;
        if(::fstat(fd_,&descriptor)!=0 || ::stat(path_,&pathInfo)!=0
            || descriptor.st_size<0 || pathInfo.st_size<0) {
            status=SdDirectoryStatus::ReadFailed; return false;
        }
        // ESP-IDF FAT fstat() sets st_mtime=0. Use stat(path) consistently for
        // preview, confirmation and the final pre-unlink comparison.
        if(!captureSdFileStamp(
            SdFileMetadata(descriptor.st_size,descriptor.st_mtime,S_ISREG(descriptor.st_mode)),
            SdFileMetadata(pathInfo.st_size,pathInfo.st_mtime,S_ISREG(pathInfo.st_mode)),
            mediaFingerprint(),stamp_)) {status=SdDirectoryStatus::ReadFailed; return false;}
        status=SdDirectoryStatus::Ready; return true;
    }
    SdFileStamp stamp() const override { return stamp_; }
    int read(uint8_t* data,size_t size) override {
        if(digitalRead(cd_)!=HIGH) return -1;
        return ::read(fd_,data,size);
    }
    const char* lastError() const override {return error_;}
    int errorCode() const override {return errorCode_;}
    bool remove() override {
        error_="none";errorCode_=0;
        if(digitalRead(cd_)!=HIGH) {error_="card removed";return false;}
        struct stat info;
        if(::stat(path_,&info)!=0) {error_="stat";errorCode_=errno;return false;}
        if(!S_ISREG(info.st_mode) || info.st_size<0) {error_="not a regular file";return false;}
        const SdFileStamp current{static_cast<uint64_t>(info.st_size),static_cast<int64_t>(info.st_mtime),mediaFingerprint()};
        if(!sameSdFileStamp(stamp_,current)) {error_="file/media stamp changed";return false;}
        const int fd=fd_; fd_=-1;
        if(::close(fd)!=0) {error_="close";errorCode_=errno;return false;}
        if(::unlink(path_)!=0) {error_="unlink";errorCode_=errno;return false;}
        return true;
    }
private:
    SdStorageLease lease_;
    SpiDiagnosticDevice device_; uint8_t cd_; bool mounted_=false;
    int fd_=-1; char path_[256]={}; SdFileStamp stamp_;
    const char* error_="none";int errorCode_=0;
};
}
bool sdStoragePinsAvailable(const Configuration& configuration,const ActuatorRuntime& actuators) {
    SdDirectoryStatus status;return sdFileSlot(configuration.sdStorage.slot,currentBoardProfile(),configuration,actuators,status)!=nullptr;
}
std::unique_ptr<ISdStoredFile> openSdCardFile(ModuleSlot slotId,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators,
    const String& path,SdDirectoryStatus& status) {
    if(!validSdDirectoryPath(path) || path=="/") {status=SdDirectoryStatus::InvalidPath; return {};}
    const auto* slot=sdFileSlot(slotId,board,configuration,actuators,status);
    if(!slot) return {};
    std::unique_ptr<StoredSdFile> file(new(std::nothrow) StoredSdFile(*slot));
    if(!file) {status=SdDirectoryStatus::ReadFailed; return {};}
    if(!file->open(path,status)) return {};
    return std::unique_ptr<ISdStoredFile>(file.release());
}

bool inspectSdCardForFormat(ModuleSlot slotId,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators,
    SdFileStamp& card,SdDirectoryStatus& status) {
    SdStorageLease lease;
    if(!lease) {status=SdDirectoryStatus::Busy;return false;}
    const auto* slot=sdFileSlot(slotId,board,configuration,actuators,status);
    if(!slot) return false;
    SpiDiagnosticDevice device(*slot);
    if(!device.cardDetected()) {status=SdDirectoryStatus::NoCard;return false;}
    const bool mounted=device.mount();
    const auto info=mounted ? device.information() : SdCardDiagnosticInfo();
    const uint64_t media=mounted ? mediaFingerprint() : 0;
    device.release();
    if(!mounted || !info.rootReadable || !media) {status=SdDirectoryStatus::MountFailed;return false;}
    card=SdFileStamp(info.cardBytes,0,media);status=SdDirectoryStatus::Ready;return true;
}
namespace {
// The raw driver gives us its actual physical drive number; never assume drive 0.
// No file handles are open while f_mkfs invalidates/recreates the entire volume.
class Fat32FormatDevice : public ISdFormatDevice {
public:
    explicit Fat32FormatDevice(const BoardModuleSlotCapability& slot) : slot_(slot),spi_(VSPI) {}
    ~Fat32FormatDevice() override {release();}
    bool inspect(SdFileStamp& card) override {
        pinMode(slot_.auxGpio1.number,INPUT_PULLUP);
        if(digitalRead(slot_.auxGpio1.number)!=HIGH) {error_=100;return false;}
        digitalWrite(slot_.spiChipSelect.number,HIGH);pinMode(slot_.spiChipSelect.number,OUTPUT);
        spi_.begin(ClockPin,MisoPin,MosiPin,slot_.spiChipSelect.number);
        drive_=sdcard_init(slot_.spiChipSelect.number,&spi_,DiagnosticClockHz);
        if(drive_==0xFF) {error_=101;return false;}
        if(!sdcard_mount(drive_,SdCardMountPath,2,false)) {error_=102;return false;}
        const uint32_t sectors=sdcard_num_sectors(drive_);
        const uint64_t capacity=static_cast<uint64_t>(sectors)*512;
        card=SdFileStamp(capacity,0,fingerprint(capacity,sectors,[this](uint8_t* data,uint32_t sector){return sd_read_raw(drive_,data,sector);}));
        if(!card.media) {error_=103;return false;}
        return true;
    }
    bool format() override {
        if(digitalRead(slot_.auxGpio1.number)!=HIGH) {error_=100;return false;}
#if FF_MULTI_PARTITION
        // Only an automatic physical-drive mapping formats the complete card.
        if(VolToPart[drive_].pd!=drive_ || VolToPart[drive_].pt!=0) {error_=104;return false;}
#endif
        char volume[4];snprintf(volume,sizeof(volume),"%u:",drive_);
        BYTE work[4096];
        const FRESULT result=f_mkfs(volume,FM_FAT32,0,work,sizeof(work));
        if(result!=FR_OK) {error_=static_cast<int>(result);return false;}
        release(); // Discard old FAT metadata and mount afresh for verification.
        return true;
    }
    bool verify() override {
        SpiDiagnosticDevice device(slot_);
        if(!device.cardDetected()) {error_=100;return false;}
        if(!device.mount()) {device.release();error_=105;return false;}
        const auto info=device.information();
        char rootPath[32];snprintf(rootPath,sizeof(rootPath),"%s/",SdCardMountPath);
        DIR* root=::opendir(rootPath);
        bool empty=root!=nullptr;
        if(root) {
            while(true) {
                errno=0;dirent* entry=::readdir(root);
                if(!entry) {if(errno) empty=false;break;}
                if(strcmp(entry->d_name,".") && strcmp(entry->d_name,"..")) {empty=false;break;}
            }
            if(::closedir(root)!=0) empty=false;
        }
        uint8_t boot[512];
        bool fat32=false;
        if(SD.readRAW(boot,0)) {
            uint32_t first=0;
            for(unsigned i=0;i<4;++i) first|=static_cast<uint32_t>(boot[454+i])<<(8*i);
            fat32=first>0 && first<SD.numSectors() && SD.readRAW(boot,first)
                && !memcmp(boot+82,"FAT32   ",8);
        }
        const bool present=digitalRead(slot_.auxGpio1.number)==HIGH;
        device.release();
        const bool valid=present && fat32 && empty && info.rootReadable && info.filesystemBytes>0
            && info.filesystemBytes<=info.cardBytes && info.usedBytes<=info.filesystemBytes;
        if(!valid) error_=106;
        return valid;
    }
    int error() const {return error_;}
private:
    void release() {
        if(drive_!=0xFF) {sdcard_unmount(drive_);sdcard_uninit(drive_);drive_=0xFF;}
        spi_.end();digitalWrite(slot_.spiChipSelect.number,HIGH);
    }
    BoardModuleSlotCapability slot_;SPIClass spi_;uint8_t drive_=0xFF;int error_=0;
};
class Fat32FormatJob : public ISdFormatJob {
public:
    Fat32FormatJob(const BoardModuleSlotCapability& slot,const SdFileStamp& expected) : slot_(slot),expected_(expected) {}
    bool ownsLease() const {return static_cast<bool>(lease_);}
    bool start() {
        if(!lease_) return false;
        TaskHandle_t task=nullptr;
        return xTaskCreatePinnedToCore(worker,"sd-format",12288,this,1,&task,1)==pdPASS;
    }
    SdFormatState state() const override {return state_.load(std::memory_order_acquire);}
    int errorCode() const override {return error_;}
private:
    static void worker(void* context) {
        auto* self=static_cast<Fat32FormatJob*>(context);
        SdFormatState result;
        { // Finish driver destruction before publishing completion to the web loop.
            Fat32FormatDevice device(self->slot_);
            result=runSdCardFormat(device,self->expected_);
            self->error_=device.error();
        }
        self->state_.store(result,std::memory_order_release);
        vTaskDelete(nullptr); // No access to self after publishing completion.
    }
    SdStorageLease lease_;
    BoardModuleSlotCapability slot_;SdFileStamp expected_;
    std::atomic<SdFormatState> state_{SdFormatState::Running};int error_=0;
};
}
std::unique_ptr<ISdFormatJob> startSdCardFormat(ModuleSlot slotId,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators,
    const SdFileStamp& expected,SdDirectoryStatus& status) {
    if(!supportedSdFormatCapacity(expected.bytes) || !expected.media) {status=SdDirectoryStatus::ReadFailed;return {};}
    const auto* slot=sdFileSlot(slotId,board,configuration,actuators,status);
    if(!slot) return {};
    std::unique_ptr<Fat32FormatJob> job(new(std::nothrow) Fat32FormatJob(*slot,expected));
    if(!job) {status=SdDirectoryStatus::ReadFailed;return {};}
    if(!job->ownsLease()) {status=SdDirectoryStatus::Busy;return {};}
    if(!job->start()) {status=SdDirectoryStatus::ReadFailed;return {};}
    status=SdDirectoryStatus::Ready;return std::unique_ptr<ISdFormatJob>(job.release());
}

namespace {
void readSdDirectoryOnSlot(const BoardModuleSlotCapability& capability,SdDirectorySnapshot& snapshot) {
    const auto* slot=&capability;
    if(!sdRuntimeStable()) {snapshot.status=SdDirectoryStatus::Busy;return;}
    SpiDiagnosticDevice device(*slot);
    if (!device.cardDetected()) { snapshot.status=SdDirectoryStatus::NoCard; return; }
    if (!device.mount()) { device.release(); snapshot.status=SdDirectoryStatus::MountFailed; return; }
    snapshot.info=device.information();
    if (!snapshot.info.rootReadable || !snapshot.info.filesystemBytes || !snapshot.info.cardBytes
        || snapshot.info.filesystemBytes>snapshot.info.cardBytes
        || snapshot.info.usedBytes>snapshot.info.filesystemBytes) {
        device.release(); snapshot.status=SdDirectoryStatus::ReadFailed; return;
    }
    char fullPath[528];
    snprintf(fullPath, sizeof(fullPath), "%s%s", SdCardMountPath, snapshot.path.c_str());
    DIR* directory=::opendir(fullPath);
    if (!directory) {
        snapshot.status=(errno==ENOENT || errno==ENOTDIR) ? SdDirectoryStatus::NotFound : SdDirectoryStatus::ReadFailed;
        device.release(); return;
    }
    snapshot.status=SdDirectoryStatus::Ready;
    unsigned seen=0;
    const uint32_t start=millis();
    while (true) {
        if (digitalRead(slot->auxGpio1.number)!=HIGH) { snapshot.status=SdDirectoryStatus::NoCard; break; }
        if (millis()-start>=2000) {
            snapshot.timedOut=true;
            snapshot.more=snapshot.count>0;
            snapshot.limited=snapshot.offset+snapshot.count>SdDirectoryMaxOffset;
            break;
        }
        errno=0;
        dirent* entry=::readdir(directory);
        if (!entry) { if (errno) snapshot.status=SdDirectoryStatus::ReadFailed; break; }
        if (!strcmp(entry->d_name,".") || !strcmp(entry->d_name,"..")) continue;
        if (seen++<snapshot.offset) continue;
        if (snapshot.count==SdDirectoryPageSize) {
            snapshot.more=true;
            snapshot.limited=snapshot.offset+snapshot.count>SdDirectoryMaxOffset;
            break;
        }
        snprintf(fullPath, sizeof(fullPath), "%s%s%s%s", SdCardMountPath, snapshot.path.c_str(), snapshot.path=="/" ? "" : "/", entry->d_name);
        struct stat info;
        if (::stat(fullPath, &info)!=0) { snapshot.status=SdDirectoryStatus::ReadFailed; break; }
        auto& row=snapshot.entries[snapshot.count];
        row.name=entry->d_name;
        if (row.name.length()!=strlen(entry->d_name)) { snapshot.status=SdDirectoryStatus::ReadFailed; break; }
        row.directory=S_ISDIR(info.st_mode);
        row.bytes=info.st_size<0 ? 0 : static_cast<uint64_t>(info.st_size);
        ++snapshot.count;
        delay(0);
    }
    if (::closedir(directory)!=0 && snapshot.status==SdDirectoryStatus::Ready) snapshot.status=SdDirectoryStatus::ReadFailed;
    if (digitalRead(slot->auxGpio1.number)!=HIGH) snapshot.status=SdDirectoryStatus::NoCard;
    device.release();
}
class SdDirectoryReadJob : public ISdDirectoryJob {
public:
    SdDirectoryReadJob(const BoardModuleSlotCapability& slot,const String& path,unsigned offset) : slot_(slot) {
        snapshot_.slot=slot.slot;snapshot_.path=path;snapshot_.offset=offset;
    }
    bool ownsLease() const {return static_cast<bool>(lease_);}
    bool start() {
        if(!lease_ || !validSdDirectoryPath(snapshot_.path)) return false;
        TaskHandle_t task=nullptr;
        return xTaskCreatePinnedToCore(worker,"sd-directory",8192,this,1,&task,1)==pdPASS;
    }
    bool done() const override {return done_.load(std::memory_order_acquire);}
    const SdDirectorySnapshot& snapshot() const override {return snapshot_;}
private:
    static void worker(void* context) {
        auto* self=static_cast<SdDirectoryReadJob*>(context);
        readSdDirectoryOnSlot(self->slot_,self->snapshot_);
        self->lease_.release(); // Release even if the browser leaves the progress page.
        self->done_.store(true,std::memory_order_release);
        vTaskDelete(nullptr); // No self access after completion is published.
    }
    SdStorageLease lease_;
    BoardModuleSlotCapability slot_;SdDirectorySnapshot snapshot_;
    std::atomic<bool> done_{false};
};
}
void readSdCardDirectory(ModuleSlot slotId,const BoardProfile& board,const Configuration& configuration,
    const ActuatorRuntime& actuators,SdDirectorySnapshot& snapshot) {
    SdStorageLease lease;
    if(!lease) {snapshot.status=SdDirectoryStatus::Busy;return;}
    if(!validSdDirectoryPath(snapshot.path) || snapshot.offset>SdDirectoryMaxOffset) {snapshot.status=SdDirectoryStatus::InvalidPath;return;}
    const auto* slot=sdFileSlot(slotId,board,configuration,actuators,snapshot.status);
    if(slot) readSdDirectoryOnSlot(*slot,snapshot);
}
std::unique_ptr<ISdDirectoryJob> startSdDirectoryRead(ModuleSlot slotId,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators,
    const String& path,unsigned offset,SdDirectoryStatus& status) {
    if(!validSdDirectoryPath(path) || offset>SdDirectoryMaxOffset) {status=SdDirectoryStatus::InvalidPath;return {};}
    const auto* slot=sdFileSlot(slotId,board,configuration,actuators,status);
    if(!slot) return {};
    std::unique_ptr<SdDirectoryReadJob> job(new(std::nothrow) SdDirectoryReadJob(*slot,path,offset));
    if(!job) {status=SdDirectoryStatus::ReadFailed;return {};}
    if(!job->ownsLease()) {status=SdDirectoryStatus::Busy;return {};}
    if(job->snapshot().path.length()!=path.length() || !job->start()) {status=SdDirectoryStatus::ReadFailed;return {};}
    status=SdDirectoryStatus::Ready;return std::unique_ptr<ISdDirectoryJob>(job.release());
}


}

namespace EnvNode {
namespace {
class ScopedSdRelease {
public:
    explicit ScopedSdRelease(SpiDiagnosticDevice& device) : device_(device) {}
    ~ScopedSdRelease() {device_.release();}
private:
    SpiDiagnosticDevice& device_;
};
bool ensureSdLogDirectory(const char* path) {
    if(::mkdir(path,0700)==0) return true;
    struct stat info;
    return errno==EEXIST && ::stat(path,&info)==0 && S_ISDIR(info.st_mode);
}
class PosixSdLogDirectory : public ISdLogDirectory {
public:
    explicit PosixSdLogDirectory(const char* directory) : directory_(directory) {}
    bool scan(uint64_t& total,char* oldest,size_t capacity,const char* protectedName) override {
        DIR* directory=::opendir(directory_);
        if(!directory) return false;
        total=0;oldest[0]=0;bool valid=true;unsigned scanned=0;
        while(true) {
            errno=0;dirent* entry=::readdir(directory);
            if(!entry) {if(errno) valid=false;break;}
            if(++scanned>128) {valid=false;break;}
            if(!managedSdLogName(entry->d_name)) continue;
            char path[96];snprintf(path,sizeof(path),"%s/%s",directory_,entry->d_name);
            struct stat info;
            if(::stat(path,&info)!=0 || info.st_size<0) {valid=false;break;}
            if(!S_ISREG(info.st_mode)) continue;
            const uint64_t size=static_cast<uint64_t>(info.st_size);
            if(UINT64_MAX-total<size) {valid=false;break;}
            total+=size;
            if(strcmp(entry->d_name,protectedName) && (!oldest[0] || strcmp(entry->d_name,oldest)<0)) {
                if(strlen(entry->d_name)>=capacity) {valid=false;break;}
                strcpy(oldest,entry->d_name);
            }
        }
        if(::closedir(directory)!=0) valid=false;
        return valid;
    }
    bool remove(const char* name) override {
        char path[96];snprintf(path,sizeof(path),"%s/%s",directory_,name);
        return ::unlink(path)==0;
    }
private:
    const char* directory_;
};
bool enforceSdLogBudget(const char* path,size_t needed,const char* protectedName,uint64_t budget) {
    PosixSdLogDirectory directory(path);
    return pruneSdSystemLogs(directory,needed,protectedName,budget);
}
bool writeAllSdLog(int fd,const char* data,size_t size) {
    while(size) {
        const int written=::write(fd,data,size);
        if(written<=0) return false;
        data+=written;size-=written;
    }
    return true;
}
class SpiSdLogStorage : public ISdLogStorage {
public:
    SpiSdLogStorage(ModuleSlot slot,uint32_t session) : slot_(slot),session_(session) {}
    // Internal test constructor: caller owns an outer SD lease and a fresh directory.
    SpiSdLogStorage(ModuleSlot slot,const char* directory,uint64_t expectedMedia) : slot_(slot),session_(1),
        directory_(directory),partLimit_(SdRotationTestPartBytes),budget_(SdRotationTestBudgetBytes),isolated_(true),expectedMedia_(expectedMedia) {}
    SdLogWriteResult append(const char* data,size_t size) override {
        if(isolated_) return appendUnderLease(data,size);
        SdStorageLease lease;
        if(!lease || !sdSystemLogSlotEnabled(slot_)) return SdLogWriteResult::Busy;
        return appendUnderLease(data,size);
    }
private:
    SdLogWriteResult appendUnderLease(const char* data,size_t size) {
        if(!sdRuntimeStable()) return SdLogWriteResult::Busy;
        const auto* slot=boardModuleSlot(currentBoardProfile(),slot_);
        if(!slot || !slot->fiveVoltSupplyAvailable || size==0 || size>4096) return SdLogWriteResult::Failed;
        // Isolated diagnostics already own the lease and a single outer mount.
        if(isolated_) return appendMounted(data,size,*slot);
        SpiDiagnosticDevice device(*slot,"system-log");ScopedSdRelease release(device);
        if(!device.cardDetected() || !device.mount()) return failed();
        return appendMounted(data,size,*slot);
    }
    SdLogWriteResult appendMounted(const char* data,size_t size,const BoardModuleSlotCapability& capability) {
        const auto* slot=&capability;
        const uint64_t media=mediaFingerprint();
        if(!media || (isolated_ && media!=expectedMedia_)) return failed();
        if(media!=media_ || (!isolated_ && rotateSdLog)) {name_[0]=0;partBytes_=0;media_=media;if(!isolated_) rotateSdLog=false;}
        if((!isolated_ && !ensureSdLogDirectory("/sd/logs")) || !ensureSdLogDirectory(directory_)) return failed();
        if(partBytes_+size>partLimit_) {name_[0]=0;partBytes_=0;}
        char header[256]={};size_t headerSize=0;
        bool fresh=name_[0]==0;
        if(fresh) {
            if(part_>=9999) return failed();
            headerSize=snprintf(header,sizeof(header),"{\"type\":\"session\",\"boot\":%lu,\"version\":\"%s\",\"build\":\"%s\",\"commit\":\"%s\",\"board_profile\":%u}\n",
                static_cast<unsigned long>(session_),FirmwareBuildInfo::SemanticVersion,
                FirmwareBuildInfo::BuildNumber,FirmwareBuildInfo::GitCommit,
                static_cast<unsigned>(currentBoardProfile().id));
            if(headerSize>=sizeof(header)) return failed();
        }
        if(!enforceSdLogBudget(directory_,size+headerSize,name_,budget_)) return failed();
        char full[96];int fd=-1;
        if(fresh) {
            for(unsigned attempts=0;attempts<32 && part_<9999;++attempts) {
                snprintf(name_,sizeof(name_),"boot-%010lu-part-%04u.jsonl",static_cast<unsigned long>(session_),++part_);
                snprintf(full,sizeof(full),"%s/%s",directory_,name_);
                fd=::open(full,O_WRONLY|O_CREAT|O_EXCL,0600);
                if(fd>=0 || errno!=EEXIST) break;
            }
        } else {
            snprintf(full,sizeof(full),"%s/%s",directory_,name_);
            struct stat info;
            if(::stat(full,&info)!=0 || !S_ISREG(info.st_mode) || info.st_size<0 || static_cast<uint64_t>(info.st_size)!=partBytes_) return failed();
            fd=::open(full,O_WRONLY|O_APPEND);
        }
        if(fd<0) return failed();
        bool ok=(!fresh || writeAllSdLog(fd,header,headerSize)) && writeAllSdLog(fd,data,size);
        if(ok) ok=::fsync(fd)==0;
        if(::close(fd)!=0) ok=false;
        if(digitalRead(slot->auxGpio1.number)!=HIGH) ok=false;
        if(!ok) return failed();
        partBytes_+=headerSize+size;
        if(!isolated_) snprintf(activeSdLogPath,sizeof(activeSdLogPath),"/logs/system/%s",name_);
        return SdLogWriteResult::Written;
    }
private:
    SdLogWriteResult failed() {name_[0]=0;partBytes_=0;return SdLogWriteResult::Failed;}
    ModuleSlot slot_;uint32_t session_;uint64_t media_=0,partBytes_=0;
    unsigned part_=0;char name_[32]={};
    const char* directory_="/sd/logs/system";size_t partLimit_=SdLogPartBytes;
    uint64_t budget_=SdLogBudgetBytes;bool isolated_=false;uint64_t expectedMedia_=0;
};

const char RotationSentinel[]="EnvNode rotation test: preserve unmanaged files\n";
void rotationPartPath(char* output,size_t size,const char* root,unsigned part) {
    snprintf(output,size,"%s/boot-0000000001-part-%04u.jsonl",root,part);
}
bool createRotationWorkspace(const BoardModuleSlotCapability& slot,char* root,size_t capacity,bool& created,uint64_t& expectedMedia,SdRotationTestResult& result) {
    // Caller owns the test's single mount and exclusive lease.
    result.stage="create workspace";
    expectedMedia=mediaFingerprint();
    if(!expectedMedia) {result.error=EIO;return false;}
    for(unsigned i=0;i<32;++i) {
        snprintf(root,capacity,"/sd/envnode-rotation-test-%02u",i);
        if(::mkdir(root,0700)==0) {created=true;break;}
        if(errno!=EEXIST) {result.error=errno;return false;}
    }
    if(!created) {result.error=EEXIST;return false;}
    snprintf(result.directory,sizeof(result.directory),"%s",root+3);
    char path[128];snprintf(path,sizeof(path),"%s/keep.txt",root);
    const int fd=::open(path,O_WRONLY|O_CREAT|O_EXCL,0600);
    if(fd<0) {result.error=errno;return false;}
    bool ok=writeAllSdLog(fd,RotationSentinel,sizeof(RotationSentinel)-1) && ::fsync(fd)==0;
    if(::close(fd)!=0) ok=false;
    if(!ok) result.error=errno ? errno : EIO;
    return ok;
}
bool exerciseRotationWorkspace(ModuleSlot slot,const char* root,uint64_t expectedMedia,SdRotationTestResult& result) {
    SpiSdLogStorage storage(slot,root,expectedMedia);
    char data[SdRotationTestBatchBytes];
    result.stage="write rotation batches";
    for(unsigned batch=0;batch<SdRotationTestBatches;++batch) {
        errno=0;
        if(!fillSdRotationTestBatch(batch,data,sizeof(data)) || storage.append(data,sizeof(data))!=SdLogWriteResult::Written) {
            result.error=errno ? errno : EIO;return false;
        }
        vTaskDelay(1);
    }
    return true;
}
bool verifyRotationPart(const char* path,unsigned part) {
    FILE* file=::fopen(path,"rb");if(!file) return false;
    char header[256];
    bool ok=::fgets(header,sizeof(header),file)!=nullptr && !strncmp(header,"{\"type\":\"session\"",17) && strchr(header,'\n');
    char expected[SdRotationTestBatchBytes],actual[256];
    for(unsigned chunk=0;ok && chunk<3;++chunk) {
        ok=fillSdRotationTestBatch((part-1)*3+chunk,expected,sizeof(expected));
        for(size_t offset=0;ok && offset<sizeof(expected);offset+=sizeof(actual))
            ok=::fread(actual,1,sizeof(actual),file)==sizeof(actual) && !memcmp(actual,expected+offset,sizeof(actual));
    }
    if(ok) ok=::fgetc(file)==EOF && !::ferror(file);
    if(::fclose(file)!=0) ok=false;
    return ok;
}
void inspectRotationWorkspace(const BoardModuleSlotCapability& slot,const char* root,uint64_t expectedMedia,SdRotationTestResult& result) {
    result.stage="verify rotation";
    if(digitalRead(slot.auxGpio1.number)!=HIGH) {result.error=EIO;return;}
    if(mediaFingerprint()!=expectedMedia) {result.error=EIO;return;}
    char path[128];uint64_t total=0;bool oldGone=true,newPresent=true,bytesMatch=true;
    for(unsigned part=1;part<=6;++part) {
        rotationPartPath(path,sizeof(path),root,part);struct stat info;
        const bool exists=::stat(path,&info)==0;
        if(part<=3) {if(exists || errno!=ENOENT) oldGone=false;}
        else {
            if(!exists || !S_ISREG(info.st_mode) || info.st_size<=0 || info.st_size>static_cast<int64_t>(SdRotationTestPartBytes)) newPresent=false;
            else total+=static_cast<uint64_t>(info.st_size);
            if(!verifyRotationPart(path,part)) bytesMatch=false;
        }
    }
    // A wrong file-count policy cannot pass merely by leaving three expected files.
    DIR* directory=::opendir(root);unsigned managed=0;bool scanOk=directory!=nullptr;
    if(directory) {
        unsigned scanned=0;
        while(true) {
            errno=0;dirent* entry=::readdir(directory);
            if(!entry) {if(errno) scanOk=false;break;}
            if(++scanned>32) {scanOk=false;break;}
            if(managedSdLogName(entry->d_name)) ++managed;
        }
        if(::closedir(directory)!=0) scanOk=false;
    }
    result.rotation=newPresent && managed==3 && scanOk;
    result.pruning=oldGone && newPresent && managed==3 && scanOk && total<=SdRotationTestBudgetBytes;
    result.content=bytesMatch && newPresent;
    snprintf(path,sizeof(path),"%s/keep.txt",root);
    const int fd=::open(path,O_RDONLY);char marker[sizeof(RotationSentinel)]={};
    if(fd>=0) {
        result.foreignFile=::read(fd,marker,sizeof(marker))==static_cast<int>(sizeof(RotationSentinel)-1)
            && !memcmp(marker,RotationSentinel,sizeof(RotationSentinel)-1);
        if(::close(fd)!=0) result.foreignFile=false;
    }
    if(!result.rotation || !result.pruning || !result.content || !result.foreignFile) result.error=EIO;
}
bool cleanupRotationWorkspace(const BoardModuleSlotCapability& slot,const char* root,uint64_t expectedMedia) {
    if(digitalRead(slot.auxGpio1.number)!=HIGH || mediaFingerprint()!=expectedMedia) return false;
    bool ok=true;char path[128];
    // Never recursively delete: remove only names this newly-created test could own.
    for(unsigned part=1;part<=SdRotationTestBatches;++part) {
        rotationPartPath(path,sizeof(path),root,part);
        if(::unlink(path)!=0 && errno!=ENOENT) ok=false;
    }
    snprintf(path,sizeof(path),"%s/keep.txt",root);
    if(::unlink(path)!=0 && errno!=ENOENT) ok=false;
    if(::rmdir(root)!=0) ok=false;
    return ok;
}
class MountedSdRotationTarget : public ISdRotationTestTarget {
public:
    explicit MountedSdRotationTarget(const BoardModuleSlotCapability& slot) : slot_(slot),device_(slot_,"rotation-test") {}
    ~MountedSdRotationTarget() override {release();}
    bool mount(SdRotationTestResult& result) override {
        result_=&result;const uint32_t started=millis();
        const bool ok=device_.cardDetected() && device_.mount();
        result.mountMs=millis()-started;
        if(!ok) result.error=EIO;
        return ok;
    }
    bool createWorkspace(SdRotationTestResult& result) override {
        return createRotationWorkspace(slot_,root_,sizeof(root_),created_,expectedMedia_,result);
    }
    bool exercise(SdRotationTestResult& result) override {
        const uint32_t started=millis();
        const bool ok=exerciseRotationWorkspace(slot_.slot,root_,expectedMedia_,result);
        result.writeMs=millis()-started;return ok;
    }
    void inspect(SdRotationTestResult& result) override {
        const uint32_t started=millis();
        inspectRotationWorkspace(slot_,root_,expectedMedia_,result);
        result.verifyMs=millis()-started;
    }
    int cleanup() override {
        const uint32_t started=millis();
        const bool ok=!created_ || cleanupRotationWorkspace(slot_,root_,expectedMedia_);
        const int error=ok ? 0 : (errno ? errno : EIO);
        if(result_) result_->cleanupMs=millis()-started;
        return error;
    }
    void release() override {
        if(!released_) {device_.release();released_=true;}
    }
private:
    SdRotationTestResult* result_=nullptr;
    BoardModuleSlotCapability slot_;SpiDiagnosticDevice device_;
    char root_[64]={};bool created_=false,released_=false;uint64_t expectedMedia_=0;
};
class SdRotationTestJob : public ISdRotationTestJob {
public:
    explicit SdRotationTestJob(const BoardModuleSlotCapability& slot) : slot_(slot) {}
    bool ownsLease() const {return static_cast<bool>(lease_);}
    bool start() {
        TaskHandle_t task=nullptr;
        return lease_ && xTaskCreatePinnedToCore(worker,"sd-rotation",12288,this,1,&task,1)==pdPASS;
    }
    bool done() const override {return done_.load(std::memory_order_acquire);}
    const SdRotationTestResult& result() const override {return result_;}
private:
    static void worker(void* context) {
        auto* self=static_cast<SdRotationTestJob*>(context);
        self->result_.startHeap=ESP.getFreeHeap();
        self->result_.startLargest=ESP.getMaxAllocHeap();
        {
            MountedSdRotationTarget target(self->slot_);
            const uint32_t started=millis();
            runSdRotationTest(target,self->result_);
            self->result_.elapsedMs=millis()-started;
            self->result_.finishHeap=ESP.getFreeHeap();
            self->result_.finishLargest=ESP.getMaxAllocHeap();
        }
        self->lease_.release();
        self->done_.store(true,std::memory_order_release);
        vTaskDelete(nullptr);
    }
    SdStorageLease lease_;BoardModuleSlotCapability slot_;SdRotationTestResult result_;
    std::atomic<bool> done_{false};
};
}
std::unique_ptr<ISdRotationTestJob> startSdRotationTest(ModuleSlot slotId,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators,SdDirectoryStatus& status) {
    const auto* slot=sdFileSlot(slotId,board,configuration,actuators,status);
    if(!slot) return {};
    std::unique_ptr<SdRotationTestJob> job(new(std::nothrow) SdRotationTestJob(*slot));
    if(!job) {status=SdDirectoryStatus::ReadFailed;return {};}
    if(!job->ownsLease()) {status=SdDirectoryStatus::Busy;return {};}
    if(!job->start()) {status=SdDirectoryStatus::ReadFailed;return {};}
    status=SdDirectoryStatus::Ready;return std::unique_ptr<ISdRotationTestJob>(job.release());
}
std::unique_ptr<ISdLogStorage> createSdSystemLogStorage(ModuleSlot slot,uint32_t session) {
    if(!session) return {};
    return std::unique_ptr<ISdLogStorage>(new(std::nothrow) SpiSdLogStorage(slot,session));
}
}
