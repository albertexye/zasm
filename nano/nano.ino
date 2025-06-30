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
constexpr uint8_t PagePin = 6;   /**< EEPROM page select pin. */
constexpr uint8_t AddrPin = 4;   /**< Shift register SER pin for address. */
constexpr uint8_t DataPin = 5;   /**< Shift register SER pin for data input. */
constexpr uint8_t Clk = 2;       /**< Shift register internal clock pin. */
constexpr uint8_t SClk = 3;      /**< Shift register external clock pin. */
constexpr uint8_t WE = 7;        /**< EEPROM write enable pin. */
/** @} */

constexpr uint16_t Timeout = 500; /**< Serial read timeout in milliseconds. */

/**
 * @enum Op_E
 * @brief Operation codes for the serial protocol.
 */
typedef enum {
  OP_PING,      /**< Ping command. */
  OP_SEND_LOW,  /**< Send data to low page. */
  OP_SEND_HIGH, /**< Send data to high page. */
  OP_ACK,       /**< Acknowledge command. */
} Op_E;

/**
 * @brief Acknowledgment packet sent after successful operation.
 */
constexpr uint8_t Ack[4] = {
  0xAA, OP_ACK, 0xA1, 0x3E
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
static uint16_t crc16(const uint16_t len) {
  uint16_t hash = 0;
  for (uint16_t i = 0; i < len; ++i)
    hash = crc16Update(buf[i], hash);
  return crc16Finalize(hash);
}

/**
 * @brief Check if the received data's CRC16 matches the transmitted hash.
 * @return true if hash matches, false otherwise.
 */
static bool checkHash() {
  const uint16_t hash = crc16(258);
  return (hash >> 8) == buf[258] && (hash & 0xFF) == buf[259];
}

/**
 * @brief Write a byte to the EEPROM at the specified address and page.
 * @param addr EEPROM address (0-255).
 * @param byte Data byte to write.
 * @param page Page select (false = low, true = high).
 */
static void eepromWrite(const uint8_t addr, const uint8_t byte, const bool page) {
  digitalWrite(PagePin, page);
  for (uint8_t i = 0; i < 8; ++i) {
    digitalWrite(AddrPin, (addr >> i) & 1);
    digitalWrite(DataPin, (byte >> i) & 1);
    digitalWrite(Clk, LOW);
    digitalWrite(Clk, HIGH);
  }
  digitalWrite(SClk, LOW);
  digitalWrite(SClk, HIGH);
  digitalWrite(WE, LOW);
  digitalWrite(WE, HIGH);
}

/**
 * @brief Write the received buffer to the EEPROM.
 * @param page Page select (false = low, true = high).
 */
static void writeBuf(const bool page) {
  uint8_t i = 255;
  const uint8_t* const data = buf + 2;
  do {
    eepromWrite(i, data[i], page);
  } while (i-- != 0);
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
  pinMode(PagePin, OUTPUT);
  pinMode(WE, OUTPUT);
  pinMode(AddrPin, OUTPUT);
  pinMode(DataPin, OUTPUT);
  pinMode(Clk, OUTPUT);
  pinMode(SClk, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(WE, HIGH);
  digitalWrite(SClk, HIGH);
  digitalWrite(Clk, HIGH);
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
      if (buf[2] != 0xA0 || buf[3] != 0x7E) goto drop;
      break;
    case OP_SEND_LOW:
    case OP_SEND_HIGH:
      if (!loadData(4, 256)) goto drop;
      if (!checkHash()) goto drop;
      writeBuf(buf[1] == OP_SEND_HIGH);
      break;
    case OP_ACK:
    default:
      goto drop;
  }
  Serial.write((const char*)Ack, 4);
  Serial.flush();
  while (Serial.available() > 0) Serial.read();
  return;
drop:
  while (Serial.available() > 0) Serial.read();
  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(13, LOW);
    delay(100);
    digitalWrite(13, HIGH);
    delay(100);
  }
}
