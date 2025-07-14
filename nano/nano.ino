/**
 * @file nano.ino
 * @brief Arduino Nano firmware for flashing EEPROM via a custom serial protocol.
 *
 * This firmware implements a simple serial protocol to receive data from a host and write it to an EEPROM.
 * It supports commands for ping, data transfer (low/high pages), and acknowledgment.
 * Data integrity is checked using CRC16.
 */

/**
 * @defgroup Pins Pin assignments
 * @{
 */
constexpr uint8_t AddressPin = A0;
constexpr uint8_t StorageClockPin = A1;
constexpr uint8_t ShiftClockPin = A2;
constexpr uint8_t OutputEnablePin = 7;
constexpr uint8_t WriteEnablePin = 6;
constexpr uint8_t PagePin = 5;
constexpr uint8_t DataPins[8] = {
  2, 3, 4, 12, 11, 10, 9, 8
};
/** @} */

constexpr uint16_t Timeout = 200; /**< Serial read timeout in milliseconds. */

/**
 * @enum Op_E
 * @brief Operation codes for the serial protocol.
 */
typedef enum {
  OP_PING,      /**< Ping command. */
  OP_SEND_LOW,  /**< Send data to low page. */
  OP_SEND_HIGH, /**< Send data to high page. */
  OP_READ_LOW,
  OP_READ_HIGH,
  OP_ACK,       /**< Acknowledge command. */
  OP_BAD_WRITE,
} Op_E;

/**
 * @brief Acknowledgment packet sent after successful operation.
 */
constexpr uint8_t Ack[4] = {
  0xAA, OP_ACK, 0xA3, 0xBE
};

/**
 * @brief Data buffer for serial communication and EEPROM writing.
 *
 * buf[0..1]: header and command
 * buf[2..257]: data payload
 * buf[258..259]: CRC16 hash
 */
uint8_t buf[260];

/**
 * @brief Update CRC16 hash with a new byte.
 * @param byte Input byte.
 * @param hash Current hash value.
 * @return Updated hash value.
 */
static uint16_t crc16Update(const uint8_t byte, uint16_t hash) {
  for (uint8_t i = 0; i < 8; ++i) {
    const bool bitFlag = hash >> 15;
    hash <<= 1;
    hash |= (byte >> i) & 1;
    if (bitFlag) hash ^= 0x8005;
  }
  return hash;
}

/**
 * @brief Finalize CRC16 hash computation.
 * @param hash Current hash value.
 * @return Final CRC16 value.
 */
static uint16_t crc16Finalize(uint16_t hash) {
  for (uint8_t i = 0; i < 16; ++i) {
    const bool bitFlag = hash >> 15;
    hash <<= 1;
    if (bitFlag) hash ^= 0x8005;
  }
  uint16_t crc = 0;
  for (uint16_t i = (1 << 15), j = 1; i; i >>= 1, j <<= 1)
    if (i & hash) crc |= j;
  return crc;
}

/**
 * @brief Compute CRC16 hash for the buffer.
 * @param len Number of bytes to hash.
 * @return CRC16 value.
 */
static uint16_t crc16(uint16_t len) {
  uint16_t hash = 0;
  for (uint16_t i = 0; i < len; ++i)
    hash = crc16Update(buf[i], hash);
  return crc16Finalize(hash);
}

/**
 * @brief Check if the received data's CRC16 matches the transmitted hash.
 * @return true if hash matches, false otherwise.
 */
static bool checkHash(const bool data) {
  if (data) {
    const uint16_t hash = crc16(258);
    return (hash >> 8) == buf[258] && (hash & 0xFF) == buf[259];
  }
  const uint16_t hash = crc16(2);
  return (hash >> 8) == buf[2] && (hash & 0xFF) == buf[3];
}

static void makeHash(const bool data) {
  if (data) {
    const uint16_t hash = crc16(258);
    buf[258] = (uint8_t)(hash >> 8);
    buf[259] = (uint8_t)(hash & 0xFF);
  } else {
    const uint16_t hash = crc16(2);
    buf[2] = (uint8_t)(hash >> 8);
    buf[3] = (uint8_t)(hash & 0xFF);
  }
}

/**
 * @brief Write a byte to the EEPROM at the specified address and page.
 * @param addr EEPROM address (0-255).
 * @param byte Data byte to write.
 * @param page Page select (false = low, true = high).
 */
static void eepromWrite(const uint8_t addr, const uint8_t byte) {
  for (uint8_t i = 0; i < 8; ++i) {
    digitalWrite(AddressPin, (addr >> (7 - i)) & 1);
    digitalWrite(DataPins[i], (byte >> i) & 1);
    digitalWrite(ShiftClockPin, HIGH);
    digitalWrite(ShiftClockPin, LOW);
  }
  digitalWrite(StorageClockPin, HIGH);
  digitalWrite(StorageClockPin, LOW);
  digitalWrite(WriteEnablePin, LOW);
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  digitalWrite(WriteEnablePin, HIGH);
}

static void ioMode(const uint8_t mode) {
  for (uint8_t i = 0; i < 8; ++i) {
    pinMode(DataPins[i], mode);
  }
}

/**
 * @brief Write the received buffer to the EEPROM.
 * @param page Page select (false = low, true = high).
 */
static void writeBuf(const bool page) {
  ioMode(OUTPUT);
  digitalWrite(PagePin, page);
  for (uint16_t i = 0; i < 256; ++i) {
    eepromWrite((uint8_t)i, buf[i + 2]);
    delay(10);
  }
}

static uint8_t eepromRead(const uint8_t addr) {
  for (uint8_t i = 0; i < 8; ++i) {
    digitalWrite(AddressPin, (addr >> (7 - i)) & 1);
    digitalWrite(ShiftClockPin, HIGH);
    digitalWrite(ShiftClockPin, LOW);
  }
  digitalWrite(StorageClockPin, HIGH);
  digitalWrite(StorageClockPin, LOW);
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  uint8_t byte = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    byte <<= 1;
    byte |= digitalRead(DataPins[7 - i]);
  }
  return byte;
}

static void readBuf(const bool page) {
  ioMode(INPUT);
  digitalWrite(PagePin, page);
  digitalWrite(OutputEnablePin, LOW);
  for (uint16_t i = 0; i < 256; ++i) {
    buf[i + 2] = eepromRead((uint8_t)i);
  }
  digitalWrite(OutputEnablePin, HIGH);
}

/**
 * @brief Load data from the serial port into the buffer.
 * @param off Buffer offset to start writing.
 * @param len Number of bytes to read.
 * @return true if all bytes are read successfully, false on timeout.
 */
static bool loadData(const uint16_t off, const uint16_t len) {
  for (uint16_t i = 0; i < len; ++i) {
    const uint32_t start = millis();
    while (Serial.available() < 1) {
      const uint32_t now = millis();
      if (now - start >= Timeout) return false;
    }
    buf[off + i] = (uint8_t)Serial.read();
  }
  return true;
}

/**
 * @brief Arduino setup function. Initializes serial and pin modes.
 */
void setup() {
  Serial.begin(115200);
  pinMode(WriteEnablePin, OUTPUT);
  pinMode(OutputEnablePin, OUTPUT);
  pinMode(StorageClockPin, OUTPUT);
  pinMode(ShiftClockPin, OUTPUT);
  pinMode(PagePin, OUTPUT);
  pinMode(AddressPin, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(WriteEnablePin, HIGH);
  digitalWrite(OutputEnablePin, HIGH);
  digitalWrite(StorageClockPin, LOW);
  digitalWrite(ShiftClockPin, LOW);
}

/**
 * @brief Arduino main loop. Handles serial protocol and EEPROM flashing.
 *
 * Protocol:
 * - Waits for a command packet.
 * - Validates header and command.
 * - Handles OP_PING, OP_SEND_LOW, OP_SEND_HIGH.
 * - Writes data to EEPROM if valid.
 * - Sends acknowledgment or error signal (LED blink).
 */
void loop() {
  digitalWrite(13, LOW);
  while (Serial.available() < 1) {}
  digitalWrite(13, HIGH);
  if (!loadData(0, 4)) goto drop;
  if (buf[0] != 0xAA) goto drop;
  switch (buf[1]) {
    case OP_PING:
      if (!checkHash(false)) goto drop;
      buf[1] = OP_ACK;
      makeHash(false);
      Serial.write(buf, 4);
      break;
    case OP_SEND_LOW:
    case OP_SEND_HIGH:
      if (!loadData(4, 256)) goto drop;
      if (!checkHash(true)) goto drop;
      writeBuf(buf[1] == OP_SEND_HIGH);
      readBuf(buf[1] == OP_SEND_HIGH);
      if (!checkHash(true)) buf[1] = OP_BAD_WRITE;
      else buf[1] = OP_ACK;
      makeHash(false);
      Serial.write(buf, 4);
      break;
    case OP_READ_LOW:
    case OP_READ_HIGH:
      if (!checkHash(false)) goto drop;
      readBuf(buf[1] == OP_READ_HIGH);
      buf[1] = OP_SEND_LOW;
      makeHash(true);
      Serial.write(buf, 260);
      break;
    case OP_ACK:
    case OP_BAD_WRITE:
    default:
      goto drop;
  }
  Serial.flush();
drop:
  while (Serial.available() > 0) Serial.read();
}
