// EnvNode: see repository LICENSE.
#if defined(ENVNODE_SD_DIAGNOSTICS)
#include "SdCardSpiDiagnostic.h"
#include "SdCardDiagnostic.h"
#include "SdCardMountPath.h"
#include "esp_vfs.h"
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
    explicit SpiDiagnosticDevice(const BoardModuleSlotCapability& slot) : slot_(slot), spi_(VSPI) {}
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
        return SD.begin(slot_.spiChipSelect.number, spi_, DiagnosticClockHz, SdCardMountPath, 2, false);
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
        info.filesystemBytes = SD.totalBytes();
        info.usedBytes = SD.usedBytes();
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
};
}
void runSdCardSpiDiagnostic(ModuleSlot slotId, const BoardProfile& board,
    const Configuration& configuration, const AutomaticActuatorDefinition* automaticActuators,
    size_t automaticCount, ILogger& logger) {
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
    SpiDiagnosticDevice device_; uint8_t cd_; bool mounted_=false;
    int fd_=-1; char path_[256]={}; SdFileStamp stamp_;
    const char* error_="none";int errorCode_=0;
};
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
    bool start() {
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
    if(!job || !job->start()) {status=SdDirectoryStatus::ReadFailed;return {};}
    status=SdDirectoryStatus::Ready;return std::unique_ptr<ISdFormatJob>(job.release());
}

void readSdCardDirectory(ModuleSlot slotId, const BoardProfile& board,
    const Configuration& configuration, const ActuatorRuntime& actuators, SdDirectorySnapshot& snapshot) {
    if (!validSdDirectoryPath(snapshot.path) || snapshot.offset>SdDirectoryMaxOffset) {
        snapshot.status=SdDirectoryStatus::InvalidPath; return;
    }
    const auto* slot=sdFileSlot(slotId,board,configuration,actuators,snapshot.status);
    if (!slot) return;
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
        if (millis()-start>=150) { snapshot.limited=true; break; }
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

}
#endif

#if !defined(ENVNODE_SD_DIAGNOSTICS)
#include "SdCardSpiDiagnostic.h"
namespace EnvNode {
bool inspectSdCardForFormat(ModuleSlot,const BoardProfile&,const Configuration&,const ActuatorRuntime&,
    SdFileStamp&,SdDirectoryStatus& status) {status=SdDirectoryStatus::Disabled;return false;}
std::unique_ptr<ISdFormatJob> startSdCardFormat(ModuleSlot,const BoardProfile&,const Configuration&,const ActuatorRuntime&,
    const SdFileStamp&,SdDirectoryStatus& status) {status=SdDirectoryStatus::Disabled;return {};}
std::unique_ptr<ISdStoredFile> openSdCardFile(ModuleSlot,const BoardProfile&,const Configuration&,
    const ActuatorRuntime&,const String&,SdDirectoryStatus& status) {status=SdDirectoryStatus::Disabled; return {};}
void readSdCardDirectory(ModuleSlot, const BoardProfile&, const Configuration&,
    const ActuatorRuntime&, SdDirectorySnapshot& snapshot) { snapshot.status=SdDirectoryStatus::Disabled; }
}
#endif
