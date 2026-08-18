# HT24LC08_RAM Library for Arduino

A lightweight library that turns the **HT24LC08** I2C EEPROM into a convenient RAM-like storage device with automatic multi-byte variable support, persistent LSB/MSB byte order, and range clearing.

## Features
- RAM-style syntax (`ram.write()`, `ram.read()`, `ram.clear()`).
- Automatic variable size handling (bytes, `int`, `long`, `float`, structures, etc.).
- Persistent byte-order mode (`LSB_FIRST` / `MSB_FIRST`) stored safely in the EEPROM cell `0x0000`.
- Memory size & address info via `ram.length()`.
- Flexible clearing (`ram.clear()`, by sector, or by address range `ram.start:end`).

---

## Installation
1. Download this repository as a `.zip` file.
2. In Arduino IDE go to **Sketch > Include Library > Add .ZIP Library...** and select the downloaded file.

---

## Pin Configuration (Optional)
You can define custom hardware pins **before** including the library. If not defined, default Arduino Nano pins (`A4`/`A5`) and GND (`-1`) are used.

```cpp
#define PIN_A0 2
#define PIN_A1 3
#define PIN_A2 4
#define PIN_WP 5

#include <HT24LC08_RAM.h>

HT24LC08_RAM ram;
```

---

## Full Usage Example

```cpp
#include <HT24LC08_RAM.h>

HT24LC08_RAM ram;

void setup() {
  Serial.begin(115200);
  ram.begin();

  // 1. Check memory info
  // Returns: [min_address][max_address][hex_length][dec_length]
  Serial.print("RAM Info: ");
  Serial.println(ram.length()); // Output: [0x0000][0x03FF][0x0400][1024]

  // 2. Set byte order mode (saved permanently in address 0x0000)
  // LSB_FIRST (0x00) or MSB_FIRST (0xFF)
  ram.mode(LSB_FIRST);

  // 3. WRITE operations
  ram.write(10, (uint8_t)99);        // Write a single byte
  ram.write(20, (int)12345);         // Write a 16-bit integer (automatically handles size)
  ram.write(30, (float)3.1415);      // Write a 32-bit float

  // 4. READ operations
  // Note: For multi-byte variables (int, float, etc.), pass a dummy typed value 
  // as the second argument so the template knows what type to return.
  uint8_t bVal = ram.read(10);       // Read single byte -> returns int (-1 on error)
  int iVal     = ram.read(20, (int)0);     // Read 16-bit integer
  float fVal   = ram.read(30, (float)0);   // Read 32-bit float

  Serial.print("Byte at 10: "); Serial.println(bVal);
  Serial.print("Int at 20:  "); Serial.println(iVal);
  Serial.print("Float at 30:"); Serial.println(fVal);

  // 5. CLEAR operations
  // ram.clear();                  // Clear entire EEPROM (preserves byte order mode at addr 0)
  // ram.clear(1);                 // Clear specific sector / block (0-7)
  // ram.clear(0x0020, 0x0050);    // Clear specific address range (start, end)
}

void loop() {
}
```
