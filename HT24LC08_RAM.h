#ifndef HT24LC08_RAM_h
#define HT24LC08_RAM_h

#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <string.h>

// ============================================
// Pin Configuration - Define your pins here
// ============================================
#ifndef PIN_SDA
    #define PIN_SDA A4
#endif

#ifndef PIN_SCL
    #define PIN_SCL A5
#endif

#ifndef PIN_A0
    #define PIN_A0 -1
#endif

#ifndef PIN_A1
    #define PIN_A1 -1
#endif

#ifndef PIN_A2
    #define PIN_A2 -1
#endif

#ifndef PIN_WP
    #define PIN_WP -1
#endif
// ============================================

enum RAM_ByteOrder {
    LSB_FIRST = 0x00, 
    MSB_FIRST = 0xFF  
};

class HT24LC08 {
private:
    uint8_t _sdaPin, _sclPin, _a0Pin, _a1Pin, _a2Pin, _wpPin, _pageSize;
    uint16_t _memorySize;
    TwoWire* _wire;
    
    uint8_t getAddressPinsState() {
        uint8_t addr = 0;
        if (_a0Pin != -1) addr |= (digitalRead(_a0Pin) << 0);
        if (_a1Pin != -1) addr |= (digitalRead(_a1Pin) << 1);
        if (_a2Pin != -1) addr |= (digitalRead(_a2Pin) << 2);
        return addr;
    }
    
    uint8_t getDeviceAddress(uint16_t address) {
        uint8_t baseAddr = 0x50;
        uint8_t targetBlock = address / 128;
        uint8_t hwPins = getAddressPinsState();
        uint8_t requiredA2 = (targetBlock >= 4) ? 1 : 0;
        if (((hwPins >> 2) & 0x01) != requiredA2) return 0xFF; 
        return baseAddr | ((targetBlock % 4) << 1);
    }
    
    uint8_t getBlockAddress(uint16_t address) { return address % 128; }
    
    void configurePins() {
        if (_a0Pin != -1) { pinMode(_a0Pin, OUTPUT); digitalWrite(_a0Pin, LOW); }
        if (_a1Pin != -1) { pinMode(_a1Pin, OUTPUT); digitalWrite(_a1Pin, LOW); }
        if (_a2Pin != -1) { pinMode(_a2Pin, OUTPUT); digitalWrite(_a2Pin, LOW); }
        if (_wpPin != -1) { pinMode(_wpPin, OUTPUT); digitalWrite(_wpPin, LOW); }
    }

public:
    HT24LC08(uint8_t a0 = PIN_A0, uint8_t a1 = PIN_A1, uint8_t a2 = PIN_A2, uint8_t wp = PIN_WP) {
        _sdaPin = PIN_SDA; _sclPin = PIN_SCL; _a0Pin = a0; _a1Pin = a1; _a2Pin = a2; _wpPin = wp;
        _pageSize = 16; _memorySize = 1024; _wire = &Wire;
        _wire->begin();
        configurePins();
    }
    
    bool begin(uint32_t frequency = 100000) {
        _wire->setClock(frequency);
        return true;
    }
    
    bool writeByte(uint16_t address, uint8_t data) {
        if (address >= _memorySize) return false;
        uint8_t deviceAddr = getDeviceAddress(address);
        if (deviceAddr == 0xFF) return false;
        
        _wire->beginTransmission(deviceAddr);
        _wire->write(getBlockAddress(address));
        _wire->write(data);
        if (_wire->endTransmission() == 0) {
            delay(5);
            return true;
        }
        return false;
    }
    
    int readByte(uint16_t address) {
        if (address >= _memorySize) return -1;
        uint8_t deviceAddr = getDeviceAddress(address);
        if (deviceAddr == 0xFF) return -1;
        
        _wire->beginTransmission(deviceAddr);
        _wire->write(getBlockAddress(address));
        if (_wire->endTransmission(false) != 0) return -1;
        if (_wire->requestFrom(deviceAddr, (uint8_t)1) != 1) return -1;
        return _wire->read();
    }
    
    size_t writeBytes(uint16_t address, const uint8_t* data, size_t length) {
        if (address >= _memorySize) return 0;
        size_t bytesWritten = 0;
        uint16_t currentAddress = address;
        
        while (bytesWritten < length && currentAddress < _memorySize) {
            uint8_t pageOffset = currentAddress % _pageSize;
            uint8_t bytesToWrite = min(_pageSize - pageOffset, length - bytesWritten);
            bytesToWrite = min(bytesToWrite, (uint16_t)(_memorySize - currentAddress));
            if (bytesToWrite == 0) break;
            
            uint8_t deviceAddr = getDeviceAddress(currentAddress);
            if (deviceAddr == 0xFF) break;
            
            _wire->beginTransmission(deviceAddr);
            _wire->write(getBlockAddress(currentAddress));
            _wire->write(data + bytesWritten, bytesToWrite);
            if (_wire->endTransmission() != 0) break;
            
            delay(6);
            bytesWritten += bytesToWrite;
            currentAddress += bytesToWrite;
        }
        return bytesWritten;
    }
    
    size_t readBytes(uint16_t address, uint8_t* buffer, size_t length) {
        if (address >= _memorySize) return 0;
        size_t bytesRead = 0;
        uint16_t currentAddress = address;
        
        while (bytesRead < length && currentAddress < _memorySize) {
            uint8_t deviceAddr = getDeviceAddress(currentAddress);
            if (deviceAddr == 0xFF) break;
            uint8_t blockAddr = getBlockAddress(currentAddress);
            uint8_t bytesToRead = min(128 - blockAddr, length - bytesRead);
            bytesToRead = min(bytesToRead, (uint16_t)(_memorySize - currentAddress));
            if (bytesToRead == 0) break;
            
            _wire->beginTransmission(deviceAddr);
            _wire->write(blockAddr);
            if (_wire->endTransmission(false) != 0) break;
            
            uint8_t received = _wire->requestFrom(deviceAddr, bytesToRead);
            if (received != bytesToRead) {
                bytesRead += received;
                break;
            }
            for (uint8_t i = 0; i < bytesToRead; i++) {
                buffer[bytesRead + i] = _wire->read();
            }
            bytesRead += bytesToRead;
            currentAddress += bytesToRead;
        }
        return bytesRead;
    }
    
    void clear(uint8_t value = 0xFF) {
        uint8_t buffer[16];
        memset(buffer, value, 16);
        for (uint16_t i = 0; i < _memorySize; i += 16) {
            writeBytes(i, buffer, 16);
            delay(6);
        }
    }
    
    uint16_t getMemorySize() { return _memorySize; }
};

class HT24LC08_RAM : public HT24LC08 {
private:
    char _infoBuffer[40];
    RAM_ByteOrder _currentMode;
    const uint16_t _modeAddress = 0x0000;

public:
    HT24LC08_RAM(uint8_t a0 = PIN_A0, uint8_t a1 = PIN_A1, uint8_t a2 = PIN_A2, uint8_t wp = PIN_WP)
        : HT24LC08(a0, a1, a2, wp), _currentMode(LSB_FIRST) {}

    bool begin(uint32_t frequency = 100000) {
        if (!HT24LC08::begin(frequency)) return false;
        
        int savedByte = readByte(_modeAddress);
        
        if (savedByte == 0x00) {
            _currentMode = LSB_FIRST;
        } 
        else if (savedByte == 0xFF) {
            _currentMode = MSB_FIRST;
        } 
        else {
            _currentMode = LSB_FIRST;
            writeByte(_modeAddress, 0x00);
        }
        return true;
    }

    void mode(RAM_ByteOrder byteOrder) {
        _currentMode = byteOrder;
        writeByte(_modeAddress, (uint8_t)byteOrder);
    }

    RAM_ByteOrder mode() {
        return _currentMode;
    }

    bool write(uint16_t address, uint8_t data) {
        return writeByte(address, data);
    }

    size_t write(uint16_t address, const uint8_t* data, size_t length) {
        return writeBytes(address, data, length);
    }

    template <typename T>
    bool write(uint16_t address, const T& data) {
        const uint8_t* ptr = (const uint8_t*)&data;
        size_t size = sizeof(T);
        uint8_t temp[size];

        if (_currentMode == MSB_FIRST) {
            for (size_t i = 0; i < size; i++) {
                temp[i] = ptr[size - 1 - i];
            }
            return writeBytes(address, temp, size) == size;
        } else {
            return writeBytes(address, ptr, size) == size;
        }
    }

    int read(uint16_t address) {
        return readByte(address);
    }

    size_t read(uint16_t address, uint8_t* buffer, size_t length) {
        return readBytes(address, buffer, length);
    }

    template <typename T>
    T read(uint16_t address, T dummy) {
        T data;
        size_t size = sizeof(T);
        uint8_t temp[size];
        
        if (readBytes(address, temp, size) != size) return 0;

        if (_currentMode == MSB_FIRST) {
            uint8_t* ptr = (uint8_t*)&data;
            for (size_t i = 0; i < size; i++) {
                ptr[i] = temp[size - 1 - i];
            }
        } else {
            memcpy(&data, temp, size);
        }
        return data;
    }

    void clear() {
        RAM_ByteOrder currentMode = _currentMode;
        HT24LC08::clear(0xFF);
        writeByte(_modeAddress, (uint8_t)currentMode);
    }

    void clear(uint8_t sector) {
        if (sector > 7) return;
        uint8_t currentBlock = getCurrentBlock();
        setBlock(sector);
        
        uint8_t buffer[16];
        memset(buffer, 0xFF, 16);
        for (uint16_t i = 0; i < 128; i += 16) {
            writeBytes(i, buffer, 16);
            delay(6);
        }
        setBlock(currentBlock);
        
        if (sector == 0) {
            writeByte(_modeAddress, (uint8_t)_currentMode);
        }
    }

    void clear(uint16_t startAddr, uint16_t endAddr) {
        if (startAddr > endAddr) {
            uint16_t temp = startAddr;
            startAddr = endAddr;
            endAddr = temp;
        }
        
        if (endAddr >= getMemorySize()) {
            endAddr = getMemorySize() - 1;
        }

        uint8_t buffer[16];
        memset(buffer, 0xFF, 16);

        uint16_t current = startAddr;
        while (current <= endAddr) {
            uint8_t chunk = min((uint16_t)16, (uint16_t)(endAddr - current + 1));
            writeBytes(current, buffer, chunk);
            delay(6);
            if (current + chunk < current) break;
            current += chunk;
        }

        if (startAddr == 0 || startAddr <= _modeAddress && endAddr >= _modeAddress) {
            writeByte(_modeAddress, (uint8_t)_currentMode);
        }
    }

    const char* length() {
        uint16_t minAddr = 0x0000;
        uint16_t maxAddr = getMemorySize() - 1;
        uint16_t totalSize = getMemorySize();

        snprintf(_infoBuffer, sizeof(_infoBuffer), "[0x%04X][0x%04X][0x%04X][%u]", 
                 minAddr, maxAddr, totalSize, totalSize);
                 
        return _infoBuffer;
    }
};

#endif